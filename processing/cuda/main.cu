#include <cuda_runtime.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stdio.h>

// CUDA kernel to process frame data
__global__ void process_frame(uint8_t *data, int width, int height) {
  int x = blockIdx.x * blockDim.x + threadIdx.x;
  int y = blockIdx.y * blockDim.y + threadIdx.y;

  if (x < width && y < height) {
    int idx = y * width + x;
    data[idx] = 255 - data[idx]; // Example: Invert pixel values
  }
}

int main(int argc, char *argv[]) {
  av_register_all();

  if (argc < 2) {
    printf("Usage: %s <video file>\n", argv[0]);
    return -1;
  }

  AVFormatContext *pFormatContext = avformat_alloc_context();
  if (!pFormatContext) {
    printf("ERROR: Could not allocate memory for Format Context\n");
    return -1;
  }

  if (avformat_open_input(&pFormatContext, argv[1], NULL, NULL) != 0) {
    printf("ERROR: Could not open the file\n");
    return -1;
  }

  if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
    printf("ERROR: Could not get the stream info\n");
    return -1;
  }

  AVCodec *pCodec = NULL;
  AVCodecParameters *pCodecParameters = NULL;
  int video_stream_index = -1;

  for (int i = 0; i < pFormatContext->nb_streams; i++) {
    AVCodecParameters *pLocalCodecParameters =
        pFormatContext->streams[i]->codecpar;
    AVCodec *pLocalCodec =
        avcodec_find_decoder(pLocalCodecParameters->codec_id);

    if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_index = i;
      pCodec = pLocalCodec;
      pCodecParameters = pLocalCodecParameters;

      printf("Video Codec: resolution %d x %d\n", pLocalCodecParameters->width,
             pLocalCodecParameters->height);
      break;
    }
  }

  if (video_stream_index == -1) {
    printf("ERROR: Could not find a video stream in the file\n");
    return -1;
  }

  AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
  if (!pCodecContext) {
    printf("ERROR: failed to allocated memory for AVCodecContext\n");
    return -1;
  }

  if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0) {
    printf("ERROR: failed to copy codec params to codec context\n");
    return -1;
  }

  if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
    printf("ERROR: failed to open codec through avcodec_open2\n");
    return -1;
  }

  AVFrame *pFrame = av_frame_alloc();
  AVPacket *pPacket = av_packet_alloc();

  while (av_read_frame(pFormatContext, pPacket) >= 0) {
    if (pPacket->stream_index == video_stream_index) {
      int response = avcodec_send_packet(pCodecContext, pPacket);
      if (response < 0) {
        printf("ERROR: Failed to decode packet\n");
        continue;
      }

      response = avcodec_receive_frame(pCodecContext, pFrame);
      if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
        continue;
      } else if (response < 0) {
        printf("ERROR: Failed to receive frame\n");
        return -1;
      }

      // Allocate device memory and copy frame data
      uint8_t *d_frame;
      int frame_size = pFrame->width * pFrame->height;
      cudaMalloc((void **)&d_frame, frame_size);
      cudaMemcpy(d_frame, pFrame->data[0], frame_size, cudaMemcpyHostToDevice);

      // Define block and grid sizes
      dim3 threadsPerBlock(16, 16);
      dim3 numBlocks(
          (pFrame->width + threadsPerBlock.x - 1) / threadsPerBlock.x,
          (pFrame->height + threadsPerBlock.y - 1) / threadsPerBlock.y);

      // Launch CUDA kernel
      process_frame<<<numBlocks, threadsPerBlock>>>(d_frame, pFrame->width,
                                                    pFrame->height);
      cudaDeviceSynchronize();

      // Copy processed data back to host
      cudaMemcpy(pFrame->data[0], d_frame, frame_size, cudaMemcpyDeviceToHost);

      // Here you can save or display the processed frame
      printf("Processed Frame %d (type=%c, size=%d bytes) pts %ld key_frame %d "
             "[DTS %d]\n",
             pCodecContext->frame_number,
             av_get_picture_type_char(pFrame->pict_type), pFrame->pkt_size,
             pFrame->pts, pFrame->key_frame, pFrame->coded_picture_number);

      // Free device memory
      cudaFree(d_frame);
    }
    av_packet_unref(pPacket);
  }

  av_frame_free(&pFrame);
  av_packet_free(&pPacket);
  avcodec_free_context(&pCodecContext);
  avformat_close_input(&pFormatContext);
  avformat_free_context(pFormatContext);

  return 0;
}

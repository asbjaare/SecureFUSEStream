#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

int main(int argc, char *argv[]) {
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

  // Loop through the streams and find the video stream
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

      // Here you can process the frame (e.g., save it as an image)
      // For simplicity, let's just count frames
      printf(
          "Frame %d (type=%c, size=%d bytes) pts %ld key_frame %d [DTS %d]\n",
          pCodecContext->frame_number,
          av_get_picture_type_char(pFrame->pict_type), pFrame->pkt_size,
          pFrame->pts, pFrame->key_frame, pFrame->coded_picture_number);
    }
    av_packet_unref(pPacket);
  }

  av_packet_free(&pPacket);
  avcodec_free_context(&pCodecContext);
  avformat_close_input(&pFormatContext);
  avformat_free_context(pFormatContext);

  return 0;
}

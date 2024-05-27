# README

## C Program: Extract Frames using FFmpeg

### Description
This program uses FFmpeg to read a video file and extract frame information, which is then written to a text file (`frame_info.txt`).

### Prerequisites
- FFmpeg library installed on your system.
- GCC compiler.

### Installation

1. **Install FFmpeg:**

   On Ubuntu:
   ```bash
   sudo apt-get update
   sudo apt-get install ffmpeg libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
   ```

2. **Compile the C Program:**

   Save the following code to `extract_frames.c`:

   ```c
   #include <stdio.h>
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

       for (int i = 0; i < pFormatContext->nb_streams; i++) {
           AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
           AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

           if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
               video_stream_index = i;
               pCodec = pLocalCodec;
               pCodecParameters = pLocalCodecParameters;

               printf("Video Codec: resolution %d x %d\n", pLocalCodecParameters->width, pLocalCodecParameters->height);
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

       // Open file to write frame information
       FILE *file = fopen("frame_info.txt", "w");
       if (file == NULL) {
           printf("ERROR: Could not open file to write frame information\n");
           return -1;
       }

       while (av_read_frame(pFormatContext, pPacket) >= 0) {
           if (pPacket->stream_index == video_stream_index) {
               int response = avcodec_send_packet(pCodecContext, pPacket);
               if (response < 0) {
                   fprintf(file, "ERROR: Failed to decode packet\n");
                   continue;
               }

               response = avcodec_receive_frame(pCodecContext, pFrame);
               if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                   continue;
               } else if (response < 0) {
                   fprintf(file, "ERROR: Failed to receive frame\n");
                   fclose(file);
                   return -1;
               }

               // Write frame information to file
               fprintf(file, "Frame %d (type=%c, size=%d bytes) pts %ld key_frame %d [DTS %d]\n",
                       pCodecContext->frame_number,
                       av_get_picture_type_char(pFrame->pict_type), pFrame->pkt_size,
                       pFrame->pts, pFrame->key_frame, pFrame->coded_picture_number);
           }
           av_packet_unref(pPacket);
       }

       // Close the file
       fclose(file);
       av_frame_free(&pFrame);
       av_packet_free(&pPacket);
       avcodec_free_context(&pCodecContext);
       avformat_close_input(&pFormatContext);
       avformat_free_context(pFormatContext);

       return 0;
   }
   ```

   Compile the program:
   ```bash
   gcc -o extract_frames extract_frames.c -lavformat -lavcodec -lavutil -lswscale
   ```

3. **Run the Program:**
   ```bash
   ./extract_frames your_video_file.mp4
   ```

## CUDA Program: Process Frames using CUDA and FFmpeg

### Description
This program uses FFmpeg to read a video file, processes each frame using CUDA, and then writes the frame information to the console.

### Prerequisites
- FFmpeg library installed on your system.
- CUDA Toolkit installed.
- CMake and a C++ compiler that supports CUDA (like `nvcc`).

### Installation

1. **Install FFmpeg:**

   On Ubuntu:
   ```bash
   sudo apt-get update
   sudo apt-get install ffmpeg libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
   ```

2. **Install CUDA Toolkit:**
   - Follow the installation instructions on the NVIDIA CUDA Toolkit website: https://developer.nvidia.com/cuda-downloads

3. **Project Structure:**

   Create the following files:
   - `CMakeLists.txt`
   - `main.cu`

4. **CMakeLists.txt:**

   ```cmake
   cmake_minimum_required(VERSION 3.8)
   project(cuda_ffmpeg_example)

   find_package(CUDA REQUIRED)
   include_directories(${CUDA_INCLUDE_DIRS})

   add_executable(cuda_ffmpeg_example main.cu)
   target_link_libraries(cuda_ffmpeg_example avcodec avformat avutil swscale ${CUDA_LIBRARIES})
   ```

5. **main.cu:**

   ```cpp
   #include <stdio.h>
   #include <cuda_runtime.h>
   #include <libavcodec/avcodec.h>
   #include <libavformat/avformat.h>
   #include <libswscale/swscale.h>

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
           AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
           AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

           if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
               video_stream_index = i;
               pCodec = pLocalCodec;
               pCodecParameters = pLocalCodecParameters;

               printf("Video Codec: resolution %d x %d\n", pLocalCodecParameters->width, pLocalCodecParameters->height);
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

       if (avcodec_open2(pCodecContext, pCodec, NULL) <

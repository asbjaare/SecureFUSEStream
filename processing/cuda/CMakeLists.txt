cmake_minimum_required(VERSION 3.8)
project(cuda_ffmpeg_example)

find_package(CUDA REQUIRED)
include_directories(${CUDA_INCLUDE_DIRS})

add_executable(cuda_ffmpeg_example main.cu)
target_link_libraries(cuda_ffmpeg_example avcodec avformat avutil swscale ${CUDA_LIBRARIES})


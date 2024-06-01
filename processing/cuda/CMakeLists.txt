cmake_minimum_required(VERSION 3.8)

# Specify the CUDA compiler
set(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc) # Adjust the path to your nvcc

# Project name and languages
project(cuda_grayscale LANGUAGES CXX CUDA)

# Find the CUDA package
find_package(CUDA REQUIRED)
include_directories(${CUDA_INCLUDE_DIRS})

# Find the OpenCV package
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Add the path to your FFmpeg include directories
include_directories(/usr/include)

# Add the path to your FFmpeg library directories
link_directories(/usr/lib/x86_64-linux-gnu)

# Add the CUDA executable
add_executable(cuda_grayscale main.cu)

# Link the necessary FFmpeg and OpenCV libraries
target_link_libraries(cuda_grayscale
    avcodec
    avformat
    avutil
    swscale
    ${CUDA_LIBRARIES}
    ${OpenCV_LIBS}
)


cmake_minimum_required(VERSION 3.4.1)

project(ffmpeg-learn)

set(CMAKE_CXX_STANDARD 14)
set(FFMPEG_DIR /opt/homebrew/Cellar/ffmpeg@4/4.4.2_3)

set(SRC_DIR ${PROJECT_SOURCE_DIR}/cppsrc)
set(COMMON_DIR ${PROJECT_SOURCE_DIR}/cppcommon)

aux_source_directory(${SRC_DIR} cppsrc)
aux_source_directory(${COMMON_DIR} common)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -fexceptions -frtti")
message(${SRC_DIR})
include_directories(${SRC_DIR})
include_directories(${COMMON_DIR})

# import ffmpeg library
include_directories(${FFMPEG_DIR}/include/)
link_directories(${FFMPEG_DIR}/lib/)


add_executable(
        ${PROJECT_NAME}
        ${cppsrc}
        ${common}
        example.cpp
)

target_link_libraries(
        ${PROJECT_NAME}
        avformat avcodec avfilter avutil swresample swscale
)
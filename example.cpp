//
// Created by juliswang on 2023/5/4.
//

#include "yuv_converter.h"
#include "video_to_jpeg.h"

#ifdef _WIN32
#include "direct.h"
#elif __APPLE__

#include <sys/stat.h>

#endif

int main(int argc, const char *argv[]) {
    string curFile(__FILE__);
    unsigned long pos = curFile.find("example");
    if (pos == string::npos) {
        return 0;
    }
    string resourceDir = curFile.substr(0, pos) + "resources/";
    string outputDir = curFile.substr(0, pos) + "output/";

    string yuv_path = resourceDir + "test_yuv_176x144.yuv";
    string test_mp4 = resourceDir + "test_mp4.mp4";
    string dst_path = outputDir + "yuv_2_mp4.mp4";

    video_to_jpeg videoToJpeg;
    string jpeg_dir = outputDir + "video2jpeg";
#ifdef _WIN32
    mkdir("../test/test_files/frame");
    mkdir("../test/test_files/output");
#elif __APPLE__
    mkdir(jpeg_dir.c_str(), 0775);
#endif
    videoToJpeg.run(test_mp4, jpeg_dir);
}

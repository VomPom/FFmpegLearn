//
// Created by juliswang on 2023/5/4.
//

#include "yuv_converter.h"
#include "video_to_jpeg.h"
#include "demuxing_decoding.h"
#include "hw_decode.h"

#ifdef _WIN32
#include "direct.h"
#elif __APPLE__

#include <sys/stat.h>

#endif

void testVideoToJpeg(const string &test_mp4, const string &outputDir) {
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

    string out_mp4_path = outputDir + "yuv_2_mp4.mp4";
    string out_pcm_path = outputDir + "video_extract_pcm.pcm";
    string out_yuv_path = outputDir + "video_extract_yuv.yuv";

    int testCase = 3;
    switch (testCase) {
        case 1:
            testVideoToJpeg(test_mp4, outputDir);
            break;
        case 2:
            YuvConverter yuvConverter;
            yuvConverter.yuv2mp4(yuv_path, out_mp4_path, 176, 144);
            break;
        case 3:
            hw_decode hwDecode;
            // "videotoolbox" for mac...
            hwDecode.run(test_mp4, outputDir, "videotoolbox");
            break;
        case 4:
            demuxing_decoding demuxingDecoding;
            demuxingDecoding.run(test_mp4, outputDir);
            break;

    }
}

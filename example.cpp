//
// Created by juliswang on 2023/5/4.
//

#include "yuv_converter.h"
#include "video_to_jpeg.h"
#include "demuxing_decoding.h"
#include "hw_decode.h"
#include "jpeg_to_video.h"
#include "encode_video.h"
#include "muxer.h"

#ifdef _WIN32
#include "direct.h"
#elif __APPLE__

#include <sys/stat.h>

#endif

void testVideoToJpeg(const string &test_mp4, const string &outputDir) {
    video_to_jpeg videoToJpeg;
    string jpeg_dir = outputDir + "video2jpeg";

#ifdef _WIN32
    mkdir(outputDir);
#elif __APPLE__
    mkdir(outputDir.c_str(), 0775);
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

#ifdef _WIN32
    mkdir(outputDir);
#elif __APPLE__
    mkdir(outputDir.c_str(), 0775);
#endif

    string yuv_path = resourceDir + "test_yuv_176x144.yuv";
    string test_mp4 = resourceDir + "test_mp4.mp4";

    int testCase = 6;
    switch (testCase) {
        // 视频文件转图片
        case 1: {
            testVideoToJpeg(test_mp4, outputDir);
            break;
        }
            // 实现yuv转mp4文件
        case 2: {
            YuvConverter::yuv2mp4(yuv_path, outputDir + "2-yuv_2_mp4.mp4", 176, 144);
            break;
        }
            // 实现硬解码
        case 3: {
            // "videotoolbox" for mac...
            hw_decode::run(test_mp4, outputDir, "videotoolbox");
            break;
        }
            // 分离mp4文件为yuv和pcm数据
        case 4: {
            demuxing_decoding demuxingDecoding;
            demuxingDecoding.run(test_mp4, outputDir);
            break;
        }
            // 图片生成视频
        case 5: {
            //run this before run case 1;
            jpeg_to_video jpegToVideo{};
            jpegToVideo.jpeg_2_video(outputDir + "jpeg_2_video.mp4", outputDir + "video2jpeg" + "/frame-%d.jpg");
            break;
        }
        case 6: {
            encode_video encodeVideo;
            encodeVideo.run(outputDir + "6-encode_video.h264", "libx264");
            break;
        }
            // 实现通过代码生成的帧(视频和音频)数据编码成指定格式并写入输出文件的功能
        case 7: {
            muxer m;
            m.run(outputDir + "/7-muxer.mp4");
        }

    }
}

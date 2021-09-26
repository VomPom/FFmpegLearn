//
// Created by julis.wang on 2021/9/22.
//

#ifndef FFMPEGLEARN_SIMPLE_PLAYER_H
#define FFMPEGLEARN_SIMPLE_PLAYER_H

class simple_player {
public:
    static int extract_av_frame();

    static int yuv_to_jpeg();

    static int yuv_to_h264();
};


#endif //FFMPEGLEARN_SIMPLE_PLAYER_H

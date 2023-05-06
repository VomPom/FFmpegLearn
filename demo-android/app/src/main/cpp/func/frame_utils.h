//
// Created by julis.wang on 2021/10/8.
//

#ifndef FFMPEGLEARN_FRAME_UTILS_H
#define FFMPEGLEARN_FRAME_UTILS_H

#include "base_func.h"
#include "opencv2/core.hpp"
#include "opencv2/opencv.hpp"

class frame_utils {
public:
    static int fetch_frame(int thread_size, int task_index,
                           int start_ms, int end_ms, int step_ms,
                           char *video_file_path, char *save_path, bool accurate_seek);
};


#endif //FFMPEGLEARN_FRAME_UTILS_H

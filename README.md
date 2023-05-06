# FFmpegLearn

FFmpeg 学习代码，主要参照：

- 雷神的博客[(leixiaohua1020)的专栏](https://blog.csdn.net/leixiaohua1020)

- 官方[FFmpeg示例程序](https://github.com/FFmpeg/FFmpeg/tree/master/doc/examples)

## 主要有以下示例

- [demuxing_decoding.cpp]() 分离 Mp4 文件为 YUV 和 PCM
- [video_to_jpeg.cpp]() 实现 YUV 格式转 MP4

## 桌面端测试

在 `CMakelists.txt`修改本地的ffmpeg地址，

```c
set(FFMPEG_DIR your_ffmpeg_dir)
```

## Android 测试

用 Android Studio 打开 [demo-android]即可
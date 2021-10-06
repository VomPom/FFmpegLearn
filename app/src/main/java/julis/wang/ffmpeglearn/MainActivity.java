package julis.wang.ffmpeglearn;

import android.os.Bundle;

public class MainActivity extends BaseActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        System.loadLibrary("native-lib");
        findViewById(R.id.btn_extract).setOnClickListener(v -> simple_extract_frame());
        findViewById(R.id.btn_yuv_to_jpeg).setOnClickListener(v -> yuv_to_jpeg());
        findViewById(R.id.btn_yuv_to_h264).setOnClickListener(v -> yuv_to_h264());
        findViewById(R.id.btn_yuv_to_mp4).setOnClickListener(v -> yuv_to_video());
        findViewById(R.id.btn_demuxing).setOnClickListener(v -> demuxing());
        findViewById(R.id.btn_video_decode).setOnClickListener(v -> video_decode());
        findViewById(R.id.btn_hw_decode).setOnClickListener(v -> hardware_decode());

    }

    public native void hardware_decode();

    public native void video_decode();

    public native void demuxing();

    public native void simple_extract_frame();

    public native void yuv_to_jpeg();

    public native void yuv_to_h264();

    public native void yuv_to_video();
}
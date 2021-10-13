package julis.wang.ffmpeglearn;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends BaseActivity implements View.OnClickListener {
    private long startTime;
    private TextView tvCost;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        System.loadLibrary("native-lib");
        findViewById(R.id.btn_extract).setOnClickListener(this);
        findViewById(R.id.btn_yuv_to_jpeg).setOnClickListener(this);
        findViewById(R.id.btn_yuv_to_h264).setOnClickListener(this);
        findViewById(R.id.btn_yuv_to_mp4).setOnClickListener(this);
        findViewById(R.id.btn_demuxing).setOnClickListener(this);
        findViewById(R.id.btn_video_decode).setOnClickListener(this);
        findViewById(R.id.btn_hw_decode).setOnClickListener(this);
        findViewById(R.id.btn_filtering_video).setOnClickListener(this);
        findViewById(R.id.btn_frame_seek).setOnClickListener(this);
        findViewById(R.id.btn_video_to_jpeg).setOnClickListener(this);
        tvCost = findViewById(R.id.tv_cost_time);
    }

    public native void video_to_jpeg();

    public native void frame_seek(int index);

    public native void filtering_video();

    public native void hardware_decode();

    public native void video_decode();

    public native void demuxing();

    public native void simple_extract_frame();

    public native void yuv_to_jpeg();

    public native void yuv_to_h264();

    public native void yuv_to_video();

    @SuppressLint("NonConstantResourceId")
    @Override
    public void onClick(View v) {
        startTime = System.currentTimeMillis();
        int vId = v.getId();
        if (vId == R.id.btn_extract) {
            simple_extract_frame();
        } else if (vId == R.id.btn_yuv_to_jpeg) {
            yuv_to_jpeg();
        } else if (vId == R.id.btn_yuv_to_h264) {
            yuv_to_h264();
        } else if (vId == R.id.btn_yuv_to_mp4) {
            yuv_to_video();
        } else if (vId == R.id.btn_demuxing) {
            demuxing();
        } else if (vId == R.id.btn_video_decode) {
            video_decode();
        } else if (vId == R.id.btn_hw_decode) {
            hardware_decode();
        } else if (vId == R.id.btn_filtering_video) {
            filtering_video();
        } else if (vId == R.id.btn_frame_seek) {
            frameSeek();
        } else if (vId == R.id.btn_video_to_jpeg) {
            video_to_jpeg();
        } else {
            throw new IllegalStateException("Unexpected value: " + vId);
        }
        costTime();
    }

    private void frameSeek() {

        final String saveFramePath = "/storage/emulated/0/saveBitmaps";
        File file = new File(saveFramePath);
        if (!file.exists()) {
            boolean flag = file.mkdirs();
        }
        for (int i = 0; i < 1; i++) {
            int finalI = i;
            Thread thread = new Thread(new Runnable() {
                @Override
                public void run() {
                    frame_seek(finalI);
                    costTime();
                }
            });
            if (i == 0) {
                thread.setPriority(Thread.MAX_PRIORITY);
            }
            thread.start();
        }
    }

    private void costTime() {
        String message = "Cost time:" + (System.currentTimeMillis() - startTime) / 1000.0 + "s";
//        tvCost.setText(message);
        Log.e(TAG, message);
    }
}
package julis.wang.ffmpeglearn;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.media.MediaMetadataRetriever;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends BaseActivity implements View.OnClickListener {
    public static final String TEST_VIDEO_PATH = "/storage/emulated/0/lolm-25min.mp4"; //lolm-
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

    public native static void frame_seek(int thread_size, int task_index, int startMs, int endMS, int stepMs,
                                         String videoFileName, String saveFilePath, boolean accurateSeek);

    public native void video_to_jpeg();

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
            for (int i = 0; i < 4; i++) {
                int finalI = i;
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        frameSeek(finalI);
                    }
                }).start();
            }
        } else if (vId == R.id.btn_video_to_jpeg) {
            video_to_jpeg();
        } else {
            throw new IllegalStateException("Unexpected value: " + vId);
        }
        costTime();
    }

    public static final int THREAD_SIZE = 4;

    private void frameSeek(int index) {
        int timeLength = videoDuration(TEST_VIDEO_PATH);
        final String saveFramePath = "/storage/emulated/0/saveJpeg";
        File file = new File(saveFramePath);
        if (!file.exists()) {
            boolean flag = file.mkdirs();
        }

        for (int i = 0; i < THREAD_SIZE; i++) {
            int finalI = i;
            Thread thread = new Thread(new Runnable() {
                @Override
                public void run() {
                    Log.e(TAG, "index:" + finalI);
                    frame_seek(THREAD_SIZE, finalI, 0, 10000 * (index + 1),
                            500, TEST_VIDEO_PATH, saveFramePath, true);
                    costTime();
                }
            });
            if (i == 0) {
                thread.setPriority(Thread.MAX_PRIORITY);
            }
            thread.start();
        }
    }

    public static int videoDuration(String inputFilePath) {
        int duration = -1;
        MediaMetadataRetriever mmr = new MediaMetadataRetriever();
        mmr.setDataSource(inputFilePath);
        String durationStr = mmr.extractMetadata(android.media.MediaMetadataRetriever.METADATA_KEY_DURATION);

        if (!TextUtils.isEmpty(durationStr)) {
            duration = Integer.parseInt(durationStr);
        }
        return duration;
    }

    private void costTime() {
        String message = "Cost time:" + (System.currentTimeMillis() - startTime) / 1000.0 + "s";
//        tvCost.setText(message);
        Log.e(TAG, message);
    }
}
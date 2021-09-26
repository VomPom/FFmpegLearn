package julis.wang.ffmpeglearn;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        System.loadLibrary("native-lib");
        findViewById(R.id.btn_extract).setOnClickListener(v -> simple_extract_frame());
        findViewById(R.id.btn_yuv_to_jpeg).setOnClickListener(v -> yuv_to_jpeg());
        findViewById(R.id.btn_yuv_to_h264).setOnClickListener(v -> yuv_to_h264());
    }

    public native String simple_extract_frame();

    public native void yuv_to_jpeg();

    public native void yuv_to_h264();
}
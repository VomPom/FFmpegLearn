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
        TextView textView = findViewById(R.id.tv_test);
        findViewById(R.id.btn_click).setOnClickListener(v -> test());
    }

    public native String test();
}
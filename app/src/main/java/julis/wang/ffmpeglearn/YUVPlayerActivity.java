package julis.wang.ffmpeglearn;

import android.os.Bundle;

/*******************************************************
 *
 * Created by juliswang on 2022/01/05 22:56 
 *
 * Description : 
 *
 *
 *******************************************************/

public class YUVPlayerActivity extends BaseActivity {
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_yuv_player);
        init();
    }

    private void init() {
    }

}

package julis.wang.ffmpeglearn.yuv;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.SurfaceHolder;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/*******************************************************
 *
 * Created by juliswang on 2022/01/05 22:58 
 *
 * Description : 
 *
 *
 *******************************************************/

public class YuvPlayer extends GLSurfaceView implements Runnable, SurfaceHolder.Callback, GLSurfaceView.Renderer {

    private final static String PATH = "/sdcard/test_yuv.yuv";

    public YuvPlayer(Context context) {
        this(context, null);
    }

    public YuvPlayer(Context context, AttributeSet attrs) {
        super(context, attrs);
        setRenderer(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        new Thread(this).start();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {

    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {

    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {

    }

    @Override
    public void onDrawFrame(GL10 gl) {

    }

    @Override
    public void run() {
        loadYuv("/sdcard/test_yuv.yuv", getHolder().getSurface());
    }

    public native void loadYuv(String url, Object surface);

}

package julis.wang.ffmpeglearn;

import android.Manifest;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/*******************************************************
 *
 * Created by juliswang on 2021/10/05 09:51 
 *
 * Description : 
 *
 *
 *******************************************************/

public class BaseActivity extends AppCompatActivity {
    public static final String TAG = "JW-Activity";
    public static final int REQUEST_CODE = 207750;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        askForPermission();
        mkdir();
        copyFile();
    }

    public void askForPermission() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_PHONE_STATE) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            Log.w(TAG, "didnt get permission,ask for it!");
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.CAMERA, Manifest.permission.RECORD_AUDIO, Manifest.permission.READ_PHONE_STATE, Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    REQUEST_CODE);
        }
    }

    private void mkdir() {
        String folderPath = "/storage/emulated/0/ffmpegLearn";
        File file = new File(folderPath);
        if (!file.exists()) {
            boolean flag = file.mkdir();
            if (!flag) Log.w(TAG, "didn't mkdir for -> " + folderPath);
        }
    }

    private void copyFile() {
        AssetManager assetManager = getAssets();
        String[] testFile = {
                "test_h264.h264",
                "test_mp4.mp4",
                "test_yuv.yuv",
                "test_png.png",
        };
        for (String fileStr : testFile) {
            File file = new File(fileStr);
            if (!file.exists()) {
                copyAsset(assetManager, fileStr, "/storage/emulated/0/" + file);
            }
        }
    }

    public boolean copyAsset(AssetManager assetManager,
                             String fromAssetPath, String toPath) {
        InputStream in;
        OutputStream out;
        try {
            in = assetManager.open(fromAssetPath);
            new File(toPath).createNewFile();
            out = new FileOutputStream(toPath);
            byte[] buffer = new byte[1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            in.close();
            out.flush();
            out.close();
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

}

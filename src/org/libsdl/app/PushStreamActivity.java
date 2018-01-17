package org.libsdl.app;

import android.app.Activity;
import android.hardware.Camera;
import android.os.Bundle;

import java.util.List;

/**
 * Created by xWX371834 on 2017/8/14.
 */
public class PushStreamActivity extends Activity {

    private CameraView mCameraView;
    private Camera camera;

    private int mWidth;
    private int mHeight;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mCameraView = new CameraView(this);
        setContentView(mCameraView);
        camera = Camera.open();
        List<Camera.Size> supportedPictureSizes = camera.getParameters().getSupportedPictureSizes();
        List<Camera.Size> supportedPreviewSizes = camera.getParameters().getSupportedPreviewSizes();
        mWidth = 720;
        mHeight = 1280;
        init();
    }

    private void init() {
        pushStart(mWidth, mHeight, getIntent().getStringExtra("url"));
        mCameraView.initCamera(camera, mWidth, mHeight);
    }

    public static native void pushStart(int width, int height, String url);

    public static native void onPreviewFrame(byte[] data, int width, int height);
}

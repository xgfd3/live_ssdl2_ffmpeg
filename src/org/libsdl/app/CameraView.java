package org.libsdl.app;

import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.io.IOException;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * Created by xWX371834 on 2017/8/14.
 */
public class CameraView extends SurfaceView implements
        SurfaceHolder.Callback, Camera.PreviewCallback {

    private SurfaceHolder mHolder;
    private Camera mCamera;
    private int mWidth;
    private int mHeight;
    private int frameRate;
    private boolean isPreviewOn;

    public CameraView(Context context) {
        this(context, null);
    }

    public CameraView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public CameraView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        mHolder = getHolder();
        mHolder.addCallback(this);
        mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
    }

    /**
     * 初始化设置视频回调
     */
    public void initCamera(Camera camera, int width, int height) {
        mCamera = camera;
        mWidth = width;
        mHeight = height;
        Camera.Parameters parameters = mCamera.getParameters();
        parameters.setPreviewSize(height, width);
        parameters.setPreviewFormat(ImageFormat.YV12);
        parameters.setPreviewFrameRate(20);
        parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE);
        mCamera.setParameters(parameters);
        mCamera.setDisplayOrientation(90);
        mCamera.setPreviewCallback(this);
    }

    public void stopPreview() {
        if (isPreviewOn &&mCamera != null) {
            isPreviewOn = false;
            mCamera.stopPreview();
        }
    }

    public void startPreview(){
        if(!isPreviewOn && mCamera!=null){
            isPreviewOn = true;
            mCamera.startPreview();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        try {
            stopPreview();
            mCamera.setPreviewDisplay(holder);
            startPreview();
        } catch (IOException e) {
            mCamera.release();
            mCamera = null;
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, final int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        mHolder.addCallback(null);
        mCamera.setPreviewCallback(null);
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera){
        Camera.Size previewSize = camera.getParameters().getPreviewSize();
        PushStreamActivity.onPreviewFrame(data, previewSize.width, previewSize.height);
    }
}

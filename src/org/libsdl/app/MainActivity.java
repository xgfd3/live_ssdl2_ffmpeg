package org.libsdl.app;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;

/**
 * Created by xWX371834 on 2017/8/14.
 */
public class MainActivity extends Activity{

    // Load the .so
    static {

        //System.loadLibrary("SDL2_image");
        //System.loadLibrary("SDL2_mixer");
        //System.loadLibrary("SDL2_net");
        //System.loadLibrary("SDL2_ttf");
        System.loadLibrary("avutil-54");
        System.loadLibrary("postproc-53");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avdevice-56");

        System.loadLibrary("SDL2");

        System.loadLibrary("SDL2main");
    }

    private EditText tv_url;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        tv_url = (EditText) findViewById(R.id.tv_url);
        // tv_url.setText("/storage/emulated/0/testffmpeg.flv");
        tv_url.setText("/sdcard/testffmpeg.flv");
        // tv_url.setText("rtmp://10.132.29.47:1935/live/test");
    }


    public void onPushStream(View view){
        Intent intent = new Intent(this, PushStreamActivity.class);
        intent.putExtra("url", tv_url.getText().toString());
        startActivity(intent);
    }

    public void onPullStream(View view){
        Intent intent = new Intent(this, SDLActivity.class);
        intent.putExtra("type", 1);
        intent.putExtra("url", tv_url.getText().toString());
        startActivity(intent);
    }

}

/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.raibow.yamahaspk.app;

import android.annotation.TargetApi;
import android.app.ActionBar;
import android.app.Activity;
import android.content.AsyncQueryHandler;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ShareActionProvider;
import android.util.Log;

import android.media.AudioManager;
import android.media.IAudioService;
import android.content.Context;
import android.os.ServiceManager;
import android.os.RemoteException;

//bluetooth
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothProfile;

import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.content.Intent;
import android.os.Handler;
import android.os.Message;

import java.io.File;

import java.util.ArrayList;
import java.util.List;

import com.raibow.yamahaspk.R;
import com.raibow.yamahaspk.common.ApiHelper;
import com.raibow.yamahaspk.common.Utils;

/**
 * This activity plays a video from a specified URI.
 *
 * The client of this activity can pass a logo bitmap in the intent (KEY_LOGO_BITMAP)
 * to set the action bar logo so the playback process looks more seamlessly integrated with
 * the original activity.
 */
public class Demo1 extends Activity {
    @SuppressWarnings("unused")
    private static final String TAG = "Yamahaspk";
    public static final String KEY_LOGO_BITMAP = "logo-bitmap";
    public static final String KEY_TREAT_UP_AS_BACK = "treat-up-as-back";

    private MoviePlayer mPlayer;
    private AudioManager mAudioManager;
    private IAudioService mAudioService;

    private int mDefaultVolume;

    private Context mContext;

        static {
                System.loadLibrary("jni_yamahaspk");
        }   

    //bluetooth
    private BluetoothA2dp mA2dp;
//    private BluetoothManager mBluetoothManager;
    List<BluetoothDevice> mCachedBluetoothDevices;
    private boolean mIsA2dpConnected = true;

    private WakeLock mWakeLock;
    private static final int MSG_CONNECTION_STATE_CHANGED = 0;

    private boolean mFinishOnCompletion;
    private Uri mUri;
    private int mScreenWidth;
    private int mScreenHeight;
    private boolean mTreatUpAsBack;

    private String VIDEO_IPY = null;
    private String VIDEO_MM = null;

    private final String DEFAULT_VIDEO_IPY = "/system/vendor/video/IPLY_movie.mp4";
    private final String DEFAULT_VIDEO_MM = "/system/vendor/video/MusiccastMovie_movie.mp4";

    private final String VIDEO_IPY_PATH = "/sdcard/ipy";
    private final String VIDEO_MM_PATH  =  "/sdcard/mm";

    private int mCurrentPlayContent;
    private final int PLAYING_CONTENT_IPLY = 0;
    private final int PLAYING_CONTENT_MM  = 1;

    private int mCurrentAudioGain;
    private final int AUDIO_GAIN_20DB = 0;
    private final int AUDIO_GAIN_60DB = 1;

    private final int mVolumeIndexof20DB = 0;
    private final int mVolumeIndexof60DB = 11;
    private final int mVolumeIndexof60DB_A2dp = 8;

    private boolean mVolumeIsMute = true;
    private boolean mTimerStarted = false;
    private boolean isMMPlaying = false;

    private int timer_count = 0; 

    private int mCurrentAudioRoute;
    private final int AUDIO_JACKET1 = 0;
    private final int AUDIO_JACKET2 = 1;
    private final int AUDIO_BLUETOOTH = 2;
  
    private Handler mHandler = new Handler();

    native private void switchAudioJackets(int jacket);

    native private void resetLed();

    native private void stopLed();
    native private void startLed();	
    native private void startTimer();

    native private void init();

    native private void release();

//    native public static void continueLed();

//    native private void setTimerSeven();

    private void getVideoPath(){
	File file=new File(VIDEO_IPY_PATH);
	File[] tempList1 = file.listFiles();
	if (tempList1.length > 0){
	    VIDEO_IPY = VIDEO_IPY_PATH+tempList1[0];
	}else{
	    VIDEO_IPY = DEFAULT_VIDEO_IPY;
	}

	file=new File(VIDEO_MM_PATH);
	File[] tempList2 = file.listFiles();
	if (tempList2.length > 0){
	    VIDEO_MM = VIDEO_MM_PATH+tempList2[0];
	}else{
	    VIDEO_MM = DEFAULT_VIDEO_MM;
	}

	return;
    }

    private BluetoothProfile.ServiceListener mProfileServiceListener =
        new BluetoothProfile.ServiceListener() {
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            mA2dp = (BluetoothA2dp) proxy;
        }   
        public void onServiceDisconnected(int profile) {
            mA2dp = null;
        }   
    };  

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

	//native init
	init();

        setContentView(R.layout.movie_view);
        View rootView = findViewById(R.id.root);


		DisplayMetrics dm = getResources().getDisplayMetrics();
		mScreenWidth = dm.widthPixels;
		mScreenHeight = dm.heightPixels;

	mContext = this;

	mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
	mAudioService = IAudioService.Stub.asInterface(
				ServiceManager.checkService(Context.AUDIO_SERVICE));


	PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
	mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Yamahaspk");

	VIDEO_IPY = DEFAULT_VIDEO_IPY;
	VIDEO_MM = DEFAULT_VIDEO_MM;

//	getVideoPath();


        muteVolume(true);
	resetLed();

	//bluetooth manager, get a bluetooth Profile Manager instant
	BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
	if (adapter != null) {
		adapter.getProfileProxy(this.getApplicationContext(), mProfileServiceListener,
			BluetoothProfile.A2DP);
	}

	// some problem ?
	if (mA2dp != null){
	 	mCachedBluetoothDevices = mA2dp.getConnectedDevices();
	}

        Uri videoUri = Uri.parse(VIDEO_IPY);
        Intent intent = new Intent();     //constrcut a empty intent
		
        mPlayer = new MoviePlayer(rootView, this, videoUri, savedInstanceState,
                /*!mFinishOnCompletion*/true, mScreenWidth, mScreenHeight, intent) {
		@Override
		public void onCompletion() {
		    if (mCurrentPlayContent == PLAYING_CONTENT_MM) {
   				muteVolume(true);
			// switch video content to IPLY
			switchVideoContent(PLAYING_CONTENT_IPLY);
			isMMPlaying = false;
								
		    } else {
			super.onCompletion();
			if( super.toQuit() ){
			    finish();
			}
		    }
		}
	};

        if (intent.hasExtra(MediaStore.EXTRA_SCREEN_ORIENTATION)) {
            int orientation = intent.getIntExtra(
                    MediaStore.EXTRA_SCREEN_ORIENTATION,
                    ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
            if (orientation != getRequestedOrientation()) {
                setRequestedOrientation(orientation);
            }
        }
        Window win = getWindow();
        WindowManager.LayoutParams winParams = win.getAttributes();
        winParams.buttonBrightness = WindowManager.LayoutParams.BRIGHTNESS_OVERRIDE_NONE;
        winParams.screenBrightness = WindowManager.LayoutParams.BRIGHTNESS_OVERRIDE_NONE;
        //winParams.flags |= WindowManager.LayoutParams.FLAG_FULLSCREEN;
        win.setAttributes(winParams);

	mCurrentPlayContent = PLAYING_CONTENT_IPLY;


    }

    private void switchVideoContent(int videoContent){
	Log.d(TAG, "switchVideoContent:"+videoContent);
	mPlayer.pauseVideo();
	Uri videoUri = Uri.parse(VIDEO_IPY);
	if (videoContent == PLAYING_CONTENT_MM){
	    videoUri = Uri.parse(VIDEO_MM);
	    mCurrentPlayContent = PLAYING_CONTENT_MM;
	}else if (videoContent == PLAYING_CONTENT_IPLY){
	    mCurrentPlayContent = PLAYING_CONTENT_IPLY;
	}
	mPlayer.setVideoUri(videoUri);
	mPlayer.playVideo();
    }

   private void pauseVideo(){
	mPlayer.pauseVideo();
	return;
   }

   private void continueVideo(){
	mPlayer.playVideo();
	return;
   }

    private Runnable mA2dpVolumeRunnable = new Runnable() {
        @Override
        public void run() {
//            muteVolume(false);
	    resetA2dpVolume();
        }   
    };  


    private boolean isStreamMute(){
	return mAudioManager.isStreamMute(AudioManager.STREAM_MUSIC);
    }

    private void muteVolume(boolean mute) {
	Log.d(TAG, "muteVolume: " + mute);
//	mAudioManager.setStreamMute(AudioManager.STREAM_MUSIC, mute);
	mVolumeIsMute = mute; 
	if (mute){
	    mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, mVolumeIndexof20DB, /*AudioManager.FLAG_SHOW_UI*/0);
	}else{
	    mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, mVolumeIndexof60DB, /*AudioManager.FLAG_SHOW_UI*/0);
	}
    }

    private void resetA2dpVolume() {
	mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, /*mDefaultVolume*/mVolumeIndexof60DB_A2dp, 
				/*AudioManager.FLAG_SHOW_UI*/0);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)  { 
	switch(keyCode){
	case KeyEvent.KEYCODE_F1: //V+
	{
	     boolean shouldStartTimer = (mVolumeIsMute == true) && (isMMPlaying == false) && (mTimerStarted == false);
//	     Log.d(TAG, "shouldStartTimer: "+shouldStartTimer+" mVolumeIsMute: "+mVolumeIsMute+
//				" isMMPlaying: "+isMMPlaying+" mTimerStarted: "+ mTimerStarted);
	     if (shouldStartTimer){
	         //stop led
	         stopLed();
		 muteVolume(false);
		 startTimer();
		 mTimerStarted = true;
	     }else{
		muteVolume(false);
	     }
	     return true;
        }
	case KeyEvent.KEYCODE_F2: //V-
	{
             boolean shouldStartTimer = (mVolumeIsMute == true) && (isMMPlaying == false) && (mTimerStarted == false);
//             Log.d(TAG, "shouldStartTimer: "+shouldStartTimer+" mVolumeIsMute: "+mVolumeIsMute+
//                                " isMMPlaying: "+isMMPlaying+" mTimerStarted: "+ mTimerStarted);
             if (shouldStartTimer){
	        //stopLed
	        stopLed();
		 muteVolume(false);
	        //mute and continue LED after 7 seconds
	        startTimer();
		mTimerStarted = true;
	    }
	    return true;
	}
	case KeyEvent.KEYCODE_F3://PLAY
	    resetLed();
            //switch audio route to s1
 	    //switch video content to MM
	    switchVideoContent(PLAYING_CONTENT_MM);
	    isMMPlaying = true;
	    muteVolume(false);
	    mTimerStarted = false;
	    return true;
	case KeyEvent.KEYCODE_F4: //SPK1
	   if (!mTimerStarted){
	   resetLed();
	   muteVolume(true);
	   }
	   return true;
	case KeyEvent.KEYCODE_F7:  //local 7 seconds timer out
		muteVolume(true);
		startLed();
		mTimerStarted = false;
		timer_count = 0;
	default:
	   break;
	}
	return super.onKeyUp(keyCode, event);
    }

    @Override
    public void onStart() {
        ((AudioManager) getSystemService(AUDIO_SERVICE))
                .requestAudioFocus(null, AudioManager.STREAM_MUSIC,
                AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
        super.onStart();
    }

    @Override
    protected void onStop() {
        ((AudioManager) getSystemService(AUDIO_SERVICE))
                .abandonAudioFocus(null);
        super.onStop();
    }

    @Override
    public void onPause() {
        mPlayer.onPause();
        super.onPause();
    }

    @Override
    public void onResume() {
        mPlayer.onResume();
        super.onResume();
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        mPlayer.onSaveInstanceState(outState);
    }

    @Override
    public void onDestroy() {
	release();
        mPlayer.onDestroy();
        super.onDestroy();
    }

}

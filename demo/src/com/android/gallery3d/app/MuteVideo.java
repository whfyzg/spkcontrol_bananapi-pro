/*
 * Copyright (C) 2012 The Android Open Source Project
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

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.net.Uri;
import android.os.Handler;
import android.provider.MediaStore;
import android.widget.Toast;

import com.raibow.yamahaspk.R;
import com.raibow.yamahaspk.data.MediaItem;
import com.raibow.yamahaspk.util.SaveVideoFileInfo;
import com.raibow.yamahaspk.util.SaveVideoFileUtils;

import java.io.IOException;
import android.os.Message;

public class MuteVideo {

    private ProgressDialog mMuteProgress;

    private String mFilePath = null;
    private Uri mUri = null;
    private SaveVideoFileInfo mDstFileInfo = null;
    private Activity mActivity = null;
	private final int VIDEO_MUTE_ERROR = -1;
	private final int VIDEO_SAVE = -2;
    private final Handler mHandler = new Handler() {
			@Override
			public void handleMessage (Message msg) {
				switch (msg.what) {
					case VIDEO_MUTE_ERROR:
						    Toast.makeText(mActivity, mActivity.getString(R.string.video_mute_err),
                            	Toast.LENGTH_SHORT).show();
							if (mMuteProgress!=null)
								mMuteProgress.dismiss();
						break;
					case VIDEO_SAVE:
						Toast.makeText(mActivity.getApplicationContext(),
                                mActivity.getString(R.string.save_into,
                                        mDstFileInfo.mFolderName),
                                Toast.LENGTH_SHORT)
                                .show();
						break;
					default:
						if (mMuteProgress!=null)
								mMuteProgress.dismiss();
						break;
				}
		}
	};

    final String TIME_STAMP_NAME = "'MUTE'_yyyyMMdd_HHmmss";

    public MuteVideo(String filePath, Uri uri, Activity activity) {
        mUri = uri;
        mFilePath = filePath;
        mActivity = activity;
    }

    public void muteInBackground() {
        mDstFileInfo = SaveVideoFileUtils.getDstMp4FileInfo(TIME_STAMP_NAME,
                mActivity.getContentResolver(), mUri,
                mActivity.getString(R.string.folder_download));

        showProgressDialog();
        new Thread(new Runnable() {
                @Override
            public void run() {
                try {
                    VideoUtils.startMute(mFilePath, mDstFileInfo);
                    SaveVideoFileUtils.insertContent(
                            mDstFileInfo, mActivity.getContentResolver(), mUri);
                } catch (Exception e) {
                    Message msg = Message.obtain();
					msg.what = VIDEO_MUTE_ERROR;
					mHandler.sendMessage(msg);
					return;
                }
                // After muting is done, trigger the UI changed.
                mHandler.post(new Runnable() {
                        @Override
                    public void run() {
                        Message msg = Message.obtain();
						msg.what = VIDEO_SAVE;
						mHandler.sendMessage(msg);
                        if (mMuteProgress != null) {
                            mMuteProgress.dismiss();
                            mMuteProgress = null;

                            // Show the result only when the activity not
                            // stopped.
                            Intent intent = new Intent(android.content.Intent.ACTION_VIEW);
                            intent.setDataAndType(Uri.fromFile(mDstFileInfo.mFile), "video/*");
                            intent.putExtra(MediaStore.EXTRA_FINISH_ON_COMPLETION, false);
                            mActivity.startActivity(intent);
                        }
                    }
                });
            }
        }).start();
    }

    private void showProgressDialog() {
        mMuteProgress = new ProgressDialog(mActivity);
        mMuteProgress.setTitle(mActivity.getString(R.string.muting));
        mMuteProgress.setMessage(mActivity.getString(R.string.please_wait));
        mMuteProgress.setCancelable(false);
        mMuteProgress.setCanceledOnTouchOutside(false);
        mMuteProgress.show();
    }
}

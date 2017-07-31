/*
 * RetroShare Android QML App
 * Copyright (C) 2016-2017  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.retroshare.android.qml_app;

import android.app.ActivityManager;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;

import android.net.Uri;

import org.qtproject.qt5.android.bindings.QtActivity;

import org.retroshare.android.qml_app.jni.NativeCalls;

public class RetroShareQmlActivity extends QtActivity
{

	static final int PICK_PHOTO = 1;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		Log.i("RetroShareQmlActivity", "onCreate()");

		if (!isMyServiceRunning(RetroShareAndroidService.class))
		{
			Log.i("RetroShareQmlActivity", "onCreate(): RetroShareAndroidService is not running, let's start it by Intent");
			Intent rsIntent = new Intent(this, RetroShareAndroidService.class);
			startService(rsIntent);
		}
		else Log.v("RetroShareQmlActivity", "onCreate(): RetroShareAndroidService already running");

		if (!isMyServiceRunning(RetroShareAndroidNotifyService.class))
		{
			Log.i("RetroShareQmlActivity", "onCreate(): RetroShareAndroidNotifyService is not running, let's start it by Intent");
			Intent rsIntent = new Intent(this, RetroShareAndroidNotifyService.class);
			startService(rsIntent);
		}
		else Log.v("RetroShareQmlActivity", "onCreate(): RetroShareAndroidNotifyService already running");

		super.onCreate(savedInstanceState);
	}

	@Override
	public void onNewIntent(Intent intent)
	{
		Log.i("RetroShareQmlActivity", "on	NewIntent(Intent intent)");

		super.onNewIntent(intent);

		String uri = intent.getDataString();
		if (uri != null)
		{
			NativeCalls.notifyIntentUri(uri);
			Log.i("RetroShareQmlActivity", "onNewIntent(Intent intent) Uri: " + uri);
		}
	}

	@UsedByNativeCode @SuppressWarnings("unused")
	public void shareUrl(String urlStr)
	{
		Intent shareIntent = new Intent()
				.setAction(Intent.ACTION_SEND)
				.putExtra(Intent.EXTRA_TEXT, urlStr)
				.setType("text/plain");
		startActivity(Intent.createChooser(shareIntent,"")); // TODO: Need proper title?
	}


	private boolean isMyServiceRunning(Class<?> serviceClass)
	{
		ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
		for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE))
			if (serviceClass.getName().equals(service.service.getClassName()))
				return true;
		return false;
	}

	private  Uri capturedImageURI;

	public void openImagePicker()
	{
		Log.i("RetroShareQmlActivity", "openImagePicker()");

		Intent pickIntent = new Intent(Intent.ACTION_PICK, android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
		pickIntent.setType("image/*");

		ContentValues values = new ContentValues();
		values.put(MediaStore.Images.Media.TITLE, "Retroshare Avatar");
		capturedImageURI = getContentResolver().insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values);
		Intent takePicture = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
		takePicture.putExtra(MediaStore.EXTRA_OUTPUT, capturedImageURI);

		Intent chooserIntent = Intent.createChooser(pickIntent, "Select Image");
		chooserIntent.putExtra(Intent.EXTRA_INITIAL_INTENTS, new Intent[] {takePicture});

		startActivityForResult( chooserIntent, PICK_PHOTO);
	};

	public void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		Log.i("RetroShareQmlActivity", "onActivityResult()" + String.valueOf(requestCode));

        if (resultCode == RESULT_OK)
		{
			if (requestCode == PICK_PHOTO)
			{
				final boolean isCamera;

				if (data == null)
				{
					isCamera = true;
				}
				else
				{
					final String action = data.getAction();
					if (action == null)
					{
						isCamera = false;
					}
					else
					{
						isCamera = action.equals(android.provider.MediaStore.ACTION_IMAGE_CAPTURE);
					}
				}

				Uri selectedImageUri;
				if (isCamera)
				{
					selectedImageUri = capturedImageURI;
				}
				else
				{
					selectedImageUri = data == null ? null : data.getData();
				}

				String uri = getRealPathFromURI(selectedImageUri);
				if (uri != null)
				{
					Log.i("RetroShareQmlActivity", "Image path from uri found!" + uri);
					NativeCalls.notifyIntentUri("//file"+uri); // Add the authority for get it on qml code
				}
			}
        }
	}

	public String getRealPathFromURI(Uri uri) {
		String[] projection = { MediaStore.Images.Media.DATA };
		@SuppressWarnings("deprecation")
		Cursor cursor = managedQuery(uri, projection, null, null, null);
		int column_index = cursor
				.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
		cursor.moveToFirst();
		String result = cursor.getString(column_index);
		return result;
	}





}

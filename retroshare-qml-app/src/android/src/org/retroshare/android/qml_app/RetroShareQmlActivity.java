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
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import org.qtproject.qt5.android.bindings.QtActivity;

import org.retroshare.android.qml_app.jni.NativeCalls;

public class RetroShareQmlActivity extends QtActivity
{
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
		Log.i("RetroShareQmlActivity", "onNewIntent(Intent intent)");

		super.onNewIntent(intent);

		String uri = intent.getDataString();
		if (uri != null) NativeCalls.notifyIntentUri(uri);
	}

	private boolean isMyServiceRunning(Class<?> serviceClass)
	{
		ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
		for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE))
			if (serviceClass.getName().equals(service.service.getClassName()))
				return true;
		return false;
	}
}

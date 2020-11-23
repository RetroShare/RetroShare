/*
 * RetroShare
 * Copyright (C) 2016-2018  Gioacchino Mazzurco <gio@altermundi.net>
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
 *
 * SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

package org.retroshare.service;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;


public class RetroShareServiceControlActivity extends Activity
{
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		setContentView(R.layout.retroshare_service_control_layout);

		final Button button = (Button) findViewById(R.id.startStopButton);
		button.setOnClickListener(new View.OnClickListener()
		{
			public void onClick(View v)
			{
				if (RetroShareServiceAndroid.isRunning(RetroShareServiceControlActivity.this))
				{
					serviceStarting = false;
					serviceStopping = true;
					button.setText("Stopping...");
					RetroShareServiceAndroid.stop(RetroShareServiceControlActivity.this);
				}
				else
				{
					button.setText("Starting...");
					RetroShareServiceAndroid.start(RetroShareServiceControlActivity.this);
					serviceStarting = true;
					serviceStopping = false;
				}
				mStatusUpdateHandler.postDelayed(mUpdateStatusTask, 500);
			}
		});

		super.onCreate(savedInstanceState);
	}

	@Override
	public void onResume()
	{
		super.onResume();

		final Button button = (Button) findViewById(R.id.startStopButton);
		button.setText(RetroShareServiceAndroid.isRunning(this) ? "Stop" : "Start");

		mStatusUpdateHandler.removeCallbacks(mUpdateStatusTask);
		mStatusUpdateHandler.postDelayed(mUpdateStatusTask, 500);
	}

	@Override
	public void onPause()
	{
		super.onPause();
		mStatusUpdateHandler.removeCallbacks(mUpdateStatusTask);
	}

	public void updateServiceStatus()
	{
		final Button button = (Button) findViewById(R.id.startStopButton);

		if(serviceStarting && RetroShareServiceAndroid.isRunning(this))
		{
			mStatusUpdateHandler.removeCallbacks(mUpdateStatusTask);
			serviceStarting = false;
			serviceStopping = false;
			button.setText("Stop");
		}
		else if (serviceStopping && !RetroShareServiceAndroid.isRunning(this))
		{
			mStatusUpdateHandler.removeCallbacks(mUpdateStatusTask);
			serviceStarting = false;
			serviceStopping = false;
			button.setText("Start");
		}
		else if(serviceStarting || serviceStopping)
			mStatusUpdateHandler.postDelayed(mUpdateStatusTask, 500);
	}

	private Runnable mUpdateStatusTask = new Runnable()
	{ public void run() { updateServiceStatus(); } };

	private boolean serviceStarting = false;
	private boolean serviceStopping = false;
	private Handler mStatusUpdateHandler = new Handler();
}

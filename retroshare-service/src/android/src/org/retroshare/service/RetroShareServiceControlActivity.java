/*
 * RetroShare
 * Copyright (C) 2016-2018  Gioacchino Mazzurco <gio@eigenlab.org>
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

package org.retroshare.service;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import org.retroshare.service.R;


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
                    RetroShareServiceAndroid.stop(RetroShareServiceControlActivity.this);
                    button.setText("Start");
                }
                else
                {
                    RetroShareServiceAndroid.start(RetroShareServiceControlActivity.this);
                    button.setText("Stop");
                }
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
    }
}

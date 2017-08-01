/*
 * RetroShare Android Service
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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class BootCompletedReceiver extends BroadcastReceiver
{
	@Override
	public void onReceive(Context context, Intent intent)
	{
		Log.i("BootCompletedReceiver", "onReceive() Starting RetroShareAndroidService on boot");
		Intent coreIntent = new Intent(context, RetroShareAndroidService.class);
		context.startService(coreIntent);

		Log.i("BootCompletedReceiver", "onReceive() Starting RetroShareAndroidNotifyService on boot");
		Intent nsIntent = new Intent(context, RetroShareAndroidNotifyService.class);
		context.startService(nsIntent);
	}
}

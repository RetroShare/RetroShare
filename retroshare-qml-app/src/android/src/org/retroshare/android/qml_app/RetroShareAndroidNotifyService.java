/*
 * RetroShare Android Service
 * Copyright (C) 2017  Gioacchino Mazzurco <gio@eigenlab.org>
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

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.media.RingtoneManager;
import android.net.Uri;

import org.qtproject.qt5.android.bindings.QtService;

public class RetroShareAndroidNotifyService extends QtService
{
	@UsedByNativeCode @SuppressWarnings("unused")
	public void notify(String title, String text, String uri)
	{
		Notification.Builder mBuilder = new Notification.Builder(this);
		mBuilder.setSmallIcon(R.drawable.retroshare_48x48)
				.setContentTitle(title)
				.setContentText(text)
				.setAutoCancel(true)
				.setSound(RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION));

		Intent intent = new Intent(this, RetroShareQmlActivity.class);
		intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
		intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);

		if(!uri.isEmpty()) intent.setData(Uri.parse(uri));

		PendingIntent pendingIntent = PendingIntent.getActivity(
				this, NOTIFY_REQ_CODE, intent,
				PendingIntent.FLAG_UPDATE_CURRENT);

		mBuilder.setContentIntent(pendingIntent);
		NotificationManager mNotificationManager =
				(NotificationManager) this.getSystemService(Context.NOTIFICATION_SERVICE);
		mNotificationManager.notify(0, mBuilder.build());
	}

	/** Must not be 0 otherwise a new activity may be created when should not
	 * (ex. the activity is already visible/on top) and deadlocks happens */
	private static final int NOTIFY_REQ_CODE = 2173;
}

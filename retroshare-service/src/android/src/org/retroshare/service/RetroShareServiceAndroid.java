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

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;

import org.qtproject.qt5.android.bindings.QtService;

public class RetroShareServiceAndroid extends QtService
{
    public static void start(Context ctx)
    {
        ctx.startService(new Intent(ctx, RetroShareServiceAndroid.class));
    }

    public static void stop(Context ctx)
    {
        ctx.stopService(new Intent(ctx, RetroShareServiceAndroid.class));
    }

    public static boolean isRunning(Context ctx)
    {
        ActivityManager manager = (ActivityManager) ctx.getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE))
            if (RetroShareServiceAndroid.class.getName().equals(service.service.getClassName()))
                return true;
        return false;
    }
}

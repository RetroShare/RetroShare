/*
 * RetroShare
 *
 * Copyright (C) 2016-2022  Gioacchino Mazzurco <gio@eigenlab.org>
 * Copyright (C) 2021-2022  Asociación Civil Altermundi <info@altermundi.net>
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

import android.app.Service;
import android.os.IBinder;
import android.os.Bundle;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.app.ActivityManager;


public class RetroShareServiceAndroid extends Service
{
    public static final int DEFAULT_JSON_API_PORT = 9092;
    public static final String DEFAULT_JSON_API_BINDING_ADDRESS = "127.0.0.1";

    static { System.loadLibrary("retroshare"); }

    public static void start(
        Context ctx, int jsonApiPort, String jsonApiBindAddress )
    {
        Log.d(TAG, "start");
        Intent intent = new Intent(ctx, RetroShareServiceAndroid.class);
        intent.putExtra(JSON_API_PORT_KEY, jsonApiPort);
        intent.putExtra(JSON_API_BIND_ADDRESS_KEY, jsonApiBindAddress);
        ctx.startService(intent);
    }

    public static void stop(Context ctx)
    {
        Log.d(TAG, "stop");
        ctx.stopService(new Intent(ctx, RetroShareServiceAndroid.class));
    }

    public static boolean isRunning(Context ctx)
    {
        Log.d(TAG, "isRunning");
        ActivityManager manager =
            (ActivityManager) ctx.getSystemService(Context.ACTIVITY_SERVICE);
        for( ActivityManager.RunningServiceInfo service :
            manager.getRunningServices(Integer.MAX_VALUE) )
            if( RetroShareServiceAndroid.class.getName()
                            .equals(service.service.getClassName()) )
                return true;
        return false;
    }

    public static Context getServiceContext()
    {
        if(sServiceContext == null)
        {
            Log.e(TAG, "getServiceContext() called before onCreate");
            throw new NullPointerException();
        }
        return sServiceContext;
    }

    @Override
    public int onStartCommand(
        Intent intent, int flags, int startId )
    {
        if(intent == null)
        {
            Log.i(TAG, "onStartCommand called without intent");
            return Service.START_REDELIVER_INTENT;
        }

        int jsonApiPort = DEFAULT_JSON_API_PORT;
        String jsonApiBindAddress = DEFAULT_JSON_API_BINDING_ADDRESS;

        Bundle args = intent.getExtras();
        if(args.containsKey(JSON_API_PORT_KEY))
            jsonApiPort = args.getInt(JSON_API_PORT_KEY);
        if(args.containsKey(JSON_API_BIND_ADDRESS_KEY))
            jsonApiBindAddress =
                args.getString(JSON_API_BIND_ADDRESS_KEY);

        ErrorConditionWrap ec = nativeStart(jsonApiPort, jsonApiBindAddress);
        if(ec.toBool()) Log.e(TAG, "onStartCommand(...) " + ec.toString());

        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onCreate ()
    {
        super.onCreate();
        sServiceContext = this;
    }

    @Override
    public void onDestroy()
    {
        ErrorConditionWrap ec = nativeStop();
        if(ec.toBool()) Log.e(TAG, "onDestroy() " + ec.toString());
        sServiceContext = null;
        super.onDestroy();
    }

    @Override
	public IBinder onBind(Intent arg0) { return null; }

    private static final String JSON_API_PORT_KEY =
        RetroShareServiceAndroid.class.getCanonicalName() +
        "/JSON_API_PORT_KEY";

    private static final String JSON_API_BIND_ADDRESS_KEY =
        RetroShareServiceAndroid.class.getCanonicalName() +
        "/JSON_API_BIND_ADDRESS_KEY" ;

    private static final String TAG = "RetroShareServiceAndroid.java";

    private static Context sServiceContext;

    protected static native ErrorConditionWrap nativeStart(
        int jsonApiPort, String jsonApiBindAddress );

    protected static native ErrorConditionWrap nativeStop();
}

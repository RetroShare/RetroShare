/*
 * RetroShare Android Autologin Service
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

import QtQml 2.2
import org.retroshare.qml_components.LibresapiLocalClient 1.0

QtObject
{
	id: am

	property bool coreReady: false

	property string profileName
	property string profileSslId
	property string hardcodedPassword: "hardcoded default password"
	property int loginAttemptCount: 0
	property bool attemptingLogin: false

	property var loginNotificationTime: 0

	function delay(msecs, func)
	{
		var tmr = Qt.createQmlObject("import QtQml 2.2; Timer {}", am);
		tmr.interval = msecs;
		tmr.repeat = false;
		tmr.triggered.connect(function() { func(); tmr.destroy(msecs) });
		tmr.start();
	}

	property Timer runStateTimer: Timer
	{
		repeat: true
		interval: 5000
		triggeredOnStart: true
		Component.onCompleted: start()
		onTriggered:
			rsApi.request("/control/runstate/", "", am.runStateCallback)
	}

	function runStateCallback(par)
	{
		var jsonReponse = JSON.parse(par.response)
		var runState = jsonReponse.data.runstate
		if(typeof(runState) !== 'string')
		{
			coreReady = false
			console.log("runStateCallback(par)",
						"Core hanged!",
						"typeof(runState):", typeof(runState),
						"par.response:", par.response)
			return
		}

		switch(runState)
		{
		case "waiting_init":
			coreReady = false
			console.log("Core is starting")
			break
		case "fatal_error":
			coreReady = false
			console.log("Core hanged! runState:", runState)
			break
		case "waiting_account_select":
			coreReady = false
			if(!attemptingLogin && loginAttemptCount < 5)
				rsApi.request("/control/locations/", "", requestLocationsListCB)
			else if (loginAttemptCount >= 5 &&
					 /* Avoid flooding non logged in with login requests, wait
					  * at least 1 hour before notifying the user again */
					 Date.now() - loginNotificationTime > 3600000)
			{
				notificationsBridge.notify(qsTr("Login needed"))
				loginNotificationTime = Date.now()
			}
			break
		case "waiting_startup":
			coreReady = false
			break
		case "running_ok":
		case "running_ok_no_full_control":
			coreReady = true
			runStateTimer.interval = 30000
			break
		}
	}

	function requestLocationsListCB(par)
	{
		console.log("requestLocationsListCB")
		var jsonData = JSON.parse(par.response).data
		if(jsonData.length === 1)
		{
			// There is only one location so we can attempt autologin
			var location = jsonData[0]
			profileName = location.name
			profileSslId = location.peer_id
			if(!attemptingLogin && loginAttemptCount < 5) attemptLogin()
		}
		else if (jsonData.length === 0)
		{
			console.log("requestLocationsListCB 0")
			// The user haven't created a location yet
			// TODO: notify user to create a location
		}
		else
		{
			console.log("requestLocationsListCB *")
			// There is more then one location to choose from
			// TODO: notify user to login manually
		}
	}

	function attemptLogin()
	{
		console.log("attemptLogin")
		attemptingLogin = true
		++loginAttemptCount
		rsApi.request(
					"/control/login/", JSON.stringify({ id: profileSslId }),
					attemptLoginCB)
	}

	function attemptLoginCB(par)
	{
		console.log("attemptLoginCB")
		var jsonRet = JSON.parse(par.response).returncode
		if (jsonRet === "ok") attemptPassTimer.start()
		else console.log("Login hanged!")
	}

	property Timer attemptPassTimer: Timer
	{
		interval: 700
		repeat: true
		triggeredOnStart: true
		onTriggered:
		{
			if(am.coreReady) attemptPasswordCBCB()
			else rsApi.request("/control/password/", "", attemptPasswordCB)
		}

		function attemptPasswordCB(par)
		{
			if(JSON.parse(par.response).data.want_password)
			{
				console.log("attemptPasswordCB want_password")
				rsApi.request(
							"/control/password/",
							JSON.stringify({ password: am.hardcodedPassword }),
							attemptPasswordCBCB)
			}
		}

		function attemptPasswordCBCB()
		{
			console.log("attemptPasswordCBCB")
			stop()
			am.runStateTimer.stop()

			/* Wait 10 seconds so the core has time to process login and update
			 * runstate */
			delay(10000, function()
			{
				am.attemptingLogin = false
				am.runStateTimer.start()
			})
		}
	}
}

/*
 * RetroShare Android QML App
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

pragma Singleton

import QtQml 2.2
import org.retroshare.qml_components.LibresapiLocalClient 1.0

QtObject
{
	id: tokensManager

	property var tokens: ({})
	function registerToken(token, callback)
	{
		if (Array.isArray(tokens[token]))
		{
			// Do not register if it is registered already
			var arrLen = tokens[token].length
			for(var i=0; i<arrLen; ++i)
			{
				if(callback === tokens[token][i])
				{
					console.warn("tokensManager.registerToken(token, callback)",
								 "Attempt to register same callback twice for:",
								 i, token, callback)
					return
				}
			}
			tokens[token].push(callback)
		}
		else tokens[token] = [callback]
	}
	function tokenExpire(token)
	{
		if(Array.isArray(tokens[token]))
		{
			var arrLen = tokens[token].length
			for(var i=0; i<arrLen; ++i)
			{
				var tokCallback = tokens[token][i]
				if (typeof tokCallback == 'function')
				{
					console.log("event token", token, tokCallback)
					tokCallback()
				}
			}
		}

		delete tokens[token]
	}
	function isTokenValid(token) { return Array.isArray(tokens[token]) }

	property alias refreshInterval: refreshTokensTimer.interval

	property LibresapiLocalClient refreshTokensApi: LibresapiLocalClient
	{
		id: refreshTokensApi

		onResponseReceived:
		{
			var jsonData = JSON.parse(msg).data
			var arrayLength = jsonData.length
			for (var i = 0; i < arrayLength; i++)
			{
				tokensManager.tokenExpire(jsonData[i])
			}
		}

		Component.onCompleted:
		{
			if(QT_DEBUG) debug = false

			openConnection(apiSocketPath)
			refreshTokensTimer.start()
		}

		function refreshTokens()
		{
			request("/statetokenservice/*",
					'['+Object.keys(tokensManager.tokens)+']')
		}
	}

	property Timer refreshTokensTimer: Timer
	{
		id: refreshTokensTimer
		interval: 1500
		repeat: true
		triggeredOnStart: true
		onTriggered: refreshTokensApi.refreshTokens()
	}
}

/*******************************************************************************
 * libresapi/api/ApiServer.h                                                   *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2016 by Gioacchino Mazzurco <gio@eigenlab.org>                    *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <QLocalServer>
#include <QString>
#include <QThread>
#include <QLocalSocket>
#include <retroshare/rsinit.h>
#include <string>

#include "ApiTypes.h"
#include "ApiServer.h"

namespace resource_api
{

class ApiLocalListener : public QObject
{
	Q_OBJECT

public:
	ApiLocalListener(ApiServer* server, const QString &listenPath, QObject *parent=0);
	~ApiLocalListener() { mLocalServer.close(); }

public slots:
	void handleConnection();

private:
	ApiServer* mApiServer;
	QLocalServer mLocalServer;
};

class ApiServerLocal : public QObject
{
	Q_OBJECT

public:
	ApiServerLocal(ApiServer* server, const QString& listenPath, QObject *parent=0);
	~ApiServerLocal();

	const static QString& loginServerPath()
	{
		const static QString sockPath(RsAccounts::ConfigDirectory()
		                              .append("/libresapi.sock").c_str());
		return sockPath;
	}

	const static QString& serverPath()
	{
		const static QString sockPath(RsAccounts::AccountDirectory()
		                              .append("/libresapi.sock").c_str());
		return sockPath;
	}

private:
	QThread serverThread;
	ApiLocalListener localListener;
};

class ApiLocalConnectionHandler : public QObject
{
	Q_OBJECT

public:
	ApiLocalConnectionHandler(ApiServer* apiServer, QLocalSocket* sock, QObject *parent = 0);
	~ApiLocalConnectionHandler();
	enum State {WAITING_PATH, WAITING_DATA};

public slots:
	void handlePendingRequests();

private:
	ApiServer* mApiServer;
	QLocalSocket* mLocalSocket;
	State mState;
	std::string reqPath;
	resource_api::Request::Method reqMeth;
};

} // namespace resource_api

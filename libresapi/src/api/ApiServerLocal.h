#pragma once
/*
 * libresapi local socket server
 * Copyright (C) 2016  Gioacchino Mazzurco <gio@eigenlab.org>
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

#include <QLocalServer>
#include <QString>
#include <QThread>
#include <QLocalSocket>
#include <retroshare/rsinit.h>
#include <string>

#include "ApiServer.h"

namespace resource_api
{

class ApiLocalListener : public QObject
{
	Q_OBJECT

public:
	ApiLocalListener(ApiServer* server, QObject *parent=0);
	~ApiLocalListener() { mLocalServer.close(); }

	const static QString& serverName()
	{
		const static QString sockPath(RsAccounts::AccountDirectory()
		                              .append("/libresapi.sock").c_str());
		return sockPath;
	}

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
	ApiServerLocal(ApiServer* server, QObject *parent=0);
	~ApiServerLocal();

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
};

} // namespace resource_api

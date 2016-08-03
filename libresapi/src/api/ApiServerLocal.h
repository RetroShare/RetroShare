#pragma once

#include <QLocalServer>
#include <QString>
#include <QThread>
#include <QLocalSocket>
#include <retroshare/rsinit.h>
#include <string>

#include "ApiServer.h"

namespace resource_api
{
class ApiServer;

class ApiServerLocal : public QThread
{
	Q_OBJECT

public:
	ApiServerLocal(ApiServer* server);
	~ApiServerLocal();

public slots:
	void handleConnection();

private:
	ApiServer* mApiServer;
	QLocalServer mLocalServer;

	const static QString& serverName()
	{
		const static QString sockPath(RsAccounts::AccountDirectory()
		                              .append("/libresapi.sock").c_str());
		return sockPath;
	}
};

class ApiLocalConnectionHandler : public QThread
{
	Q_OBJECT

public:
	ApiLocalConnectionHandler(ApiServer* apiServer, QLocalSocket* sock);
	~ApiLocalConnectionHandler();

public slots:
	void handleRequest();

private:
	ApiServer* mApiServer;
	QLocalSocket* mLocalSocket;
	std::string reqPath;
	void _die();
};

} // namespace resource_api

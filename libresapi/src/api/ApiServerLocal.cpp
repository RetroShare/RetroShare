#include "ApiServerLocal.h"
#include "JsonStream.h"

namespace resource_api{

ApiServerLocal::ApiServerLocal(ApiServer* server) : QThread(),
    mApiServer(server), mLocalServer(this)
{
	start();
	mLocalServer.removeServer(serverName());
#if QT_VERSION >= 0x050000
	mLocalServer.setSocketOptions(QLocalServer::UserAccessOption);
#endif
	connect(&mLocalServer, SIGNAL(newConnection()), this, SLOT(handleConnection()));
	mLocalServer.listen(serverName());
}

ApiServerLocal::~ApiServerLocal()
{
	mLocalServer.close();
	quit();
}

void ApiServerLocal::handleConnection()
{
	new ApiLocalConnectionHandler(mApiServer, mLocalServer.nextPendingConnection());
}

ApiLocalConnectionHandler::ApiLocalConnectionHandler(ApiServer* apiServer, QLocalSocket* sock) :
    QThread(), mApiServer(apiServer), mLocalSocket(sock)
{
	sock->moveToThread(this);
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
	connect(mLocalSocket, SIGNAL(disconnected()), this, SLOT(quit()));
	connect(sock, SIGNAL(readyRead()), this, SLOT(handleRequest()));
	start();
}

void ApiLocalConnectionHandler::handleRequest()
{
	char path[1024];
	if(mLocalSocket->readLine(path, 1023) > 0)
	{
		reqPath = path;
		char jsonData[20480];
		if(mLocalSocket->read(jsonData, 20479) > 0)
		{
			resource_api::JsonStream reqJson;
			reqJson.setJsonString(std::string(jsonData));
			resource_api::Request req(reqJson);
			req.setPath(reqPath);
			std::string resultString = mApiServer->handleRequest(req);
			mLocalSocket->write(resultString.c_str(), resultString.length());
			mLocalSocket->write("\n\0");
		}
		else mLocalSocket->write("\"{\"data\":null,\"debug_msg\":\"ApiLocalConnectionHandler::handleRequest() Error: timeout waiting for path.\\n\",\"returncode\":\"fail\"}\"\n\0");
	}
	else mLocalSocket->write("{\"data\":null,\"debug_msg\":\"ApiLocalConnectionHandler::handleRequest() Error: timeout waiting for JSON data.\\n\",\"returncode\":\"fail\"}\"\n\0");

	quit();
}

ApiLocalConnectionHandler::~ApiLocalConnectionHandler()
{
	mLocalSocket->flush();
	mLocalSocket->close();
	mLocalSocket->deleteLater();
}

} // namespace resource_api

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
    QThread(), mApiServer(apiServer), mLocalSocket(sock), mState(WAITING_PATH)
{
	sock->moveToThread(this);
	connect(mLocalSocket, SIGNAL(disconnected()), this, SLOT(quit()));
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
	connect(sock, SIGNAL(readyRead()), this, SLOT(handleRequest()));
	start();
}

void ApiLocalConnectionHandler::handleRequest()
{
	switch(mState)
	{
	case WAITING_PATH:
	{
		char path[1024];
		mLocalSocket->readLine(path, 1023);
		reqPath = path;
		mState = WAITING_DATA;
		break;
	}
	case WAITING_DATA:
	{
		char jsonData[1024];
		mLocalSocket->read(jsonData, 1023);
		resource_api::JsonStream reqJson;
		reqJson.setJsonString(std::string(jsonData));
		resource_api::Request req(reqJson);
		req.setPath(reqPath);
		std::string resultString = mApiServer->handleRequest(req);
		mLocalSocket->write(resultString.data(), resultString.length());
		mState = WAITING_PATH;
		break;
	}
	}
}

ApiLocalConnectionHandler::~ApiLocalConnectionHandler()
{
	mLocalSocket->close();
	delete mLocalSocket;
}


} // namespace resource_api

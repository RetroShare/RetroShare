/*******************************************************************************
 * libresapi/api/ApiServer.cpp                                                 *
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

#include <QStringList>
#include <QFileInfo>
#include <QDir>

#include "ApiServerLocal.h"
#include "JsonStream.h"


namespace resource_api{

ApiServerLocal::ApiServerLocal(ApiServer* server,
                               const QString &listenPath, QObject *parent) :
    QObject(parent), serverThread(this),
    // Must have no parent to be movable to other thread
    localListener(server, listenPath)
{
	qRegisterMetaType<QAbstractSocket::SocketState>();
	localListener.moveToThread(&serverThread);
	serverThread.start();
}

ApiServerLocal::~ApiServerLocal()
{
	serverThread.quit();
	serverThread.wait();
}

ApiLocalListener::ApiLocalListener(ApiServer *server,
                                   const QString &listenPath,
                                   QObject *parent) :
    QObject(parent), mApiServer(server), mLocalServer(this)
{
	QFileInfo fileInfo(listenPath);
	if(fileInfo.exists())
	{
		std::cerr << __PRETTY_FUNCTION__ << listenPath.toLatin1().data()
		          << " already exists. "
		          << "Removing it assuming it's a past crash leftover! "
		          << "Are you sure another instance is not listening there?"
		          << std::endl;
		mLocalServer.removeServer(listenPath);
	}
#if QT_VERSION >= 0x050000
	mLocalServer.setSocketOptions(QLocalServer::UserAccessOption);
#endif
	connect( &mLocalServer, &QLocalServer::newConnection,
	         this, &ApiLocalListener::handleConnection );

	QDir&& lDir(fileInfo.absoluteDir());
	if(!lDir.exists())
	{
		std::cerr << __PRETTY_FUNCTION__ << " Directory for listening socket "
		          << listenPath.toLatin1().data() << " doesn't exists. "
		          << " Creating it!" << std::endl;
		lDir.mkpath(lDir.absolutePath());
	}

	if(!mLocalServer.listen(listenPath))
	{
		std::cerr << __PRETTY_FUNCTION__ << " mLocalServer.listen("
		          << listenPath.toLatin1().data() << ") failed with: "
		          << mLocalServer.errorString().toLatin1().data() << std::endl;
	}
}

void ApiLocalListener::handleConnection()
{
	new ApiLocalConnectionHandler(mApiServer,
	                              mLocalServer.nextPendingConnection(), this);
}

ApiLocalConnectionHandler::ApiLocalConnectionHandler(
        ApiServer* apiServer, QLocalSocket* sock, QObject *parent) :
    QObject(parent), mApiServer(apiServer), mLocalSocket(sock),
    mState(WAITING_PATH)
{
	connect(mLocalSocket, SIGNAL(disconnected()), this, SLOT(deleteLater()));
	connect(sock, SIGNAL(readyRead()), this, SLOT(handlePendingRequests()));
}

ApiLocalConnectionHandler::~ApiLocalConnectionHandler()
{
	/* Any attempt of closing the socket here also deferred method call, causes
	 * crash when the core is asked to stop, at least from the JSON API call.
	 * QMetaObject::invokeMethod(&app, "close", Qt::QueuedConnection)
	 * mLocalSocket->disconnectFromServer()
	 * mLocalSocket->close() */
	mLocalSocket->deleteLater();
}

void ApiLocalConnectionHandler::handlePendingRequests()
{
	switch(mState)
	{
	case WAITING_PATH:
	{
		if(mLocalSocket->canReadLine())
		{
readPath:
			QString rString(mLocalSocket->readLine());
			rString = rString.simplified();
			if (!rString.isEmpty())
			{
				if(rString.startsWith("PUT", Qt::CaseInsensitive)) reqMeth = resource_api::Request::PUT;
				else if (rString.startsWith("DELETE", Qt::CaseInsensitive)) reqMeth = resource_api::Request::DELETE_AA;
				else reqMeth = resource_api::Request::GET;
				if(rString.contains(' ')) rString = rString.split(' ')[1];

				reqPath = rString.toStdString();
				mState = WAITING_DATA;

				/* Because QLocalSocket is SOCK_STREAM some clients implementations
				 * like the one based on QLocalSocket feel free to send the whole
				 * request (PATH + DATA) in a single write(), causing readyRead()
				 * signal being emitted only once, in that case we should continue
				 * processing without waiting for readyRead() being fired again, so
				 * we don't break here as there may be more lines to read */
			}
			else break;
		}
	}
	case WAITING_DATA:
	{
		if(mLocalSocket->canReadLine())
		{
			resource_api::JsonStream reqJson;
			reqJson.setJsonString(std::string(mLocalSocket->readLine().constData()));
			resource_api::Request req(reqJson);
			req.mMethod = reqMeth;
			req.setPath(reqPath);

			// Need this idiom because binary result may contains \0
			std::string&& resultString = mApiServer->handleRequest(req);
			QByteArray rB(resultString.data(), resultString.length());

			// Dirty trick to support avatars answers
			if(rB.contains("\n") || !rB.startsWith("{") || !rB.endsWith("}"))
				mLocalSocket->write(rB.toBase64());
			else mLocalSocket->write(rB);
			mLocalSocket->write("\n\0");

			mState = WAITING_PATH;

			/* Because QLocalSocket is SOCK_STREAM some clients implementations
			 * like the one based on QLocalSocket feel free to coalesce multiple
			 * upper level write() into a single socket write(), causing
			 * readyRead() signal being emitted only once, in that case we should
			 * keep processing without waiting for readyRead() being fired again */
			if(mLocalSocket->canReadLine()) goto readPath;

			// Now there are no more requests to process we can break
			break;
		}
	}
	}
}

} // namespace resource_api

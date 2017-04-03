/*
 * RetroShare Android QML App
 * Copyright (C) 2016-2017  Gioacchino Mazzurco <gio@eigenlab.org>
 * Copyright (C) 2016  Manu Pineda <manu@cooperativa.cat>
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

#include "libresapilocalclient.h"

#include <QJSEngine>
#include <QtDebug>

void LibresapiLocalClient::openConnection(QString socketPath)
{
	connect(& mLocalSocket, SIGNAL(error(QLocalSocket::LocalSocketError)),
	        this, SLOT(socketError(QLocalSocket::LocalSocketError)));
	connect(& mLocalSocket, SIGNAL(readyRead()),
	        this, SLOT(read()));
	mLocalSocket.connectToServer(socketPath);
}

int LibresapiLocalClient::request( const QString& path, const QString& jsonData,
                                   QJSValue callback )
{
#ifdef QT_DEBUG
	if(mDebug)
		qDebug() << reqCount++ << __PRETTY_FUNCTION__ << path << jsonData
		         << callback.toString();
#endif // QT_DEBUG

	QByteArray data;
	data.append(path); data.append('\n');
	data.append(jsonData); data.append('\n');
	processingQueue.enqueue(PQRecord(path, jsonData, callback));
	return mLocalSocket.write(data);
}

void LibresapiLocalClient::socketError(QLocalSocket::LocalSocketError)
{
	qCritical() << __PRETTY_FUNCTION__ << "Socket Eerror! "
	            << mLocalSocket.errorString();
}

void LibresapiLocalClient::read()
{
	if(processingQueue.isEmpty())
	{
		qCritical() << __PRETTY_FUNCTION__ << "callbackQueue is empty "
		            << "something really fishy is happening!";
		return;
	}

	if(!mLocalSocket.canReadLine())
	{
		qWarning() << __PRETTY_FUNCTION__
		           << "Strange, can't read a complete line!";
		return;
	}

	QByteArray&& ba(mLocalSocket.readLine());
	if(ba.size() < 2)
	{
		qWarning() << __PRETTY_FUNCTION__ << "Got answer of less then 2 bytes,"
		           << "something fishy is happening!";
		return;
	}

	QString receivedMsg(ba);
	PQRecord&& p(processingQueue.dequeue());

#ifdef QT_DEBUG
	if(mDebug)
		qDebug() << ansCount++ << __PRETTY_FUNCTION__ << receivedMsg << p.mPath
		         << p.mJsonData << p.mCallback.toString();
#endif // QT_DEBUG

	emit goodResponseReceived(receivedMsg); /// @deprecated
	emit responseReceived(receivedMsg);

	if(p.mCallback.isCallable())
	{
		QJSValue&& params(p.mCallback.engine()->newObject());
		params.setProperty("response", receivedMsg);

		p.mCallback.call(QJSValueList { params });
	}

	// In case of multiple reply coaleshed in the same signal
	if(mLocalSocket.bytesAvailable() > 0) read();
}

LibresapiLocalClient::PQRecord::PQRecord( const QString&
#ifdef QT_DEBUG
                                          path
#endif //QT_DEBUG
                                          , const QString&
#ifdef QT_DEBUG
                                          jsonData
#endif //QT_DEBUG
                                          , const QJSValue& callback) :
#ifdef QT_DEBUG
    mPath(path), mJsonData(jsonData),
#endif //QT_DEBUG
    mCallback(callback) {}

#ifdef QT_DEBUG
void LibresapiLocalClient::setDebug(bool v)
{
	if(v != mDebug)
	{
		mDebug = v;
		emit debugChanged();
	}
}
#endif // QT_DEBUG

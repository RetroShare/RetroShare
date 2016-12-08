/*
 * RetroShare Android QML App
 * Copyright (C) 2016  Gioacchino Mazzurco <gio@eigenlab.org>
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
	QByteArray data;
	data.append(path); data.append('\n');
	data.append(jsonData); data.append('\n');
	callbackQueue.enqueue(callback);
	mLocalSocket.write(data);

	return 1;
}

void LibresapiLocalClient::socketError(QLocalSocket::LocalSocketError)
{
	qDebug() << "Socket Eerror!!" << mLocalSocket.errorString();
}

void LibresapiLocalClient::read()
{
	QString receivedMsg(mLocalSocket.readLine());
	QJSValue callback(callbackQueue.dequeue());
	if(callback.isCallable())
	{
		QJSValue params = callback.engine()->newObject();
		params.setProperty("response", receivedMsg);

		callback.call(QJSValueList { params });
	}

	emit goodResponseReceived(receivedMsg); /// @deprecated
	emit responseReceived(receivedMsg);
}

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
#include "debugutils.h"
#include <QChar>


void LibresapiLocalClient::openConnection(QString socketPath)
{
	connect(& mLocalSocket, SIGNAL(error(QLocalSocket::LocalSocketError)),
	        this, SLOT(socketError(QLocalSocket::LocalSocketError)));
	connect(& mLocalSocket, SIGNAL(readyRead()),
	        this, SLOT(read()));
	mLocalSocket.connectToServer(socketPath);
}

int LibresapiLocalClient::request(const QString & path, const QString & jsonData)
{
	qDebug() << "LibresapiLocalClient::request()" << path << jsonData;
    QByteArray data;
    data.append(path); data.append('\n');
    data.append(jsonData); data.append('\n');
    mLocalSocket.write(data);

    return 1;
}

void LibresapiLocalClient::socketError(QLocalSocket::LocalSocketError)
{
	myDebug("error!!!!\n" + mLocalSocket.errorString());
}

void LibresapiLocalClient::read()
{
	receivedBytes = mLocalSocket.readLine();

	qDebug() << receivedBytes;

	if(parseResponse()) // pensar en fer un buffer per parsejar, per evitar errors.
		emit goodResponseReceived(QString(receivedBytes));
	else
	{
		QString errMess = "The message was not understood!\n"
		"It should be a JSON formatted text file\n"
		"Its contents were:\n" + receivedBytes;
		myDebug(errMess.replace(QChar('\n'), QChar::LineSeparator));
	}
}

bool LibresapiLocalClient::parseResponse()
{
    QJsonParseError error;
    json = QJsonDocument::fromJson(receivedBytes, &error);
    myDebug(QString(json.toJson()).replace(QChar('\n'), QChar::LineSeparator));

    if(error.error == QJsonParseError::NoError){
        return true;
    }
    myDebug(error.errorString());

    return false;
}

const QJsonDocument & LibresapiLocalClient::getJson()
{
    return json;
}

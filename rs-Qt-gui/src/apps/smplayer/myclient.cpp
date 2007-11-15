/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "myclient.h"
#include <QTcpSocket>
#include <QTextStream>
#include <QHostAddress>
#include <QRegExp>

MyClient::MyClient(quint16 port, QObject * parent) : QObject(parent) 
{
	qDebug("MyClient::MyClient");

	this->port = port;
	timeout = 200;

	socket = new QTcpSocket(this);
}

MyClient::~MyClient() {
	delete socket;
}

QString MyClient::readLine() {
	QString line;

	int n = 0;
	while (!socket->canReadLine() && n < 5) {
		//qDebug("Bytes available: %d", (int) socket->bytesAvailable());
		socket->waitForReadyRead( timeout );
		n++;
	}
	if (socket->canReadLine()) {
		line = QString::fromUtf8(socket->readLine());
		line.remove( QRegExp("[\r\n]") );
        qDebug("MyClient::readLine: '%s'", line.toUtf8().data());
	} 

	return line;
}

void MyClient::writeLine(QString l) {
	socket->write( l.toUtf8() );
	socket->flush();
	socket->waitForBytesWritten( timeout );
}

bool MyClient::openConnection() {
	socket->connectToHost( QHostAddress::LocalHost, port, QIODevice::ReadWrite);
	if (!socket->waitForConnected( timeout )) return false; // Can't connect

	QString line = readLine();
	if (!line.startsWith("SMPlayer")) return false;
	qDebug("MyClient::sendFiles: connected to a SMPlayer instance!");

	line = readLine(); // Read help message

	return true;
}


bool MyClient::sendFiles( const QStringList & files, bool addToPlaylist) {
	QString line;

	writeLine("open_files_start\r\n");
	line = readLine();
	if (!line.startsWith("OK")) return false;

	for (int n=0; n < files.count(); n++) {
		writeLine("open_files " + files[n] + "\r\n");
		line = readLine();
		if (!line.startsWith("OK")) return false;
	}

	if (!addToPlaylist) 
		writeLine("open_files_end\r\n");
	else
		writeLine("add_files_end\r\n");

	writeLine("quit\r\n");

	do {
		line = readLine();
	} while (!line.isNull());


	socket->disconnectFromHost();
	socket->waitForDisconnected( timeout );

	return true;
}

bool MyClient::sendAction( const QString & action ) {
	QString line;

	writeLine("f " + action + "\r\n");
	line = readLine();
	if (!line.startsWith("OK")) return false;

	socket->disconnectFromHost();
	socket->waitForDisconnected( timeout );

	return true;
}

/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

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

#include "myserver.h"
#include "version.h"
#include <QHostAddress>
#include <QRegExp>

Connection::Connection(QTcpSocket * s) 
{
	socket = s;

	//connect(s, SIGNAL(disconnected()), this, SLOT(deleteLater()));
	connect(s, SIGNAL(readyRead()), this, SLOT(readData()));

	sendText(QString("SMPlayer %1").arg(smplayerVersion()));
	sendText("Type help for a list of commands");
}

Connection::~Connection() {
	delete socket;
}

void Connection::sendText(QString l) {
	qDebug("Connection::sendText: '%s'", l.toUtf8().data());

    socket->write( l.toUtf8() + "\r\n" );
    socket->flush();
}

void Connection::readData() {
	while (socket->canReadLine()) {
		QString l = QString::fromUtf8( socket->readLine() );
		l.remove( QRegExp("[\r\n]") );
		parseLine( l );
	}
}

void Connection::parseLine(QString str) {
	qDebug("Connection::parseLine: '%s'", str.toUtf8().data());

	QRegExp rx_open("^open (.*)");
	QRegExp rx_open_files("(^open_files|^add_files) (.*)");
	QRegExp rx_function("^(function|f) (.*)");


	if (str.toLower() == "hello") {
		sendText(QString("Hello, this is SMPlayer %1").arg(smplayerVersion()));
	}
	else
	if (str.toLower() == "help") {
		sendText("Available commands:");
		sendText(" help");
		sendText(" quit");
		sendText(" list functions");
		sendText(" function [function_name]");
		sendText(" f [function_name]");
		sendText(" open [file]");
	}
	else
	if (str.toLower() == "quit") {
		sendText("Goodbye");
		socket->disconnectFromHost();
	}
	else
	if (str.toLower() == "list functions") {
		for (int n=0; n < actions_list.count(); n++) {
			sendText( actions_list[n] );
		}
	}
	else 
	if (rx_open.indexIn(str) > -1) {
		QString file = rx_open.cap(1);
		qDebug("Connection::parseLine: asked to open '%s'", file.toUtf8().data());
		sendText("OK, file sent to GUI");
		emit receivedOpen(file);
	} 
	else
	if ( (str.toLower() == "open_files_start") ||
         (str.toLower() == "add_files_start") ) 
	{
		files_to_open.clear();
		sendText("OK, send first file");
	}
	else
	if ( (str.toLower() == "open_files_end") ||
         (str.toLower() == "add_files_end") ) 
	{
		qDebug("Connection::parseLine: files_to_open:");
		for (int n=0; n < files_to_open.count(); n++) 
			qDebug("Connection::parseLine: %d: '%s'", n, files_to_open[n].toUtf8().data());
		sendText("OK, sending files to GUI");

		if (str.toLower() == "open_files_end")
			emit receivedOpenFiles(files_to_open);
		else
			emit receivedAddFiles(files_to_open);
	}
	else
	if (rx_open_files.indexIn(str) > -1) {
		QString file = rx_open_files.cap(2);
		qDebug("Connection::parseLine: file: '%s'", file.toUtf8().data());
		files_to_open.append(file);
		sendText("OK, file received");
	}
	else
	if (rx_function.indexIn(str) > -1) {
		QString function = rx_function.cap(2).toLower();
		qDebug("Connection::parseLine: asked to process function '%s'", function.toUtf8().data());
		sendText("OK, function sent to GUI");
		emit receivedFunction(function);
	}
	else {
		sendText("Unknown command");
	}
}


MyServer::MyServer( QObject * parent ) : QTcpServer(parent)
{
	connect(this, SIGNAL(newConnection()), this, SLOT(newConnection_slot()));
}

bool MyServer::listen( quint16 port ) {
	return QTcpServer::listen(QHostAddress::LocalHost, port);
}

void MyServer::newConnection_slot() {
	Connection * c = new Connection( nextPendingConnection() );
	c->setActionsList( actionsList() );

	connect(c, SIGNAL(receivedOpen(QString)),
            this, SIGNAL(receivedOpen(QString)));
	connect(c, SIGNAL(receivedOpenFiles(QStringList)),
            this, SIGNAL(receivedOpenFiles(QStringList)));
	connect(c, SIGNAL(receivedAddFiles(QStringList)),
            this, SIGNAL(receivedAddFiles(QStringList)));
	connect(c, SIGNAL(receivedFunction(QString)),
            this, SIGNAL(receivedFunction(QString)));
}

#include "moc_myserver.cpp"

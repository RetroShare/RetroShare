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

#ifndef _MYSERVER_H_
#define _MYSERVER_H_

#include <QTcpServer>
#include <QTcpSocket>
#include <QStringList>

//! Connection holds a connection from MyServer to a client.

/*!
 Connection objects are created by MyServer every time a new
 connection is made.
 It reads the text sent by the client, parses it, respond, and send
 signals to the server to report client requests.
 This class is for private use by MyServer.
*/

class Connection : public QObject
{
	Q_OBJECT

public:
	Connection(QTcpSocket * s);
	~Connection();

    void setActionsList(QStringList l) { actions_list = l; };
    QStringList actionsList() { return actions_list; };

signals:
	void receivedOpen(QString);
	void receivedOpenFiles(QStringList);
	void receivedAddFiles(QStringList);
	void receivedFunction(QString);

protected slots:
	void readData();

protected:
	void sendText(QString l);
	void parseLine(QString str);

private:
	QTcpSocket * socket;
	QStringList actions_list;
	QStringList files_to_open;
};

//! MyServer listens a port and waits for connections from other instances.

/*!
 MyServer will listen the specified port and will send signals
 when another instance request something.
*/

class MyServer : public QTcpServer
{
	Q_OBJECT

public:
	MyServer( QObject * parent = 0 );

	//! Tells the server to listen for incoming connections on port \a port.
	bool listen( quint16 port );

	//! Sets the list of actions.
	//! The list is printed when the client requests it.
    void setActionsList(QStringList l) { actions_list = l; };

	//! Returns the list of actions.
    QStringList actionsList() { return actions_list; };

signals:
	//! Emitted when the client request to open a new file.
	void receivedOpen(QString);

	//! Emitted when the client request to open a list of files.
	void receivedOpenFiles(QStringList);

	//! Emitted when the client request to add a list of files to the playlist.
	void receivedAddFiles(QStringList);

	//! Emitted when the client request to perform an action.
	void receivedFunction(QString);

protected slots:
	void newConnection_slot();

private:
	QStringList actions_list;
};

#endif

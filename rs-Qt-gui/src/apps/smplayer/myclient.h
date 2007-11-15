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

#ifndef _MYCLIENT_H_
#define _MYCLIENT_H_

#include <QStringList>

class QTextStream;
class QTcpSocket;


//! MyClient communicates with other running instances.

/*!
 It can be used to know if there's another instance of smplayer running.
 It also allows to send the file(s) that the user wants to open to
 the other instance.
*/

class MyClient : public QObject
{
public:
	MyClient(quint16 port, QObject * parent = 0);
	~MyClient();

	//! Sets the maximum time that should wait in the waitFor... functions.
	void setTimeOut(int ms) { timeout = ms; };
	int timeOut() { return timeout; };

	//! Return true if it can open a connection to another instance.
	bool openConnection();
	//! Send the list of files to the other instance. Return true on success.
	bool sendFiles( const QStringList & files, bool addToPlaylist = false);
	//! Pass an action (pause, fullscreen...) to GUI.
	bool sendAction( const QString & action );

protected:
	QString readLine();
	void writeLine(QString);

private:
	quint16 port;
	QTcpSocket * socket;
	int timeout;
};

#endif

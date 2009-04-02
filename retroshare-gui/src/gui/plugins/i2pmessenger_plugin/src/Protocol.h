/***************************************************************************
 *   Copyright (C) 2008 by normal   *
 *   normal@Desktop2   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef PROTOCOL
#define PROTOCOL

#include <QtGui>
#include <QThread>


/*
	First packet on connection must be:
		CHATSYSTEM\tProtocolVersion\n
		CHATSYSTEMFILETRANSFER\tProtocolVersion\nSizeinBit\nFileName
	else
		send <the html info-page > 	//maybe with information about the user ???
						//maybe good for usersearch ?

	Every packet must be >= 8 Byte
	1-4 Byte = Paketlength in Byte (HEX) without the 4 Byte Paketlength
	5-8 Byte = PaketInfo
	 >8 Byte = PaketData
*/

namespace Protocol_Info{
	const QString PROTOCOLVERSION="0.2";
	const QString FIRSTPAKETCHAT="CHATSYSTEM\t"+PROTOCOLVERSION+"\n";
	const QString HTTPPAGE="<html><header></header><body>This is not a eepsite,this is a I2PMessenger Destination<br><br></body></html>\n\n\n";
};

namespace PROTOCOL_TAGS{
	enum COMMANDS_TAGS{
		PING,
		GET_PROTOCOLVERSION,
		GET_CLIENTVERSION,
		GET_CLIENTNAME,
		GET_USER_ONLINESTATUS
	};
	enum MESSAGES_TAGS{
		CHATMESSAGE,
		ECHO_OF_PING,
		ANSWER_OF_GET_CLIENTVERSION,
		ANSWER_OF_GET_CLIENTNAME,
		USER_ONLINESTATUS_ONLINE,
		USER_ONLINESTATUS_OFFLINE,
		USER_ONLINESTATUS_INVISIBLE,
		USER_ONLINESTATUS_WANTTOCHAT,
		USER_ONLINESTATUS_AWAY,
		USER_ONLINESTATUS_DONT_DISTURB	
	};
	

};
using namespace Protocol_Info;
using namespace PROTOCOL_TAGS;
class cCore;
class cUser;
class cProtocol:public QObject{
Q_OBJECT
public:
	cProtocol(cCore* Core);
	QString get_ProtocolVersion(){return PROTOCOLVERSION;};
public slots:
	QByteArray inputUnknown(const QString ID,const QByteArray Data);
	void inputKnown(const QString ID, const QByteArray Data);
	void send(const MESSAGES_TAGS TAG,const QString ID,QString Data);
	void send(const COMMANDS_TAGS TAG,const QString ID);
	void newConnectionChat(const QString ID);

signals:
	void eventUserChanged();//also used for newchatMessage
private:
	cCore* Core;
	

	
};

#endif

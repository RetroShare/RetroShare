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
#ifndef USER_H
#define USER_H

#include <QtGui>
#include <QStringList>


namespace User
{
	enum CONNECTIONTOUSER{
		OFFLINE,
		ONLINE,
		TRYTOCONNECT,
		ERROR
	};

	enum ONLINESTATE{
		USERONLINE,
		USEROFFLINE,
		USERINVISIBLE,
		USERWANTTOCHAT,
		USERAWAY,
		USERDONT_DISTURB,
		USERTRYTOCONNECT
	};

}//namespace user

using namespace User;
class cProtocol;
class cUser: public QObject
{
	Q_OBJECT
	public:
	cUser(	cProtocol* Protocol,
		QString Name,
		QString I2PDestination,
		QString I2PStream_ID,
		QString TorDestination
		);
	
	const QString get_Name()const;
	const QString get_I2PDestination()const;
	const QString get_I2PStreamID()const;
	const QString get_TORDestination()const;
	const QString get_TORStream_ID()const;
	const QString get_ProtocolVersion()const;
	const QString get_ClientName()const;
	const QString get_ClientVersion()const;
	CONNECTIONTOUSER get_ConnectionStatus()const;
	ONLINESTATE get_OnlineState()const;

	const QStringList &get_ChatMessages();
	bool getHaveAllreadyOneChatWindow()const;	
	bool getHaveNewUnreadMessages();

	void set_ConnectionStatus(CONNECTIONTOUSER Status);
	void set_OnlineState(const ONLINESTATE newState);
	void set_Name(QString newName);
	void set_I2PStreamID(QString ID);
	void set_ReadyToSend(bool b);
	void set_ProtocolVersion(QString Version);
	void set_HaveAllreadyOneChatWindow(bool t);
	void set_ClientName(QString Name);
	void set_ClientVersion(QString Version);
	void IncomingNewChatMessage(QString newMessage);
	
	public slots:
	void  sendChatMessage(QString Message);

	signals:
		void OnlineStateChanged();
		void newMessageRecived();
	private:
	bool HaveAllreadyOneChatWindow;
	bool newUnreadMessages;
	void sendAllunsendedMessages();

	const QString I2PDestination;
	const QString TorDestination;

	QString Name;
	QString I2PStream_ID;
	QString TORStream_ID;	
	
	bool 	ReadyToSend;

	CONNECTIONTOUSER ConnectionStatus;
	ONLINESTATE	 CurrentOnlineState;
	QString ProtocolVersion;
	QString ClientName;
	QString ClientVersion;
	QStringList Messages;
	QStringList unsendedMessages;
	cProtocol* Protocol;
};

#endif 

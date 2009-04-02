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

#ifndef CORE_H
#define CORE_H

#include <QtGui>
#include <QSettings>
#include <QtXml>
#include <QTextStream>
#include <QList>

#include "User.h"
#include "ConnectionI2P.h"
#include "DebugMessageManager.h"
#include "I2PSamMessageAnalyser.h"

#include "UserConnectThread.h"
#include "Protocol.h"
#include "PacketManager.h"
#include "FileTransferSend.h"
#include "FileTransferRecive.h"



#define CLIENTVERSION "0.2.7 Beta"
#define CLIENTNAME "I2P-Messenger (QT)"



struct cConnection
{
	cConnection ( QString ID,QString Destination )
	{
		this->ID=ID;
		this->Destination=Destination;
	}
	void operator= ( const cConnection& t )
	{
		this->ID=t.ID;
		this->Destination=t.Destination;
	}
	QString ID;
	QString Destination;
};

using namespace SAM_Message_Types;
using namespace User;
class cUserConnectThread;

class cCore :public QObject
{

		Q_OBJECT
	public:
		cCore();
		~cCore();
		cDebugMessageManager* get_DebugMessageHandler();
		const QList<cUser*> get_userList();
		bool isThisIDunknown ( QString ID );
		bool removeUnknownID ( QString ID );
		bool isThisDestinationInunknownConnections ( QString Destination );

		void removeUnknownIDCreateUserIfNeeded( const QString ID,const QString ProtocolVersion );

		QString get_UserProtocolVersionByStreamID ( QString ID );
		void set_UserProtocolVersionByStreamID ( QString ID,QString Version );
		cUser* getUserByI2P_ID ( QString ID );
		cUser* getUserByI2P_Destination ( QString Destination );
		const QString getMyDestination() const;
		ONLINESTATE getOnlineStatus()const;
		QString get_ClientName() {return CLIENTNAME;};
		QString get_ClientVersion(){return CLIENTVERSION;};
		QString get_ProtocolVersion(){return Protocol->get_ProtocolVersion();};
		void setOnlineStatus(const ONLINESTATE newStatus);
		void startFileTransfer(QString FilePath,QString Destination);
		void addNewFileRecive(QString ID,QString FileName,QString FileSize);
		void StreamSendData(QString ID,QByteArray Data);
		void StreamSendData(QString ID,QString Data);
		void StreamClose(QString ID);
		QString StreamConnect (QString Destination );
		bool checkIfAFileTransferOrReciveisActive();

		

	public slots:
		bool addNewUser (QString Name,QString I2PDestination,QString TorDestination,QString I2PStream_ID="");
		bool deleteUserByTorDestination (const QString TorDestination );
		bool deleteUserByI2PDestination (const QString I2PDestination );
		bool renameuserByI2PDestination (const QString Destination, const QString newNickname);
		void doNamingLookUP ( QString Name );
		

		QString get_connectionDump();

	private slots:
		// <SIGNALS FROM cConnectionI2P>
		void StreamClosedRecived ( const SAM_Message_Types::RESULT result,QString ID,QString Message );
		void StreamStatusRecived ( const SAM_Message_Types::RESULT result,const QString ID,QString Message );
		void StreamConnectedRecived ( const QString Destinaton,const QString ID );
		void StreamReadyToSendRecived ( const QString ID );
		void StreamSendRecived ( const QString ID,const SAM_Message_Types::RESULT result,SAM_Message_Types::STATE state );
		void SessionStatusRecived ( const SAM_Message_Types::RESULT result,const QString Destination,const QString Message );
		void StreamDataRecived ( const QString ID,const QString Size,const QByteArray Data );
		void NamingReplyRecived ( const SAM_Message_Types::RESULT result,QString Name,QString Value="",QString Message="" );
	signals:
		void eventUserChanged();
		void eventOnlineStatusChanged();

	private:
		cConnectionI2P* I2P;
		cDebugMessageManager* DebugMessageHandler;
		cProtocol* Protocol;
		QList<cUser*> users;
		QList<cConnection> unknownConnections;
		cUserConnectThread* UserConnectThread;
		QString MyDestination;
		QList<cPacketManager*> DataPacketsManagers;
		ONLINESTATE currentOnlineStatus;
		QList<cFileTransferSend*> FileSends;
		QList<cFileTransferRecive*> FileRecives;

		bool doesUserAllReadyExitsByTorDestination ( QString TorDestination );
		bool doesUserAllReadyExitsByI2PDestination ( QString I2PDestination );
		void init();
		void saveUserList();
		void loadUserList();
		void stopCore();
		void restartCore();
		void closeAllActiveConenctions();
		void deletePacketManagerByID ( QString ID );
		bool isThisID_a_FileSendID(QString ID);
		bool isThisID_a_FileReciveID(QString ID);


};
#endif


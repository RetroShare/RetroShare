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
#include "SoundManager.h"



#define CLIENTVERSION "0.2.9 Beta"
#define CLIENTNAME "I2P-Messenger (QT)"



struct cConnection
{
	cConnection ( qint32 ID,QString Destination )
	{
		this->ID=ID;
		this->Destination=Destination;
	}
	void operator= ( const cConnection& t )
	{
		this->ID=t.ID;
		this->Destination=t.Destination;
	}
	qint32 ID;
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
		bool isThisIDunknown ( qint32 ID );
		bool removeUnknownID ( qint32 ID );
		bool isThisDestinationInunknownConnections ( QString Destination );

		void removeUnknownIDCreateUserIfNeeded( const qint32 ID,const QString ProtocolVersion );

		QString get_UserProtocolVersionByStreamID ( qint32 ID );
		void set_UserProtocolVersionByStreamID ( qint32 ID,QString Version );
		cUser* getUserByI2P_ID ( qint32 ID );
		cUser* getUserByI2P_Destination ( QString Destination );
		const QString getMyDestination() const;
		ONLINESTATE getOnlineStatus()const;
		QString get_ClientName() {return CLIENTNAME;};
		QString get_ClientVersion(){return CLIENTVERSION;};
		QString get_ProtocolVersion(){return Protocol->get_ProtocolVersion();};
		void setOnlineStatus(const ONLINESTATE newStatus);
		void addNewFileTransfer(QString FilePath,QString Destination);
		void addNewFileRecive(qint32 ID,QString FileName,QString FileSize);
		void StreamSendData(qint32 ID,QByteArray Data);
		void StreamSendData(qint32 ID,QString Data);
		void StreamClose(qint32 ID);
		qint32 StreamConnect (QString Destination );
		bool checkIfAFileTransferOrReciveisActive();

		

	public slots:
		bool addNewUser (QString Name,QString I2PDestination,qint32 I2PStream_ID=0);
		bool deleteUserByI2PDestination (const QString I2PDestination );
		bool renameuserByI2PDestination (const QString Destination, const QString newNickname);
		void doNamingLookUP ( QString Name );
		void MuteSound(bool t);
		

		QString get_connectionDump();

	private slots:
		// <SIGNALS FROM cConnectionI2P>
		void StreamClosedRecived ( const SAM_Message_Types::RESULT result,qint32 ID,QString Message );
		void StreamStatusRecived ( const SAM_Message_Types::RESULT result,const qint32 ID,QString Message );
		void StreamConnectedRecived ( const QString Destinaton,const qint32 ID );
		void StreamReadyToSendRecived ( const qint32 ID );
		void StreamSendRecived ( const qint32 ID,const SAM_Message_Types::RESULT result,SAM_Message_Types::STATE state );
		void SessionStatusRecived ( const SAM_Message_Types::RESULT result,const QString Destination,const QString Message );
		void StreamDataRecived ( const qint32 ID,const QString Size,const QByteArray Data );
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
		cSoundManager* SoundManager;

		bool doesUserAllReadyExitsByI2PDestination ( QString I2PDestination );
		void init();
		void saveUserList();
		void loadUserList();
		void stopCore();
		void restartCore();
		void closeAllActiveConnections();
		void deletePacketManagerByID ( qint32 ID );
		bool isThisID_a_FileSendID(qint32 ID);
		bool isThisID_a_FileReciveID(qint32 ID);


};
#endif

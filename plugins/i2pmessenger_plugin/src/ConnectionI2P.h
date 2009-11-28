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
#ifndef CONENCTIONI2P_H
#define CONENCTIONI2P_H

#include <QtGui> 
#include <QTcpSocket>
#include <Qt>
#include <QSocketNotifier>

#include "I2PSamMessageAnalyser.h"

#define SAM_HANDSHAKE "HELLO VERSION MIN=2.0 MAX=2.0\n"

namespace SESSION_Types
{
	enum SESSION_STYLE
	{
		STREAM,
		DATAGRAMM,
		RAW
	};
	enum SESSION_DIRECTION
	{
		BOTH,
		RECEIVE,
		CREATE
	};
};
//class cConnectionI2P
//
//Handshake with SAM an Session Creation are done atomatically
//all STREAMs will be init with no limit | limit is possible with the function doSendStreamSessionLimit
//
class cConnectionI2P :public QObject
{
	Q_OBJECT

	public:
	cConnectionI2P( QString SamHost,
			QString SamPort,
			SESSION_Types::SESSION_STYLE SessionStyle,
			QString SessionDestination,
			SESSION_Types::SESSION_DIRECTION SessionDirection,
			QString SessionOptions="",
			QObject *parent = 0
		      );
	

	~cConnectionI2P();
		
	public slots:	
	void doConnect();
	void doDisconnect();
	qint32 doStreamConnect(QString Destination);//base64key	|| return the new id for the stream
	void doStreamSend(qint32 ID,QByteArray Data);
	void doStreamSend(qint32 ID,QString Data);
	void doStreamClose(qint32 ID);
	void doSendStreamSessionLimit(qint32 ID,quint64 value=0);
	void doNamingLookUP(QString Name);
	
	private slots:
	void connected();
	void readFromSocket();


	signals:
	void debugMessages(const QString Message);
	void HelloReplayRecived(const SAM_Message_Types::RESULT result);
	void SessionStatusRecived(const SAM_Message_Types::RESULT result,const QString Destination,const QString Message);
	void StreamStatusRecived(const SAM_Message_Types::RESULT result,const qint32 ID,QString Message);
	void StreamConnectedRecived(const QString Destinaton,const qint32 ID);
	void StreamSendRecived(const qint32 ID,const SAM_Message_Types::RESULT result,SAM_Message_Types::STATE state);
	void StreamClosedRecived(const SAM_Message_Types::RESULT result,qint32 ID,QString Message);
	void StreamReadyToSendRecived(const qint32 ID);
	void StreamDataRecived(const qint32 ID,const QString Size,const QByteArray Data);
	void NamingReplyRecived(const SAM_Message_Types::RESULT result,QString Name,QString Value="",QString Message="");

	private:
	const QString SamHost;
	const QString SamPort;
	qint32 nextFreeID;
	QByteArray* IncomingPackets;
	QTcpSocket* tcpSocket;
	I2PSamMessageAnalyser* Analyser;
	

	bool HandShakeWasSuccesfullDone;
	bool SessionWasSuccesfullCreated;
	const SESSION_Types::SESSION_STYLE SessionStyle;
	const QString SessionDestination;
	const SESSION_Types::SESSION_DIRECTION SessionDirection;
	const QString SessionOptions;
	void doSessionCreate();
	qint32 get_NextFreeId();

	inline void ConnectionReadyCheck()
	{
		if(	HandShakeWasSuccesfullDone==false ||
			SessionWasSuccesfullCreated==false||
			tcpSocket->state()!=QAbstractSocket::ConnectedState)
		return;
	}
	
};
#endif

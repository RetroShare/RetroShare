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
#include "ConnectionI2P.h"

cConnectionI2P::cConnectionI2P( 
			QString SamHost,
			QString SamPort,
			SESSION_Types::SESSION_STYLE SessionStyle,
			QString SessionDestination,
			SESSION_Types::SESSION_DIRECTION SessionDirection,
			QString SessionOptions,
			QObject *parent)
:SamHost(SamHost),SamPort(SamPort),SessionStyle(SessionStyle),SessionDestination(SessionDestination),
SessionDirection(SessionDirection),SessionOptions(SessionOptions)//, QObject(parent)
{

	IncomingPackets= new QByteArray();
	tcpSocket= new QTcpSocket(parent);

	tcpSocket->moveToThread(this->thread());

	Analyser= new I2PSamMessageAnalyser();
	this->HandShakeWasSuccesfullDone=false;
	this->SessionWasSuccesfullCreated=false;
	this->nextFreeID=1;
	
	connect(tcpSocket,SIGNAL(connected() ),this,SLOT(connected() ),Qt::DirectConnection );
	connect(tcpSocket,SIGNAL(readyRead() ),this, SLOT(readFromSocket()),Qt::DirectConnection);	
	
}

cConnectionI2P::~cConnectionI2P()
{
	doDisconnect();
	delete tcpSocket;
	delete Analyser;
}
void cConnectionI2P::doConnect()
{	

	if(tcpSocket->state()==QAbstractSocket::UnconnectedState) 
	{		
		tcpSocket->connectToHost(SamHost,SamPort.toInt( ));
	}
	if(!tcpSocket->waitForConnected(1000))
		doDisconnect();
	
}

void cConnectionI2P::doDisconnect()
{
	
	if(tcpSocket->state()!=0)
	{	
		tcpSocket->disconnectFromHost();
		emit debugMessages("<-- I2P Socket Disconnected -->\n");	
	}
	else if(tcpSocket->state()==QAbstractSocket::UnconnectedState)
		emit debugMessages("<-- I2P Socket Disconnected -->\n");

	this->nextFreeID=1;
}

void cConnectionI2P::connected()
{

	emit debugMessages("<-- I2P Socket Connected -->\n");
	//emit debugMessages("<-- SAM Connection Handshake send -->\n");
	emit debugMessages(SAM_HANDSHAKE);

	if(tcpSocket->state()==QAbstractSocket::ConnectedState)
	{
		tcpSocket->write(SAM_HANDSHAKE);
		tcpSocket->flush();
	}
}

void cConnectionI2P::readFromSocket()
{	
	


	using namespace SAM_Message_Types;

	QByteArray newData =tcpSocket->readAll();

	QByteArray CurrentPacket;
	IncomingPackets->append(newData);


	while(IncomingPackets->contains("\n")==true){
		
		CurrentPacket=IncomingPackets->left(IncomingPackets->indexOf("\n",0)+1);
		//else return;//Not the complead Packet recived ??? maybe possible ???
		
		QString t(CurrentPacket.data());
				
		SAM_MESSAGE sam=Analyser->Analyse(t);
		switch(sam.type)
		{	//emit the signals
			case HELLO_REPLAY:{
				emit debugMessages(t);
				emit HelloReplayRecived(sam.result);
				if(sam.result==OK){
					this->HandShakeWasSuccesfullDone=true;
					doSessionCreate();
				}
				break;
			}
			case SESSION_STATUS:{
				emit debugMessages(t);
				if(sam.result==OK)this->SessionWasSuccesfullCreated=true;
						
				emit SessionStatusRecived(sam.result,sam.Destination,sam.Message);
				break;
			}
			case STREAM_STATUS:{
				emit debugMessages(t);
				if(sam.result==OK)
					this->doSendStreamSessionLimit(sam.ID,0);

				emit StreamStatusRecived(sam.result,sam.ID,sam.Message);
				break;
			}
			case STREAM_CONNECTED:{
				emit debugMessages(t);
				
				this->doSendStreamSessionLimit(sam.ID,0);

				emit StreamConnectedRecived(sam.Destination,sam.ID);
				break;
			}
			case STREAM_CLOSED:{
				emit debugMessages(t);
				emit StreamClosedRecived(sam.result,sam.ID,sam.Message);
				break;
			}
			case STREAM_SEND:{
				emit debugMessages(t);
				emit StreamSendRecived(sam.ID,sam.result,sam.state);
				break;
			}
	
			case STREAM_READY_TO_SEND:{
				emit debugMessages(t);
				emit StreamReadyToSendRecived(sam.ID);
				break;
			}
			case NAMING_REPLY:{
				emit debugMessages(t);
				emit NamingReplyRecived(sam.result,sam.Name,sam.Value,sam.Message);
				break;
			}
			case STREAM_RECEIVED:{
				
				if( sam.Size.toLong() > (IncomingPackets->length() - CurrentPacket.length()) ) 
					return;//Not the complead Packet recived ??? maybe possible ???
				
				QByteArray Data=IncomingPackets->mid(CurrentPacket.length(),sam.Size.toLong());
				emit debugMessages(t+Data);
				emit StreamDataRecived(sam.ID,sam.Size,Data);
				IncomingPackets->remove(CurrentPacket.length(),sam.Size.toLong());
				break;
			}
	
	
			case ERROR_IN_ANALYSE:{
				emit debugMessages("<ERROR_IN_ANALYSE>\n"+t);
				break;
			}
			default:
				{
					emit debugMessages("<Unknown Packet>\n"+t);
					break;
				}
		}
		IncomingPackets->remove(0,IncomingPackets->indexOf("\n",0)+1);
	}//while
}

void cConnectionI2P::doSessionCreate(){
using namespace SESSION_Types;
ConnectionReadyCheck();

QByteArray Message="SESSION CREATE STYLE=";

	switch(this->SessionStyle)
	{
		case STREAM:
				{
					Message+="STREAM";
					break;
				}
		case DATAGRAMM: 
				{
					Message+="DATAGRAMM";
					break;
				}
		case RAW:
				{
					Message+="RAW";
					break;
				}
	}

	Message+=" DESTINATION="+this->SessionDestination+" DIRECTION=";

	switch(this->SessionDirection)
	{
		case BOTH:
				{
					Message+="BOTH";
					break;
				}
		case RECEIVE:
				{
					Message+="RECEIVE";
					break;
				}
		case CREATE:	
				{
					Message+="CREATE";
					break;
				}
	}	
	if(this->SessionOptions.isEmpty()==false)
		Message+=" "+SessionOptions;

	Message+="\n";
	
	//emit debugMessages("<-Send Create Session Message->\n");
	emit debugMessages(Message);

	tcpSocket->write(Message);
	tcpSocket->flush();

}

qint32 cConnectionI2P::get_NextFreeId()
{
	return nextFreeID++;
}

qint32 cConnectionI2P::doStreamConnect(QString Destination)
{	

	ConnectionReadyCheck();
	qint32 ID=get_NextFreeId();

	QByteArray Message="STREAM CONNECT ID=";
	Message+=QString::number(ID,10);
	Message+=" DESTINATION="+Destination+"\n";
	
	emit debugMessages(Message);
	tcpSocket->write(Message);
	tcpSocket->flush();
	return ID;
}

void cConnectionI2P::doStreamClose(qint32 ID)
{	
	ConnectionReadyCheck();
	QByteArray Message="STREAM CLOSE ID=";
	Message+=QString::number(ID,10)+"\n";

	emit debugMessages(Message);
	tcpSocket->write(Message);
	tcpSocket->flush();
}

void cConnectionI2P::doSendStreamSessionLimit(qint32 ID,quint64 value)
{	
	ConnectionReadyCheck();
	QByteArray Message="STREAM RECEIVE ID=";
	Message+=QString::number(ID,10)+" LIMIT=";
	if(value==0)
		Message+="NONE\n";
	else
	{
		QString Svalue;
		Svalue.setNum(value,10);
		Message+=Svalue+"\n";
	}
	emit debugMessages(Message);
	tcpSocket->write(Message);
	tcpSocket->flush();	
}
void cConnectionI2P::doStreamSend(qint32 ID,QString Data)
{
	QByteArray t="";
	t.insert(0,Data);
	doStreamSend(ID,t);
}
void cConnectionI2P::doStreamSend(qint32 ID,QByteArray Data)
{
	ConnectionReadyCheck();

	QString Size;
	Size.setNum(Data.length());

	QByteArray Message="STREAM SEND ID=";
	Message+=QString::number(ID,10)+" SIZE="+Size+"\n";
	Message.append(Data+="\n");
	
	emit debugMessages(Message);
	tcpSocket->write(Message);
	tcpSocket->flush();
	
}
void cConnectionI2P::doNamingLookUP(QString Name)
{	

	ConnectionReadyCheck();
	
	QByteArray Message="NAMING LOOKUP NAME=";
	Message+=Name+"\n";
	emit debugMessages(Message);
	tcpSocket->write(Message);
	tcpSocket->flush();

}

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
#include "User.h"
#include "Protocol.h"

cUser::cUser(	cProtocol* Protocol,
		QString Name,
		QString I2PDestination,
		qint32 I2PStream_ID
		):I2PDestination(I2PDestination)
{
	this->Protocol=Protocol;
	this->Name=Name;
	this->ReadyToSend=true;
	this->I2PStream_ID=I2PStream_ID;
	this->ConnectionStatus=OFFLINE;
	this->HaveAllreadyOneChatWindow=false;
	this->newUnreadMessages=false;
	this->ClientName="";
	this->ClientVersion="";
	this->CurrentOnlineState=USEROFFLINE;

	this->textColor=Qt::black;
	this->textFont=QFont("Comic Sans MS", 10);
}

const QString cUser::get_Name()const{
	return this->Name;
}
const QString cUser::get_I2PDestination()const{
	return this->I2PDestination;
}
qint32 cUser::get_I2PStreamID()const{
	return this->I2PStream_ID;
}

CONNECTIONTOUSER cUser::get_ConnectionStatus()const{
	return this->ConnectionStatus;
}

void cUser::set_Name(QString newName){
	this->Name=newName;
}
void cUser::set_ConnectionStatus(CONNECTIONTOUSER Status){
	
	this->ConnectionStatus=Status;
	
	if(Status==ONLINE){
		this->ConnectionStatus=Status;
		//get some Infos from the CHATSYSTEM - client
		Protocol->send(GET_CLIENTNAME,I2PStream_ID);
		Protocol->send(GET_CLIENTVERSION,I2PStream_ID);
		Protocol->send(GET_USER_ONLINESTATUS,I2PStream_ID);
		//emit connectionOnline();
	}

	
	if(Status==OFFLINE ||Status==ERROR)
	{
		I2PStream_ID=0;
		this->CurrentOnlineState=USEROFFLINE;
		this->ConnectionStatus=Status;
	}
}
void cUser::set_I2PStreamID(qint32 ID){
	this->I2PStream_ID=ID;
}
void cUser::set_ReadyToSend(bool b){
	ReadyToSend=b;
}
void cUser::set_ProtocolVersion(QString Version){
	this->ProtocolVersion=Version;
}
const QString cUser::get_ProtocolVersion()const{
	return this->ProtocolVersion;
}

void cUser::IncomingNewChatMessage(QString newMessage){
	this->Messages.push_back(Name+"("+ QTime::currentTime().toString("hh:mm:ss") +"): "+newMessage+"<br>");	
	this->newUnreadMessages=true;
	emit newMessageRecived();
	emit newIncomingMessageRecived();
}
void cUser::sendChatMessage(QString Message){
	using namespace PROTOCOL_TAGS;
	if(this->ReadyToSend==false)return; 
	
	if(ConnectionStatus==ONLINE && 
		CurrentOnlineState != USEROFFLINE &&
		CurrentOnlineState != USERINVISIBLE
	){
		Protocol->send(CHATMESSAGE,I2PStream_ID,Message);
		this->Messages.push_back("Me("+QTime::currentTime().toString("hh:mm:ss")  +"): "+Message+"<br>");
		//this->Messages.push_back(Message);
		emit newMessageRecived();
	}
	else{
		
		this->Messages.push_back("[SYSTEM]("+QTime::currentTime().toString("hh:mm:ss") +"): Sending the Message when the user come online<br>When you close the client the Message is lost<br>");
		
		unsendedMessages.push_back(Message);
		emit newMessageRecived();
		
	}
}
void cUser::set_HaveAllreadyOneChatWindow(bool t){
	HaveAllreadyOneChatWindow=t;
}
bool cUser::getHaveAllreadyOneChatWindow() const{
	return HaveAllreadyOneChatWindow;
}
const QStringList& cUser::get_ChatMessages(){
	newUnreadMessages=false;
	return Messages;
}
void cUser::sendAllunsendedMessages(){ 
	using namespace PROTOCOL_TAGS;
	if(unsendedMessages.empty())return;


	for(int i=0;i<unsendedMessages.count();i++)
		Protocol->send(CHATMESSAGE,I2PStream_ID,unsendedMessages.at(i));
	


	this->Messages.push_back("[SYSTEM]("+QTime::currentTime().toString("hh:mm:ss")+"): All unsended Messages were sended<br><br>");
	unsendedMessages.clear();
	this->newUnreadMessages=true;
	emit newMessageRecived();
}

bool cUser::getHaveNewUnreadMessages(){
	return newUnreadMessages;
}

const QString cUser::get_ClientName() const
{
	return ClientName;
}

void cUser::set_ClientName(QString Name)
{
	ClientName=Name;
}


const QString cUser::get_ClientVersion() const
{
	return ClientVersion;
}

void cUser::set_ClientVersion(QString Version)
{
	this->ClientVersion=Version;
}

ONLINESTATE cUser::get_OnlineState() const
{
	return CurrentOnlineState;
}

void cUser::set_OnlineState(const ONLINESTATE newState)
{
	
	if(newState!=USEROFFLINE && 
	   newState!=USERINVISIBLE){		
		if(CurrentOnlineState==USEROFFLINE || CurrentOnlineState==USERINVISIBLE)
			emit connectionOnline();
		this->sendAllunsendedMessages();
	}
	else if(newState==USEROFFLINE || newState==USERINVISIBLE){
		if(newState!=CurrentOnlineState)
			emit connectionOffline();

	}

	this->CurrentOnlineState=newState;
	emit OnlineStateChanged();
}

QColor cUser::get_textColor()
{
	return textColor;
}

void cUser::set_textColor(QColor textColor)
{
	this->textColor=textColor;
}

void cUser::set_textFont(QFont textFont)
{
	this->textFont=textFont;
}

QFont cUser::get_textFont()
{
	return textFont;
}

void cUser::IncomingMessageFromSystem(QString newMessage)
{
	this->Messages.push_back("[System]("+ QTime::currentTime().toString("hh:mm:ss") +"): "+newMessage+"<br>");	
	this->newUnreadMessages=true;
	emit newMessageRecived();
	emit newIncomingMessageRecived();
}
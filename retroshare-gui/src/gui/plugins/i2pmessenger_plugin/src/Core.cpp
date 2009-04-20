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
#include "Core.h"

cCore::cCore(){
	init();
}

cCore::~cCore(){
	this->UserConnectThread->stop();
	this->saveUserList();
	this->closeAllActiveConnections();
	for(int i=0;i<this->users.count();i++)
		delete users.at(i);
		
	QList<cPacketManager*>::Iterator it;
	for(it=DataPacketsManagers.begin(); it<DataPacketsManagers.end() ;++it){
		DataPacketsManagers.erase(it);
	}
		
	delete this->DebugMessageHandler;
	delete this->Protocol;
	delete this->I2P;
	delete this->SoundManager;
}

cDebugMessageManager* cCore::get_DebugMessageHandler(){
	return this->DebugMessageHandler;
}

bool cCore::addNewUser(QString Name,QString I2PDestination,qint32 I2PStream_ID){
	//TODO I2PDestination verify check
	//check if user already exist

	if(I2PDestination.length()!=516){
		//Destination must be 516 length
		return false;
	}
	
	
	if(!I2PDestination.right(4).contains("AAAA",Qt::CaseInsensitive)){
		//the last 4 char must be "AAAA"
		return false;
	}


	if(I2PDestination.length()>0)
		if(this->doesUserAllReadyExitsByI2PDestination(I2PDestination)==true)
			return false;


	//add newuser
	cUser* newuser=new cUser(Protocol,Name,I2PDestination,I2PStream_ID);
	connect(newuser,SIGNAL(newIncomingMessageRecived()),SoundManager,
		SLOT(event_NewChatMessage()));

	connect(newuser,SIGNAL(connectionOnline()),SoundManager,
		SLOT(event_User_go_Online()));

	connect(newuser,SIGNAL(connectionOffline()),SoundManager,
		SLOT(event_User_go_Offline()));

	this->users.append(newuser);
	saveUserList();
	
	emit eventUserChanged();
	return true;
}

bool cCore::deleteUserByI2PDestination(QString I2PDestination){
	for(int i=0;i<users.count();i++){
		if(users.at(i)->get_I2PDestination()==I2PDestination){
			if(users.at(i)->get_ConnectionStatus()==ONLINE ||users.at(i)->get_ConnectionStatus()==TRYTOCONNECT)
			{
				this->StreamClose(users.at(i)->get_I2PStreamID());
				deletePacketManagerByID(users.at(i)->get_I2PStreamID());
			}
			
			users.removeAt(i);
			saveUserList();
			emit eventUserChanged();
			return true;
		}
	}
	return false;
}


bool cCore::doesUserAllReadyExitsByI2PDestination(const QString I2PDestination){
	if(I2PDestination==MyDestination) return true;

	for(int i=0;i<users.count();i++){
		if(users.at(i)->get_I2PDestination()==I2PDestination){
			
			return true;
		}
	}
	
	return false;
}
void cCore::doNamingLookUP(QString Name){
	I2P->doNamingLookUP(Name);
}

void cCore::init(){
	using namespace SESSION_Types;
	this->MyDestination="";
	this->currentOnlineStatus=User::USERTRYTOCONNECT;

	QSettings* settings= new QSettings(QApplication::applicationDirPath()+"/application.ini",QSettings::IniFormat);
	settings->beginGroup("Network");

	this->I2P= new cConnectionI2P(	settings->value("SamHost","127.0.0.1").toString(),
					settings->value("SamPort","7656").toString(),
					STREAM,
					settings->value("Destination","test").toString(),
					BOTH,
					settings->value("SessionOptionString","").toString()
					,this);

	this->I2P=I2P;
	
	//signals from I2PConnection Core
	connect (I2P,SIGNAL(StreamClosedRecived(const SAM_Message_Types::RESULT, qint32, QString)),this,
		SLOT(StreamClosedRecived(const SAM_Message_Types::RESULT, qint32, QString)),Qt::DirectConnection);
	
	connect (I2P,SIGNAL(StreamStatusRecived(const SAM_Message_Types::RESULT, const qint32, const QString)),this,
		SLOT(StreamStatusRecived(const SAM_Message_Types::RESULT, const qint32, QString)),Qt::DirectConnection);

	connect (I2P,SIGNAL(SessionStatusRecived(const SAM_Message_Types::RESULT, const QString, const QString)),this,
		SLOT(SessionStatusRecived(const SAM_Message_Types::RESULT, const QString, QString)),Qt::DirectConnection);
	
	connect (I2P,SIGNAL(StreamConnectedRecived(const QString, const qint32)),this,
		SLOT(StreamConnectedRecived(const QString, const qint32)),Qt::DirectConnection);

	connect (I2P,SIGNAL(StreamReadyToSendRecived(const qint32)),this,
		SLOT(StreamReadyToSendRecived(const qint32)),Qt::DirectConnection);

	connect (I2P,SIGNAL(StreamSendRecived(const qint32, const SAM_Message_Types::RESULT, SAM_Message_Types::STATE)),this,
		SLOT(StreamSendRecived(const qint32, const SAM_Message_Types::RESULT, SAM_Message_Types::STATE)),Qt::DirectConnection);
	
	connect (I2P,SIGNAL(StreamDataRecived(const qint32, const QString, const QByteArray)),this,
		SLOT(StreamDataRecived(const qint32, const QString, const QByteArray)),Qt::DirectConnection);

	connect (I2P,SIGNAL(NamingReplyRecived(const SAM_Message_Types::RESULT, QString, QString, QString)),this,
		SLOT(NamingReplyRecived(const SAM_Message_Types::RESULT, QString, QString, QString)),Qt::DirectConnection);

	this->SoundManager=new cSoundManager();
	this->DebugMessageHandler= new cDebugMessageManager(I2P);
	this->Protocol= new cProtocol(this);
	this->loadUserList();
	this->I2P->doConnect();

	this->UserConnectThread= new cUserConnectThread(this,settings->value("Waittime_between_rechecking_offline_users","30000").toInt());

	delete settings;	
}
void cCore::saveUserList(){
     QFile file(QApplication::applicationDirPath()+"/users.config");
     file.open(QIODevice::WriteOnly | QIODevice::Text);
     QTextStream out(&file);

	for(int i=0;i<this->users.count();i++){		
		out<<"Nick:\t"<<(users.at(i)->get_Name())<<endl
		<<"I2PDest:\t"<<(users.at(i)->get_I2PDestination())<<endl;
	}
	out.flush();
	file.close();
}
void cCore::loadUserList(){
	QFile file(QApplication::applicationDirPath()+"/users.config");
     	if (!file.open(QIODevice::ReadOnly|QIODevice::Text))
		return;

 	QTextStream in(&file);
	in.skipWhiteSpace();
	QString NickName;QString I2PDest;
     	while (!in.atEnd()) {
		QString line = in.readLine();
		QStringList temp=line.split("\t");
	
		if(temp[0]=="Nick:")NickName=temp[1];	
		if(temp[0]=="I2PDest:"){
			I2PDest=temp[1];
			this->addNewUser(NickName,I2PDest);
			NickName.clear();
			I2PDest.clear();
		}
		else if(temp[0]=="TorDest:"){
			//ignore it
		}
	file.close();	
	}
}

void cCore::StreamClosedRecived(const SAM_Message_Types::RESULT result,qint32 ID,QString Message){

	if(isThisID_a_FileSendID(ID)){
		//FileSend
		for(int i=0;i<FileSends.size();i++){
			if(FileSends.at(i)->get_StreamID()==ID){
				FileSends.at(i)->StreamClosed(result,ID,Message);
				FileSends.removeAt(i);
				return;
			}
		}
		
	}
	else if(isThisID_a_FileReciveID(ID)){
		//Filerecive
		for(int i=0;i<FileRecives.size();i++){
			if(FileRecives.at(i)->get_StreamID()==ID){
				FileRecives.at(i)->StreamClosed(result,ID,Message);
				FileRecives.removeAt(i);
				return;
			}
		}
	}
	else if(this->isThisIDunknown(ID)==true){
	//if ID=UnKnown then remove the ID
		this->removeUnknownID(ID);
	}

	//known ID

	for(int i=0;i<users.count();i++){
		if(users.at(i)->get_I2PStreamID()==ID){
			if(	result==SAM_Message_Types::OK ||
				result==SAM_Message_Types::CANT_REACH_PEER ||
				result==SAM_Message_Types::TIMEOUT
			){
				users.at(i)->set_ConnectionStatus(OFFLINE);
				break;
			}
			else {
				//on I2P_ERROR or PEER_NOT_FOUND
				users.at(i)->set_ConnectionStatus(ERROR);
				break;
			}
		}
	}
	deletePacketManagerByID(ID);
	emit eventUserChanged();
	
}

void cCore::StreamStatusRecived(const SAM_Message_Types::RESULT result,const qint32 ID,QString Message){
	
	if(isThisID_a_FileSendID(ID)){
		//FileSend
		for(int i=0;i<FileSends.size();i++){
			if(FileSends.at(i)->get_StreamID()==ID){
				FileSends.at(i)->StreamStatus(result,ID,Message);
				if(result!=SAM_Message_Types::OK){
					I2P->doStreamClose(ID);
					FileSends.removeAt(i);	
				}
				return;
			}
		}
		
	}
	else if(isThisID_a_FileReciveID(ID)){
		//Filerecive
		for(int i=0;i<FileRecives.size();i++){
			if(FileRecives.at(i)->get_StreamID()==ID){
				FileRecives.at(i)->StreamStatus(result,ID,Message);
				if(result!=SAM_Message_Types::OK){
					I2P->doStreamClose(ID);
					FileRecives.removeAt(i);
				}
				return;
			}
		}
	}
	else if(this->isThisIDunknown(ID)){
		if(result==SAM_Message_Types::OK){
			Protocol->newConnectionChat(ID);
			return;
		}
		else{
			//I2P->doStreamClose(ID);
			removeUnknownID(ID);
		}
		
	}

	for(int i=0;i<users.count();i++){
		if(users.at(i)->get_I2PStreamID()==ID){
			if(result==SAM_Message_Types::OK){
				users.at(i)->set_I2PStreamID(ID);
				users.at(i)->set_ConnectionStatus(ONLINE);
				
                                //this->Protocol->newConnectionChat(ID);
			}
			else if( result==SAM_Message_Types::CANT_REACH_PEER || 
				 result==SAM_Message_Types::TIMEOUT
				){
				
				if(users.at(i)->get_OnlineState()!=USEROFFLINE && users.at(i)->get_OnlineState()!=USERTRYTOCONNECT)
				{
					users.at(i)->IncomingMessageFromSystem("The Connection is broken: "+Message+"\nConnection closed");
					I2P->doStreamClose(ID);
				}
				deletePacketManagerByID(ID);
				users.at(i)->set_ConnectionStatus(OFFLINE);
			}
			else if( result==SAM_Message_Types::INVALID_KEY){
				users.at(i)->IncomingMessageFromSystem("Invalid User - Destination: please delete the user\n");
				
				if(users.at(i)->get_ConnectionStatus()==ONLINE)
					I2P->doStreamClose(ID);

				deletePacketManagerByID(ID);
				users.at(i)->set_ConnectionStatus(ERROR);
				
			}
			else if(result==SAM_Message_Types::I2P_ERROR){
				users.at(i)->IncomingMessageFromSystem("I2P_Error: "+Message);

				if(users.at(i)->get_ConnectionStatus()==ONLINE)
					I2P->doStreamClose(ID);
				
				deletePacketManagerByID(ID);
				users.at(i)->set_ConnectionStatus(ERROR);
			}
		
			emit eventUserChanged();
			break;	
		}
	}
	
}
void cCore::SessionStatusRecived(const SAM_Message_Types::RESULT result,const QString Destination,const QString Message)
{
	if(result==(SAM_Message_Types::OK)){
		currentOnlineStatus=User::USERONLINE;
		emit eventOnlineStatusChanged();
		I2P->doNamingLookUP("ME");//get the current Destination from this client
		this->UserConnectThread->start();
		
	}
}
void cCore::StreamConnectedRecived(const QString Destinaton,const qint32 ID){
	//Someone connected you
	//this->Protocol->newConnection(ID);
	cConnection t(ID,Destinaton);
	this->unknownConnections.push_back(t);
}
bool cCore::removeUnknownID(qint32 ID)
{
	for(int i=0;i<this->unknownConnections.size();i++){
		if(unknownConnections.at(i).ID==ID){
			unknownConnections.removeAt(i);
			return true;
		}
	}	
	
	return false;
}
QString cCore::get_UserProtocolVersionByStreamID(qint32 ID){

	for(int i=0;i< users.size();i++)
		if(users.at(i)->get_I2PStreamID()==ID){
			
			return users.at(i)->get_ProtocolVersion();
		}
	
	return "";
}
void cCore::set_UserProtocolVersionByStreamID(qint32 ID,QString Version){

	for(int i=0;i< users.size();i++)
		if(users.at(i)->get_I2PStreamID()==ID){
			users.at(i)->set_ProtocolVersion(Version);
			
			return;
		}
	
}

void cCore::removeUnknownIDCreateUserIfNeeded(const qint32 ID,const QString ProtocolVersion){
	//TODO add some security thinks for adding !!! at the moment all user are allowed to connect	
	
	QString Destinaton;

	for(int i=0;i<unknownConnections.size();i++)
		if(unknownConnections.at(i).ID==ID){
	
			Destinaton=unknownConnections.at(i).Destination;
			removeUnknownID(ID);
			break;
		}
	
	
	if(doesUserAllReadyExitsByI2PDestination(Destinaton)==false){
		addNewUser("Unknown",Destinaton,ID);
	}

	for(int i=0;i<users.count();i++){
		if(users.at(i)->get_I2PDestination()==Destinaton){	
			if(users.at(i)->get_ConnectionStatus()==OFFLINE){
					users.at(i)->set_ProtocolVersion(ProtocolVersion);
					users.at(i)->set_I2PStreamID(ID);
					users.at(i)->set_ConnectionStatus(ONLINE);
				
					cPacketManager* newPacket=new cPacketManager(ID);
					connect(newPacket,SIGNAL(aPacketIsComplead(const qint32, const QByteArray)),Protocol,
					SLOT(inputKnown(const qint32,const QByteArray)));

					DataPacketsManagers.push_back(newPacket);
				}
			else if(users.at(i)->get_ConnectionStatus()==ONLINE){		
					/*
						//close both Streams
						if(ID!=users.at(i)->get_I2PStreamID())
						{
							I2P->doStreamClose(ID);
							I2P->doStreamClose(users.at(i)->get_I2PStreamID());
						}
					*/
				//close new connection and use the old
				if(ID!=users.at(i)->get_I2PStreamID())
					I2P->doStreamClose(ID);
			}
			else if(users.at(i)->get_ConnectionStatus()==TRYTOCONNECT){
					/*
						//Stop the TRYTOCONNECT
						if(ID!=users.at(i)->get_I2PStreamID())
							I2P->doStreamClose(users.at(i)->get_I2PStreamID());
		
						users.at(i)->set_ProtocolVersion(ProtocolVersion);
						users.at(i)->set_I2PStreamID(ID);
						users.at(i)->set_ConnectionStatus(ONLINE);
					*/
				//close the tryconnection use the new one
				if(ID!=users.at(i)->get_I2PStreamID())
						I2P->doStreamClose(users.at(i)->get_I2PStreamID());
				users.at(i)->set_ProtocolVersion(ProtocolVersion);
				users.at(i)->set_I2PStreamID(ID);
				users.at(i)->set_ConnectionStatus(ONLINE);

				cPacketManager* newPacket=new cPacketManager(ID);
				connect(newPacket,SIGNAL(aPacketIsComplead(const qint32, const QByteArray)),Protocol,
				SLOT(inputKnown(const qint32,const QByteArray)));

				DataPacketsManagers.push_back(newPacket);
			}

			
			
			
			break;
		}
	}
	
	emit eventUserChanged();
}

void cCore::StreamReadyToSendRecived(const qint32 ID){
	
	//FileSendsConnections
	for(int i=0;i<FileSends.size();i++){
		if(FileSends.at(i)->get_StreamID()==ID){
			FileSends.at(i)->StreamReadyToSend(true);	
			return;
		}
	}
	
	//the Rest (Chatconnections)
	for(int i=0;i<users.count();i++){
		if(users.at(i)->get_I2PStreamID()==ID){
			users.at(i)->set_ReadyToSend(true);
			return;	
		}
	}
	
}
void cCore::StreamSendRecived(const qint32 ID,const SAM_Message_Types::RESULT result,SAM_Message_Types::STATE state){
//FIXME what do when result = FAILED ?, impl. a stack ?

	//FileSendsConnections
	for(int i=0;i<FileSends.size();i++){
		if(FileSends.at(i)->get_StreamID()==ID){
			if(state==SAM_Message_Types::READY){
				FileSends.at(i)->StreamReadyToSend(true);
				return;
			}
			else{
				FileSends.at(i)->StreamReadyToSend(false);
				return;
			}
		}
	}
	
	//the Rest (Chatconnections)
	for(int i=0;i<users.count();i++){
		if(users.at(i)->get_I2PStreamID()==ID){
			if(state==SAM_Message_Types::BUFFER_FULL)users.at(i)->set_ReadyToSend(false);
			if(state==SAM_Message_Types::READY)users.at(i)->set_ReadyToSend(true);
		}
	}
}
const QList<cUser*> cCore::get_userList(){
	return users;
}
qint32 cCore::StreamConnect(QString Destination){
	qint32 ID;
	
	ID=I2P->doStreamConnect(Destination);
	
	cConnection t(ID,Destination);
	this->unknownConnections.push_back(t);
	return (ID);
}

void cCore::StreamDataRecived(const qint32 ID,const QString Size,const QByteArray Data){
	
	if(Data.isEmpty()==true)return;

	//FileRecive
	for(int i=0;i<FileRecives.size();i++){
		if(FileRecives.at(i)->get_StreamID()==ID){
			FileRecives.at(i)->operator <<(Data);
			return;
		}
	}

	//FileSend
	for(int i=0;i<FileSends.size();i++){
		if(FileSends.at(i)->get_StreamID()==ID){
			FileSends.at(i)->operator <<(Data);
			return;
		}
	}
	//unknown connection
	if(this->isThisIDunknown(ID)){
		QByteArray Data2=Protocol->inputUnknown(ID,Data);
		if(Data2.isEmpty()==true)
			return;
		else{
			QList<cPacketManager*>::Iterator it;
			for(it=DataPacketsManagers.begin(); it!=DataPacketsManagers.end() ;++it){
				if((*(it))->getID()==ID){
					//(*(it))->operator <<(Data);
					(*(*it))<<Data2;
					return;
				}
			}

			/*
			QString StringSize=QString::number(Data2.length());
			
			StreamDataRecived(ID,StringSize,Data2);
			
			return;
			*/
		}		
	}
	
	//normal connection
	QList<cPacketManager*>::Iterator it;
	for(it=DataPacketsManagers.begin(); it!=DataPacketsManagers.end() ;++it){
		if((*(it))->getID()==ID){
			//(*(it))->operator <<(Data);
			(*(*it))<<Data;
			return;
		}
	}


	
	//Ignore Data
	//if you close a stream i can happen that the QSocket already have recive same data of that Stream (and send it to the Socket)
	//if it happen forget the Data
}

void cCore::closeAllActiveConnections(){
	//close all known Online||TrytoConnect Connections

	for(int i=0;i<users.size();i++)
		if(users.at(i)->get_ConnectionStatus()==ONLINE ||
			users.at(i)->get_ConnectionStatus()==TRYTOCONNECT)
		{
			deletePacketManagerByID(users.at(i)->get_I2PStreamID());
			I2P->doStreamClose(users.at(i)->get_I2PStreamID());
			//removeUnknownID(users.at(i)->get_I2PStreamID());

			users.at(i)->set_ConnectionStatus(User::OFFLINE);
			users.at(i)->set_OnlineState(USEROFFLINE);
				
		}
	//close all unknownConnections
	for(int i=0;i<unknownConnections.size();i++)
	{
		I2P->doStreamClose(unknownConnections.at(i).ID);
		removeUnknownID( unknownConnections.at(i).ID);
	}

	//DataPacketsManagers.clear();

	emit eventUserChanged();
	
}

bool cCore::isThisIDunknown(qint32 ID){

	for(int i=0;i<this->unknownConnections.size();i++)
		if(unknownConnections.at(i).ID==ID){
			
			return true;
		}
	
	return false;
}
cUser* cCore::getUserByI2P_ID(qint32 ID){

	for(int i=0;i<users.size();i++){
		if(users.at(i)->get_I2PStreamID()==ID){
			
			return users.at(i);
		}
	}
	
	return NULL;
}
cUser* cCore::getUserByI2P_Destination(QString Destination){

	for(int i=0;i<users.size();i++){
		if(users.at(i)->get_I2PDestination()==Destination){
			
			return users.at(i);
		}
	}
	
	return NULL;
}
void cCore::NamingReplyRecived(const SAM_Message_Types::RESULT result,QString Name,QString Value,QString Message){
	if(result==SAM_Message_Types::OK && Name=="ME")
		this->MyDestination=Value;
}
const QString cCore::getMyDestination()const{
	return this->MyDestination;
}
bool cCore::isThisDestinationInunknownConnections(QString Destination){
	for(int i=0;i<unknownConnections.size();i++){
		if(unknownConnections.at(i).Destination==Destination)return true;
	}
	return false;
}
	
void cCore::deletePacketManagerByID(qint32 ID){
	if(this->isThisIDunknown(ID)==true) 
		return;
	else
	{
		QList<cPacketManager*>::Iterator it;
		for(it=DataPacketsManagers.begin(); it!=DataPacketsManagers.end() ;++it){
			if((*(it))->getID()==ID){
				//delete (*(it));
				DataPacketsManagers.erase(it);
				break;
				
			}
		}
	}
}

QString cCore::get_connectionDump(){
	QString Message;

	Message+="< Current open Unknown IDs: >\n";
	for(int i=0;i<unknownConnections.size();i++)
		Message+=QString::number(unknownConnections.at(i).ID,10)+"\n";

	Message+="\n\n< Current open Known IDs(packetManager): >\n";

	QList<cPacketManager*>::Iterator it;
	for(it=DataPacketsManagers.begin(); it!=DataPacketsManagers.end();++it)
		Message+= (QString::number((*(it))->getID(),10)+"\n");

	Message+="\n\n< Active FileTransfer: >\n";
	for(int i=0;i<FileSends.count();i++){
		Message+="FileName:\t\t:"+FileSends.at(i)->get_FileName()+"\n";
		Message+="StreamID:\t\t"+QString::number(FileSends.at(i)->get_StreamID(),10)+"\n";
	}

	Message+="\n\n< Active Filerecive: >\n";
	for(int i=0;i<FileRecives.count();i++){
		Message+="FileName:\t\t"+FileRecives.at(i)->get_FileName()+"\n";
		Message+="StreamID:\t\t"+QString::number(FileRecives.at(i)->get_StreamID(),10)+"\n";
	}
		

	Message+="\n\n< USER-Infos: >\n";
	for(int i=0;i< users.count();i++){
		Message+="Name:\t\t"+users.at(i)->get_Name()+"\n";
		Message+="ClientName:\t\t"+users.at(i)->get_ClientName()+"\n";
		Message+="ClientVersion:\t\t"+users.at(i)->get_ClientVersion()+"\n";
		Message+="ProtocolVersion:\t" +users.at(i)->get_ProtocolVersion()+"\n";
		Message+="I2PStreamID:\t\t" +QString::number(users.at(i)->get_I2PStreamID(),10)+"\n";
		
		switch(users.at(i)->get_ConnectionStatus())
		{
			case User::ONLINE:
			{	
				Message+="ConnectionStatus:\tOnline\n\n";
				break;
			}
			case User::OFFLINE:
			{	
				Message+="ConnectionStatus:\tOffline/Invisible\n\n";
				break;
			}
			case User::TRYTOCONNECT:
			{
				Message+="ConnectionStatus:\tTryToConnect\n\n";
				break;

			}

			case User::ERROR:
			{
				Message+="ConnectionStatus: (Stream)\tError\n\n";
				break;

			}
		};
	}
	return Message;
}

bool cCore::renameuserByI2PDestination(const QString Destination, const QString newNickname){
	for(int i=0;i<users.size();i++){
		if(users.at(i)->get_I2PDestination()==Destination){
			users.at(i)->set_Name(newNickname);
			emit eventUserChanged();
			return true;
		}
	}
	return false;
}

ONLINESTATE cCore::getOnlineStatus() const
{
	return this->currentOnlineStatus;
}

void cCore::setOnlineStatus(const ONLINESTATE newStatus)
{
	if(currentOnlineStatus==newStatus) return;

	if(newStatus==User::USEROFFLINE)
	{
		stopCore();
		this->currentOnlineStatus=newStatus;
	}
	else if(newStatus==User::USERTRYTOCONNECT)
	{
		if(currentOnlineStatus==User::USEROFFLINE)
		{
			restartCore();	
			this->currentOnlineStatus=newStatus;
		}
	}
	else
	{
		//send new Status to every connected User
		this->currentOnlineStatus=newStatus;

		for(int i=0;i<users.size();i++){
			if(users.at(i)->get_ConnectionStatus()==ONLINE)
			{
				switch(this->currentOnlineStatus)
				{
					case USERONLINE:
					{
						Protocol->send(USER_ONLINESTATUS_ONLINE,users.at(i)->get_I2PStreamID(),"");
						break;
					}
					case USEROFFLINE:
					case USERINVISIBLE:
					{
						Protocol->send(USER_ONLINESTATUS_OFFLINE,users.at(i)->get_I2PStreamID(),"");
						break;
					}
					case USERAWAY:
					{
						Protocol->send(USER_ONLINESTATUS_AWAY,users.at(i)->get_I2PStreamID(),"");
						break;
						
					}
					case USERWANTTOCHAT:
					{
						Protocol->send(USER_ONLINESTATUS_WANTTOCHAT,users.at(i)->get_I2PStreamID(),"");
						break;
					}
					case USERDONT_DISTURB:
					{
						Protocol->send(USER_ONLINESTATUS_DONT_DISTURB,users.at(i)->get_I2PStreamID(),"");
						break;
					}
					default:
					{
						QMessageBox* msgBox= new QMessageBox(NULL);
						msgBox->setIcon(QMessageBox::Warning);
						msgBox->setText("cCore(setOnlineStatus)");
						msgBox->setInformativeText("Unknown USERSTATE");
						msgBox->setStandardButtons(QMessageBox::Ok);
						msgBox->setDefaultButton(QMessageBox::Ok);
						msgBox->setWindowModality(Qt::NonModal);
						msgBox->show();
	
					}
				}
			
	
			}//if
		}//for
	}
	emit eventOnlineStatusChanged();
}

void cCore::stopCore()
{
	UserConnectThread->stop();
	closeAllActiveConnections();
	I2P->doDisconnect();
}

void cCore::restartCore()
{
	I2P->doConnect();
	UserConnectThread->start();
}

void cCore::addNewFileTransfer(QString FilePath, QString Destination)
{
	cFileTransferSend * t= new cFileTransferSend(this,FilePath,Destination);
	connect (t,SIGNAL(event_FileTransferFinishedOK()),SoundManager,
		SLOT(event_FileSend_Finished()));

	FileSends.append(t);

}

bool cCore::isThisID_a_FileSendID(qint32 ID)
{
	for(int i=0;i<FileSends.size();i++){
		if(FileSends.at(i)->get_StreamID()==ID){
			return true;
		}
	}
	return false;
}

bool cCore::isThisID_a_FileReciveID(qint32 ID)
{
	for(int i=0;i<FileRecives.size();i++){
		if(FileRecives.at(i)->get_StreamID()==ID){
			return true;
		}
	}
	return false;
}

void cCore::addNewFileRecive(qint32 ID, QString FileName, QString FileSize)
{
	if(isThisIDunknown(ID)) removeUnknownID(ID);

	quint64 Size;
	bool OK;
	Size=FileSize.toULongLong(&OK,10);
	if(OK==false)
	{
		QMessageBox* msgBox= new QMessageBox(NULL);
		msgBox->setIcon(QMessageBox::Critical);
		msgBox->setText("cCore(addNewFileRecive)");
		msgBox->setInformativeText("Error convert QString to Quint64\nValue: "+FileSize +"\nFilerecive aborted");
		msgBox->setStandardButtons(QMessageBox::Ok);
		msgBox->setDefaultButton(QMessageBox::Ok);
		msgBox->setWindowModality(Qt::NonModal);
		msgBox->exec();

		//abort the Filerecive
		this->StreamSendData(ID,QString("1"));//false
		this->StreamClose(ID);
		return;
	}
	SoundManager->event_FileRecive_Incoming();
	cFileTransferRecive* t= new cFileTransferRecive(this,ID,FileName,Size);
	connect(t,SIGNAL(event_FileRecivedFinishedOK()),SoundManager,
		SLOT(event_FileRecive_Finished()));

	FileRecives.append(t);
	t->start();
}


void cCore::StreamSendData(qint32 ID, QByteArray Data)
{
	I2P->doStreamSend(ID,Data);
}


void cCore::StreamSendData(qint32 ID, QString Data)
{
	I2P->doStreamSend(ID,Data);
}

void cCore::StreamClose(qint32 ID)
{
	if(FileSends.count()>0){
		for(int i=0;i<FileSends.count();i++){
			if(FileSends.at(i)->get_StreamID()==ID){
				FileSends.removeAt(i);
				break;
			}
		}	
	}
	
	if(FileRecives.count()>0){
		for(int i=0;i<FileRecives.count();i++){
			if(FileRecives.at(i)->get_StreamID()==ID){
				FileRecives.removeAt(i);
				break;
			}
		}
	}
	I2P->doStreamClose(ID);
}

bool cCore::checkIfAFileTransferOrReciveisActive()
{
	if(FileSends.count()>0) return true;
	if(FileRecives.count()>0) return true;

	return false;
}

void cCore::MuteSound(bool t)
{
	SoundManager->doMute(t);
}


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
#include "Protocol.h"
#include "Core.h"
#include "User.h"

cProtocol::cProtocol(cCore * Core){
	this->Core=Core;

	connect (this,SIGNAL(eventUserChanged()),Core,SIGNAL(eventUserChanged()));

}

void cProtocol::newConnectionChat(const qint32 ID){
	using namespace Protocol_Info;
	//send the ChatSystem\tProtocolVersion
	Core->StreamSendData(ID,FIRSTPAKETCHAT);
}
void cProtocol::inputKnown(const qint32 ID, const QByteArray Data){
using namespace Protocol_Info;
	
	if(Data.length()<4) 
		return;

	QString ProtocolInfoTag(Data.left(4));
	
		//COMMANDS
			if(ProtocolInfoTag=="1000"){//PING:
				send(ECHO_OF_PING,ID,"");
			}
			else if(ProtocolInfoTag=="1001"){//GET_CLIENTVERSION:
				send(ANSWER_OF_GET_CLIENTVERSION,ID,Core->get_ClientVersion());
			}
			else if(ProtocolInfoTag=="1002"){//GET_CLIENTNAME:
				send(ANSWER_OF_GET_CLIENTNAME,ID,Core->get_ClientName());
			}
			else if(ProtocolInfoTag=="1003"){//GET_USER_ONLINESTATUS:
				//TODO sendOnlineStatus
				switch(Core->getOnlineStatus())
				{
					case USERONLINE:
					{
						send(USER_ONLINESTATUS_ONLINE,ID,"");
						break;
					}
					case USEROFFLINE:
					case USERINVISIBLE:
					{
						send(USER_ONLINESTATUS_OFFLINE,ID,"");
						break;
					}
					case USERAWAY:
					{
						send(USER_ONLINESTATUS_AWAY,ID,"");
						break;
						
					}
					case USERWANTTOCHAT:
					{
						send(USER_ONLINESTATUS_WANTTOCHAT,ID,"");
						break;
					}
					case USERDONT_DISTURB:
					{
						send(USER_ONLINESTATUS_DONT_DISTURB,ID,"");
						break;
					}
					default:
					{
						QMessageBox* msgBox= new QMessageBox(NULL);
						msgBox->setIcon(QMessageBox::Warning);
						msgBox->setText("cProtocol(inputKnown)");
						msgBox->setInformativeText("Unknown USERSTATE");
						msgBox->setStandardButtons(QMessageBox::Ok);
						msgBox->setDefaultButton(QMessageBox::Ok);
						msgBox->setWindowModality(Qt::NonModal);
						msgBox->show();

					}
				}

			}

		//end of commands
		

		//Messages
			else if(ProtocolInfoTag=="0001"){//ANSWER_OF_GET_CLIENTVERSION
				cUser* thisUser=Core->getUserByI2P_ID(ID);
				if(thisUser!=NULL){	
					QString ClientVersion=Data.mid(4);
					thisUser->set_ClientVersion(ClientVersion);
				}
			
			}
			else if(ProtocolInfoTag=="0002"){//ANSWER_OF_GET_CLIENTNAME
				cUser* thisUser=Core->getUserByI2P_ID(ID);
				if(thisUser!=NULL){	
					QString Clientname=Data.mid(4);
					thisUser->set_ClientName(Clientname);
				}
			
			}
			else if(ProtocolInfoTag=="0003"){//chatmessage
				cUser* thisUser=Core->getUserByI2P_ID(ID);
				if(thisUser!=NULL){	
					QByteArray temp=Data.mid(4);
					thisUser->IncomingNewChatMessage(temp);
					emit eventUserChanged();
				}	
			}
			else if(ProtocolInfoTag=="0004"){//USER_ONLINESTATUS_ONLINE
				cUser* thisUser=Core->getUserByI2P_ID(ID);
				if(thisUser!=NULL){	
					QByteArray temp=Data.mid(4);
					thisUser->set_OnlineState(USERONLINE);
					emit eventUserChanged();
				}
			}
			else if(ProtocolInfoTag=="0005"){//USER_ONLINESTATUS_OFFLINE || USER_ONLINESTATUS_INVISIBLE
				cUser* thisUser=Core->getUserByI2P_ID(ID);
				if(thisUser!=NULL){	
					QByteArray temp=Data.mid(4);
					thisUser->set_OnlineState(USEROFFLINE);
					emit eventUserChanged();

				}
			}
			else if(ProtocolInfoTag=="0006"){//USER_ONLINESTATUS_WANTTOCHAT
				cUser* thisUser=Core->getUserByI2P_ID(ID);
				if(thisUser!=NULL){	
					QByteArray temp=Data.mid(4);
					thisUser->set_OnlineState(USERWANTTOCHAT);
					emit eventUserChanged();

				}
			}
			else if(ProtocolInfoTag=="0007"){//USER_ONLINESTATUS_AWAY
				cUser* thisUser=Core->getUserByI2P_ID(ID);
				if(thisUser!=NULL){	
					QByteArray temp=Data.mid(4);
					thisUser->set_OnlineState(USERAWAY);
					emit eventUserChanged();
				}
			}
			else if(ProtocolInfoTag=="0008"){//USER_ONLINESTATUS_DONT_DISTURB
				cUser* thisUser=Core->getUserByI2P_ID(ID);
				if(thisUser!=NULL){	
					QByteArray temp=Data.mid(4);
					thisUser->set_OnlineState(USERDONT_DISTURB);
					emit eventUserChanged();
				}
			}	
		//end Messages
}

QByteArray cProtocol::inputUnknown(const qint32 ID, const QByteArray Data){
using namespace Protocol_Info;

	if(Core->isThisIDunknown(ID)==true){
	//check if First Paket = from a other CHATSYSTEM
		if(Data.contains("CHATSYSTEM\t")==true){
			QByteArray temp=Data.mid(Data.indexOf("\t")+1,Data.indexOf("\n")-Data.indexOf("\t")-1);
			QString version(temp);

			//dont send the firstpacket if you have connected someone
			//(the firstpacket is sended from core::StreamStatusRecived)
						
			if(ID < 0)
				newConnectionChat(ID);//someone connect you

			Core->removeUnknownIDCreateUserIfNeeded(ID,version);
			//remove Firstpacket
			QByteArray Data2=Data;
			Data2=Data2.remove(0,Data.indexOf("\n")+1);
			
			return Data2;
		}
		else if(Data.contains("CHATSYSTEMFILETRANSFER\t")==true)
		{
			//FIRSTPAKET ="CHATSYSTEMFILETRANSFER\t"+PROTOCOLVERSION+"\nSizeinBit\nFileName";
			QByteArray Data2=Data;

			QString ProtovolVersion=Data2.mid(Data.indexOf("\t")+1,Data2.indexOf("\n")-Data2.indexOf("\t")-1);
				Data2.remove(0,Data2.indexOf("\n")+1);//CHATSYSTEMFILETRANSFER\tPROTOCOLVERSION
			
			QString FileSize=Data2.mid(0,Data2.indexOf("\n"));
				Data2.remove(0,Data2.indexOf("\n")+1);

			QString FileName=Data2;
			
			Core->removeUnknownID(ID);
			Core->addNewFileRecive(ID,FileName,FileSize);
			Data2.clear();
			return Data2;

		}
		else{
			// not from a other CHATSYSTEM
			Core->StreamSendData(ID,HTTPPAGE);
			Core->StreamClose(ID);
			Core->removeUnknownID(ID);
			QByteArray Data2;
			Data2.clear();

			return Data2;
		}

	}
	//not possible
	return Data;
}

void cProtocol::send(const COMMANDS_TAGS TAG,const qint32 ID){
	using namespace Protocol_Info;
	QString ProtocolInfoTag;
	QString Data="";
	switch(TAG){
		case PING:
		{	
			ProtocolInfoTag="1000";
			break;
		}
		case GET_CLIENTVERSION:
		{
			ProtocolInfoTag="1001";
			break;
		}
		case GET_CLIENTNAME:
		{
			ProtocolInfoTag="1002";
			break;
		}
		case GET_USER_ONLINESTATUS:
		{
			ProtocolInfoTag="1003";
			break;
		}
		default:
		{
			break;
		}

	}
	Data.insert(0,ProtocolInfoTag);
	Data.insert(0,"0004");//No PaketData
	Core->StreamSendData(ID,Data);
}

void cProtocol::send(const MESSAGES_TAGS TAG,const qint32 ID,QString Data){
	QString ProtocolInfoTag;
	
	switch(TAG)
	{
		case ECHO_OF_PING:
		{
			ProtocolInfoTag="0000";
			break;
		}
		case ANSWER_OF_GET_CLIENTVERSION:
		{
			ProtocolInfoTag="0001";
			break;
		}
		case ANSWER_OF_GET_CLIENTNAME:
		{
			ProtocolInfoTag="0002";
			break;
		}
		case CHATMESSAGE:
		{	
			ProtocolInfoTag="0003";
			break;
		}
		case USER_ONLINESTATUS_ONLINE:
		{
			ProtocolInfoTag="0004";
			break;
		}
		case USER_ONLINESTATUS_OFFLINE:
		case USER_ONLINESTATUS_INVISIBLE:
		{
			ProtocolInfoTag="0005";
			break;
		}
		case USER_ONLINESTATUS_WANTTOCHAT:
		{
			ProtocolInfoTag="0006";
			break;
		}
		case USER_ONLINESTATUS_AWAY:
		{
			ProtocolInfoTag="0007";
			break;
		}
		case USER_ONLINESTATUS_DONT_DISTURB:
		{
			ProtocolInfoTag="0008";
			break;
		}
			
		default:
		{
			break;
		}	
	}
	QString temp;
	
	
	temp.setNum(Data.length()+4,16);//hex
	QString Paketlength=QString("%1").arg(temp,4,'0');

	Data.insert(0,ProtocolInfoTag);
	Data.insert(0,Paketlength);
	Core->StreamSendData(ID,Data);
}

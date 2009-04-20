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
#include "I2PSamMessageAnalyser.h"

I2PSamMessageAnalyser::I2PSamMessageAnalyser()
{
	//Constructor
}

const SAM_MESSAGE I2PSamMessageAnalyser::Analyse(QString Message)
{
	using namespace SAM_Message_Types;

	SAM_MESSAGE t;
	QStringList list =Message.split(" ",QString::SkipEmptyParts);

		
		if((list[0]=="STREAM") && (list[1]=="RECEIVED")){
			t.type=STREAM_RECEIVED;
			
			//get ID
			QStringList temp=list[2].split("=");
			QString sID=temp[1];
			

			t.ID=QStringToQint32(sID);
			//get Size
			temp= list[3].split("=");
			t.Size=temp[1].remove("\n");
		}
		else if((list[0].contains("STREAM")==true) && (list[1].contains("READY_TO_SEND")==true)){
			t.type=STREAM_READY_TO_SEND;

			//GET ID
			QStringList temp=list[2].split("=");
			QString sID=temp[1].remove("\n");

			t.ID=QStringToQint32(sID);
		}
		else if((list[0]=="HELLO") && (list[1]=="REPLY")){
			t.type=HELLO_REPLAY;

			if(list[2].contains("RESULT=OK")==true) t.result=OK;
			else if(list[2].contains("RESULT=NOVERSION")==true)t.result=NOVERSION;

		}	
		else if((list[0].contains("SESSION")==true) && (list[1].contains("STATUS")==true)){
			t.type=SESSION_STATUS;
			
			//Get Result
			if(list[2].contains("RESULT=OK",Qt::CaseInsensitive))t.result=OK;
			else if(list[2].contains("RESULT=DUPLICATED_DEST",Qt::CaseInsensitive))t.result=DUPLICATED_DEST;
			else if(list[2].contains("RESULT=I2P_ERROR",Qt::CaseInsensitive))t.result=I2P_ERROR;
			else if(list[2].contains("RESULT=INVALID_KEY",Qt::CaseInsensitive))t.result=INVALID_KEY;
			else
				t.type=ERROR_IN_ANALYSE;
			//----------------

			
			//Get Destination
			QStringList temp=list[3].split("=");
			
			t.Destination=temp[1].remove("\n");
			//----------------
			//Get Message 
			if(Message.contains("MESSAGE=",Qt::CaseInsensitive)){
				t.Message=Message.mid(Message.indexOf("MESSAGE=")+8);
				t.Message.remove("\n");
			}

			//----------------

		}
		else if((list[0].contains("STREAM")==true) && (list[1].contains("STATUS")==true)){
			t.type=STREAM_STATUS;

			//Get Result
			if(list[2].contains("RESULT=OK",Qt::CaseInsensitive))t.result=OK;
			else if(list[2].contains("RESULT=CANT_REACH_PEER",Qt::CaseInsensitive))t.result=CANT_REACH_PEER;
			else if(list[2].contains("RESULT=I2P_ERROR",Qt::CaseInsensitive))t.result=I2P_ERROR;
			else if(list[2].contains("RESULT=INVALID_KEY",Qt::CaseInsensitive))t.result=INVALID_KEY;
			else if(list[2].contains("RESULT=TIMEOUT",Qt::CaseInsensitive))t.result=TIMEOUT;
			else
			{
				t.type=ERROR_IN_ANALYSE;
				return t;
			}

			//----------------
			//Get ID
			QStringList temp=list[3].split("=");
			QString sID=temp[1].remove("\n");
			
			t.ID=QStringToQint32(sID);
			//----------------
			//Get Message 
			if(Message.contains("MESSAGE=",Qt::CaseInsensitive)){
				t.Message=Message.mid(Message.indexOf("MESSAGE=")+8);
				t.Message.remove("\n");
			}
			//----------------
		
		}
		else if((list[0].contains("STREAM")==true) && (list[1].contains("CONNECTED")==true)){
			t.type=STREAM_CONNECTED;

			QStringList temp=list[2].split("=");
			QString sID;
			t.Destination=temp[1];

			temp=list[3].split("=");
			sID=temp[1].remove("\n");

			t.ID=QStringToQint32(sID);
	
		}
		else if((list[0].contains("STREAM")==true) && (list[1].contains("SEND")==true)){
			t.type=STREAM_SEND;
		
			//get ID
			QStringList temp=list[2].split("=");
			QString sID=temp[1];
			t.ID=QStringToQint32(sID);
			
			//Get Result
			if(list[3].contains("RESULT=OK",Qt::CaseInsensitive))t.result=OK;
			else if(list[2].contains("RESULT=FAILED",Qt::CaseInsensitive))t.result=FAILED;

			//get STATE
			if(list[4].contains("STATE=BUFFER_FULL",Qt::CaseInsensitive))t.state=BUFFER_FULL;
			else if(list[4].contains("STATE=READY",Qt::CaseInsensitive))t.state=READY;
		
		}
		else if((list[0].contains("STREAM")==true) && (list[1].contains("CLOSED")==true)){
			t.type=STREAM_CLOSED;

			//Get RESULT
			if(list[2].contains("RESULT",Qt::CaseInsensitive))
			{

				if(list[2].contains("RESULT=OK",Qt::CaseInsensitive))t.result=OK;
				else if(list[2].contains("RESULT=CANT_REACH_PEER",Qt::CaseInsensitive))t.result=CANT_REACH_PEER;
				else if(list[2].contains("RESULT=I2P_ERROR",Qt::CaseInsensitive))t.result=I2P_ERROR;
				else if(list[2].contains("RESULT=PEER_NOT_FOUND",Qt::CaseInsensitive))t.result=PEER_NOT_FOUND;
				else if(list[2].contains("RESULT=TIMEOUT",Qt::CaseInsensitive))t.result=TIMEOUT;
				else{
					t.type=ERROR_IN_ANALYSE;
					return t;
				}

				//Get ID
				QStringList temp=list[3].split("=");
				QString sID=temp[1];
				t.ID=QStringToQint32(sID);
			}
			else
			{
				if(list[3].contains("RESULT=OK",Qt::CaseInsensitive))t.result=OK;
				else if(list[3].contains("RESULT=CANT_REACH_PEER",Qt::CaseInsensitive))t.result=CANT_REACH_PEER;
				else if(list[3].contains("RESULT=I2P_ERROR",Qt::CaseInsensitive))t.result=I2P_ERROR;
				else if(list[3].contains("RESULT=PEER_NOT_FOUND",Qt::CaseInsensitive))t.result=PEER_NOT_FOUND;
				else if(list[3].contains("RESULT=TIMEOUT",Qt::CaseInsensitive))t.result=TIMEOUT;
				else{
					t.type=ERROR_IN_ANALYSE;
					return t;
				}

				//Get ID
				QStringList temp=list[2].split("=");
				QString sID=temp[1];
	
				t.ID=QStringToQint32(sID);

			}

			//----------------
			//Get Message 
			if(Message.contains("MESSAGE=",Qt::CaseInsensitive)){
				t.Message=Message.mid(Message.indexOf("MESSAGE=")+8);
				t.Message.remove("\n");
			}
			//----------------
		}
		else if((list[0].contains("NAMING")==true) && (list[1].contains("REPLY")==true)){
			t.type=NAMING_REPLY;

			//get Result
			if(list[2].contains("RESULT=OK",Qt::CaseInsensitive))t.result=OK;
			else if(list[2].contains("RESULT=INVALID_KEY",Qt::CaseInsensitive))t.result=INVALID_KEY;
			else if(list[2].contains("RESULT=KEY_NOT_FOUND",Qt::CaseInsensitive))t.result=KEY_NOT_FOUND;
			else{
				t.type=ERROR_IN_ANALYSE;
				return t;
			}
			//get Name
			QStringList temp=list[3].split("=");
			t.Name=temp[1].remove("\n");
			
			//get Value
			if(list.count()-1>=4){
				QStringList temp=list[4].split("=");
				t.Value=temp[1].remove("\n");
			}
			//Get Message
			if(Message.contains("MESSAGE=",Qt::CaseInsensitive)){
				t.Message=Message.mid(Message.indexOf("MESSAGE=")+8);
				t.Message.remove("\n");
			}
		}
		else{
			t.type=ERROR_IN_ANALYSE;
		}
				
	return t;
}

qint32 I2PSamMessageAnalyser::QStringToQint32(QString value)
{
	bool OK=false;
	qint32 iValue =value.toInt ( &OK,10 );

	if(OK==false)
	{
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setText("I2PSamMessageAnalyser");
		msgBox.setInformativeText("cant parse value: "+value );
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
	}
	return iValue;	
}


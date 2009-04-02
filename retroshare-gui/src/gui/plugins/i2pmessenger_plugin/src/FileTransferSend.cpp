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

#include "FileTransferSend.h"
#include "gui/form_fileSend.h"
#include "Core.h"

cFileTransferSend::cFileTransferSend(cCore * Core, QString FilePath,QString Destination)
:FilePath(FilePath),Destination(Destination)
{
	this->Core=Core;
	StreamID=Core->StreamConnect(Destination);
	Core->removeUnknownID(StreamID);
	
	sendFirstPaket=true;
	FileName=FilePath.mid(FilePath.lastIndexOf("/")+1);
	FileTransferAccepted=false;
	FileForSend  = new QFile(FilePath);
	FileSize=FileForSend->size();

	Dialog=new form_fileSend(this);
	Dialog->show();
}

void cFileTransferSend::abbortFileSend()
{
	FileForSend->close();
	Core->StreamClose(StreamID);
}

void cFileTransferSend::StreamStatus(const SAM_Message_Types::RESULT result, const QString ID, QString Message)
{
	using namespace FileTransferProtocol;	

	if(result==SAM_Message_Types::OK)
	{
		if(sendFirstPaket==true)
		{
			QString StringFileSize;
			StringFileSize.setNum(FileSize);

			Core->StreamSendData(StreamID,FIRSTPAKET+StringFileSize+"\n"+FileName);
			sendFirstPaket=false;
		}

	}
	else
	{
		FileForSend->close();
		emit event_FileTransferError();
	}


}

void cFileTransferSend::StreamClosed(const SAM_Message_Types::RESULT result, QString ID, QString Message)
{
	if(result==SAM_Message_Types::OK){
		if(allreadySendedSize==FileSize){
			emit event_FileTransferFinishedOK();	
		}
		else
			emit event_FileTransferAborted();
	}
	else{
		emit event_FileTransferAborted();
	}

	FileForSend->close();
}

cFileTransferSend::~ cFileTransferSend()
{
	
	delete FileForSend;
	delete Dialog;
}

void cFileTransferSend::StreamReadyToSend(bool t)
{
	if(t==false)return;
	if(FileTransferAccepted==false)return;

	if(allreadySendedSize==FileSize)
	{
		emit event_FileTransferFinishedOK();
		Core->StreamClose(StreamID);
		return;
	}

	QByteArray Buffer;

	Buffer=FileForSend->read(MAXPAKETSIZE);
	allreadySendedSize+=Buffer.length();

	
	Core->StreamSendData(StreamID,Buffer);
	emit event_allreadySendedSizeChanged(allreadySendedSize);
}

void cFileTransferSend::operator <<(QByteArray t)
{
	if(t.length()==1){
		if(t.contains("0")){//true
			emit event_FileTransferAccepted(true);
			FileTransferAccepted=true;
			StartFileTransfer();
			
		}
		else{
			emit event_FileTransferAccepted(false);
			Core->StreamClose(StreamID);
			
		}
	}
	else{
		emit event_FileTransferAccepted(false);
		Core->StreamClose(StreamID);
	}
}

void cFileTransferSend::StartFileTransfer()
{
	allreadySendedSize=0;
	FileForSend->open(QIODevice::ReadOnly);
	StreamReadyToSend(true);
}

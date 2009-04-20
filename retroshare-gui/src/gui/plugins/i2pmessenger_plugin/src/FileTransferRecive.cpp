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
#include "FileTransferRecive.h"



cFileTransferRecive::cFileTransferRecive(cCore * Core, qint32 StreamID, QString FileName, quint64 FileSize)
:StreamID(StreamID),FileName(FileName),FileSize(FileSize)
{
	this->Core=Core;
	allreadyRecivedSize=0;
}

cFileTransferRecive::~ cFileTransferRecive()
{
	delete Dialog;
	delete FileForRecive;
}

void cFileTransferRecive::StreamStatus(const SAM_Message_Types::RESULT result, const qint32 ID, QString Message)
{
	if(result==SAM_Message_Types::OK)
	{
		
	}
	else
	{
		FileForRecive->close();
		FileForRecive->remove();
		emit event_FileReciveError();

	}

}

void cFileTransferRecive::StreamClosed(const SAM_Message_Types::RESULT result, qint32 ID, QString Message)
{
	if(allreadyRecivedSize==FileSize){
		
		FileForRecive->close();	
		emit event_FileRecivedFinishedOK();

	}
	else{
		FileForRecive->close();
		FileForRecive->remove();
		emit event_FileReciveAbort();
	}
	
}

void cFileTransferRecive::operator <<(QByteArray t)
{

	allreadyRecivedSize+=t.length();
	FileForRecive->write(t);
	FileForRecive->flush();

	emit event_allreadyRecivedSizeChanged(allreadyRecivedSize);

	if(allreadyRecivedSize==FileSize)
	{
		Core->StreamClose(StreamID);
		FileForRecive->close();
		emit event_FileRecivedFinishedOK();	
	}
}

void cFileTransferRecive::abbortFileRecive()
{
	Core->StreamClose(StreamID);
	FileForRecive->close();
	FileForRecive->remove();
}

void cFileTransferRecive::start()
{
	QString SFileSize;
	SFileSize.setNum(FileSize);


	QMessageBox* msgBox= new QMessageBox(NULL);
 	msgBox->setText("Incoming FileTransfer");
 	msgBox->setInformativeText("Do you want to accept it ?\nFileName: "+FileName+"\nFileSize: " +SFileSize+"  Bit");
 	msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
 	msgBox->setDefaultButton(QMessageBox::Yes);
	msgBox->setWindowModality(Qt::WindowModal);
 	int ret = msgBox->exec();
	
	if(ret==QMessageBox::Yes){
		QString FilePath=QFileDialog::getSaveFileName(NULL,"File Save","./"+FileName);

		if(!FilePath.isEmpty()){
			FileForRecive= new QFile(FilePath);
			FileForRecive->open(QIODevice::WriteOnly);
			Core->StreamSendData(StreamID,QString("0"));//true
	
			Dialog= new form_fileRecive(this);
			Dialog->show();
		}
		else{
			Core->StreamSendData(StreamID,QString("1"));//false
			Core->StreamClose(StreamID);
		}

	}
	else{
		Core->StreamSendData(StreamID,QString("1"));//false
		Core->StreamClose(StreamID);
	}
}

void cFileTransferRecive::start_withAutoAccept(QString Path)
{
	
	QString SFileSize;
	SFileSize.setNum(FileSize);

		QString FilePath=Path+="/"+FileName;

		if(!FilePath.isEmpty()){
			FileForRecive= new QFile(FilePath);
			FileForRecive->open(QIODevice::WriteOnly);
			Core->StreamSendData(StreamID,QString("0"));//true
	
			Dialog= new form_fileRecive(this);
			Dialog->show();
		}
}



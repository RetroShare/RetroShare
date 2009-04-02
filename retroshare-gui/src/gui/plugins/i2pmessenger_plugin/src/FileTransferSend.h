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
#ifndef FILETRANSFERSEND_H
#define FILETRANSFERSEND_H

#include "ConnectionI2P.h"

namespace FileTransferProtocol
{
	const QString PROTOCOLVERSION= "0.1";
	const QString FIRSTPAKET ="CHATSYSTEMFILETRANSFER\t"+PROTOCOLVERSION+"\n";
//+sizeinbit\nFileName
};

//limited to 30Kb
#define MAXPAKETSIZE 30720
class cCore;
class form_fileSend;
class cFileTransferSend:public QObject
{
	Q_OBJECT

	public:
	cFileTransferSend(cCore* Core,QString FilePath,QString Destination);
	~cFileTransferSend();
	void StreamStatus(const SAM_Message_Types::RESULT result,const QString ID,QString Message);
	void StreamClosed(const SAM_Message_Types::RESULT result,QString ID,QString Message);
	void StreamReadyToSend(bool t);
	void operator << ( QByteArray t );
	quint64 get_FileSize(){return FileSize;};
	QString get_StreamID(){return StreamID;};
	QString get_FileName(){return FileName;};
	

	public slots:
	void abbortFileSend();

	signals:
	void event_allreadySendedSizeChanged(quint64 Size);
	void event_FileTransferAccepted(bool t);
	void event_FileTransferFinishedOK();
	void event_FileTransferError();
	void event_FileTransferAborted();//the otherSide abort it
	
	private:
	void StartFileTransfer();

	cCore* Core;
	const QString FilePath;
	qint64 FileSize;
	qint64 allreadySendedSize;
	const QString Destination;
	QString StreamID;
	QFile* FileForSend;
	bool sendFirstPaket;
	bool FileTransferAccepted;
	QString FileName;
	form_fileSend* Dialog;

}; 
#endif
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
#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include "gui/form_fileRecive.h"
#include "ConnectionI2P.h"

class cCore;
class cFileTransferRecive:public QObject
{
	Q_OBJECT
	public:
	cFileTransferRecive(cCore* Core,QString StreamID,QString FileName,quint64 FileSize);
	~cFileTransferRecive();
	void start();

	void StreamStatus(const SAM_Message_Types::RESULT result,const QString ID,QString Message);
	void StreamClosed(const SAM_Message_Types::RESULT result,QString ID,QString Message);
	void operator << ( QByteArray t );
	quint64 get_FileSize(){return FileSize;};
	QString get_FileName(){return FileName;};
	QString get_StreamID(){return StreamID;};

	public slots:
	void abbortFileRecive();

	signals:
	void event_allreadyRecivedSizeChanged(quint64 Size);
	void event_FileReciveError();
	void event_FileRecivedFinishedOK();
	void event_FileReciveAbort();

	private:
	cCore* Core;
	form_fileRecive* Dialog;
	const quint64 FileSize;
	quint64 allreadyRecivedSize;
	const QString FileName;
	const QString StreamID;
	QFile* FileForRecive;
};
#endif
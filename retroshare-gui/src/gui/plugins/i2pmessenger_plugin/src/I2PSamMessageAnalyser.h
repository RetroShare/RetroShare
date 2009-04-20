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
#ifndef I2PSAMMESSAGEANALYSER_H
#define I2PSAMMESSAGEANALYSER_H

#include <QtGui> 

namespace SAM_Message_Types
{


	enum TYPE
	{	
		HELLO_REPLAY,
		SESSION_STATUS,
		STREAM_STATUS,
		STREAM_CONNECTED,
		STREAM_SEND,
		STREAM_READY_TO_SEND,
		STREAM_CLOSED,
		STREAM_RECEIVED,
		NAMING_REPLY,
	
		ERROR_IN_ANALYSE
	};
	
	enum RESULT
	{	
		OK,
		DUPLICATED_DEST,
		I2P_ERROR,
		INVALID_KEY,
		CANT_REACH_PEER,
		TIMEOUT,	
		FAILED,
		NOVERSION,
		KEY_NOT_FOUND,
		PEER_NOT_FOUND
	};
	
	enum STATE
	{
		BUFFER_FULL,
		READY
	};
}

struct SAM_MESSAGE
{	
	public:
	QString Message;
	qint32 ID;
	QString Destination;
	QString Size;
	QString Name;
	QString Value;
	SAM_Message_Types::TYPE type;
	SAM_Message_Types::RESULT result;
	SAM_Message_Types::STATE state;
	/*
	SAM_MESSAGE(TYPE t,RESULT r,QString Message,QString Id,QString Destination,QString Size)
	:type(t),result(r),Message(Message),Id(Id),Destination(Destination),Size(Size)
	{	
	}
	*/
};


class I2PSamMessageAnalyser: public QObject
{
	Q_OBJECT
	public:
	I2PSamMessageAnalyser();
	const SAM_MESSAGE Analyse(QString Message);

	private:
	qint32 QStringToQint32(QString value);
};
#endif

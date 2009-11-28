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
#ifndef PACKETMANAGER_H
#define PACKETMANAGER_H

#include <Qt>
#include <QtGui>

class cPacketManager :public QObject
{
		Q_OBJECT


	public:
		//cPacketManager() {}
		cPacketManager ( qint32 ID );
		~cPacketManager();
		void operator << ( QByteArray t );
		qint32 getID();

		void checkifOnePacketIsComplead();
	signals:
		void aPacketIsComplead ( const qint32 ID,const QByteArray CurrentPacket );
	private:
		const qint32 ID;
		QByteArray Data;
};
#endif

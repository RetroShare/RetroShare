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
#include "PacketManager.h"



cPacketManager::cPacketManager ( qint32 ID )
:ID ( ID ) 
{
	Data.clear();
}

cPacketManager::~cPacketManager() 
{
}
void cPacketManager::operator << ( QByteArray t )
{
	Data.append ( t );
	checkifOnePacketIsComplead();
}
qint32 cPacketManager::getID()
{
	return ID;
}

void  cPacketManager::checkifOnePacketIsComplead()
{
	if ( Data.length() >=8 )
	{
		QString sPacketLength=Data.mid ( 0,4 );
		bool OK=false;
		int iPacketLength =sPacketLength.toInt ( &OK,16 );
		if(OK==false)
		{
			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Warning);
			msgBox.setText("cPacketManager ("+QString(ID)+")");
			msgBox.setInformativeText("cant parse PacketLength\nHexValue: "+sPacketLength );
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.exec();
		}
		

		if ( Data.length() >=iPacketLength+4 )
		{
			QByteArray CurrentPacket ( Data.mid ( 4 ),iPacketLength );
			Data.remove ( 0,iPacketLength+4 );
			emit aPacketIsComplead ( ID,CurrentPacket );
			checkifOnePacketIsComplead();
		}
	}
	return;
}


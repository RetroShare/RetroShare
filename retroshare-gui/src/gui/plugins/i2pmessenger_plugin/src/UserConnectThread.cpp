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
#include "UserConnectThread.h"


cUserConnectThread::cUserConnectThread(cCore* core,quint32 timeToWait=30000)
{	this->core=core;
	this->timeToWait=timeToWait;
	this->timer= new QTimer();
  	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
}

cUserConnectThread::~cUserConnectThread()
{
	stop();
	disconnect(timer, SIGNAL(timeout()), this, SLOT(update()));
}

void cUserConnectThread::update()
{
	QList<cUser*> users=core->get_userList();
		for(int i=0;i<users.count();i++){
			if(users.at(i)->get_ConnectionStatus()==OFFLINE){
				if(core->isThisDestinationInunknownConnections(users.at(i)->get_I2PDestination())==false){
					users.at(i)->set_ConnectionStatus(TRYTOCONNECT);
					users.at(i)->set_I2PStreamID(core->StreamConnect(users.at(i)->get_I2PDestination()));

					//users.at(i)->set_I2PStreamID(emit doStreamConnect(users.at(i)->get_I2PDestination()));
				}
			}
		}
}
void cUserConnectThread::start()
{
	timer->start(timeToWait);
}
void cUserConnectThread::stop()
{
	timer->stop();

}

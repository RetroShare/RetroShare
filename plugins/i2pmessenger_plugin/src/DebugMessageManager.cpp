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
#include "DebugMessageManager.h"


cDebugMessageManager::cDebugMessageManager(cConnectionI2P* I2P)
{

	this->I2P=I2P;
	
	this->settings= new QSettings("./application.ini",QSettings::IniFormat);
	settings->beginGroup("General");
		this->MaxMessageCount=settings->value("Debug_Max_Message_count","20").toInt();
	settings->endGroup();
	connect(I2P,SIGNAL(debugMessages(QString) ),this,SLOT(NewIncomingDebugMessage(const QString)) );
}

cDebugMessageManager::~cDebugMessageManager()
{
	disconnect(I2P,SIGNAL(debugMessages(QString)),this,SLOT(NewIncomingDebugMessage(const QString)));
}

void cDebugMessageManager::clearAllMessages()
{
	Messages.clear();
}

const QStringList cDebugMessageManager::getAllMessages()
{
	return Messages;
}

void cDebugMessageManager::NewIncomingDebugMessage(const QString Message){
	while(Messages.count()>= (signed int)MaxMessageCount){
		Messages.removeLast();
	}

	Messages.prepend(Message);
	emit newDebugMessage(Message);
}

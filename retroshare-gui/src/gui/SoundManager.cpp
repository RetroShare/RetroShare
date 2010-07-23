/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QSound>

#include "SoundManager.h"
#include "settings/rsharesettings.h"

SoundManager::SoundManager()
{
	isMute=false;
	reInit();
}

void SoundManager::doMute(bool t)
{
	isMute=t;
}

void SoundManager::event_User_go_Online()
{
	if(isMute==true) return;
		
		if(enable_eventUser_go_Online) 
			QSound::play(SoundFileUser_go_Online);
}
void SoundManager::event_User_go_Offline()
{
	if(isMute==true) return;
	if(enable_eventUser_go_Offline)
		QSound::play(SoundFileUser_go_Offline);	
}
void SoundManager::event_FileSend_Finished()
{
	if(isMute==true) return;
	if(enable_eventFileSend_Finished)
		QSound::play(SoundFileFileSend_Finished);
}
void SoundManager::event_FileRecive_Incoming()
{
	if(isMute==true) return;
	if(enable_eventFileRecive_Incoming)
		QSound::play(SoundFileFileRecive_Incoming);
}

void SoundManager::event_FileRecive_Finished()
{
	if(isMute==true) return;
	if(enable_eventFileRecive_Finished)
		QSound::play(SoundFileFileRecive_Finished);
}


void SoundManager::event_NewChatMessage()
{
	if(isMute==true) return;
	if(enable_eventNewChatMessage)
	{
		QSound::play(SoundFileNewChatMessage);

	}
}

void SoundManager::reInit()
{
        Settings->beginGroup("Sound");
		Settings->beginGroup("Enable");
			enable_eventUser_go_Online = Settings->value("User_go_Online",false).toBool();
			enable_eventUser_go_Offline = Settings->value("User_go_Offline",false).toBool();
			enable_eventFileSend_Finished = Settings->value("FileSend_Finished",false).toBool();
			enable_eventFileRecive_Incoming = Settings->value("FileRecive_Incoming",false).toBool();
			enable_eventFileRecive_Finished = Settings->value("FileRecive_Finished",false).toBool();
			enable_eventNewChatMessage = Settings->value("NewChatMessage",false).toBool();
		Settings->endGroup();

		Settings->beginGroup("SoundFilePath");
		SoundFileUser_go_Online = Settings->value("User_go_Online","").toString();
		SoundFileUser_go_Offline = Settings->value("User_go_Offline","").toString();
		SoundFileFileSend_Finished = Settings->value("FileSend_Finished","").toString();
		SoundFileFileRecive_Incoming = Settings->value("FileRecive_Incoming","").toString();
		SoundFileFileRecive_Finished = Settings->value("FileRecive_Finished","").toString();
		SoundFileNewChatMessage = Settings->value("NewChatMessage","").toString();
		Settings->endGroup();
	Settings->endGroup();
}

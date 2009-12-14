/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
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
#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H
 
#include <QSound>
#include <QSettings>
#include <QtGui>

#include "settings/rsharesettings.h"


class SoundManager :public QObject
{
	Q_OBJECT
	public:
		SoundManager();

	public slots:
		void doMute(bool t);
		void event_User_go_Online();
		void event_User_go_Offline();
		void event_FileSend_Finished();
		void event_FileRecive_Incoming();
		void event_FileRecive_Finished();
		void event_NewChatMessage();
		void reInit();
		

	private:
		bool isMute;
		QString SoundFileUser_go_Online;
		QString SoundFileUser_go_Offline;
		QString SoundFileFileSend_Finished;
		QString SoundFileFileRecive_Incoming;
		QString SoundFileFileRecive_Finished;
		QString SoundFileNewChatMessage;

		bool enable_eventUser_go_Online;
		bool enable_eventUser_go_Offline;
		bool enable_eventFileSend_Finished;
		bool enable_eventFileRecive_Incoming;
		bool enable_eventFileRecive_Finished;
		bool enable_eventNewChatMessage;
		
		/** A RshareSettings object used for saving/loading settings */
    RshareSettings* _settings;
};
#endif

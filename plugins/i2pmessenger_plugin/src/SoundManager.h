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
#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H
 
#include <QSound>
#include <QSettings>
#include <QtGui>


class cSoundManager :public QObject
{
	Q_OBJECT
	public:
		cSoundManager();

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
};
#endif
/*******************************************************************************
 * gui/SoundManager.h                                                          *
 *                                                                             *
 * Copyright (c) 2012 Retroshare Team  <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QObject>
#include <QMap>

#define SOUND_USER_ONLINE       "User_go_Online"
#define SOUND_NEW_CHAT_MESSAGE  "NewChatMessage"
#define SOUND_MESSAGE_ARRIVED   "MessageArrived"
#define SOUND_DOWNLOAD_COMPLETE "DownloadComplete"
#define SOUND_NEW_LOBBY_MESSAGE "NewLobbyMessage"
#define SOUND_LOBBY_INCOMING    "LobbyIncoming"

class SoundEvents
{
public:
	class SoundEventInfo
	{
	public:
		SoundEventInfo() {}

	public:
		QString mGroupName;
		QString mEventName;
		QString mDefaultFilename;
	};

public:
	SoundEvents();

	void addEvent(const QString &groupName, const QString &eventName, const QString &event, const QString &defaultFilename);

public:
	QString mDefaultPath;
	QMap<QString, SoundEventInfo> mEventInfos;
};

class SoundManager : public QObject
{
	Q_OBJECT

public slots:
	void setMute(bool m);

signals:
	void mute(bool isMute);

public:
	static void create();

#ifdef Q_OS_LINUX
	static QString soundDetectPlayer();
#endif

	static void initDefault();
	static QString defaultFilename(const QString &event, bool check);
	static QString convertFilename(const QString &filename);
	static QString realFilename(const QString &filename);

	static void soundEvents(SoundEvents &events);

	static bool isMute();

	static void play(const QString &event);
	static void playFile(const QString &filename);

	static bool eventEnabled(const QString &event);
	static void setEventEnabled(const QString &event, bool enabled);

	static QString eventFilename(const QString &event);
	static void setEventFilename(const QString &event, const QString &filename);

private:
	SoundManager();
};

extern SoundManager *soundManager;

#endif	//SOUNDMANAGER_H

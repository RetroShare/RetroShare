/*******************************************************************************
 * gui/SoundManager.cpp                                                        *
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

#include <QApplication>
#include <QFile>
#include <QProcess>
#include <QSound>
#include <QDir>

#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
#include <QAudio>
#include <QAudioDeviceInfo>
#endif

// #ifdef QMEDIAPLAYER
// #nclude <QMediaPlayer>
// #endif

#include "SoundManager.h"
#include "settings/rsharesettings.h"
#include "retroshare/rsinit.h"
#include <retroshare/rsplugin.h>

#define GROUP_MAIN      "Sound"
#define GROUP_ENABLE    "Enable"
#define GROUP_SOUNDFILE "SoundFilePath"

SoundManager *soundManager = NULL;

SoundEvents::SoundEvents()
{
}

void SoundEvents::addEvent(const QString &groupName, const QString &eventName, const QString &event, const QString &defaultFilename)
{
	if (event.isEmpty()) {
		return;
	}

	SoundEventInfo info;
	info.mGroupName = groupName;
	info.mEventName = eventName;
	info.mDefaultFilename = defaultFilename;

	mEventInfos[event] = info;
}

void SoundManager::create()
{
	if (soundManager == NULL) {
		soundManager = new SoundManager;
	}
}

SoundManager::SoundManager() : QObject()
{
}

void SoundManager::soundEvents(SoundEvents &events)
{
	QDir baseDir = QDir(QString::fromUtf8(RsAccounts::systemDataDirectory().c_str()) + "/sounds");

	events.mDefaultPath = baseDir.absolutePath();

	/* add standard events */
	events.addEvent(tr("Friend"), tr("Go Online"), SOUND_USER_ONLINE, QFileInfo(baseDir, "online1.wav").absoluteFilePath());
	events.addEvent(tr("Chat Message"), tr("New Msg"), SOUND_NEW_CHAT_MESSAGE, QFileInfo(baseDir, "incomingchat.wav").absoluteFilePath());
	events.addEvent(tr("Message"), tr("Message arrived"), SOUND_MESSAGE_ARRIVED, QFileInfo(baseDir, "receive.wav").absoluteFilePath());
	events.addEvent(tr("Download"), tr("Download complete"), SOUND_DOWNLOAD_COMPLETE, QFileInfo(baseDir, "ft_complete.wav").absoluteFilePath());
	events.addEvent(tr("Chat Room"), tr("Message arrived"), SOUND_NEW_LOBBY_MESSAGE, QFileInfo(baseDir, "incomingchat.wav").absoluteFilePath());
	events.addEvent(tr("Chat Room"), tr("Specific User incoming in Chat Room"), SOUND_LOBBY_INCOMING, QFileInfo(baseDir, "incomingchat.wav").absoluteFilePath());

	/* add plugin events */
	int pluginCount = rsPlugins->nbPlugins();
	for (int i = 0; i < pluginCount; ++i) {
		RsPlugin *plugin = rsPlugins->plugin(i);

		if (plugin) {
			plugin->qt_sound_events(events);
		}
	}
}

QString SoundManager::defaultFilename(const QString &event, bool check)
{
	SoundEvents events;
	soundEvents(events);

	QMap<QString, SoundEvents::SoundEventInfo>::iterator eventIt = events.mEventInfos.find(event);
	if (eventIt == events.mEventInfos.end()) {
		return "";
	}

	QString filename = eventIt.value().mDefaultFilename;
	if (filename.isEmpty()) {
		return "";
	}

	if (!check) {
		return convertFilename(filename);
	}

	if (QFileInfo::exists(filename)) {
		return convertFilename(filename);
	}

	return "";
}

void SoundManager::initDefault()
{
	SoundEvents events;
	soundEvents(events);

	QString event;
	foreach (event, events.mEventInfos.keys()) {
		SoundEvents::SoundEventInfo &eventInfo = events.mEventInfos[event];

		if (QFileInfo::exists(eventInfo.mDefaultFilename)) {
			setEventFilename(event, convertFilename(eventInfo.mDefaultFilename));
			setEventEnabled(event, true);
		}
	}
}

void SoundManager::setMute(bool m)
{
	Settings->beginGroup(GROUP_MAIN);
	Settings->setValue("mute", m);
	Settings->endGroup();

	emit mute(m);
}

bool SoundManager::isMute()
{
	Settings->beginGroup(GROUP_MAIN);
	bool mute = Settings->value("mute", false).toBool();
	Settings->endGroup();

	return mute;
}

bool SoundManager::eventEnabled(const QString &event)
{
	Settings->beginGroup(GROUP_MAIN);
	Settings->beginGroup(GROUP_ENABLE);
	bool enabled = Settings->value(event, false).toBool();
	Settings->endGroup();
	Settings->endGroup();

	return enabled;
}

void SoundManager::setEventEnabled(const QString &event, bool enabled)
{
	Settings->beginGroup(GROUP_MAIN);
	Settings->beginGroup(GROUP_ENABLE);
	Settings->setValue(event, enabled);
	Settings->endGroup();
	Settings->endGroup();
}

QString SoundManager::eventFilename(const QString &event)
{
	Settings->beginGroup(GROUP_MAIN);
	Settings->beginGroup(GROUP_SOUNDFILE);
	QString filename = Settings->value(event).toString();
	Settings->endGroup();
	Settings->endGroup();

	return filename;
}

void SoundManager::setEventFilename(const QString &event, const QString &filename)
{
	Settings->beginGroup(GROUP_MAIN);
	Settings->beginGroup(GROUP_SOUNDFILE);
	Settings->setValue(event, filename);
	Settings->endGroup();
	Settings->endGroup();
}

QString SoundManager::convertFilename(const QString &filename)
{
	if (RsInit::isPortable ()) {
		// Save path relative to application path
		QDir baseDir = QDir(qApp->applicationDirPath());
		QString relativeFilename = baseDir.relativeFilePath(filename);
		if (!relativeFilename.startsWith("..")) {
			// Save only subfolders as relative path
			return relativeFilename;
		}
	}

	return filename;
}

QString SoundManager::realFilename(const QString &filename)
{
	if (RsInit::isPortable ()) {
		// Path relative to application path
		QDir baseDir = QDir(qApp->applicationDirPath());
		return baseDir.absoluteFilePath(filename);
	}

	return filename;
}

void SoundManager::play(const QString &event)
{
	if (isMute() || !eventEnabled(event)) {
		return;
	}

	QString filename = eventFilename(event);
	playFile(filename);
}

void SoundManager::playFile(const QString &filename)
{
	if (filename.isEmpty()) {
		return;
	}

	QString playFilename = realFilename(filename);
        bool played = false ;
    
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
    if (!QAudioDeviceInfo::availableDevices(QAudio::AudioOutput).isEmpty())
#else
    if (QSound::isAvailable())
#endif
        {
        QSound::play(playFilename);
            played = true ;
        }

        if(!played)	// let's go for the hard core stuff
    {
        // #ifdef QMEDIAPLAYER
        //         static QMediaPlayer *qmplayer;
        //         if (qmplayer == NULL) {
        //             qmplayer = new QMediaPlayer();
        //             qmplayer->setMedia(QMediaContent(QUrl::fromLocalFile(playFilename)));
        //         }
        //         std::cerr << "Play QMediaPlayer" << std::endl;
        //         qmplayer->play();
        //         return;
        // #endif

#ifdef Q_OS_LINUX
        QString player_cmd = soundDetectPlayer();
        QStringList args = player_cmd.split(' ');
        args += filename;
        QString prog = args.takeFirst();
        //std::cerr << "Play " << prog.toStdString() << std::endl;
        QProcess::startDetached(prog, args);
#endif
    }
}


#ifdef Q_OS_LINUX
/** Detect default player helper on unix like systems
 * Inspired by Psi IM (0.15) in common.cpp
 */
QString SoundManager::soundDetectPlayer()
{
    // prefer ALSA on linux

    if (QFile("/proc/asound").exists()) {
        return "aplay -q";
    }
    // fallback to "play"
    return "play";
}
#endif






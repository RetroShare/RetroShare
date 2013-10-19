/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012 RetroShare Team
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

#define GROUP_MAIN      "Sound"
#define GROUP_ENABLE    "Enable"
#define GROUP_SOUNDFILE "SoundFilePath"

SoundManager *soundManager = NULL;

SoundEvents::SoundEvents()
{
}

void SoundEvents::addEvent(const QString &groupName, const QString &eventName, const QString &event)
{
	SoundEventInfo info;
	info.mGroupName = groupName;
	info.mEventName = eventName;
	info.mEvent = event;

	mEventInfos.push_back(info);
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

void SoundManager::setMute(bool mute)
{
	Settings->beginGroup(GROUP_MAIN);
	Settings->setValue("mute", mute);
	Settings->endGroup();

	emit SoundManager::mute(mute);
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

void SoundManager::play(const QString &event)
{
#if QT_VERSION < QT_VERSION_CHECK (5, 0, 0)
	if (!QSound::isAvailable()) {
		return;
	}
#endif

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
	QSound::play(filename);
}

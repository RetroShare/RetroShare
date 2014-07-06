/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014 RetroShare Team
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

#include "GxsChannelUserNotify.h"
#include "gui/settings/rsharesettings.h"
#include "gui/MainWindow.h"

GxsChannelUserNotify::GxsChannelUserNotify(RsGxsIfaceHelper *ifaceImpl, QObject *parent) :
    GxsUserNotify(ifaceImpl, parent)
{
}

bool GxsChannelUserNotify::hasSetting(QString &name)
{
	name = tr("Channel Post");

	return true;
}

bool GxsChannelUserNotify::notifyEnabled()
{
	return (Settings->getTrayNotifyFlags() & TRAYNOTIFY_CHANNELS);
}

bool GxsChannelUserNotify::notifyCombined()
{
	return (Settings->getTrayNotifyFlags() & TRAYNOTIFY_CHANNELS_COMBINED);
}

bool GxsChannelUserNotify::notifyBlink()
{
	return (Settings->getTrayNotifyBlinkFlags() & TRAYNOTIFY_BLINK_CHANNELS);
}

void GxsChannelUserNotify::setNotifyEnabled(bool enabled, bool combined, bool blink)
{
	uint notifyFlags = Settings->getTrayNotifyFlags();
	uint blinkFlags = Settings->getTrayNotifyBlinkFlags();

	if (enabled) {
		notifyFlags |= TRAYNOTIFY_CHANNELS;
	} else {
		notifyFlags &= ~TRAYNOTIFY_CHANNELS;
	}

	if (combined) {
		notifyFlags |= TRAYNOTIFY_CHANNELS_COMBINED;
	} else {
		notifyFlags &= ~TRAYNOTIFY_CHANNELS_COMBINED;
	}

	if (blink) {
		blinkFlags |= TRAYNOTIFY_BLINK_CHANNELS;
	} else {
		blinkFlags &= ~TRAYNOTIFY_BLINK_CHANNELS;
	}

	Settings->setTrayNotifyFlags(notifyFlags);
	Settings->setTrayNotifyBlinkFlags(blinkFlags);
}

QIcon GxsChannelUserNotify::getIcon()
{
	return QIcon(":/images/channels16.png");
}

QIcon GxsChannelUserNotify::getMainIcon(bool hasNew)
{
	return hasNew ? QIcon(":/images/channels_new.png") : QIcon(":/images/channels.png");
}

void GxsChannelUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::Channels);
}

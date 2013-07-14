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

#include "TransferUserNotify.h"
#include "gui/settings/rsharesettings.h"
#include "gui/notifyqt.h"
#include "gui/MainWindow.h"

TransferUserNotify::TransferUserNotify(QObject *parent) :
	UserNotify(parent)
{
	newTransferCount = 0;

	connect(NotifyQt::getInstance(), SIGNAL(downloadCompleteCountChanged(int)), this, SLOT(downloadCountChanged(int)));
}

bool TransferUserNotify::hasSetting(QString &name)
{
	name = tr("Download completed");

	return true;
}

bool TransferUserNotify::notifyEnabled()
{
	return (Settings->getTrayNotifyFlags() & TRAYNOTIFY_TRANSFERS);
}

bool TransferUserNotify::notifyCombined()
{
	return (Settings->getTrayNotifyFlags() & TRAYNOTIFY_TRANSFERS_COMBINED);
}

bool TransferUserNotify::notifyBlink()
{
	return (Settings->getTrayNotifyBlinkFlags() & TRAYNOTIFY_BLINK_TRANSFERS);
}

void TransferUserNotify::setNotifyEnabled(bool enabled, bool combined, bool blink)
{
	uint notifyFlags = Settings->getTrayNotifyFlags();
	uint blinkFlags = Settings->getTrayNotifyBlinkFlags();

	if (enabled) {
		notifyFlags |= TRAYNOTIFY_TRANSFERS;
	} else {
		notifyFlags &= ~TRAYNOTIFY_TRANSFERS;
	}

	if (combined) {
		notifyFlags |= TRAYNOTIFY_TRANSFERS_COMBINED;
	} else {
		notifyFlags &= ~TRAYNOTIFY_TRANSFERS_COMBINED;
	}

	if (blink) {
		blinkFlags |= TRAYNOTIFY_BLINK_TRANSFERS;
	} else {
		blinkFlags &= ~TRAYNOTIFY_BLINK_TRANSFERS;
	}

	Settings->setTrayNotifyFlags(notifyFlags);
	Settings->setTrayNotifyBlinkFlags(blinkFlags);
}

QIcon TransferUserNotify::getIcon()
{
	return QIcon(":/images/ktorrent32.png");
}

QIcon TransferUserNotify::getMainIcon(bool hasNew)
{
	return hasNew ? QIcon(":/images/transfers_new.png") : QIcon(":/images/ktorrent32.png");
}

unsigned int TransferUserNotify::getNewCount()
{
	return newTransferCount;
}

QString TransferUserNotify::getTrayMessage(bool plural)
{
	return plural ? tr("You have %1 completed downloads") : tr("You have %1 completed download");
}

QString TransferUserNotify::getNotifyMessage(bool plural)
{
	return plural ? tr("%1 completed downloads") : tr("%1 completed download");
}

void TransferUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::Transfers);
}

void TransferUserNotify::downloadCountChanged(int count)
{
	newTransferCount = count;
	updateIcon();
}

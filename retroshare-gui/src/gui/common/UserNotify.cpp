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

#include <QMenu>
#include <QToolBar>
#include <QToolButton>

#include "UserNotify.h"
#include "rshare.h"
#include "gui/settings/rsharesettings.h"

UserNotify::UserNotify(QObject *parent) :
	QObject(parent)
{
	mMainToolButton = NULL;
	mMainAction = NULL;
	mListItem = NULL;
	mTrayIcon = NULL;
	mNotifyIcon = NULL;
	mNewCount = 0;
	mLastBlinking = false;

	connect(rApp, SIGNAL(blink(bool)), this, SLOT(blink(bool)));
}

UserNotify::~UserNotify()
{
}

bool UserNotify::notifyEnabled()
{
	QString group;
	if (!hasSetting(NULL, &group) || group.isEmpty()) {
		return false;
	}

	return Settings->valueFromGroup(group, "TrayNotifyEnable", true).toBool();
}

bool UserNotify::notifyCombined()
{
	QString group;
	if (!hasSetting(NULL, &group) || group.isEmpty()) {
		return false;
	}

	return Settings->valueFromGroup(group, "TrayNotifyCombined", false).toBool();
}

bool UserNotify::notifyBlink()
{
	QString group;
	if (!hasSetting(NULL, &group) || group.isEmpty()) {
		return false;
	}

	return Settings->valueFromGroup(group, "TrayNotifyBlink", false).toBool();
}

void UserNotify::setNotifyEnabled(bool enabled, bool combined, bool blink)
{
	QString group;
	if (!hasSetting(NULL, &group) || group.isEmpty()) {
		return;
	}

	Settings->beginGroup(group);
	Settings->setValue("TrayNotifyEnable", enabled);
	Settings->setValue("TrayNotifyCombined", combined);
	Settings->setValue("TrayNotifyBlink", blink);
	Settings->endGroup();
}

void UserNotify::initialize(QToolBar *mainToolBar, QAction *mainAction, QListWidgetItem *listItem)
{
	mMainAction = mainAction;
	if (mMainAction) {
		mButtonText = mMainAction->text();
		if (mainToolBar) {
			mMainToolButton = dynamic_cast<QToolButton*>(mainToolBar->widgetForAction(mMainAction));
		}
	}
	mListItem = listItem;
	if (mListItem && mMainAction) {
		mButtonText = mMainAction->text();
	}
}

void UserNotify::createIcons(QMenu *notifyMenu)
{
#define DELETE_OBJECT(x) if (x) { delete(x); x = NULL; }

	/* Create systray icons or actions */
	if (notifyEnabled()) {
		if (notifyCombined()) {
			DELETE_OBJECT(mTrayIcon);

			if (mNotifyIcon == NULL) {
				mNotifyIcon = notifyMenu->addAction(getIcon(), "", this, SLOT(trayIconClicked()));
				mNotifyIcon->setVisible(false);
				connect(mNotifyIcon, SIGNAL(hovered()), this, SLOT(trayIconHovered()));
			}
		} else {
			DELETE_OBJECT(mNotifyIcon);

			if (mTrayIcon == NULL) {
				/* Create the tray icon for messages */
				mTrayIcon = new QSystemTrayIcon(this);
				mTrayIcon->setIcon(getIcon());
				connect(mTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)));
			}
		}
	} else {
		DELETE_OBJECT(mTrayIcon);
		DELETE_OBJECT(mNotifyIcon);
	}

#undef DELETE_OBJECT
}

void UserNotify::updateIcon()
{
	startUpdate();
}

void UserNotify::startUpdate()
{
	update();
}

void UserNotify::update()
{
	unsigned int count = getNewCount();

	if (mMainAction) {
		mMainAction->setIcon(getMainIcon(count > 0));
		mMainAction->setText((count > 0) ? QString("%1 (%2)").arg(mButtonText).arg(count) : mButtonText);

		QFont font = mMainAction->font();
		font.setBold(count > 0);
		mMainAction->setFont(font);
	}

	if (mListItem) {
		mListItem->setIcon(getMainIcon(count > 0));
		mListItem->setText((count > 0) ? QString("%1 (%2)").arg(mButtonText).arg(count) : mButtonText);

		QFont font = mListItem->font();
		font.setBold(count > 0);
		mListItem->setFont(font);
	}
	if (mMainToolButton) {
		mMainToolButton->setStyleSheet((count > 0) ? "QToolButton { color: #E21D3A; }" : "");

		QFont font = mMainToolButton->font();
		font.setBold(count > 0);
		mMainToolButton->setFont(font);
	}

	if (mTrayIcon) {
		if (count) {
			mTrayIcon->setToolTip("RetroShare\n" + getTrayMessage(count > 1).arg(count));
			mTrayIcon->show();
		} else {
			mTrayIcon->hide();
		}
	}

	if (mNotifyIcon) {
		mNotifyIcon->setData(count);
		if (count) {
			mNotifyIcon->setText(getNotifyMessage(count > 1).arg(count));
			mNotifyIcon->setVisible(true);
		} else {
			mNotifyIcon->setVisible(false);
		}
	}

	if (mNewCount != count) {
		emit countChanged();
	}

	mNewCount = count;
}

QString UserNotify::getTrayMessage(bool plural)
{
	return plural ? tr("You have %1 new messages") : tr("You have %1 new message");
}

QString UserNotify::getNotifyMessage(bool plural)
{
	return plural ? tr("%1 new messages") : tr("%1 new message");
}

void UserNotify::trayIconClicked(QSystemTrayIcon::ActivationReason e)
{
	if (e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick || e == QSystemTrayIcon::Context) {
		iconClicked();
	}
}

void UserNotify::trayIconHovered()
{
	iconHovered();
}

void UserNotify::blink(bool on)
{
	if (mTrayIcon) {
		bool blinking = notifyBlink();

		if (blinking) {
			/* blink icon */
			mTrayIcon->setIcon(on ? getIcon() : QIcon());
		} else {
			if (mLastBlinking) {
				/* reset icon */
				mTrayIcon->setIcon(getIcon());
			}
		}

		mLastBlinking = blinking;
	}
}

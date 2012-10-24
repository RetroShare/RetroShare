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

UserNotify::UserNotify(QObject *parent) :
	QObject(parent)
{
	mMainToolButton = NULL;
	mMainAction = NULL;
	mTrayIcon = NULL;
	mNotifyIcon = NULL;
	mNewCount = 0;
}

void UserNotify::initialize(QToolBar *mainToolBar, QAction *mainAction)
{
	mMainAction = mainAction;
	if (mMainAction) {
		mButtonText = mMainAction->text();
		if (mainToolBar) {
			mMainToolButton = dynamic_cast<QToolButton*>(mainToolBar->widgetForAction(mMainAction));
		}
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
	unsigned int count = getNewCount();

	if (mMainAction) {
		mMainAction->setIcon(getMainIcon(count > 0));
		mMainAction->setText((count > 0) ? QString("%1 (%2)").arg(mButtonText).arg(count) : mButtonText);

		QFont font = mMainAction->font();
		font.setBold(count > 0);
		mMainAction->setFont(font);
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
	if (e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick) {
		iconClicked();
	}
}

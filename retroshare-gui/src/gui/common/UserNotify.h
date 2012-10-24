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

#ifndef USERNOTIFY_H
#define USERNOTIFY_H

#include <QObject>
#include <QSystemTrayIcon>

class QToolBar;
class QToolButton;
class QAction;
class QTimer;

class UserNotify : public QObject
{
	Q_OBJECT

public:
	UserNotify(QObject *parent = 0);

	void initialize(QToolBar *mainToolBar, QAction *mainAction);
	void createIcons(QMenu *notifyMenu);

	virtual bool hasSetting(QString &/*name*/) { return false; }
	virtual bool notifyEnabled() { return false; }
	virtual bool notifyCombined() { return false; }
	virtual void setNotifyEnabled(bool /*enabled*/, bool /*combined*/) {}

signals:
	void countChanged();

public slots:
	void updateIcon();

private slots:
	void trayIconClicked(QSystemTrayIcon::ActivationReason e = QSystemTrayIcon::Trigger);
	void timer();

private:
	virtual QIcon getIcon() { return QIcon(); }
	virtual QIcon getMainIcon(bool /*hasNew*/) { return QIcon(); }
	virtual unsigned int getNewCount() { return 0; }
	virtual bool isBlinking() { return false; }

	virtual QString getTrayMessage(bool plural);
	virtual QString getNotifyMessage(bool plural);

	virtual void iconClicked() {}

	QToolButton *mMainToolButton;
	QAction *mMainAction;
	QSystemTrayIcon *mTrayIcon;
	QAction *mNotifyIcon;
	unsigned int mNewCount;
	QString mButtonText;
	QTimer *mTimer;
	bool mLastBlinking;
};

#endif // USERNOTIFY_H

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

#ifndef CHATLOBBYUSERNOTIFY_H
#define CHATLOBBYUSERNOTIFY_H

#include "gui/common/UserNotify.h"

class ChatLobbyUserNotify : public UserNotify
{
	Q_OBJECT

public:
	ChatLobbyUserNotify(QObject *parent = 0);

	virtual bool hasSetting(QString *name, QString *group);

public slots:
	void unreadCountChanged(uint unreadCount);

private:
	virtual QIcon getIcon();
	virtual QIcon getMainIcon(bool hasNew);
	virtual unsigned int getNewCount();
	virtual void iconClicked();

private:
	uint mUnreadCount;
};

#endif // CHATLOBBYUSERNOTIFY_H

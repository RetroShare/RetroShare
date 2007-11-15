/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 crypton
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

#ifndef CHATAVATARFRAME_H
#define CHATAVATARFRAME_H

#include "ui_AvatarFrame.h"

#include <QtGui/QWidget>
#include <QtCore/QList>
#include <QtCore/QStringList>

class QHBoxLayout;
class ChatAvatarWidget;


class ChatAvatarFrame : public QWidget {
	Q_OBJECT
public:

	ChatAvatarFrame(QWidget * parent);

	void setUserPixmap(QPixmap pixmap);

	void addRemoteContact(const QString & id, const QString & displayName, const QString & contactId, QPixmap avatar);

	void removeRemoteContact(const QString & contactId);

	void updateContact(const QString & id, QPixmap avatar, const QString & displayName);

private:

	typedef QList<ChatAvatarWidget*> WidgetList;

	WidgetList _widgetList;

	QVBoxLayout * _layout;

	Ui::AvatarFrame ui;

	QStringList _contactIdList;
};

#endif	//CHATAVATARFRAME_H

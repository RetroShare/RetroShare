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

#ifndef CHATAVATARWIDGET_H
#define CHATAVATARWIDGET_H

#include "ui_AvatarWidget.h"

#include <QtGui/QWidget>


class ChatAvatarWidget : public QWidget {
	Q_OBJECT
public:

	enum OWPictureMode {
		HUGE,			// 96x96
		BIG,			// 60x60
		MEDIUM,			// 48x48
		SMALL,			// 24x24
		TINY,			// 12x12
	};

	enum NicknameMode {
		RIGHT,
		LEFT,
		TOP,
		BOTTOM,
		NONE,
	};

	ChatAvatarWidget(QWidget * parent, const QString & id, QPixmap picture,
		const QString & nickname, const QString & contactId, OWPictureMode pmode = SMALL, NicknameMode nmode = NONE);

	const QString getContactId() {
		return _contactId;
	}

	void setupPixmap(QPixmap pixmap);

	void setToolTip(const QString & nickname);

private:

	void setupNickname(const QString & nickname);

	OWPictureMode _pictureMode;

	NicknameMode _nicknameMode;

	QString _contactId;

	Ui::AvatarWidget ui;
};

#endif	//CHATAVATARWIDGET_H

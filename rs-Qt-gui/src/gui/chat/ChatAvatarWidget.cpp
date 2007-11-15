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

#include "ChatAvatarWidget.h"

#include <QtGui/QtGui>

//#include <profile/AvatarList.h>



ChatAvatarWidget::ChatAvatarWidget(QWidget * parent, const QString & id,
	QPixmap picture, const QString & nickname, const QString & contactId, OWPictureMode pmode, NicknameMode nmode)
	: QWidget(parent), _pictureMode(pmode), _nicknameMode(nmode), _contactId(id) {

	ui.setupUi(this);
	//setupPixmap(picture);
	setToolTip(nickname);
	//setupNickname(contactId);
	/*
	if (_nicknameMode != NONE) {
		setupNickname(nickname);
	} else {
		_ui.nicknameLabel->hide();
	}*/
}

void ChatAvatarWidget::setupPixmap(QPixmap pixmap) {

	//TODO:: resize fond_avatar.png
	QPixmap background = QPixmap(":/images/avatar_background.png");
	QPixmap defaultAvatar = QPixmap(":/images/nopic.png");
	//QPixmap defaultAvatar(QString::fromStdString(AvatarList::getInstance().getDefaultAvatar().getFullPath()));
	QPainter painter(&background);

	if (!pixmap.isNull()) {
			switch (_pictureMode) {
			case HUGE:
				painter.drawPixmap(0, 0, pixmap.scaled(96, 96, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
				ui.pictureLabel->resize(96, 96);
				setMinimumSize(96, 96);
				break;
			case BIG:
				painter.drawPixmap(5, 5, pixmap.scaled(60, 60, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
				ui.pictureLabel->resize(70, 70);
				setMinimumSize(70, 70);
				break;
			case MEDIUM:
				painter.drawPixmap(0, 0, pixmap.scaled(48, 48, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
				ui.pictureLabel->resize(48, 48);
				setMinimumSize(48, 48);
				break;
			case SMALL:
				painter.drawPixmap(0, 0, pixmap.scaled(24, 24, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
				ui.pictureLabel->resize(24, 24);
				setMinimumSize(24, 24);
				break;
			case TINY:
				painter.drawPixmap(0, 0, pixmap.scaled(12, 12, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
				ui.pictureLabel->resize(12, 12);
				setMinimumSize(12, 12);
				break;
			//default:
				//LOG_WARN("unknown picture mode: " + String::fromNumber(_pictureMode));
		}
	} else {
		painter.drawPixmap(5, 5, defaultAvatar.scaled(60, 60, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		ui.pictureLabel->resize(70, 70);
		setMinimumSize(70, 70);
	}

	painter.end();
	ui.pictureLabel->setPixmap(background);
}

void ChatAvatarWidget::setupNickname(const QString & nickname) {
	//TODO: limit string length
	QFontMetrics fontMetrics(ui.nicknameLabel->font());
	int width = 60;
	QString temp;
	for(int i = 0; i < nickname.length(); i++) {
		if (fontMetrics.width(temp) > width) {
			break;
		}
		temp += nickname[i];
	}
	ui.nicknameLabel->setText(temp);
}

void ChatAvatarWidget::setToolTip(const QString & nickname) {
	ui.pictureLabel->setToolTip(nickname);
}

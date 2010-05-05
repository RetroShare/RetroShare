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

#include "ChatToaster.h"

#include "ui_ChatToaster.h"

#include "QtToaster.h"


#include <QtGui/QtGui>

ChatToaster::ChatToaster()
	: QObject(NULL) {

	_chatToasterWidget = new QWidget(NULL);

	_ui = new Ui::ChatToaster();
	_ui->setupUi(_chatToasterWidget);

	_ui->chatButton->setPixmaps(QPixmap(":/images/toaster/chat.png"),
			QPixmap(),
			QPixmap(),
			QPixmap(":/images/toaster/chat.png"),
			QPixmap(),
			QPixmap());

	connect(_ui->chatButton, SIGNAL(clicked()), SLOT(chatButtonSlot()));

	connect(_ui->closeButton, SIGNAL(clicked()), SLOT(close()));

	_toaster = new QtToaster(this, _chatToasterWidget, _ui->windowFrame);
	_toaster->setTimeOnTop(5000);
}

ChatToaster::~ChatToaster() {
	delete(_ui);
}

void ChatToaster::setMessage(const QString & message) {
	_ui->messageLabel->setText(message);
}

void ChatToaster::setPixmap(const QPixmap & pixmap) {
	_ui->pixmapLabel->setPixmap(pixmap);
}

void ChatToaster::show() {
	_toaster->show();
}

void ChatToaster::close() {
	_toaster->close();
}

void ChatToaster::chatButtonSlot() {
	chatButtonClicked();
	close();
}

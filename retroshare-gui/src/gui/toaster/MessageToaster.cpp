/*
 * RetroShare
 * Copyright (C) 2006,2007  crypton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "MessageToaster.h"

#include "ui_MessageToaster.h"

#include "QtToaster.h"

#include <QtGui/QtGui>

MessageToaster::MessageToaster()
	: QObject(NULL) {

	_messageToasterWidget = new QWidget(NULL);

	_ui = new Ui::MessageToaster();
	_ui->setupUi(_messageToasterWidget);

	connect(_ui->messageButton, SIGNAL(clicked()), SLOT(chatButtonSlot()));

	connect(_ui->closeButton, SIGNAL(clicked()), SLOT(close()));

	_toaster = new QtToaster(_messageToasterWidget, _ui->windowFrame);
	_toaster->setTimeOnTop(5000);
}

MessageToaster::~MessageToaster() {
	delete _ui;
}

void MessageToaster::setMessage(const QString & message) {
	_ui->messagelabel2->setText(message);
}

void MessageToaster::setPixmap(const QPixmap & pixmap) {
	_ui->pixmaplabel->setPixmap(pixmap);
}

void MessageToaster::show() {
	_toaster->show();
}

void MessageToaster::close() {
	_toaster->close();
}

void MessageToaster::chatButtonSlot() {
	chatButtonClicked();
	close();
}

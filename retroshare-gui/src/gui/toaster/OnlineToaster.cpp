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

#include "OnlineToaster.h"

#include "ui_OnlineToaster.h"

#include "QtToaster.h"

#include <QtGui/QtGui>

OnlineToaster::OnlineToaster()
	: QObject(NULL) {

	_onlineToasterWidget = new QWidget(NULL);

	_ui = new Ui::OnlineToaster();
	_ui->setupUi(_onlineToasterWidget);

	connect(_ui->messageButton, SIGNAL(clicked()), SLOT(chatButtonSlot()));

	connect(_ui->closeButton, SIGNAL(clicked()), SLOT(close()));

	_toaster = new QtToaster(_onlineToasterWidget, _ui->windowFrame);
	_toaster->setTimeOnTop(5000);
}

OnlineToaster::~OnlineToaster() {
	delete _ui;
}

void OnlineToaster::setMessage(const QString & message) {
	_ui->messageLabel->setText(message);
}

void OnlineToaster::setPixmap(const QPixmap & pixmap) {
	_ui->pixmaplabel->setPixmap(pixmap);
}

void OnlineToaster::show() {
	_toaster->show();
}

void OnlineToaster::close() {
	_toaster->close();
}

void OnlineToaster::chatButtonSlot() {
	chatButtonClicked();
	close();
}

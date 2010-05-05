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

#include "CallToaster.h"

#include "QtToaster.h"

#include "ui_CallToaster.h"

//#include <util/SafeDelete.h>
//#include <util/SafeConnect.h>

#include <QtGui/QtGui>

CallToaster::CallToaster()
	: QObject(NULL) {

	_callToasterWidget = new QWidget(NULL);

	_ui = new Ui::CallToaster();
	_ui->setupUi(_callToasterWidget);

	_ui->hangUpButton->setPixmaps(QPixmap(":/images/toaster/hangup.png"),
			QPixmap(),
			QPixmap(),
			QPixmap(":/images/toaster/hangup.png"),
			QPixmap(),
			QPixmap());

	_ui->pickUpButton->setPixmaps(QPixmap(":/images/toaster/pickup.png"),
			QPixmap(),
			QPixmap(),
			QPixmap(":/images/toaster/pickup.png"),
			QPixmap(),
			QPixmap());

	_ui->pickUpButton->setMinimumSize(QSize(48, 56));
	_ui->pickUpButton->setMaximumSize(QSize(48, 56));
	connect(_ui->pickUpButton, SIGNAL(clicked()), SLOT(pickUpButtonSlot()));

	_ui->hangUpButton->setMinimumSize(QSize(28, 56));
	_ui->hangUpButton->setMaximumSize(QSize(28, 56));
	connect(_ui->hangUpButton, SIGNAL(clicked()), SLOT(hangUpButtonSlot()));

	connect(_ui->closeButton, SIGNAL(clicked()), SLOT(close()));

	_toaster = new QtToaster(this, _callToasterWidget, _ui->windowFrame);
	_toaster->setTimeOnTop(10000);
}

CallToaster::~CallToaster() {
	delete(_ui);
}

void CallToaster::setMessage(const QString & message) {
	_ui->messageLabel->setText(message);
}

void CallToaster::setPixmap(const QPixmap & pixmap) {
	_ui->pixmapLabel->setPixmap(pixmap);
}

void CallToaster::show() {
	_toaster->show();
}

void CallToaster::close() {
	_toaster->close();
}

void CallToaster::hangUpButtonSlot() {
	hangUpButtonClicked();
	close();
}

void CallToaster::pickUpButtonSlot() {
	pickUpButtonClicked();
	close();
}

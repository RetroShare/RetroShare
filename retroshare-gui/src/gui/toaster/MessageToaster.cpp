/*
 * RetroShare
 * Copyright (C) 2006 - 2009  RetroShare Team
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
#include "../MainWindow.h"

#include <retroshare/rspeers.h>

MessageToaster::MessageToaster(const std::string &peerId, const QString &title, const QString &message) : QWidget(NULL)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	/* connect buttons */
	connect(ui.closeButton, SIGNAL(clicked()), this, SLOT(hide()));
	//connect(ui.openmessagebtn, SIGNAL(clicked()), this, SLOT(openmessageClicked()));
	connect(ui.toasterButton, SIGNAL(clicked()), this, SLOT(openmessageClicked()));

	/* set informations */
	ui.subjectLabel->setText(tr("Sub:") + " " + title);
	ui.subjectLabel->setToolTip(title);
	ui.textLabel->setText(message);
	ui.textLabel->setToolTip(message);
	ui.toasterLabel->setText(ui.toasterLabel->text() + " " + QString::fromUtf8(rsPeers->getPeerName(peerId).c_str()));
}

void MessageToaster::openmessageClicked()
{
	MainWindow::showWindow(MainWindow::Messages);
	hide();
}

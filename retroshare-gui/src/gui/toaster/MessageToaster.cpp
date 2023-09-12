/*******************************************************************************
 * gui/toaster/MessageToaster.cpp                                              *
 *                                                                             *
 * Copyright (C) 2010 Retroshare Team <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "MessageToaster.h"
#include "../MainWindow.h"

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>

#include "gui/msgs/MessageInterface.h"

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

	MessageInfo mi;

	if (!rsMail->getMessage(peerId, mi))
		return;

	QString srcName;

	if(mi.msgflags & RS_MSG_DISTANT)
	{
		RsIdentityDetails details ;
		rsIdentity->getIdDetails(mi.from.toGxsId(), details) ;

		srcName = QString::fromUtf8(details.mNickname.c_str());
	}
	else
		srcName = QString::fromUtf8(rsPeers->getPeerName(mi.from.toRsPeerId()).c_str());

	ui.toasterLabel->setText(ui.toasterLabel->text() + " " + srcName);
}

void MessageToaster::openmessageClicked()
{
	MainWindow::showWindow(MainWindow::Messages);
	hide();
}

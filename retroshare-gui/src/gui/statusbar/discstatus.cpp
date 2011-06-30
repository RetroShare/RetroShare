/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 RetroShare Team
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

#include <QHBoxLayout>
#include <QLabel>

#include "discstatus.h"
#include "gui/settings/rsharesettings.h"

#include <retroshare/rsdisc.h>

DiscStatus::DiscStatus(QWidget *parent)
 : QWidget(parent)
{
	hide (); // show only, when pending operations are available

	QHBoxLayout *hbox = new QHBoxLayout(this);
	hbox->setMargin(0);
	hbox->setSpacing(6);

	QLabel *iconLabel = new QLabel(this);
	iconLabel->setPixmap(QPixmap(":/images/uploads.png"));
	iconLabel->setToolTip(tr("Waiting outgoing discovery operations"));
	hbox->addWidget(iconLabel);

	sendLabel = new QLabel("0", this);
	sendLabel->setToolTip(iconLabel->toolTip());
	hbox->addWidget(sendLabel);

	iconLabel = new QLabel(this);
	iconLabel->setPixmap(QPixmap(":/images/download.png"));
	iconLabel->setToolTip(tr("Waiting incoming discovery operations"));
	hbox->addWidget(iconLabel);

	recvLabel = new QLabel("0", this);
	recvLabel->setToolTip(iconLabel->toolTip());
	hbox->addWidget(recvLabel);

	hbox->addSpacing(2);

	setLayout(hbox);
}

void DiscStatus::update()
{
	if (rsDisc == NULL || (Settings->getStatusBarFlags() & STATUSBAR_DISC) == 0) {
		hide();
		return;
	}

	unsigned int sendCount = 0;
	unsigned int recvCount = 0;

	rsDisc->getWaitingDiscCount(&sendCount, &recvCount);

	sendLabel->setText(QString::number(sendCount));
	recvLabel->setText(QString::number(recvCount));

	setVisible(sendCount || recvCount);
}

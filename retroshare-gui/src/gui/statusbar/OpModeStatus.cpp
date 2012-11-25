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

#include "gui/statusbar/OpModeStatus.h"
#include <retroshare/rsconfig.h>
#include <iostream>

OpModeStatus::OpModeStatus(QWidget *parent)
 : QComboBox(parent)
{
	/* add the options */
	addItem(tr("Normal Mode"), RS_OPMODE_FULL);
	addItem(tr("No Anon D/L"), RS_OPMODE_NOTURTLE);
	addItem(tr("Gaming Mode"), RS_OPMODE_GAMING);
	addItem(tr("Low Traffic"), RS_OPMODE_MINIMAL);

	connect(this, SIGNAL(activated( int )), this, SLOT(setOpMode()));
}


void OpModeStatus::getOpMode()
{
	int opMode = rsConfig->getOperatingMode();
	switch(opMode)
	{
		default:
		case RS_OPMODE_FULL:
			setCurrentIndex(0);
			break;
		case RS_OPMODE_NOTURTLE:
			setCurrentIndex(1);
			break;
		case RS_OPMODE_GAMING:
			setCurrentIndex(2);
			break;
		case RS_OPMODE_MINIMAL:
			setCurrentIndex(3);
			break;
	}
}

void OpModeStatus::setOpMode()
{
	std::cerr << "OpModeStatus::setOpMode()";
	std::cerr << std::endl;

        int idx = currentIndex();
        QVariant var = itemData(idx);
        uint32_t opMode = var.toUInt();

	rsConfig->setOperatingMode(opMode);

	// reload to be safe.
	getOpMode();
}



/*******************************************************************************
 * gui/statusbar/OpModeStatus.cpp                                              *
 *                                                                             *
 * Copyright (c) 2008 Retroshare Team <retroshare.project@gmail.com>           *
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

#include <QHBoxLayout>
#include <QMessageBox>
#include <QLabel>

#include "gui/statusbar/OpModeStatus.h"
#include "gui/settings/rsharesettings.h"

#include <retroshare/rsconfig.h>

#include <iostream>

OpModeStatus::OpModeStatus(QWidget *parent)
	: QComboBox(parent)
{
	onUpdate = false;
	opMode_Full_Color = QColor("#CCFFCC");
	opMode_NoTurtle_Color = QColor("#CCCCFF");
	opMode_Gaming_Color = QColor("#FFFFCC");
	opMode_Minimal_Color = QColor("#FFCCCC");

	/* add the options */
	addItem(tr("Normal Mode"), RS_OPMODE_FULL);
	setItemData(0, opMode_Full_Color, Qt::BackgroundRole);
	addItem(tr("No Anon D/L"), RS_OPMODE_NOTURTLE);
	setItemData(1, opMode_NoTurtle_Color, Qt::BackgroundRole);
	addItem(tr("Gaming Mode"), RS_OPMODE_GAMING);
	setItemData(2, opMode_Gaming_Color, Qt::BackgroundRole);
	addItem(tr("Low Traffic"), RS_OPMODE_MINIMAL);
	setItemData(3, opMode_Minimal_Color, Qt::BackgroundRole);

	connect(this, SIGNAL(activated( int )), this, SLOT(setOpMode()));

	setCurrentIndex(Settings->valueFromGroup("StatusBar", "OpMode", QVariant(0)).toInt());
	setOpMode();
	setToolTip(tr("Use this DropList to quickly change Retroshare's behaviour\n No Anon D/L: switches off file forwarding\n Gaming Mode: 25% standard traffic and TODO: reduced popups\n Low Traffic: 10% standard traffic and TODO: pauses all file-transfers"));

	setFocusPolicy(Qt::ClickFocus);
}

void OpModeStatus::getOpMode()
{
	int opMode = rsConfig->getOperatingMode();
	switch(opMode)
	{
		default:
		case RS_OPMODE_FULL:
			setCurrentIndex(0);
			setProperty("opMode", "Full");
		break;
		case RS_OPMODE_NOTURTLE:
			setCurrentIndex(1);
			setProperty("opMode", "NoTurtle");
		break;
		case RS_OPMODE_GAMING:
			setCurrentIndex(2);
			setProperty("opMode", "Gaming");
		break;
		case RS_OPMODE_MINIMAL:
			setCurrentIndex(3);
			setProperty("opMode", "Minimal");
		break;
	}
	onUpdate = true;
	style()->unpolish(this);
	style()->polish(this);
	update();
	onUpdate = false;
}

void OpModeStatus::setOpMode()
{
	std::cerr << "OpModeStatus::setOpMode()";
	std::cerr << std::endl;

	int idx = currentIndex();
	QVariant var = itemData(idx);
	uint32_t opMode = var.toUInt();

	QString message = tr("<p>Warning: This Operating mode disables the tunneling service. This means you can use distant chat not anonymously download files and the mail service will be slower.</p><p>This state will be saved after restart, so do not forget that you changed it!</p>");

	if(opMode == RS_OPMODE_NOTURTLE && ! Settings->getPageAlreadyDisplayed(QString("RS_OPMODE_NO_TURTLE")))
	{
		QMessageBox::warning(NULL,tr("Turtle routing disabled!"),message);
		Settings->setPageAlreadyDisplayed(QString("RS_OPMODE_NO_TURTLE"),true) ;
	}
	if( (opMode == RS_OPMODE_MINIMAL  && ! Settings->getPageAlreadyDisplayed(QString("RS_OPMODE_MINIMAL"))))
	{
		QMessageBox::warning(NULL,tr("Turtle routing disabled!"),message);
		Settings->setPageAlreadyDisplayed(QString("RS_OPMODE_MINIMAL"),true) ;
	}

	rsConfig->setOperatingMode(opMode);

	// reload to be safe.
	getOpMode();
	Settings->setValueToGroup("StatusBar", "OpMode", idx);
}

QColor OpModeStatus::getOpMode_Full_Color() const
{
	return opMode_Full_Color;
}

void OpModeStatus::setOpMode_Full_Color( QColor c )
{
	opMode_Full_Color = c;
	setItemData(0, opMode_Full_Color, Qt::BackgroundRole);
	if (!onUpdate)
		getOpMode();
}

QColor OpModeStatus::getOpMode_NoTurtle_Color() const
{
	return opMode_NoTurtle_Color;
}

void OpModeStatus::setOpMode_NoTurtle_Color( QColor c )
{
	opMode_NoTurtle_Color = c;
	setItemData(1, opMode_NoTurtle_Color, Qt::BackgroundRole);
	if (!onUpdate)
		getOpMode();
}

QColor OpModeStatus::getOpMode_Gaming_Color() const
{
	return opMode_Gaming_Color;
}

void OpModeStatus::setOpMode_Gaming_Color( QColor c )
{
	opMode_Gaming_Color = c;
	setItemData(2, opMode_Gaming_Color, Qt::BackgroundRole);
	if (!onUpdate)
		getOpMode();
}

QColor OpModeStatus::getOpMode_Minimal_Color() const
{
	return opMode_Minimal_Color;
}

void OpModeStatus::setOpMode_Minimal_Color( QColor c )
{
	opMode_Minimal_Color = c;
	setItemData(3, opMode_Minimal_Color, Qt::BackgroundRole);
	if (!onUpdate)
		getOpMode();
}

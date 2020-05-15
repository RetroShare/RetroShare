/*******************************************************************************
 * gui/TheWire/PulseMessage.cpp                                                *
 *                                                                             *
 * Copyright (c) 2020-2020 Robert Fernie   <retroshare.project@gmail.com>      *
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

#include "PulseMessage.h"

/** Constructor */

PulseMessage::PulseMessage(QWidget *parent)
:QWidget(parent)
{
	setupUi(this);
}

void PulseMessage::setup(RsWirePulseSPtr pulse)
{
	if (!pulse) {
		return;
	}

	setMessage(QString::fromStdString(pulse->mPulseText));

	// setup images.
	if (!pulse->mImage1.empty()) {
		// install image.
	} else {
		// leave this visible for a bit.
		// label_image1->setVisible(false);
	}

	if (!pulse->mImage2.empty()) {
		// install image.
	} else {
		label_image2->setVisible(false);
	}

	if (!pulse->mImage3.empty()) {
		// install image.
	} else {
		label_image3->setVisible(false);
	}

	if (!pulse->mImage4.empty()) {
		// install image.
	} else {
		label_image4->setVisible(false);
	}
}

void PulseMessage::setMessage(QString msg)
{
	textBrowser->setPlainText(msg);
}


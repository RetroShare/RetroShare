/*******************************************************************************
 * gui/LogoBar.cpp                                                             *
 *                                                                             *
 * Copyright (c) 2006 Crypton          <retroshare.project@gmail.com>          *
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

#include "LogoBar.h"

#include <util/RetroStyleLabel.h>
#include <util/MouseEventFilter.h>

#include <QGridLayout>

LogoBar::LogoBar(QWidget * parent)
	: QFrame(parent) {

	init();
}

LogoBar::~LogoBar() {
}

void LogoBar::init() {
	setFrameShape(QFrame::NoFrame);


	
	//LogoButton
	_logoButton = new RetroStyleLabel(this);
	_logoButton->setPixmaps(
			QPixmap(":/images/logobar/rslogo2.png"), //Start
			QPixmap(), //End
			QPixmap(), //Fill

			QPixmap(":/images/logobar/rslogo2.png"), //Start
			QPixmap(), //End
			QPixmap() //Fill
			);
	_logoButton->setMaximumSize(QSize(110, 65));
	_logoButton->setMinimumSize(QSize(110, 65));
	connect(_logoButton, SIGNAL(clicked()), SLOT(logoButtonClickedSlot()));

	//FillLabel1
	RetroStyleLabel * FillLabel1 = new RetroStyleLabel(this);
	FillLabel1->setPixmaps(
			QPixmap(), //Start
			QPixmap(), //End
			QPixmap(":/images/logobar/logo_bar_fill.png"), //Fill

			QPixmap(), //Start
			QPixmap(), //End
			QPixmap(":/images/logobar/logo_bar_fill.png") //Fill
			);

    //FillLabel2
	RetroStyleLabel * FillLabel2 = new RetroStyleLabel(this);
	FillLabel2->setPixmaps(
			QPixmap(), //Start
			QPixmap(), //End
			QPixmap(":/images/logobar/logo_bar_fill.png"), //Fill

			QPixmap(), //Start
			QPixmap(), //End
			QPixmap(":/images/logobar/logo_bar_fill.png") //Fill
			);

	QGridLayout * layout = new QGridLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);

	layout->addWidget(FillLabel1, 0, 0);
	layout->addWidget(_logoButton, 0, 1);
	layout->addWidget(FillLabel2, 0, 2);


}

void LogoBar::setEnabledLogoButton(bool enable) {
	_logoButton->setEnabled(enable);
}


void LogoBar::logoButtonClickedSlot() {
	logoButtonClicked();
}





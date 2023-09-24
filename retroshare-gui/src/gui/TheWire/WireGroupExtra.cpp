/*******************************************************************************
 * retroshare-gui/src/gui/TheWire/WireGroupExtra.cpp                           *
 *                                                                             *
 * Copyright (C) 2020 by Robert Fernie  <retroshare.project@gmail.com>         *
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

#include "WireGroupExtra.h"
#include "util/misc.h"

WireGroupExtra::WireGroupExtra(QWidget *parent) :
	QWidget(NULL)
{
	ui.setupUi(this);
	setUp();
}

WireGroupExtra::~WireGroupExtra()
{
}

void WireGroupExtra::setUp()
{
	connect(ui.pushButton_masthead, SIGNAL(clicked() ), this , SLOT(addMasthead()));

	int desired_height = ui.pushButton_masthead->height() + ui.removeButton->height() + ui.lineEdit_Tagline->height();
	int desired_width =  3/1.0 * desired_height + ui.lineEdit_Tagline->width(); 

	ui.label_masthead->setFixedSize(desired_width, desired_height);

	setMasthead(QPixmap());
}

void WireGroupExtra::addMasthead()
{
    QPixmap img = misc::getOpenThumbnailedPicture(this, tr("Load Masthead"), 800, 600);

    if (img.isNull())
        return;

    setMasthead(img);
}

void WireGroupExtra::setTagline(const std::string &str)
{
	ui.lineEdit_Tagline->setText(QString::fromStdString(str));
}

void WireGroupExtra::setLocation(const std::string &str)
{
	ui.lineEdit_Location->setText(QString::fromStdString(str));
}

void WireGroupExtra::setMasthead(const QPixmap &pixmap)
{
	mMasthead = pixmap;

	if (!mMasthead.isNull()) {
		ui.label_masthead->setPicture(mMasthead);
		ui.label_masthead->setToolTip(tr("Use the mouse to zoom and adjust the image for your background."));
	} else {
		ui.label_masthead->setPicture(QPixmap());
		ui.label_masthead->setText(tr("MastHead background Image"));
	}
}

void WireGroupExtra::on_removeButton_clicked()
{
	ui.label_masthead->setPicture(QPixmap());
	ui.label_masthead->setText(tr("MastHead background Image"));
}

std::string WireGroupExtra::getTagline()
{
	return ui.lineEdit_Tagline->text().toStdString();
}

std::string WireGroupExtra::getLocation()
{
	return ui.lineEdit_Location->text().toStdString();
}

QPixmap WireGroupExtra::getMasthead()
{
	return ui.label_masthead->extractCroppedScaledPicture();
}

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
#include "gui/Posted/PhotoView.h"
#include "util/misc.h"

/** Constructor */

PulseMessage::PulseMessage(QWidget *parent)
:QWidget(parent)
{
	setupUi(this);

	connect(label_image1, SIGNAL(clicked()), this, SLOT(viewPictureOne()));
	connect(label_image2, SIGNAL(clicked()), this, SLOT(viewPictureTwo()));
	connect(label_image3, SIGNAL(clicked()), this, SLOT(viewPictureThree()));
	connect(label_image4, SIGNAL(clicked()), this, SLOT(viewPictureFour()));
}

void PulseMessage::setup(RsWirePulseSPtr pulse)
{
	if (!pulse) {
		return;
	}

	mPulse = pulse;

	setMessage(QString::fromStdString(pulse->mPulseText));

	// show indent if republish (both RESPONSE or REF)
	bool showIndent = (pulse->mPulseType & WIRE_PULSE_TYPE_REPUBLISH);
	widget_indent->setVisible(showIndent);

	// setup images.
	int width  = 256;
	int height = 128;
	bool imagesShown = false;

	if (pulse->mImage2.empty()) {
		// allow wider space for image 1.
		width = 512;
	}

	if (!pulse->mImage1.empty()) {
		// install image.
		QPixmap qtn;
		qtn.loadFromData(pulse->mImage1.mData, pulse->mImage1.mSize);
		label_image1->setPixmap(qtn.scaled(width, height,
			Qt::KeepAspectRatio, Qt::SmoothTransformation));
		imagesShown = true;
	} else {
		label_image1->setVisible(false);
	}

	if (!pulse->mImage2.empty()) {
		// install image.
		QPixmap qtn;
		qtn.loadFromData(pulse->mImage2.mData, pulse->mImage2.mSize);
		label_image2->setPixmap(qtn.scaled(width, height,
			Qt::KeepAspectRatio, Qt::SmoothTransformation));
		imagesShown = true;
	} else {
		label_image2->setVisible(false);
	}

	width = 256;
	if (pulse->mImage4.empty()) {
		// allow wider space for image 3.
		width = 512;
	}

	if (!pulse->mImage3.empty()) {
		// install image.
		QPixmap qtn;
		qtn.loadFromData(pulse->mImage3.mData, pulse->mImage3.mSize);
		label_image3->setPixmap(qtn.scaled(width, height,
			Qt::KeepAspectRatio, Qt::SmoothTransformation));
		imagesShown = true;
	} else {
		label_image3->setVisible(false);
	}

	if (!pulse->mImage4.empty()) {
		// install image.
		QPixmap qtn;
		qtn.loadFromData(pulse->mImage4.mData, pulse->mImage4.mSize);
		label_image4->setPixmap(qtn.scaled(width, height,
			Qt::KeepAspectRatio, Qt::SmoothTransformation));
		imagesShown = true;
	} else {
		label_image4->setVisible(false);
	}

	frame_expand->setVisible(imagesShown);
}

void PulseMessage::setMessage(QString msg)
{
	textBrowser->setPlainText(msg);
}

void PulseMessage::setRefImageCount(uint32_t count)
{
	QString msg = "Follow to see Image";
	label_image1->setText(msg);
	label_image2->setText(msg);
	label_image3->setText(msg);
	label_image4->setText(msg);

	label_image1->setVisible(false);
	label_image2->setVisible(false);
	label_image3->setVisible(false);
	label_image4->setVisible(false);

	switch(count) {
		case 4:
			label_image4->setVisible(true);
		case 3:
			label_image3->setVisible(true);
		case 2:
			label_image2->setVisible(true);
		case 1:
			label_image1->setVisible(true);
		default:
			break;
	}

	if (count < 1) {
		frame_expand->setVisible(false);
		label_image1->setDisabled(true);
		label_image2->setDisabled(true);
		label_image3->setDisabled(true);
		label_image4->setDisabled(true);
	}
}

void PulseMessage::viewPictureOne()
{
	PhotoView *photoView = new PhotoView(this);

	if (!mPulse->mImage1.empty()) {
		// install image.
		QPixmap pixmap;
		pixmap.loadFromData(mPulse->mImage1.mData, mPulse->mImage1.mSize);
		photoView->setPixmap(pixmap);
	}

	QString timestamp = misc::timeRelativeToNow(mPulse->mRefPublishTs);

	photoView->setTitle(QString::fromStdString(mPulse->mPulseText));
	photoView->setGroupNameString(QString::fromStdString(mPulse->mRefGroupName));
	photoView->setTime(timestamp);
	//photoView->setGroupId(mPulse->mRefGroupId);

	photoView->show();

	/* window will destroy itself! */
}

void PulseMessage::viewPictureTwo()
{
	PhotoView *photoView = new PhotoView(this);

	if (!mPulse->mImage2.empty()) {
		// install image.
		QPixmap pixmap;
		pixmap.loadFromData(mPulse->mImage2.mData, mPulse->mImage2.mSize);
		photoView->setPixmap(pixmap);
	}

	QString timestamp = misc::timeRelativeToNow(mPulse->mRefPublishTs);

	photoView->setTitle(QString::fromStdString(mPulse->mPulseText));
	photoView->setGroupNameString(QString::fromStdString(mPulse->mRefGroupName));
	photoView->setTime(timestamp);
	//photoView->setGroupId(mPulse->mRefGroupId);

	photoView->show();

	/* window will destroy itself! */
}

void PulseMessage::viewPictureThree()
{
	PhotoView *photoView = new PhotoView(this);

	if (!mPulse->mImage3.empty()) {
		// install image.
		QPixmap pixmap;
		pixmap.loadFromData(mPulse->mImage3.mData, mPulse->mImage3.mSize);
		photoView->setPixmap(pixmap);
	}

	QString timestamp = misc::timeRelativeToNow(mPulse->mRefPublishTs);

	photoView->setTitle(QString::fromStdString(mPulse->mPulseText));
	photoView->setGroupNameString(QString::fromStdString(mPulse->mRefGroupName));
	photoView->setTime(timestamp);
	//photoView->setGroupId(mPulse->mRefGroupId);

	photoView->show();

	/* window will destroy itself! */
}

void PulseMessage::viewPictureFour()
{
	PhotoView *photoView = new PhotoView(this);

	if (!mPulse->mImage4.empty()) {
		// install image.
		QPixmap pixmap;
		pixmap.loadFromData(mPulse->mImage4.mData, mPulse->mImage4.mSize);
		photoView->setPixmap(pixmap);
	}

	QString timestamp = misc::timeRelativeToNow(mPulse->mRefPublishTs);

	photoView->setTitle(QString::fromStdString(mPulse->mPulseText));
	photoView->setGroupNameString(QString::fromStdString(mPulse->mRefGroupName));
	photoView->setTime(timestamp);
	//photoView->setGroupId(mPulse->mRefGroupId);

	photoView->show();

	/* window will destroy itself! */
}

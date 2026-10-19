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
#include "PulseViewItem.h"
#include "gui/Posted/PhotoView.h"
#include "util/misc.h"

#include <QAbstractTextDocumentLayout>
#include <QTextDocument>
#include <QtMath>

// Feed column is ~600px wide, minus avatar and margins.
#define WIRE_IMAGE_FULL_WIDTH  500
#define WIRE_IMAGE_HALF_WIDTH  248
#define WIRE_IMAGE_MAX_HEIGHT  300
#define WIRE_IMAGE_RADIUS      14

/** Constructor */

PulseMessage::PulseMessage(QWidget *parent)
:QWidget(parent)
{
	setupUi(this);

	connect(label_image1, SIGNAL(clicked()), this, SLOT(viewPictureOne()));
	connect(label_image2, SIGNAL(clicked()), this, SLOT(viewPictureTwo()));
	connect(label_image3, SIGNAL(clicked()), this, SLOT(viewPictureThree()));
	connect(label_image4, SIGNAL(clicked()), this, SLOT(viewPictureFour()));

	// Text: no frame, no scrollbars, height follows the text (like Twitter).
	textBrowser->setFrameShape(QFrame::NoFrame);
	textBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	textBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	textBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	textBrowser->document()->setDocumentMargin(0);
	connect(textBrowser->document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
	        this, [this](const QSizeF &size) {
		textBrowser->setFixedHeight(qCeil(size.height()) + 4);
	});

	// Images: no box around them, small gap between them.
	frame_expand->setFrameShape(QFrame::NoFrame);
	gridLayout->setContentsMargins(0, 6, 0, 0);
	gridLayout->setSpacing(4);
	gridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

	ClickableLabel *labels[4] = { label_image1, label_image2, label_image3, label_image4 };
	for (int i = 0; i < 4; ++i) {
		labels[i]->setMinimumSize(0, 0);
		labels[i]->setCursor(Qt::PointingHandCursor);
	}
}

static QPixmap loadWireImage(const RsGxsImage &image)
{
	QPixmap pixmap;
	if (!image.empty()) {
		pixmap.loadFromData(image.mData, image.mSize);
	}
	return pixmap;
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

	// setup images, Twitter style:
	//  1 image  : full width, natural aspect (max height capped)
	//  2 images : side by side
	//  3/4      : 2x2 grid
	// each label keeps showing its own mImageN so the click handlers stay valid.
	const RsGxsImage *images[4] = { &pulse->mImage1, &pulse->mImage2, &pulse->mImage3, &pulse->mImage4 };
	ClickableLabel *labels[4] = { label_image1, label_image2, label_image3, label_image4 };

	int count = 0;
	for (int i = 0; i < 4; ++i) {
		if (!images[i]->empty()) {
			++count;
		}
	}

	bool imagesShown = false;
	for (int i = 0; i < 4; ++i)
	{
		QPixmap pixmap = loadWireImage(*images[i]);
		if (pixmap.isNull()) {
			labels[i]->setVisible(false);
			continue;
		}

		int w = WIRE_IMAGE_HALF_WIDTH;
		int h = (count == 2) ? 280 : 160;
		if (count == 1) {
			w = WIRE_IMAGE_FULL_WIDTH;
			h = qMin(WIRE_IMAGE_MAX_HEIGHT, qRound(pixmap.height() * (double) w / qMax(1, pixmap.width())));
		}

		labels[i]->setPixmap(WireRoundedPixmap(pixmap, w, h, WIRE_IMAGE_RADIUS));
		labels[i]->setFixedSize(w, h);
		labels[i]->setVisible(true);
		imagesShown = true;
	}

	frame_expand->setVisible(imagesShown);
}

void PulseMessage::setMessage(QString msg)
{
	textBrowser->setPlainText(msg);
	textBrowser->setVisible(!msg.trimmed().isEmpty());
}

void PulseMessage::setLargeText(bool large)
{
	QFont f = textBrowser->font();
	f.setPointSizeF(large ? 13.5 : 11);
	textBrowser->setFont(f);
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
	}
}

void PulseMessage::viewPictureOne()
{
	PhotoView *photoView = new PhotoView(window());

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
	PhotoView *photoView = new PhotoView(window());

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
	PhotoView *photoView = new PhotoView(window());

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
	PhotoView *photoView = new PhotoView(window());

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

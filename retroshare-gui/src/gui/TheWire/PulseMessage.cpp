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
#include <QMovie>
#include <QShowEvent>
#include <QHideEvent>
#include "gui/Posted/BoardPostImageHelper.h"
#include <QTextDocument>
#include <QtMath>

// Feed column is ~600px wide, minus avatar and margins.
#define WIRE_IMAGE_FULL_WIDTH  500
#define WIRE_IMAGE_HALF_WIDTH  248
#define WIRE_IMAGE_MAX_HEIGHT  300
// thread view: no avatar column, so the media can use the full width of the 600px column.
#define WIRE_IMAGE_LARGE_FULL_WIDTH  530
#define WIRE_IMAGE_LARGE_HALF_WIDTH  263
#define WIRE_IMAGE_LARGE_MAX_HEIGHT  420
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

	// the republish indent line is only shown by setup() for republishes.
	widget_indent->setVisible(false);

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
		// remove an animation of a previous setup()
		if (mMovies[i]) {
			mMovies[i]->stop();
			mMovies[i]->deleteLater();
			mMovies[i] = nullptr;
		}

		// Animated GIF (data from other nodes): only after the safety checks,
		// else it is shown as a still image (first frame) like before.
		QMovie *movie = nullptr;
		if (!images[i]->empty() && WireIsSafeAnimatedGif(images[i]->mData, images[i]->mSize)) {
			movie = BoardPostImageHelper::createMovieFromData(images[i]->mData, images[i]->mSize);
		}

		QPixmap pixmap;
		if (movie) {
			movie->setParent(labels[i]);
			movie->setCacheMode(QMovie::CacheNone); // don't keep all frames in memory
			movie->jumpToFrame(0);
			pixmap = movie->currentPixmap();
		} else {
			pixmap = loadWireImage(*images[i]);
		}

		if (pixmap.isNull()) {
			if (movie) {
				movie->deleteLater();
			}
			labels[i]->setVisible(false);
			continue;
		}

		int w = mLarge ? WIRE_IMAGE_LARGE_HALF_WIDTH : WIRE_IMAGE_HALF_WIDTH;
		int h = (count == 2) ? 280 : 160;
		if (count == 1) {
			w = mLarge ? WIRE_IMAGE_LARGE_FULL_WIDTH : WIRE_IMAGE_FULL_WIDTH;
			h = qMin(mLarge ? WIRE_IMAGE_LARGE_MAX_HEIGHT : WIRE_IMAGE_MAX_HEIGHT,
			         qRound(pixmap.height() * (double) w / qMax(1, pixmap.width())));
		}

		labels[i]->setPixmap(WireRoundedPixmap(pixmap, w, h, WIRE_IMAGE_RADIUS));
		labels[i]->setFixedSize(w, h);
		labels[i]->setVisible(true);
		imagesShown = true;

		if (movie)
		{
			// every frame with the same size and rounded corners as a still image.
			ClickableLabel *label = labels[i];
			connect(movie, &QMovie::frameChanged, label, [label, movie, w, h](int) {
				label->setPixmap(WireRoundedPixmap(movie->currentPixmap(), w, h, WIRE_IMAGE_RADIUS));
			});
			connect(movie, &QMovie::finished, movie, &QMovie::start); // loop
			mMovies[i] = movie;
			labels[i]->installEventFilter(this); // hover: play / pause (feed)

			if (mLarge) {
				// thread view: the pulse is open, play it.
				labels[i]->setToolTip(tr("Animated GIF - click to view"));
				if (isVisible()) {
					movie->start();
				}
			} else {
				// feed: first frame only, plays while the mouse is over it.
				labels[i]->setToolTip(tr("Animated GIF - hover to play, click to view"));
			}
		}
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
	mLarge = large; // used by setup() for the image sizes

	// thread view: same side margins as the header/date rows (the message had none,
	// so text and image started further left than the rest), and the images centered.
	if (large) {
		gridLayout_2->setContentsMargins(10, 0, 10, 0);
		gridLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
	} else {
		gridLayout_2->setContentsMargins(0, 0, 0, 0);
		gridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	}
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

static void playMovie(QMovie *movie)
{
	if (!movie) return;
	if (movie->state() == QMovie::Paused) {
		movie->setPaused(false);
	} else if (movie->state() == QMovie::NotRunning) {
		movie->start();
	}
}

void PulseMessage::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);

	// only the thread view plays without hover.
	if (mLarge) {
		for (QMovie *movie : mMovies) {
			playMovie(movie);
		}
	}
}

bool PulseMessage::eventFilter(QObject *watched, QEvent *event)
{
	if (!mLarge && (event->type() == QEvent::Enter || event->type() == QEvent::Leave))
	{
		ClickableLabel *labels[4] = { label_image1, label_image2, label_image3, label_image4 };
		for (int i = 0; i < 4; ++i)
		{
			if (watched == labels[i] && mMovies[i])
			{
				if (event->type() == QEvent::Enter) {
					playMovie(mMovies[i]);
				} else if (mMovies[i]->state() == QMovie::Running) {
					mMovies[i]->setPaused(true);
				}
			}
		}
	}
	return QWidget::eventFilter(watched, event);
}

void PulseMessage::hideEvent(QHideEvent *event)
{
	QWidget::hideEvent(event);
	for (QMovie *movie : mMovies) {
		if (movie && movie->state() == QMovie::Running) {
			movie->setPaused(true);
		}
	}
}

void PulseMessage::openPhotoView(const RsGxsImage &image)
{
	if (!mPulse) {
		return;
	}

	PhotoView *photoView = new PhotoView(window());

	if (!image.empty())
	{
		QMovie *movie = nullptr;
		if (WireIsSafeAnimatedGif(image.mData, image.mSize)) {
			movie = BoardPostImageHelper::createMovieFromData(image.mData, image.mSize);
		}

		if (movie) {
			movie->setParent(photoView); // PhotoView doesn't delete it, Qt does
			movie->setCacheMode(QMovie::CacheNone);
			photoView->setMovie(movie);
		} else {
			QPixmap pixmap;
			pixmap.loadFromData(image.mData, image.mSize);
			photoView->setPixmap(pixmap);
		}
	}

	QString timestamp = misc::timeRelativeToNow(mPulse->mRefPublishTs);

	photoView->setTitle(QString::fromStdString(mPulse->mPulseText));
	photoView->setGroupNameString(QString::fromStdString(mPulse->mRefGroupName));
	photoView->setTime(timestamp);

	photoView->show();

	emit pictureOpened();

	/* window will destroy itself! */
}

void PulseMessage::viewPictureOne()
{
	if (mPulse) openPhotoView(mPulse->mImage1);
}

void PulseMessage::viewPictureTwo()
{
	if (mPulse) openPhotoView(mPulse->mImage2);
}

void PulseMessage::viewPictureThree()
{
	if (mPulse) openPhotoView(mPulse->mImage3);
}

void PulseMessage::viewPictureFour()
{
	if (mPulse) openPhotoView(mPulse->mImage4);
}

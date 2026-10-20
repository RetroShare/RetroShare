/*******************************************************************************
 * gui/TheWire/PulseAddDialog.cpp                                              *
 *                                                                             *
 * Copyright (c) 2012-2020 Robert Fernie   <retroshare.project@gmail.com>      *
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

#include <iostream>
#include <QtGui>
#include <QMessageBox>
#include <QComboBox>
#include <QFile>
#include <QImageReader>

#include "PulseReply.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/FilesDefs.h"
#include "util/misc.h"
#include "util/qtthreadsutils.h"
#include "gui/Posted/BoardPostImageHelper.h"

#include "PulseAddDialog.h"

const uint32_t PULSE_MAX_SIZE = 1000; // 1k char.

// A GXS message can't be bigger than RsSerialiser::MAX_SERIAL_SIZE (256 KB) in total
// (text + 4 images + signatures), keep some room for the rest.
const uint32_t PULSE_MAX_IMAGE_BYTES = 230 * 1024;

/** Constructor */
PulseAddDialog::PulseAddDialog(QWidget *parent)
: QWidget(parent), mAccountChooser(nullptr), mIsReply(false)
{
	ui.setupUi(this);

	// "From": combo box to choose the account, in place of the group name label.
	// (The headshot / name / id labels are shown by the combo box items now.)
	mAccountChooser = new QComboBox(ui.frame_from);
	mAccountChooser->setIconSize(QSize(20, 20));
	mAccountChooser->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	mAccountChooser->setToolTip(tr("Post as"));
	{
		int idx = ui.frame_from_GL->indexOf(ui.label_groupName);
		int row = 0, col = 1, rowSpan = 1, colSpan = 1;
		if (idx >= 0) {
			ui.frame_from_GL->getItemPosition(idx, &row, &col, &rowSpan, &colSpan);
		}
		ui.frame_from_GL->addWidget(mAccountChooser, row, col, rowSpan, colSpan);
	}
	ui.label_groupName->hide();
	ui.label_idName->hide();
	ui.topheadshot->hide();
	connect(mAccountChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(accountChanged(int)));

	// no Cancel button: the window has its close button.
	ui.pushButton_Cancel->hide();

	connect(ui.postButton, SIGNAL( clicked( void ) ), this, SLOT( postPulse( void ) ) );
	connect(ui.pushButton_AddURL, SIGNAL( clicked( void ) ), this, SLOT( addURL( void ) ) );
	connect(ui.pushButton_ClearDisplayAs, SIGNAL( clicked( void ) ), this, SLOT( clearDisplayAs( void ) ) );
	connect(ui.pushButton_Cancel, SIGNAL( clicked( void ) ), this, SLOT( cancelPulse( void ) ) );
	connect(ui.textEdit_Pulse, SIGNAL( textChanged( void ) ), this, SLOT( pulseTextChanged( void ) ) );
	connect(ui.pushButton_picture, SIGNAL(clicked()), this, SLOT( toggle()));
	connect(ui.pushButton_remove, SIGNAL(clicked()), this, SLOT( removePictures()));


    // this connection is from browse push button to the slot function onBrowseButtonClicked()
    connect(ui.pushButton_Browse, SIGNAL(clicked()), this, SLOT( onBrowseButtonClicked()));

	ui.pushButton_picture->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/photo.png")));
	ui.pushButton_Browse->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/add-image.png")));
	ui.pushButton_remove->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/mail/delete.png")));

	ui.frame_picture->hide();
	//ui.pushButton_picture->hide();

    // initially hiding the browse button as the attach image button is not pressed
	//ui.frame_PictureBrowse->hide();

	ui.label_image1->hide();
	ui.label_image2->hide();
	ui.label_image3->hide();
	ui.label_image4->hide();

	setAcceptDrops(true);
}

void PulseAddDialog::setGroup(RsWireGroup &group)
{
	ui.label_groupName->setText(QString::fromStdString(group.mMeta.mGroupName));
	ui.label_idName->setText(QString::fromStdString(group.mMeta.mAuthorId.toStdString()));
	
	if (group.mHeadshot.mData )
	{
		QPixmap pixmap;
		if (GxsIdDetails::loadPixmapFromData(
				group.mHeadshot.mData,
				group.mHeadshot.mSize,
				pixmap,GxsIdDetails::ORIGINAL))
		{
				pixmap = pixmap.scaled(50,50);
				ui.headshot->setPixmap(pixmap);
				ui.topheadshot->setPixmap(pixmap);
		}
	}
	else
	{
		// default.
		QPixmap pixmap = FilesDefs::getPixmapFromQtResourcePath(":/icons/wire.png").scaled(50,50);
		ui.headshot->setPixmap(pixmap);
		ui.topheadshot->setPixmap(pixmap);
	}
	
	mGroup = group;
}

void PulseAddDialog::setOwnGroups(const std::vector<RsWireGroup> &groups, int selected)
{
	mOwnGroups = groups;

	mAccountChooser->blockSignals(true);
	mAccountChooser->clear();
	for (const RsWireGroup &group : mOwnGroups)
	{
		// same picture as on the main page: headshot, else the default group icon.
		QPixmap pixmap;
		if (group.mHeadshot.mData) {
			GxsIdDetails::loadPixmapFromData(group.mHeadshot.mData, group.mHeadshot.mSize, pixmap, GxsIdDetails::ORIGINAL);
		}
		if (pixmap.isNull()) {
			pixmap = GxsIdDetails::makeDefaultGroupIcon(group.mMeta.mGroupId, ":/icons/wire.png", GxsIdDetails::ORIGINAL);
		}
		mAccountChooser->addItem(QIcon(WireRoundPixmap(pixmap, 32)), QString::fromStdString(group.mMeta.mGroupName));
		mAccountChooser->setItemData(mAccountChooser->count() - 1,
			QString::fromStdString(group.mMeta.mAuthorId.toStdString()), Qt::ToolTipRole);
	}

	if (selected < 0 || selected >= (int)mOwnGroups.size()) {
		selected = 0;
	}
	mAccountChooser->setCurrentIndex(selected);
	mAccountChooser->blockSignals(false);

	accountChanged(selected);
}

void PulseAddDialog::accountChanged(int index)
{
	if (index < 0 || index >= (int)mOwnGroups.size()) {
		return;
	}
	setGroup(mOwnGroups[index]); // sets mGroup = the account we post / reply with
}

// set ReplyWith Group.
void PulseAddDialog::setGroup(const RsGxsGroupId &grpId)
{
    if(grpId.isNull()){
        return;
    }

    RsThread::async([this,grpId](){

        RsWireGroupSPtr pGroup;
        if(!rsWire->getWireGroup(grpId,pGroup))
        {
            std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve wire group info for wire id:  " << grpId << std::endl;
            return;
        }

        RsQThreadUtils::postToObject( [pGroup,this]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete, note that
             * Qt::QueuedConnection is important!
             */

            setGroup(*pGroup);
        }, this );

    });
}

void PulseAddDialog::cleanup()
{
	resize(700, 400 );

	if (mIsReply)
	{
		std::cerr << "PulseAddDialog::cleanup() cleaning up old replyto";
		std::cerr << std::endl;
		QLayout *layout = ui.widget_replyto->layout();
		if (layout)
		{
			// completely delete layout and sublayouts
			QLayoutItem * item;
			while ((item = layout->takeAt(0)))
			{
				if (QWidget *widget = item->widget())
				{
					std::cerr << "PulseAddDialog::cleanup() removing widget";
					std::cerr << std::endl;
					widget->hide();
					delete widget;
				}
				else
				{
					std::cerr << "PulseAddDialog::cleanup() removing item";
					std::cerr << std::endl;
					delete item;
				}
			}
			// then finally
			delete layout;
		}
		mIsReply = false;
	}

	mReplyToPulse = RsWirePulse();
	mReplyType = 0;

	ui.frame_reply->setVisible(false);
	ui.comboBox_sentiment->setCurrentIndex(0);
	ui.lineEdit_URL->setText("");
	ui.lineEdit_DisplayAs->setText("");
	ui.textEdit_Pulse->setPlainText("");
	// disable URL until functionality finished.
	ui.frame_URL->setEnabled(false);
	ui.frame_URL->hide();

	ui.postButton->setEnabled(false);
	ui.postButton->setText(tr("Post"));
	ui.textEdit_Pulse->setPlaceholderText(tr("Whats happening?"));
	ui.frame_input->setVisible(true);
	ui.widget_sentiment->setVisible(true);
	ui.pushButton_picture->show();
	ui.topheadshot->setVisible(mAccountChooser == nullptr);

	// cleanup images.
	mImage1.clear();
	ui.label_image1->clear();
	ui.label_image1->setText(tr("Drag and Drop Image"));

	mImage2.clear();
	ui.label_image2->clear();
	ui.label_image2->setText(tr("Drag and Drop Image"));

	mImage3.clear();
	ui.label_image3->clear();
	ui.label_image3->setText(tr("Drag and Drop Image"));

	mImage4.clear();
	ui.label_image4->clear();
	ui.label_image4->setText(tr("Drag and Drop Image"));

    // Hide Drag & Drop Frame and the browse frame
    ui.frame_picture->hide();
    //ui.frame_PictureBrowse->hide();
	ui.pushButton_picture->hide();

	ui.pushButton_picture->setChecked(false);
}

void PulseAddDialog::pulseTextChanged()
{
	std::string pulseText = ui.textEdit_Pulse->toPlainText().toStdString();
	bool enable = (pulseText.size() > 0) && (pulseText.size() < PULSE_MAX_SIZE);

	// A reply can only be posted once the parent pulse is loaded (setReplyTo() fetches it
	// in the background). Before, typing enabled "Post" while it was still loading and the
	// click was silently ignored ("reply data not ready yet").
	if (mIsReply && mReplyToPulse.mMeta.mGroupId.isNull()) {
		enable = false;
	}
	ui.postButton->setEnabled(enable);
}

// Old Interface, deprecate / make internal.
// TODO: Convert mReplyToPulse to be an SPtr, and remove &pulse parameter.
void PulseAddDialog::setReplyTo(const RsWirePulse &pulse, RsWirePulseSPtr pPulse, std::string &/*groupName*/, uint32_t replyType)
{
	mIsReply = true;
	mReplyToPulse = pulse;
	mReplyType = replyType;
	ui.frame_reply->setVisible(true);
	ui.pushButton_picture->hide();
	ui.topheadshot->hide();

	{
		PulseReply *reply = new PulseReply(NULL, pPulse);

		// add extra widget into layout.
		QVBoxLayout *vbox = new QVBoxLayout();
		vbox->addWidget(reply);
		vbox->setSpacing(1);
		vbox->setContentsMargins(0,0,0,0);
		ui.widget_replyto->setLayout(vbox);
		ui.widget_replyto->setVisible(true);
	}

	if (mReplyType & WIRE_PULSE_TYPE_REPLY)
	{
		ui.postButton->setText(tr("Reply to Pulse"));
		ui.textEdit_Pulse->setPlaceholderText(tr("Pulse your reply"));
		pulseTextChanged(); // parent loaded: enable "Post" if text was typed meanwhile
	}
	else
	{
		// cannot add msg for like / republish.
		ui.postButton->setEnabled(true);
		ui.frame_input->setVisible(false);
		ui.widget_sentiment->setVisible(false);
		if (mReplyType & WIRE_PULSE_TYPE_REPUBLISH) {
			ui.postButton->setText(tr("Republish Pulse"));
			ui.pushButton_picture->hide();
			ui.pushButton_Browse->hide();
		}
		else if (mReplyType & WIRE_PULSE_TYPE_LIKE) {
			ui.postButton->setText(tr("Like Pulse"));
			ui.pushButton_picture->hide();
			ui.pushButton_Browse->hide();
		}
	}

}

void PulseAddDialog::setReplyTo(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId, uint32_t replyType)
{
    if(grpId.isNull()){
        return;
    }

	mIsReply = true;
	mReplyType = replyType;
	ui.postButton->setEnabled(false);
	ui.postButton->setText(tr("Loading..."));
	show();

	/* fetch in the background */

    RsThread::async([this,grpId,msgId,replyType](){

        RsWireGroupSPtr pGroup;
        RsWirePulseSPtr pPulse;
        if(!rsWire->getWireGroup(grpId,pGroup))
        {
            std::cerr << __PRETTY_FUNCTION__ << "PulseAddDialog::setRplyTo() failed to fetch group id: "  << grpId << std::endl;
            RsQThreadUtils::postToObject([this]() { hide(); }, this);
            return;
        }

        if (!rsWire->getWirePulse(grpId, msgId, pPulse))
        {
            std::cerr << "PulseAddDialog::setRplyTo() failed to fetch pulse of group id: " << grpId << std::endl;
            RsQThreadUtils::postToObject([this]() { hide(); }, this);
            return;
        }

        // update GroupPtr
        // TODO - this should be handled in libretroshare if possible.
        if (pPulse->mGroupPtr == NULL) {
            pPulse->mGroupPtr = pGroup;
        }

        RsQThreadUtils::postToObject( [pGroup,this,pPulse,replyType]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete, note that
             * Qt::QueuedConnection is important!
             */

            // NB: don't overwrite mGroup here: it is the account we reply WITH
            // (set by setGroup() before), pGroup is the group of the parent pulse.
            setReplyTo(*pPulse, pPulse, pGroup->mMeta.mGroupName, replyType);
        }, this );

    });

}

void PulseAddDialog::addURL()
{
	std::cerr << "PulseAddDialog::addURL()";
	std::cerr << std::endl;

	return;
}

void PulseAddDialog::clearDisplayAs()
{
	std::cerr << "PulseAddDialog::clearDisplayAs()";
	std::cerr << std::endl;
	return;
}


void PulseAddDialog::cancelPulse()
{
	std::cerr << "PulseAddDialog::cancelPulse()";
	std::cerr << std::endl;

	clearDialog();
	hide();

	return;
}

void PulseAddDialog::postPulse()
{
	std::cerr << "PulseAddDialog::postPulse()";
	std::cerr << std::endl;
	if (mIsReply)
	{
		postReplyPulse();
	}
	else
	{
		postOriginalPulse();
	}
}


void PulseAddDialog::postOriginalPulse()
{
	std::cerr << "PulseAddDialog::postOriginalPulse()";
	std::cerr << std::endl;

    RsWirePulseSPtr pPulse(new RsWirePulse());

    pPulse->mSentiment = WIRE_PULSE_SENTIMENT_NO_SENTIMENT;
    pPulse->mPulseText = ui.textEdit_Pulse->toPlainText().toStdString();
    // set images here too.
    pPulse->mImage1 = mImage1;
    pPulse->mImage2 = mImage2;
    pPulse->mImage3 = mImage3;
    pPulse->mImage4 = mImage4;

    RsThread::async([this,pPulse](){

        if (!rsWire->createOriginalPulse(mGroup.mMeta.mGroupId, pPulse))
        {
            std::cerr << "PulseAddDialog::postOriginalPulse() FAILED";
            std::cerr << std::endl;
            return;
        }

        RsQThreadUtils::postToObject( [this]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete, note that
             * Qt::QueuedConnection is important!
             */

            clearDialog();
            hide();
        }, this );

    });

}

uint32_t PulseAddDialog::toPulseSentiment(int index)
{
	switch(index)
	{
		case 1:
			return WIRE_PULSE_SENTIMENT_POSITIVE;
			break;
		case 2:
			return WIRE_PULSE_SENTIMENT_NEUTRAL;
			break;
		case 3:
			return WIRE_PULSE_SENTIMENT_NEGATIVE;
			break;
		case -1:
		case 0:
		default:
			return WIRE_PULSE_SENTIMENT_NO_SENTIMENT;
			break;
	}
	return 0;
}

void PulseAddDialog::postReplyPulse()
{
	std::cerr << "PulseAddDialog::postReplyPulse()";
	std::cerr << std::endl;

	if (mReplyToPulse.mMeta.mGroupId.isNull()) {
		std::cerr << "PulseAddDialog::postReplyPulse() reply data not ready yet";
		std::cerr << std::endl;
		return;
	}

	if (mGroup.mMeta.mGroupId.isNull()) {
		std::cerr << "PulseAddDialog::postReplyPulse() group not set";
		std::cerr << std::endl;
		return;
	}

	ui.postButton->setEnabled(false);
	setCursor(Qt::WaitCursor);

    RsWirePulseSPtr pPulse(new RsWirePulse());

    pPulse->mSentiment = toPulseSentiment(ui.comboBox_sentiment->currentIndex());
    pPulse->mPulseText = ui.textEdit_Pulse->toPlainText().toStdString();
    // set images here too.
    pPulse->mImage1 = mImage1;
    pPulse->mImage2 = mImage2;
    pPulse->mImage3 = mImage3;
    pPulse->mImage4 = mImage4;

	if (mReplyType & WIRE_PULSE_TYPE_REPUBLISH) {
		// Copy details from parent, and override
		pPulse->mSentiment = mReplyToPulse.mSentiment;
		pPulse->mPulseText = mReplyToPulse.mPulseText;

		// Copy images.
		pPulse->mImage1 = mReplyToPulse.mImage1;
		pPulse->mImage2 = mReplyToPulse.mImage2;
		pPulse->mImage3 = mReplyToPulse.mImage3;
		pPulse->mImage4 = mReplyToPulse.mImage4;
	}

	// some pulses have no OrigMsgId, use the MsgId then (same fallback as libretroshare).
	RsGxsMessageId replyToMsgId = mReplyToPulse.mMeta.mOrigMsgId;
	if (replyToMsgId.isNull()) {
		replyToMsgId = mReplyToPulse.mMeta.mMsgId;
	}

    RsThread::async([this, pPulse, replyToMsgId](){

        if (!rsWire->createReplyPulse(mReplyToPulse.mMeta.mGroupId,
                replyToMsgId,
                mGroup.mMeta.mGroupId,
                mReplyType,
                pPulse))
        {
            std::cerr << "PulseAddDialog::postReplyPulse() FAILED";
            std::cerr << std::endl;
            RsQThreadUtils::postToObject([this]()
            {
                setCursor(Qt::ArrowCursor);
                ui.postButton->setEnabled(true);
                QMessageBox::warning(this, tr("RetroShare"),
                    tr("Could not post the pulse.\nYou can only reply to, republish or like pulses of Wire groups you follow."),
                    QMessageBox::Ok, QMessageBox::Ok);
            }, this);
            return;
        }

        RsQThreadUtils::postToObject( [this]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete, note that
             * Qt::QueuedConnection is important!
             */

            setCursor(Qt::ArrowCursor);
            clearDialog();
            hide();
        }, this );

    });

}

void PulseAddDialog::clearDialog()
{
	ui.textEdit_Pulse->setPlainText("");
}

//---------------------------------------------------------------------
// Drag and Drop Images.

void PulseAddDialog::dragEnterEvent(QDragEnterEvent *event)
{
	std::cerr << "PulseAddDialog::dragEnterEvent()";
	std::cerr << std::endl;

	if (event->mimeData()->hasUrls())
	{
		std::cerr << "PulseAddDialog::dragEnterEvent() Accepting";
		std::cerr << std::endl;
		event->accept();
	}
	else
	{
		std::cerr << "PulseAddDialog::dragEnterEvent() Ignoring";
		std::cerr << std::endl;
		event->ignore();
	}
}

void PulseAddDialog::dragLeaveEvent(QDragLeaveEvent *event)
{
	std::cerr << "PulseAddDialog::dragLeaveEvent()";
	std::cerr << std::endl;

	event->ignore();
}

void PulseAddDialog::dragMoveEvent(QDragMoveEvent *event)
{
	std::cerr << "PulseAddDialog::dragMoveEvent()";
	std::cerr << std::endl;

	event->accept();
}

void PulseAddDialog::dropEvent(QDropEvent *event)
{
	std::cerr << "PulseAddDialog::dropEvent()";
	std::cerr << std::endl;

	if (event->mimeData()->hasUrls())
	{
		std::cerr << "PulseAddDialog::dropEvent() Urls:" << std::endl;

		QList<QUrl> urls = event->mimeData()->urls();
		QList<QUrl>::iterator uit;
		for (uit = urls.begin(); uit != urls.end(); ++uit)
		{
			QString localpath = uit->toLocalFile();
			std::cerr << "Whole URL: " << uit->toString().toStdString() << std::endl;
			std::cerr << "or As Local File: " << localpath.toStdString() << std::endl;

			addImage(localpath);
		}
		event->setDropAction(Qt::CopyAction);
		event->accept();
	}
	else
	{
		std::cerr << "PulseAddDialog::dropEvent Ignoring";
		std::cerr << std::endl;
		event->ignore();
	}
}


void PulseAddDialog::addImage(const QString &path)
{
	std::cerr << "PulseAddDialog::addImage() loading image from: " << path.toStdString();
	std::cerr << std::endl;

	// Animated GIF: keep the original bytes (saving it again would drop the animation).
	{
		QImageReader probe(path);
		if (QString(probe.format()).toUpper() == "GIF" && probe.imageCount() > 1)
		{
			QFile file(path);
			if (!file.open(QIODevice::ReadOnly)) {
				return;
			}
			addAnimatedGif(file.readAll());
			return;
		}
	}

    QPixmap qtn = FilesDefs::getPixmapFromQtResourcePath(path);
	if (qtn.isNull()) {
		std::cerr << "PulseAddDialog::addImage() Invalid Image";
		std::cerr << std::endl;
		return;
	}

	QPixmap image;
	if ((qtn.width() <= 512) && (qtn.height() <= 512)) {
		image = qtn;
	} else {
		image = qtn.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}

	// scaled down for display, allow wide images.
	QPixmap icon = qtn.scaled(256, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	QByteArray ba;
	QBuffer buffer(&ba);

	buffer.open(QIODevice::WriteOnly);
	image.save(&buffer, "JPG");

	installImage(ba, icon);
}

// Animated GIF: checked like the GIFs we receive (format, size, frames, canvas).
void PulseAddDialog::addAnimatedGif(const QByteArray &bytes)
{
	if ((uint32_t) bytes.size() > BoardPostImageHelper::MAX_ANIMATED_SIZE)
	{
		QMessageBox::warning(this, tr("Animated GIF"),
			tr("This animated GIF is too large (%1 KB). The maximum is %2 KB.")
				.arg(bytes.size() / 1024).arg(BoardPostImageHelper::MAX_ANIMATED_SIZE / 1024));
		return;
	}

	if (!WireIsSafeAnimatedGif(reinterpret_cast<const uint8_t *>(bytes.constData()), bytes.size()))
	{
		QMessageBox::warning(this, tr("Animated GIF"),
			tr("This animated GIF can't be used: it may have at most 300 frames and 1024 x 1024 pixels."));
		return;
	}

	QPixmap firstFrame;
	firstFrame.loadFromData(bytes, "GIF");
	QPixmap icon = firstFrame.scaled(256, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	installImage(bytes, icon);
}

uint32_t PulseAddDialog::totalImageBytes() const
{
	return mImage1.mSize + mImage2.mSize + mImage3.mSize + mImage4.mSize;
}

// put the image into the first free slot, if the pulse stays under the size limit.
bool PulseAddDialog::installImage(const QByteArray &bytes, const QPixmap &icon)
{
	if (totalImageBytes() + (uint32_t) bytes.size() > PULSE_MAX_IMAGE_BYTES)
	{
		QMessageBox::warning(this, tr("Image too large"),
			tr("The images of a pulse can be at most %1 KB together (this one: %2 KB, already added: %3 KB).")
				.arg(PULSE_MAX_IMAGE_BYTES / 1024).arg(bytes.size() / 1024).arg(totalImageBytes() / 1024));
		return false;
	}

	uint8_t *data = (uint8_t *) bytes.data();
	if (mImage1.empty()) {
		ui.label_image1->setPixmap(icon);
		ui.label_image1->show();
		ui.frame_picture->show();
		mImage1.copy(data, bytes.size());
	}
	else if (mImage2.empty()) {
		ui.label_image2->setPixmap(icon);
		ui.label_image2->show();
		mImage2.copy(data, bytes.size());
	}
	else if (mImage3.empty()) {
		ui.label_image3->setPixmap(icon);
		ui.label_image3->show();
		mImage3.copy(data, bytes.size());
	}
	else if (mImage4.empty()) {
		ui.label_image4->setPixmap(icon);
		ui.label_image4->show();
		mImage4.copy(data, bytes.size());
	}
	else {
		std::cerr << "PulseAddDialog::installImage() Images all full";
		std::cerr << std::endl;
		return false;
	}
	return true;
}

void PulseAddDialog::toggle()
{
	if (ui.pushButton_picture->isChecked())
	{
        // Show the input methods (drag and drop field and the browse button)
		ui.frame_picture->show();

		ui.pushButton_picture->setToolTip(tr("Hide Pictures"));
	}
	else
	{
        // Hide the input methods (drag and drop field and the browse button)
		ui.frame_picture->hide();

		ui.pushButton_picture->setToolTip(tr("Add Pictures"));
	}
}

// Function to get the file dialog for the browse button
void PulseAddDialog::onBrowseButtonClicked()
{
    QString filePath;
    misc::getOpenFileName(this, RshareSettings::LASTDIR_IMAGES, tr("Load Picture File"), "Pictures (*.png *.xpm *.jpg *.jpeg *.gif *.webp )", filePath);
    if (!filePath.isEmpty()) {
        //ui.lineEdit_FilePath->setText(filePath);
        addImage(filePath);
    }
}

void PulseAddDialog::removePictures()
{

	mImage1.clear();
	ui.label_image1->clear();
	mImage2.clear();
	ui.label_image2->clear();
	mImage3.clear();
	ui.label_image3->clear();
	mImage4.clear();
	ui.label_image4->clear();

	ui.label_image1->hide();
	ui.label_image2->hide();
	ui.label_image3->hide();
	ui.label_image4->hide();

	ui.frame_picture->hide();
}

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

#include "PulseReply.h"

#include "PulseAddDialog.h"

const uint32_t PULSE_MAX_SIZE = 1000; // 1k char.

/** Constructor */
PulseAddDialog::PulseAddDialog(QWidget *parent)
: QWidget(parent), mIsReply(false)
{
	ui.setupUi(this);

	connect(ui.pushButton_Post, SIGNAL( clicked( void ) ), this, SLOT( postPulse( void ) ) );
	connect(ui.pushButton_AddURL, SIGNAL( clicked( void ) ), this, SLOT( addURL( void ) ) );
	connect(ui.pushButton_ClearDisplayAs, SIGNAL( clicked( void ) ), this, SLOT( clearDisplayAs( void ) ) );
	connect(ui.pushButton_Cancel, SIGNAL( clicked( void ) ), this, SLOT( cancelPulse( void ) ) );
	connect(ui.textEdit_Pulse, SIGNAL( textChanged( void ) ), this, SLOT( pulseTextChanged( void ) ) );

	setAcceptDrops(true);
}

void PulseAddDialog::setGroup(RsWireGroup &group)
{
	ui.label_groupName->setText(QString::fromStdString(group.mMeta.mGroupName));
	ui.label_idName->setText(QString::fromStdString(group.mMeta.mAuthorId.toStdString()));
	mGroup = group;
}

// set ReplyWith Group.
void PulseAddDialog::setGroup(const RsGxsGroupId &grpId)
{
	/* fetch in the background */
	RsWireGroupSPtr pGroup;
	rsWire->getWireGroup(grpId, pGroup);

	setGroup(*pGroup);
}

void PulseAddDialog::cleanup()
{
	if (mIsReply)
	{
		std::cerr << "PulseAddDialog::cleanup() cleaning up old replyto";
		std::cerr << std::endl;
		QLayout *layout = ui.widget_replyto->layout();
		// completely delete layout and sublayouts
		QLayoutItem * item;
		QWidget * widget;
		while ((item = layout->takeAt(0)))
		{
			if ((widget = item->widget()) != 0)
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
		mIsReply = false;
	}
	ui.frame_reply->setVisible(false);
	ui.comboBox_sentiment->setCurrentIndex(0);
	ui.lineEdit_URL->setText("");
	ui.lineEdit_DisplayAs->setText("");
	ui.textEdit_Pulse->setPlainText("");
	// disable URL until functionality finished.
	ui.frame_URL->setEnabled(false);

	ui.pushButton_Post->setEnabled(false);
	ui.pushButton_Post->setText("Post Pulse to Wire");
	ui.frame_input->setVisible(true);
	ui.widget_sentiment->setVisible(true);

	// cleanup images.
	mImage1.clear();
	ui.label_image1->clear();
	ui.label_image1->setText("Drag and Drop Image");

	mImage2.clear();
	ui.label_image2->clear();
	ui.label_image2->setText("Drag and Drop Image");

	mImage3.clear();
	ui.label_image3->clear();
	ui.label_image3->setText("Drag and Drop Image");

	mImage4.clear();
	ui.label_image4->clear();
	ui.label_image4->setText("Drag and Drop Image");
}

void PulseAddDialog::pulseTextChanged()
{
	std::string pulseText = ui.textEdit_Pulse->toPlainText().toStdString();
	bool enable = (pulseText.size() > 0) && (pulseText.size() < PULSE_MAX_SIZE);
	ui.pushButton_Post->setEnabled(enable);
}

// Old Interface, deprecate / make internal.
// TODO: Convert mReplyToPulse to be an SPtr, and remove &pulse parameter.
void PulseAddDialog::setReplyTo(RsWirePulse &pulse, RsWirePulseSPtr pPulse, std::string &groupName, uint32_t replyType)
{
	mIsReply = true;
	mReplyToPulse = pulse;
	mReplyType = replyType;
	ui.frame_reply->setVisible(true);

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
		ui.pushButton_Post->setText("Reply to Pulse");
	}
	else
	{
		// cannot add msg for like / republish.
		ui.pushButton_Post->setEnabled(true);
		ui.frame_input->setVisible(false);
		ui.widget_sentiment->setVisible(false);
		if (mReplyType & WIRE_PULSE_TYPE_REPUBLISH) {
			ui.pushButton_Post->setText("Republish Pulse");
		}
		else if (mReplyType & WIRE_PULSE_TYPE_LIKE) {
			ui.pushButton_Post->setText("Like Pulse");
		}
	}

}

void PulseAddDialog::setReplyTo(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId, uint32_t replyType)
{
	/* fetch in the background */
	RsWireGroupSPtr pGroup;
	if (!rsWire->getWireGroup(grpId, pGroup))
	{
		std::cerr << "PulseAddDialog::setRplyTo() failed to fetch group";
		std::cerr << std::endl;
		return;
	}

	RsWirePulseSPtr pPulse;
	if (!rsWire->getWirePulse(grpId, msgId, pPulse))
	{
		std::cerr << "PulseAddDialog::setRplyTo() failed to fetch pulse";
		std::cerr << std::endl;
		return;
	}

	// update GroupPtr
	// TODO - this should be handled in libretroshare if possible.
	if (pPulse->mGroupPtr == NULL) {
		pPulse->mGroupPtr = pGroup;
	}

	setReplyTo(*pPulse, pPulse, pGroup->mMeta.mGroupName, replyType);
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

	// this should be in async thread, so doesn't block UI thread.
	if (!rsWire->createOriginalPulse(mGroup.mMeta.mGroupId, pPulse))
	{
		std::cerr << "PulseAddDialog::postOriginalPulse() FAILED";
		std::cerr << std::endl;
		return;
	}

	clearDialog();
	hide();
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

	// this should be in async thread, so doesn't block UI thread.
	if (!rsWire->createReplyPulse(mReplyToPulse.mMeta.mGroupId,
			mReplyToPulse.mMeta.mOrigMsgId,
			mGroup.mMeta.mGroupId,
			mReplyType,
			pPulse))
	{
		std::cerr << "PulseAddDialog::postReplyPulse() FAILED";
		std::cerr << std::endl;
		return;
	}

	clearDialog();
	hide();
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

	if (mImage1.empty()) {
		std::cerr << "PulseAddDialog::addImage() Installing in Image1";
		std::cerr << std::endl;
		ui.label_image1->setPixmap(icon);
		mImage1.copy((uint8_t *) ba.data(), ba.size());
		std::cerr << "PulseAddDialog::addImage() Installing in Image1 Size: " << mImage1.mSize;
		std::cerr << std::endl;
	}
	else if (mImage2.empty()) {
		ui.label_image2->setPixmap(icon);
		mImage2.copy((uint8_t *) ba.data(), ba.size());
		std::cerr << "PulseAddDialog::addImage() Installing in Image2 Size: " << mImage2.mSize;
		std::cerr << std::endl;
	}
	else if (mImage3.empty()) {
		ui.label_image3->setPixmap(icon);
		mImage3.copy((uint8_t *) ba.data(), ba.size());
		std::cerr << "PulseAddDialog::addImage() Installing in Image3 Size: " << mImage3.mSize;
		std::cerr << std::endl;
	}
	else if (mImage4.empty()) {
		ui.label_image4->setPixmap(icon);
		mImage4.copy((uint8_t *) ba.data(), ba.size());
		std::cerr << "PulseAddDialog::addImage() Installing in Image4 Size: " << mImage4.mSize;
		std::cerr << std::endl;
	}
	else {
		std::cerr << "PulseAddDialog::addImage() Images all full";
		std::cerr << std::endl;
	}
}


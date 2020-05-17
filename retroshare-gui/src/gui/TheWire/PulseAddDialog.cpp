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

#include "PulseDetails.h"

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
}

void PulseAddDialog::pulseTextChanged()
{
	std::string pulseText = ui.textEdit_Pulse->toPlainText().toStdString();
	bool enable = (pulseText.size() > 0) && (pulseText.size() < PULSE_MAX_SIZE);
	ui.pushButton_Post->setEnabled(enable);
}

// Old Interface, deprecate / make internal.
void PulseAddDialog::setReplyTo(RsWirePulse &pulse, std::string &groupName, uint32_t replyType)
{
	mIsReply = true;
	mReplyToPulse = pulse;
	mReplyType = replyType;
	ui.frame_reply->setVisible(true);

	{
		std::map<rstime_t, RsWirePulse *> replies;
		PulseDetails *details = new PulseDetails(NULL, &pulse, groupName, replies);
		// add extra widget into layout.
		QVBoxLayout *vbox = new QVBoxLayout();
		vbox->addWidget(details);
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

	setReplyTo(*pPulse, pGroup->mMeta.mGroupName, replyType);
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
	// set images too.

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




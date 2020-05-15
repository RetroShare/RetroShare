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
: QWidget(parent), mIsReply(false), mWaitingRefMsg(false)
{
	ui.setupUi(this);

	mWireQueue = new TokenQueue(rsWire->getTokenService(), this);

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


void PulseAddDialog::cleanup()
{
	if (mIsReply)
	{
		std::cerr << "PulseAddDialog::setReplyTo() cleaning up old replyto";
		std::cerr << std::endl;
		QLayout *layout = ui.widget_replyto->layout();
		// completely delete layout and sublayouts
		QLayoutItem * item;
		QWidget * widget;
		while ((item = layout->takeAt(0)))
		{
			if ((widget = item->widget()) != 0)
			{
				std::cerr << "PulseAddDialog::setReplyTo() removing widget";
				std::cerr << std::endl;
				widget->hide();
				delete widget;
			}
			else
			{
				std::cerr << "PulseAddDialog::setReplyTo() removing item";
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
	ui.pushButton_Post->setEnabled(false);
	// disable URL until functionality finished.
	ui.frame_URL->setEnabled(false);
}

void PulseAddDialog::pulseTextChanged()
{
	std::string pulseText = ui.textEdit_Pulse->toPlainText().toStdString();
	bool enable = (pulseText.size() > 0) && (pulseText.size() < PULSE_MAX_SIZE);
	ui.pushButton_Post->setEnabled(enable);
}

void PulseAddDialog::setReplyTo(RsWirePulse &pulse, std::string &groupName)
{
	mIsReply = true;
	mReplyToPulse = pulse;
	mReplyGroupName = groupName;
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

	RsWirePulse pulse;

	pulse.mMeta.mGroupId  = mGroup.mMeta.mGroupId;
	pulse.mMeta.mAuthorId = mGroup.mMeta.mAuthorId;
	pulse.mMeta.mThreadId.clear();
	pulse.mMeta.mParentId.clear();
	pulse.mMeta.mOrigMsgId.clear();

	pulse.mPulseType = WIRE_PULSE_TYPE_ORIGINAL;
	pulse.mReplySentiment = WIRE_PULSE_SENTIMENT_NO_SENTIMENT;
	pulse.mPulseText = ui.textEdit_Pulse->toPlainText().toStdString();
	// all mRefs should empty.

	uint32_t token;
	rsWire->createPulse(token, pulse);

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

	RsWirePulse pulse;

	pulse.mMeta.mGroupId  = mGroup.mMeta.mGroupId;
	pulse.mMeta.mAuthorId = mGroup.mMeta.mAuthorId;
	pulse.mMeta.mThreadId.clear();
	pulse.mMeta.mParentId.clear();
	pulse.mMeta.mOrigMsgId.clear();

	pulse.mPulseType = WIRE_PULSE_TYPE_RESPONSE | WIRE_PULSE_TYPE_REPLY;
	pulse.mReplySentiment = toPulseSentiment(ui.comboBox_sentiment->currentIndex());
	pulse.mPulseText = ui.textEdit_Pulse->toPlainText().toStdString();

	// mRefs refer to parent post.
	pulse.mRefGroupId   = mReplyToPulse.mMeta.mGroupId;
	pulse.mRefGroupName = mReplyGroupName;
	pulse.mRefOrigMsgId = mReplyToPulse.mMeta.mOrigMsgId;
	pulse.mRefAuthorId  = mReplyToPulse.mMeta.mAuthorId;
	pulse.mRefPublishTs = mReplyToPulse.mMeta.mPublishTs;
	pulse.mRefPulseText = mReplyToPulse.mPulseText;

	// Need Pulse MsgID before we can create associated Reference.
	mWaitingRefMsg = true;

	uint32_t token;
	rsWire->createPulse(token, pulse);
	mWireQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, 0);
}


void PulseAddDialog::postRefPulse(RsWirePulse &pulse)
{
	std::cerr << "PulseAddDialog::postRefPulse() create Reference!";
	std::cerr << std::endl;

	// Reference Pulse. posted on Parent's Group.
	RsWirePulse refPulse;

	refPulse.mMeta.mGroupId  = mReplyToPulse.mMeta.mGroupId;
	refPulse.mMeta.mAuthorId = mGroup.mMeta.mAuthorId; // own author Id.
	refPulse.mMeta.mThreadId = mReplyToPulse.mMeta.mOrigMsgId;
	refPulse.mMeta.mParentId = mReplyToPulse.mMeta.mOrigMsgId;
	refPulse.mMeta.mOrigMsgId.clear();

	refPulse.mPulseType = WIRE_PULSE_TYPE_REFERENCE | WIRE_PULSE_TYPE_REPLY;
	refPulse.mReplySentiment = toPulseSentiment(ui.comboBox_sentiment->currentIndex());

	// Dont put parent PulseText into refPulse - it is available on Thread Msg.
	// otherwise gives impression it is correctly setup Parent / Reply...
	// when in fact the parent PublishTS, and AuthorId are wrong.
	refPulse.mPulseText = "";

	// refs refer back to own Post.
	refPulse.mRefGroupId   = mGroup.mMeta.mGroupId;
	refPulse.mRefGroupName = mGroup.mMeta.mGroupName;
	refPulse.mRefOrigMsgId = pulse.mMeta.mOrigMsgId;
	refPulse.mRefAuthorId  = mGroup.mMeta.mAuthorId;
	refPulse.mRefPublishTs = pulse.mMeta.mPublishTs;
	refPulse.mRefPulseText = pulse.mPulseText;

	uint32_t token;
	rsWire->createPulse(token, refPulse);

	clearDialog();
	hide();
}

void PulseAddDialog::clearDialog()
{
	ui.textEdit_Pulse->setPlainText("");
}


void PulseAddDialog::acknowledgeMessage(const uint32_t &token)
{
	std::cerr << "PulseAddDialog::acknowledgeMessage()";
	std::cerr << std::endl;

	std::pair<RsGxsGroupId, RsGxsMessageId> p;
	rsWire->acknowledgeMsg(token, p);

	if (mWaitingRefMsg)
	{
		std::cerr << "PulseAddDialog::acknowledgeMessage() Waiting Ref Msg";
		std::cerr << std::endl;
		mWaitingRefMsg = false;

		// request photo data.
		GxsMsgReq req;
		std::set<RsGxsMessageId> msgIds;
		msgIds.insert(p.second);
		req[p.first] = msgIds;

		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
		uint32_t token;
		mWireQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, req, 0);
	}
	else
	{
		std::cerr << "PulseAddDialog::acknowledgeMessage() Not Waiting Ref Msg";
		std::cerr << std::endl;
	}
}

void PulseAddDialog::loadPulseData(const uint32_t &token)
{
	std::cerr << "PulseAddDialog::loadPulseData()";
	std::cerr << std::endl;
	std::vector<RsWirePulse> pulses;
	rsWire->getPulseData(token, pulses);

	if (pulses.size() != 1)
	{
		std::cerr << "PulseAddDialog::loadPulseData() Error Too many pulses";
		std::cerr << std::endl;
		return;
	}

	std::cerr << "PulseAddDialog::loadPulseData() calling postRefMsg";
	std::cerr << std::endl;

	RsWirePulse& pulse = pulses[0];
	postRefPulse(pulse);
}


/**************************** Request / Response Filling of Data ************************/

void PulseAddDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	if (queue == mWireQueue)
	{
		/* now switch on req */
		switch(req.mType)
		{
			case TOKENREQ_MSGINFO:
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_ACK:
						acknowledgeMessage(req.mToken);
						break;
					case RS_TOKREQ_ANSTYPE_DATA:
						loadPulseData(req.mToken);
						break;
					default:
						std::cerr << "PulseAddDialog::loadRequest() ERROR: MSG: INVALID ANS TYPE";
						std::cerr << std::endl;
						break;
				}
				break;
			default:
				std::cerr << "PulseAddDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}
}
	

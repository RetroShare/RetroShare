/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include "GxsMessageFrameWidget.h"
#include "gui/common/UIStateHelper.h"

#include <retroshare/rsgxsifacehelper.h>

GxsMessageFrameWidget::GxsMessageFrameWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent)
    : RsGxsUpdateBroadcastWidget(ifaceImpl, parent)
{
	mNextTokenType = 0;

	mTokenQueue = new TokenQueue(ifaceImpl->getTokenService(), this);
	mStateHelper = new UIStateHelper(this);

	/* Set read status */
	mTokenTypeAcknowledgeReadStatus = nextTokenType();
	mAcknowledgeReadStatusToken = 0;

	/* Add dummy entry to store waiting status */
	mStateHelper->addWidget(mTokenTypeAcknowledgeReadStatus, NULL, 0);
}

GxsMessageFrameWidget::~GxsMessageFrameWidget()
{
	if (mStateHelper->isLoading(mTokenTypeAcknowledgeReadStatus)) {
		mStateHelper->setLoading(mTokenTypeAcknowledgeReadStatus, false);

		emit waitingChanged(this);
	}

	delete(mTokenQueue);
}

const RsGxsGroupId &GxsMessageFrameWidget::groupId()
{
	return mGroupId;
}

bool GxsMessageFrameWidget::isLoading()
{
	return false;
}

bool GxsMessageFrameWidget::isWaiting()
{
	if (mStateHelper->isLoading(mTokenTypeAcknowledgeReadStatus)) {
		return true;
	}

	return false;
}

void GxsMessageFrameWidget::setGroupId(const RsGxsGroupId &groupId)
{
	if (mGroupId == groupId) {
		if (!groupId.isNull()) {
			return;
		}
	}

	mAcknowledgeReadStatusToken = 0;
	if (mStateHelper->isLoading(mTokenTypeAcknowledgeReadStatus)) {
		mStateHelper->setLoading(mTokenTypeAcknowledgeReadStatus, false);

		emit waitingChanged(this);
	}

	mGroupId = groupId;
	groupIdChanged();
}

void GxsMessageFrameWidget::setAllMessagesRead(bool read)
{
	uint32_t token = 0;
	setAllMessagesReadDo(read, token);

	if (token) {
		/* Wait for acknowlegde of the token */
		mAcknowledgeReadStatusToken = token;
		mTokenQueue->queueRequest(mAcknowledgeReadStatusToken, 0, 0, mTokenTypeAcknowledgeReadStatus);
		mStateHelper->setLoading(mTokenTypeAcknowledgeReadStatus, true);

		emit waitingChanged(this);
	}
}

void GxsMessageFrameWidget::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	if (queue == mTokenQueue)
	{
		if (req.mUserType == mTokenTypeAcknowledgeReadStatus) {
			if (mAcknowledgeReadStatusToken == req.mToken) {
				/* Set read status is finished */
				mStateHelper->setLoading(mTokenTypeAcknowledgeReadStatus, false);

				emit waitingChanged(this);
			}
			return;
		}
	}

	std::cerr << "GxsMessageFrameWidget::loadRequest() ERROR: INVALID TYPE";
	std::cerr << std::endl;
}

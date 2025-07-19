/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsMessageFrameWidget.cpp                        *
 *                                                                             *
 * Copyright 2014 Retroshare Team           <retroshare.project@gmail.com>     *
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

#include "GxsMessageFrameWidget.h"
#include "gui/common/UIStateHelper.h"

#include <retroshare/rsgxsifacehelper.h>

GxsMessageFrameWidget::GxsMessageFrameWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent)
    : QWidget(parent)
{
	mNextTokenType = 0;

	mStateHelper = new UIStateHelper(this);

	/* Set read status */
	mTokenTypeAcknowledgeReadStatus = nextTokenType();
	mAcknowledgeReadStatusToken = 0;

	/* Add dummy entry to store waiting status */
	mStateHelper->addWidget(mTokenTypeAcknowledgeReadStatus, NULL, UIStates());
}

GxsMessageFrameWidget::~GxsMessageFrameWidget()
{
	if (mStateHelper->isLoading(mTokenTypeAcknowledgeReadStatus)) {
		mStateHelper->setLoading(mTokenTypeAcknowledgeReadStatus, false);

		emit waitingChanged(this);
	}
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
	if (mGroupId == groupId  && !groupId.isNull())
		return;

    if(!groupId.isNull())
	{
		mAcknowledgeReadStatusToken = 0;
		if (mStateHelper->isLoading(mTokenTypeAcknowledgeReadStatus)) {
			mStateHelper->setLoading(mTokenTypeAcknowledgeReadStatus, false);

			emit waitingChanged(this);
		}

		mGroupId = groupId;
		groupIdChanged();
	}
    else
    {
        mGroupId.clear();
        blank();	// clear the displayed data, because no group is selected.
    }
}

void GxsMessageFrameWidget::setAllMessagesRead(bool read)
{
    setAllMessagesReadDo(read);
}


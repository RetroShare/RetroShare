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
}

GxsMessageFrameWidget::~GxsMessageFrameWidget()
{
	delete(mTokenQueue);
}

const RsGxsGroupId &GxsMessageFrameWidget::groupId()
{
	return mGroupId;
}

void GxsMessageFrameWidget::setGroupId(const RsGxsGroupId &groupId)
{
	if (mGroupId == groupId) {
		if (!groupId.isNull()) {
			return;
		}
	}

	mGroupId = groupId;

	groupIdChanged();
}

void GxsMessageFrameWidget::loadRequest(const TokenQueue */*queue*/, const TokenRequest &/*req*/)
{
	std::cerr << "GxsMessageFrameWidget::loadRequest() ERROR: INVALID TYPE";
	std::cerr << std::endl;
}

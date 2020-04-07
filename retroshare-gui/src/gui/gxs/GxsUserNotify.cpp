/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsUserNotify.cpp                                *
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

#include "GxsUserNotify.h"
//#include "gui/gxs/RsGxsUpdateBroadcastBase.h"

#include "retroshare/rsgxsifacehelper.h"

#define TOKEN_TYPE_STATISTICS  1

GxsUserNotify::GxsUserNotify(RsGxsIfaceHelper *ifaceImpl, QObject *parent) :
    UserNotify(parent), TokenResponse()
{
	mNewThreadMessageCount = 0;
	mNewChildMessageCount = 0;
	mCountChildMsgs = false;

	mInterface = ifaceImpl;
	mTokenService = mInterface->getTokenService();
	mTokenQueue = new TokenQueue(mInterface->getTokenService(), this);

	//mBase = new RsGxsUpdateBroadcastBase(ifaceImpl);
	//connect(mBase, SIGNAL(fillDisplay(bool)), this, SLOT(updateIcon()));
}

GxsUserNotify::~GxsUserNotify()
{
	if (mTokenQueue) {
		delete(mTokenQueue);
	}
	//if (mBase) {
		//delete(mBase);
	//}
}

void GxsUserNotify::startUpdate()
{
	mNewThreadMessageCount = 0;
	mNewChildMessageCount = 0;

	uint32_t token;
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_SERVICE_STATS;

	mTokenService->requestServiceStatistic(token,opts);
	mTokenQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, TOKEN_TYPE_STATISTICS);
}

void GxsUserNotify::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	if (queue == mTokenQueue) {
		/* now switch on req */
		switch(req.mUserType) {
		case TOKEN_TYPE_STATISTICS:
			{
				GxsServiceStatistic stats;
				mInterface->getServiceStatistic(req.mToken, stats);

				mNewThreadMessageCount = stats.mNumThreadMsgsNew;
				mNewChildMessageCount = stats.mNumChildMsgsNew;

				update();
			}
			break;
		}
	}
}

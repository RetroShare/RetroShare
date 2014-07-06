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

#include "GxsUserNotify.h"
#include "gui/gxs/RsGxsUpdateBroadcastBase.h"

#include "retroshare/rsgxsifacehelper.h"

#define TOKEN_TYPE_STATISTICS  1
#define TOKEN_TYPE_GROUP_META  2

GxsUserNotify::GxsUserNotify(RsGxsIfaceHelper *ifaceImpl, QObject *parent) :
    UserNotify(parent), TokenResponse()
{
	mNewMessageCount = 0;

	mInterface = ifaceImpl;
	mTokenQueue = new TokenQueue(mInterface->getTokenService(), this);

	mBase = new RsGxsUpdateBroadcastBase(ifaceImpl);
	connect(mBase, SIGNAL(fillDisplay(bool)), this, SLOT(updateIcon()));
}

GxsUserNotify::~GxsUserNotify()
{
	if (mTokenQueue) {
		delete(mTokenQueue);
	}
	if (mBase) {
		delete(mBase);
	}
}

void GxsUserNotify::startUpdate()
{
//	uint32_t token;
//	GxsServiceStatistic stats;
//	mInterface->getServiceStatistic(token, stats);
//	TokenQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, TOKEN_TYPE_STATISTICS);

	mNewMessageCount = 0;

	/* Get all messages until statistics are available */
	mTokenQueue->cancelActiveRequestTokens(TOKEN_TYPE_GROUP_META);
	mTokenQueue->cancelActiveRequestTokens(TOKEN_TYPE_STATISTICS);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token;
	mTokenQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, TOKEN_TYPE_GROUP_META);
}

void GxsUserNotify::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	if (queue == mTokenQueue) {
		/* now switch on req */
		switch(req.mUserType) {
		case TOKEN_TYPE_STATISTICS:
			{
				GxsMsgMetaMap msgList;
				mInterface->getMsgSummary(req.mToken, msgList);

				GxsMsgMetaMap::const_iterator groupIt;
				for (groupIt = msgList.begin(); groupIt != msgList.end(); ++groupIt) {
					const std::vector<RsMsgMetaData> &groupData = groupIt->second;

					std::vector<RsMsgMetaData>::const_iterator msgIt;
					for (msgIt = groupData.begin(); msgIt != groupData.end(); ++msgIt) {
						const RsMsgMetaData &metaData = *msgIt;
						if (IS_MSG_NEW(metaData.mMsgStatus)) {
							++mNewMessageCount;
						}
					}
				}

				update();
			}
			break;
		case TOKEN_TYPE_GROUP_META:
			{
				std::list<RsGroupMetaData> groupMeta;
				mInterface->getGroupSummary(req.mToken, groupMeta);

				if (!groupMeta.size()) {
					update();
					return;
				}

				std::list<RsGxsGroupId> groupIds;
				std::list<RsGroupMetaData>::const_iterator groupIt;
				for (groupIt = groupMeta.begin(); groupIt != groupMeta.end(); groupIt++) {
					uint32_t flags = groupIt->mSubscribeFlags;
					if (IS_GROUP_SUBSCRIBED(flags)) {
						groupIds.push_back(groupIt->mGroupId);
					}
				}

				if (!groupIds.size()) {
					update();
					return;
				}

				RsTokReqOptions opts;
				opts.mReqType = GXS_REQUEST_TYPE_MSG_META;

				uint32_t token;
				mTokenQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, TOKEN_TYPE_STATISTICS);
			}
			break;
		}
	}
}

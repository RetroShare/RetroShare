/*******************************************************************************
 * gui/NewsFeed.cpp                                                            *
 *                                                                             *
 * Copyright (c) 2008 Robert Fernie    <retroshare.project@gmail.com>          *
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

#include <QTimer>
#include <QDateTime>

#include "NewsFeed.h"
#include "ui_NewsFeed.h"

#include <retroshare/rsbanlist.h>
#include <retroshare/rsgxschannels.h>
#include <retroshare/rsgxsforums.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsnotify.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsplugin.h>
#include <retroshare/rsposted.h>

#include "feeds/ChatMsgItem.h"
#include "feeds/GxsCircleItem.h"
#include "feeds/GxsChannelGroupItem.h"
#include "feeds/GxsChannelPostItem.h"
#include "feeds/GxsForumGroupItem.h"
#include "feeds/GxsForumMsgItem.h"
#include "feeds/MsgItem.h"
#include "feeds/NewsFeedUserNotify.h"
#include "feeds/PeerItem.h"
#include "feeds/PostedGroupItem.h"
#include "feeds/SecurityItem.h"
#include "feeds/SecurityIpItem.h"

#include "settings/rsettingswin.h"
#include "settings/rsharesettings.h"

#include "chat/ChatDialog.h"
#include "Posted/PostedItem.h"
#include "msgs/MessageComposer.h"
#include "msgs/MessageInterface.h"

#include "common/FeedNotify.h"
#include "notifyqt.h"

#define ROLE_RECEIVED FEED_TREEWIDGET_SORTROLE

#define TOKEN_TYPE_GROUP      1
#define TOKEN_TYPE_MESSAGE    2
#define TOKEN_TYPE_PUBLISHKEY 3

/*****
 * #define NEWS_DEBUG  1
 ****/

static NewsFeed *instance = NULL;

/** Constructor */
NewsFeed::NewsFeed(QWidget *parent) :
    RsAutoUpdatePage(1000,parent),
    ui(new Ui::NewsFeed)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	mTokenQueueChannel = NULL;
	mTokenQueueCircle = NULL;
	mTokenQueueForum = NULL;
	mTokenQueuePosted = NULL;

	setUpdateWhenInvisible(true);

	if (!instance) {
		instance = this;
	}

	ui->feedWidget->enableRemove(true);

	ui->sortComboBox->addItem(tr("Newest on top"), Qt::DescendingOrder);
	ui->sortComboBox->addItem(tr("Oldest on top"), Qt::AscendingOrder);

	connect(ui->sortComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(sortChanged(int)));

	connect(ui->removeAllButton, SIGNAL(clicked()), ui->feedWidget, SLOT(clear()));
	connect(ui->feedWidget, SIGNAL(feedCountChanged()), this, SLOT(sendNewsFeedChanged()));

	connect(ui->feedOptionsButton, SIGNAL(clicked()), this, SLOT(feedoptions()));
    ui->feedOptionsButton->hide();	// (csoler) Hidden until we repare the system to display a specific settings page.

QString hlp_str = tr(
 " <h1><img width=\"32\" src=\":/icons/help_64.png\">&nbsp;&nbsp;News Feed</h1>                                                          \
   <p>The Log Feed displays the last events on your network, sorted by the time you received them.                \
   This gives you a summary of the activity of your friends.                                                       \
   You can configure which events to show by pressing on <b>Options</b>. </p>                                      \
   <p>The various events shown are:                                                                                \
   <ul>                                                                                                         \
   <li>Connection attempts (useful to make friends with new people and control who's trying to reach you)</li> \
   <li>Channel and Forum posts</li>                                                                            \
   <li>New Channels and Forums you can subscribe to</li>                                                       \
   <li>Private messages from your friends</li>                                                                 \
   </ul> </p>                                                                                                      \
 ") ;

	registerHelpButton(ui->helpButton,hlp_str,"NewFeed") ;

	// load settings
	processSettings(true);
}

NewsFeed::~NewsFeed()
{
	// save settings
	processSettings(false);

	if (instance == this) {
		instance = NULL;
	}

	if (mTokenQueueChannel) {
		delete(mTokenQueueChannel);
	}
	if (mTokenQueueCircle) {
		delete(mTokenQueueCircle);
	}
	if (mTokenQueueForum) {
		delete(mTokenQueueForum);
	}
	if (mTokenQueuePosted) {
		delete(mTokenQueuePosted);
	}
}

UserNotify *NewsFeed::getUserNotify(QObject *parent)
{
	return new NewsFeedUserNotify(this, parent);
}

void NewsFeed::processSettings(bool load)
{
	Settings->beginGroup("NewsFeed");

	if (load) {
		// load settings

		// state of sort order
		Qt::SortOrder sortOrder = (Qt::SortOrder) Settings->value("SortOrder", Qt::AscendingOrder).toInt();
		ui->sortComboBox->setCurrentIndex(ui->sortComboBox->findData(sortOrder));
		sortChanged(ui->sortComboBox->currentIndex());
	} else {
		// save settings

		// state of sort order
		Settings->setValue("SortOrder", ui->sortComboBox->itemData(ui->sortComboBox->currentIndex()).toInt());
	}

	Settings->endGroup();
}

void NewsFeed::sortChanged(int index)
{
	Qt::SortOrder sortOrder = (Qt::SortOrder) ui->sortComboBox->itemData(index).toInt();
	ui->feedWidget->setSortRole(ROLE_RECEIVED, sortOrder);
}

void NewsFeed::updateDisplay()
{
	if (!rsNotify)
		return;

	uint flags = Settings->getNewsFeedFlags();

	/* check for new messages */
	RsFeedItem fi;
	if (rsNotify->GetFeedItem(fi))
	{
		switch(fi.mType)
		{
			case RS_FEED_ITEM_PEER_CONNECT:
				if (flags & RS_FEED_TYPE_PEER)
					addFeedItemPeerConnect(fi);
				break;
			case RS_FEED_ITEM_PEER_DISCONNECT:
				if (flags & RS_FEED_TYPE_PEER)
					addFeedItemPeerDisconnect(fi);
				break;
			case RS_FEED_ITEM_PEER_HELLO:
				if (flags & RS_FEED_TYPE_PEER)
					addFeedItemPeerHello(fi);
				break;
			case RS_FEED_ITEM_PEER_NEW:
				if (flags & RS_FEED_TYPE_PEER)
					addFeedItemPeerNew(fi);
				break;
			case RS_FEED_ITEM_PEER_OFFSET:
				//if (flags & RS_FEED_TYPE_PEER) //Always allow this feed even if Friend notify is disabled.
					addFeedItemPeerOffset(fi);
				break;

			case RS_FEED_ITEM_SEC_CONNECT_ATTEMPT:
			case RS_FEED_ITEM_SEC_WRONG_SIGNATURE:
			case RS_FEED_ITEM_SEC_BAD_CERTIFICATE:
			case RS_FEED_ITEM_SEC_MISSING_CERTIFICATE:
			case RS_FEED_ITEM_SEC_INTERNAL_ERROR:
				if (Settings->getMessageFlags() & RS_MESSAGE_CONNECT_ATTEMPT) {
					MessageComposer::sendConnectAttemptMsg(RsPgpId(fi.mId1), RsPeerId(fi.mId2), QString::fromUtf8(fi.mId3.c_str()));
				}
				if (flags & RS_FEED_TYPE_SECURITY)
					addFeedItemSecurityConnectAttempt(fi);
				break;
			case RS_FEED_ITEM_SEC_AUTH_DENIED:
				if (flags & RS_FEED_TYPE_SECURITY)
					addFeedItemSecurityAuthDenied(fi);
				break;
			case RS_FEED_ITEM_SEC_UNKNOWN_IN:
				if (flags & RS_FEED_TYPE_SECURITY)
					addFeedItemSecurityUnknownIn(fi);
				break;
			case RS_FEED_ITEM_SEC_UNKNOWN_OUT:
				if (flags & RS_FEED_TYPE_SECURITY)
					addFeedItemSecurityUnknownOut(fi);
				break;

			case RS_FEED_ITEM_SEC_IP_BLACKLISTED:
				if (flags & RS_FEED_TYPE_SECURITY_IP)
					addFeedItemSecurityIpBlacklisted(fi, false);
				break;

			case RS_FEED_ITEM_SEC_IP_WRONG_EXTERNAL_IP_REPORTED:
				if (flags & RS_FEED_TYPE_SECURITY_IP)
					addFeedItemSecurityWrongExternalIpReported(fi, false);
				break;

			case RS_FEED_ITEM_CHANNEL_NEW:
				if (flags & RS_FEED_TYPE_CHANNEL)
					addFeedItemChannelNew(fi);
				break;
//			case RS_FEED_ITEM_CHANNEL_UPDATE:
//				if (flags & RS_FEED_TYPE_CHANNEL)
//					addFeedItemChannelUpdate(fi);
//				break;
			case RS_FEED_ITEM_CHANNEL_MSG:
				if (flags & RS_FEED_TYPE_CHANNEL)
					addFeedItemChannelMsg(fi);
				break;
			case RS_FEED_ITEM_CHANNEL_PUBLISHKEY:
				{
					if (!mTokenQueueChannel) {
						mTokenQueueChannel = new TokenQueue(rsGxsChannels->getTokenService(), instance);
					}

					addFeedItemChannelPublishKey(fi);

//					RsGxsGroupId grpId(fi.mId1);
//					if (!grpId.isNull()) {
//						RsTokReqOptions opts;
//						opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
//
//						std::list<RsGxsGroupId> grpIds;
//						grpIds.push_back(grpId);
//
//						uint32_t token;
//						mTokenQueueChannel->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, TOKEN_TYPE_PUBLISHKEY);
//					}
				}
				break;

			case RS_FEED_ITEM_FORUM_NEW:
				if (flags & RS_FEED_TYPE_FORUM)
					addFeedItemForumNew(fi);
				break;
//			case RS_FEED_ITEM_FORUM_UPDATE:
//				if (flags & RS_FEED_TYPE_FORUM)
//					addFeedItemForumUpdate(fi);
//				break;
			case RS_FEED_ITEM_FORUM_MSG:
				if (flags & RS_FEED_TYPE_FORUM)
					addFeedItemForumMsg(fi);
				break;
			case RS_FEED_ITEM_FORUM_PUBLISHKEY:
				{
					if (!mTokenQueueForum) {
						mTokenQueueForum = new TokenQueue(rsGxsForums->getTokenService(), instance);
					}

					RsGxsGroupId grpId(fi.mId1);
					if (!grpId.isNull()) {
						RsTokReqOptions opts;
						opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

						std::list<RsGxsGroupId> grpIds;
						grpIds.push_back(grpId);

						uint32_t token;
						mTokenQueueForum->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, TOKEN_TYPE_PUBLISHKEY);
					}
				}
//				if (flags & RS_FEED_TYPE_FORUM)
//					addFeedItemForumPublishKey(fi);
				break;

			case RS_FEED_ITEM_POSTED_NEW:
				if (flags & RS_FEED_TYPE_POSTED)
					addFeedItemPostedNew(fi);
				break;
//			case RS_FEED_ITEM_POSTED_UPDATE:
//				if (flags & RS_FEED_TYPE_POSTED)
//					addFeedItemPostedUpdate(fi);
//				break;
			case RS_FEED_ITEM_POSTED_MSG:
				if (flags & RS_FEED_TYPE_POSTED)
					addFeedItemPostedMsg(fi);
				break;

#if 0
			case RS_FEED_ITEM_BLOG_NEW:
				if (flags & RS_FEED_TYPE_BLOG)
					addFeedItemBlogNew(fi);
				break;
			case RS_FEED_ITEM_BLOG_MSG:
				if (flags & RS_FEED_TYPE_BLOG)
					addFeedItemBlogMsg(fi);
				break;
#endif

			case RS_FEED_ITEM_CHAT_NEW:
				if (flags & RS_FEED_TYPE_CHAT)
					addFeedItemChatNew(fi, false);
				break;

			case RS_FEED_ITEM_MESSAGE:
				if (flags & RS_FEED_TYPE_MSG)
					addFeedItemMessage(fi);
				break;

			case RS_FEED_ITEM_FILES_NEW:
				if (flags & RS_FEED_TYPE_FILES)
					addFeedItemFilesNew(fi);
				break;

			case RS_FEED_ITEM_CIRCLE_MEMB_REQ:
				if (flags & RS_FEED_TYPE_CIRCLE)
				{
					if (!mTokenQueueCircle) {
						mTokenQueueCircle = new TokenQueue(rsGxsCircles->getTokenService(), instance);
					}

					RsGxsGroupId grpId(fi.mId1);
					RsGxsMessageId msgId(fi.mId2);
					if (!grpId.isNull() && !msgId.isNull()) {
						RsTokReqOptions opts;
						opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

						GxsMsgReq msgIds;
						std::set<RsGxsMessageId> &vect_msgIds = msgIds[grpId];
						vect_msgIds.insert(msgId);

						uint32_t token;
						mTokenQueueCircle->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, TOKEN_TYPE_MESSAGE);
					}
				}
				//	addFeedItemCircleMembReq(fi);
				break;
			case RS_FEED_ITEM_CIRCLE_INVIT_REC:
				if (flags & RS_FEED_TYPE_CIRCLE)
				{
					if (!mTokenQueueCircle) {
						mTokenQueueCircle = new TokenQueue(rsGxsCircles->getTokenService(), instance);
					}

					RsGxsGroupId grpId(fi.mId1);
					if (!grpId.isNull()) {
						RsTokReqOptions opts;
						opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

						std::list<RsGxsGroupId> grpIds;
						grpIds.push_back(grpId);

						uint32_t token;
						mTokenQueueCircle->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, TOKEN_TYPE_GROUP);
					}
				}
				//	addFeedItemCircleInvitRec(fi);
				break;

			default:
				std::cerr << "(EE) Unknown type " << std::hex << fi.mType << std::dec << " in news feed." << std::endl;
				break;
		}
	} else {
		/* process plugin feeds */
		int pluginCount = rsPlugins->nbPlugins();
		for (int i = 0; i < pluginCount; ++i) {
			RsPlugin *rsPlugin = rsPlugins->plugin(i);
			if (rsPlugin) {
				FeedNotify *feedNotify = rsPlugin->qt_feedNotify();
				if (feedNotify && feedNotify->notifyEnabled()) {
					FeedItem *item = feedNotify->feedItem(this);
					if (item) {
						addFeedItem(item);
						break;
					}
				}
			}
		}
	}
}

void NewsFeed::testFeeds(uint notifyFlags)
{
	if (!instance) {
		return;
	}

	instance->ui->feedWidget->enableCountChangedSignal(false);

	uint pos = 0;

	while (notifyFlags) {
		uint type = notifyFlags & (1 << pos);
		notifyFlags &= ~(1 << pos);
		++pos;

		RsFeedItem fi;

		switch(type) {
		case RS_FEED_TYPE_PEER:
			fi.mId1 = rsPeers->getOwnId().toStdString();

			instance->addFeedItemPeerConnect(fi);
			instance->addFeedItemPeerDisconnect(fi);
			instance->addFeedItemPeerHello(fi);
			instance->addFeedItemPeerNew(fi);
			instance->addFeedItemPeerOffset(fi);
			break;

		case RS_FEED_TYPE_SECURITY:
			fi.mId1 = rsPeers->getGPGOwnId().toStdString();
			fi.mId2 = rsPeers->getOwnId().toStdString();

			instance->addFeedItemSecurityConnectAttempt(fi);
			instance->addFeedItemSecurityAuthDenied(fi);
			instance->addFeedItemSecurityUnknownIn(fi);
			instance->addFeedItemSecurityUnknownOut(fi);

			break;

		case RS_FEED_TYPE_SECURITY_IP:
			fi.mId1 = rsPeers->getOwnId().toStdString();
			fi.mId2 = "0.0.0.0";
			fi.mResult1 = RSBANLIST_CHECK_RESULT_BLACKLISTED;
			instance->addFeedItemSecurityIpBlacklisted(fi, true);

			//fi.mId1 = rsPeers->getOwnId().toStdString();
			fi.mId2 = "0.0.0.1";
			fi.mId3 = "0.0.0.2";
			fi.mResult1 = 0;
			instance->addFeedItemSecurityWrongExternalIpReported(fi, true);

			break;

		case RS_FEED_TYPE_CHANNEL:
		{
			if (!instance->mTokenQueueChannel) {
				instance->mTokenQueueChannel = new TokenQueue(rsGxsChannels->getTokenService(), instance);
			}

			RsTokReqOptions opts;
			opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
			uint32_t token;
			instance->mTokenQueueChannel->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, TOKEN_TYPE_GROUP);

			break;
		}

		case RS_FEED_TYPE_FORUM:
		{
			if (!instance->mTokenQueueForum) {
				instance->mTokenQueueForum = new TokenQueue(rsGxsForums->getTokenService(), instance);
			}

			RsTokReqOptions opts;
			opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
			uint32_t token;
			instance->mTokenQueueForum->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, TOKEN_TYPE_GROUP);

			break;
		}

		case RS_FEED_TYPE_POSTED:
		{
			if (!instance->mTokenQueuePosted) {
				instance->mTokenQueuePosted = new TokenQueue(rsPosted->getTokenService(), instance);
			}

			RsTokReqOptions opts;
			opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
			uint32_t token;
			instance->mTokenQueuePosted->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, TOKEN_TYPE_GROUP);

			break;
		}

#if 0
		case RS_FEED_TYPE_BLOG:
// not used
//			instance->addFeedItemBlogNew(fi);
//			instance->addFeedItemBlogMsg(fi);
			break;
#endif

		case RS_FEED_TYPE_CHAT:
			fi.mId1 = rsPeers->getOwnId().toStdString();
			fi.mId2 = tr("This is a test.").toUtf8().constData();

			instance->addFeedItemChatNew(fi, true);
			break;

		case RS_FEED_TYPE_MSG:
		{
			std::list<MsgInfoSummary> msgList;
			rsMail->getMessageSummaries(msgList);

			std::list<MsgInfoSummary>::const_iterator msgIt;
			for (msgIt = msgList.begin(); msgIt != msgList.end(); ++msgIt) {
				if (fi.mId1.empty()) {
					/* store first message */
					fi.mId1 = msgIt->msgId;
				}

				if (msgIt->msgflags & RS_MSG_TRASH) {
					continue;
				}

				if ((msgIt->msgflags & RS_MSG_BOXMASK) == RS_MSG_INBOX) {
					/* take message from inbox */
					fi.mId1 = msgIt->msgId;
					break;
				}
			}

			instance->addFeedItemMessage(fi);
			break;
		}

		case RS_FEED_TYPE_FILES:
// not used
//			instance->addFeedItemFilesNew(fi);
			break;

		case RS_FEED_TYPE_CIRCLE:
		{
			if (!instance->mTokenQueueCircle) {
				instance->mTokenQueueCircle = new TokenQueue(rsGxsCircles->getTokenService(), instance);
			}

			RsTokReqOptions opts;
			opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
			uint32_t token;
			instance->mTokenQueueCircle->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, TOKEN_TYPE_GROUP);

			break;
		}
//			instance->addFeedItemCircleMembReq(fi);
//			instance->addFeedItemCircleInvitRec(fi);
			break;

		}
	}

	instance->ui->feedWidget->enableCountChangedSignal(true);

	instance->sendNewsFeedChanged();
}

void NewsFeed::loadCircleGroup(const uint32_t &token)
{
	std::vector<RsGxsCircleGroup> groups;
	if (!rsGxsCircles->getGroupData(token, groups)) {
		std::cerr << "NewsFeed::loadCircleGroup() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	std::list<RsGxsId> own_identities;
	rsIdentity->getOwnIds(own_identities);

	std::vector<RsGxsCircleGroup>::const_iterator circleIt;
	for (circleIt = groups.begin(); circleIt != groups.end(); ++circleIt) {
		RsGxsCircleGroup group = *(circleIt);
		RsGxsCircleDetails details;
		if(rsGxsCircles->getCircleDetails(group.mMeta.mCircleId,details))
		{
			for(std::list<RsGxsId>::const_iterator it(own_identities.begin());it!=own_identities.end();++it) {
				std::map<RsGxsId,uint32_t>::const_iterator vit = details.mSubscriptionFlags.find(*it);
				uint32_t subscribe_flags = (vit == details.mSubscriptionFlags.end())?0:(vit->second);

				if( !(subscribe_flags & GXS_EXTERNAL_CIRCLE_FLAGS_SUBSCRIBED)
				    && (subscribe_flags & GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST) ) {

					RsFeedItem fi;
					fi.mId1 = group.mMeta.mGroupId.toStdString();
					fi.mId2 = it->toStdString();

					instance->addFeedItemCircleInvitRec(fi);

				}
			}
		}
	}
}

void NewsFeed::loadCircleMessage(const uint32_t &token)
{
	std::vector<RsGxsCircleMsg> msgs;
	if (!rsGxsCircles->getMsgData(token, msgs)) {
		std::cerr << "NewsFeed::loadCircleMessage() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	std::list<RsGxsId> own_identities;
	rsIdentity->getOwnIds(own_identities);

	std::vector<RsGxsCircleMsg>::iterator msgIt;
	for (msgIt = msgs.begin(); msgIt != msgs.end(); ++msgIt) {
		RsGxsCircleMsg msg = *(msgIt);
		RsGxsCircleDetails details;
		if(rsGxsCircles->getCircleDetails(RsGxsCircleId(msg.mMeta.mGroupId),details)) {
			//for(std::list<RsGxsId>::const_iterator it(own_identities.begin());it!=own_identities.end();++it) {
			//	std::map<RsGxsId,uint32_t>::const_iterator vit = details.mSubscriptionFlags.find(*it);
			//	if (vit != details.mSubscriptionFlags.end()) {
					RsFeedItem fi;
					fi.mId1 = msgIt->mMeta.mGroupId.toStdString();
					fi.mId2 = msgIt->mMeta.mAuthorId.toStdString();

					if (msgIt->stuff == "SUBSCRIPTION_REQUEST_UNSUBSCRIBE")
						instance->remFeedItemCircleMembReq(fi);
					else
						instance->addFeedItemCircleMembReq(fi);

				//}
			//}
		}
	}
}

void NewsFeed::loadChannelGroup(const uint32_t &token)
{
	std::vector<RsGxsChannelGroup> groups;
	if (!rsGxsChannels->getGroupData(token, groups)) {
		std::cerr << "NewsFeed::loadChannelGroup() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	RsFeedItem fi;
	std::vector<RsGxsChannelGroup>::iterator channelIt;
	for (channelIt = groups.begin(); channelIt != groups.end(); ++channelIt) {
		if (fi.mId1.empty()) {
			/* store first channel */
			fi.mId1 = channelIt->mMeta.mGroupId.toStdString();
		}

		if (!channelIt->mDescription.empty()) {
			/* take channel with description */
			fi.mId1 = channelIt->mMeta.mGroupId.toStdString();
			break;
		}
	}

	if (fi.mId1.empty()) {
		return;
	}

	instance->addFeedItemChannelNew(fi);
//	instance->addFeedItemChanUpdate(fi);

	/* Prepare group ids for message request */
	std::list<RsGxsGroupId> grpIds;
	for (channelIt = groups.begin(); channelIt != groups.end(); ++channelIt) {
		grpIds.push_back(channelIt->mMeta.mGroupId);
	}
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_THREAD;
	uint32_t msgToken;
	instance->mTokenQueueChannel->requestMsgInfo(msgToken, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, TOKEN_TYPE_MESSAGE);
}

void NewsFeed::loadChannelPost(const uint32_t &token)
{
	std::vector<RsGxsChannelPost> posts;
	if (!rsGxsChannels->getPostData(token, posts)) {
		std::cerr << "NewsFeed::loadChannelPost() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	RsFeedItem fi;
	std::vector<RsGxsChannelPost>::iterator postIt;
	for (postIt = posts.begin(); postIt != posts.end(); ++postIt) {
		if (fi.mId2.empty()) {
			/* store first channel message */
			fi.mId1 = postIt->mMeta.mGroupId.toStdString();
			fi.mId2 = postIt->mMeta.mMsgId.toStdString();
		}

		if (!postIt->mMsg.empty()) {
			/* take channel message with description */
			fi.mId1 = postIt->mMeta.mGroupId.toStdString();
			fi.mId2 = postIt->mMeta.mMsgId.toStdString();
			break;
		}
	}

	if (!fi.mId1.empty()) {
		instance->addFeedItemChannelMsg(fi);
	}
}

void NewsFeed::loadChannelPublishKey(const uint32_t &token)
{
	std::vector<RsGxsChannelGroup> groups;
	if (!rsGxsChannels->getGroupData(token, groups)) {
		std::cerr << "NewsFeed::loadChannelPublishKey() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	if (groups.size() != 1)
	{
		std::cerr << "NewsFeed::loadChannelPublishKey() Wrong number of Items";
		std::cerr << std::endl;
		return;
	}
#ifdef UNUSED_CODE
	MessageComposer::sendChannelPublishKey(groups[0]);
#endif

	RsGxsChannelGroup& grp = *groups.begin();

	RsFeedItem fi;
	fi.mId1 = grp.mMeta.mGroupId.toStdString();


	addFeedItemChannelPublishKey(fi);
}

void NewsFeed::loadForumGroup(const uint32_t &token)
{
	std::vector<RsGxsForumGroup> forums;
	if (!rsGxsForums->getGroupData(token, forums)) {
		std::cerr << "NewsFeed::loadForumGroup() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	RsFeedItem fi;
	std::vector<RsGxsForumGroup>::iterator forumIt;
	for (forumIt = forums.begin(); forumIt != forums.end(); ++forumIt) {
		if (fi.mId1.empty()) {
			/* store first forum */
			fi.mId1 = forumIt->mMeta.mGroupId.toStdString();
		}

		if (!forumIt->mDescription.empty()) {
			/* take forum with description */
			fi.mId1 = forumIt->mMeta.mGroupId.toStdString();
			break;
		}
	}

	if (fi.mId1.empty()) {
		return;
	}

	instance->addFeedItemForumNew(fi);
//	instance->addFeedItemForumUpdate(fi);

	/* Prepare group ids for message request */
	std::list<RsGxsGroupId> grpIds;
	for (forumIt = forums.begin(); forumIt != forums.end(); ++forumIt) {
		grpIds.push_back(forumIt->mMeta.mGroupId);
	}
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_THREAD;
	uint32_t msgToken;
	instance->mTokenQueueForum->requestMsgInfo(msgToken, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, TOKEN_TYPE_MESSAGE);
}

void NewsFeed::loadForumMessage(const uint32_t &token)
{
	std::vector<RsGxsForumMsg> msgs;
	if (!rsGxsForums->getMsgData(token, msgs)) {
		std::cerr << "NewsFeed::loadForumPost() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	RsFeedItem fi;
	std::vector<RsGxsForumMsg>::iterator msgIt;
	for (msgIt = msgs.begin(); msgIt != msgs.end(); ++msgIt) {
		if (fi.mId2.empty()) {
			/* store first forum message */
			fi.mId1 = msgIt->mMeta.mGroupId.toStdString();
			fi.mId2 = msgIt->mMeta.mMsgId.toStdString();
		}

		if (!msgIt->mMsg.empty()) {
			/* take forum message with description */
			fi.mId1 = msgIt->mMeta.mGroupId.toStdString();
			fi.mId2 = msgIt->mMeta.mMsgId.toStdString();
			break;
		}
	}

	if (!fi.mId1.empty()) {
		instance->addFeedItemForumMsg(fi);
	}
}

void NewsFeed::loadForumPublishKey(const uint32_t &token)
{
	std::vector<RsGxsForumGroup> groups;
	if (!rsGxsForums->getGroupData(token, groups)) {
		std::cerr << "NewsFeed::loadForumPublishKey() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	if (groups.size() != 1)
	{
		std::cerr << "NewsFeed::loadForumPublishKey() Wrong number of Items";
		std::cerr << std::endl;
		return;
	}
#ifdef UNUSED_CODE
	MessageComposer::sendForumPublishKey(groups[0]);
#endif

	std::cerr << "(EE) Unimplemented code: received an order to load/display item for received forum publish key, but the implementation is missing." << std::endl;
}

void NewsFeed::loadPostedGroup(const uint32_t &token)
{
	std::vector<RsPostedGroup> posted;
	if (!rsPosted->getGroupData(token, posted)) {
		std::cerr << "NewsFeed::loadPostedGroup() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	RsFeedItem fi;
	std::vector<RsPostedGroup>::iterator postedIt;
	for (postedIt = posted.begin(); postedIt != posted.end(); ++postedIt) {
		if (fi.mId1.empty()) {
			/* store first posted */
			fi.mId1 = postedIt->mMeta.mGroupId.toStdString();
		}

		if (!postedIt->mDescription.empty()) {
			/* take posted with description */
			fi.mId1 = postedIt->mMeta.mGroupId.toStdString();
			break;
		}
	}

	if (fi.mId1.empty()) {
		return;
	}

	instance->addFeedItemPostedNew(fi);
//	instance->addFeedItemPostedUpdate(fi);

	/* Prepare group ids for message request */
	std::list<RsGxsGroupId> grpIds;
	for (postedIt = posted.begin(); postedIt != posted.end(); ++postedIt) {
		grpIds.push_back(postedIt->mMeta.mGroupId);
	}
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_THREAD;
	uint32_t msgToken;
	instance->mTokenQueuePosted->requestMsgInfo(msgToken, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, TOKEN_TYPE_MESSAGE);
}

void NewsFeed::loadPostedMessage(const uint32_t &token)
{
	std::vector<RsPostedPost> msgs;
	if (!rsPosted->getPostData(token, msgs)) {
		std::cerr << "NewsFeed::loadPostedPost() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	RsFeedItem fi;
	std::vector<RsPostedPost>::iterator msgIt;
	for (msgIt = msgs.begin(); msgIt != msgs.end(); ++msgIt) {
		if (fi.mId2.empty()) {
			/* store first posted message */
			fi.mId1 = msgIt->mMeta.mGroupId.toStdString();
			fi.mId2 = msgIt->mMeta.mMsgId.toStdString();
		}

		if (!msgIt->mLink.empty()) {
			/* take posted message with description */
			fi.mId1 = msgIt->mMeta.mGroupId.toStdString();
			fi.mId2 = msgIt->mMeta.mMsgId.toStdString();
			break;
		}
	}

	if (!fi.mId1.empty()) {
		instance->addFeedItemPostedMsg(fi);
	}
}

void NewsFeed::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	if (queue == mTokenQueueChannel) {
		switch (req.mUserType) {
		case TOKEN_TYPE_GROUP:
			loadChannelGroup(req.mToken);
			break;

		case TOKEN_TYPE_MESSAGE:
			loadChannelPost(req.mToken);
			break;

		case TOKEN_TYPE_PUBLISHKEY:
			loadChannelPublishKey(req.mToken);
			break;

		default:
			std::cerr << "NewsFeed::loadRequest() ERROR: INVALID CHANNEL TYPE";
			std::cerr << std::endl;
			break;
		}
	}

	if (queue == mTokenQueueCircle) {
		switch (req.mUserType) {
		case TOKEN_TYPE_GROUP:
			loadCircleGroup(req.mToken);
			break;

		case TOKEN_TYPE_MESSAGE:
			loadCircleMessage(req.mToken);
			break;

		default:
			std::cerr << "NewsFeed::loadRequest() ERROR: INVALID CIRCLE TYPE";
			std::cerr << std::endl;
			break;
		}
	}

	if (queue == mTokenQueueForum) {
		switch (req.mUserType) {
		case TOKEN_TYPE_GROUP:
			loadForumGroup(req.mToken);
			break;

		case TOKEN_TYPE_MESSAGE:
			loadForumMessage(req.mToken);
			break;

		case TOKEN_TYPE_PUBLISHKEY:
			loadForumPublishKey(req.mToken);
			break;

		default:
			std::cerr << "NewsFeed::loadRequest() ERROR: INVALID FORUM TYPE";
			std::cerr << std::endl;
			break;
		}
	}

	if (queue == mTokenQueuePosted) {
		switch (req.mUserType) {
		case TOKEN_TYPE_GROUP:
			loadPostedGroup(req.mToken);
			break;

		case TOKEN_TYPE_MESSAGE:
			loadPostedMessage(req.mToken);
			break;

		default:
			std::cerr << "NewsFeed::loadRequest() ERROR: INVALID POSTED TYPE";
			std::cerr << std::endl;
			break;
		}
	}
}

void NewsFeed::testFeed(FeedNotify *feedNotify)
{
	if (!instance) {
		return;
	}

	if (!feedNotify) {
		return;
	}

	FeedItem *feedItem = feedNotify->testFeedItem(instance);
	if (!feedItem) {
		return;
	}

	instance->addFeedItem(feedItem);
}

void NewsFeed::addFeedItem(FeedItem *item)
{
	static const int MAX_FEEDITEM_COUNT = 500 ;

	item->setAttribute(Qt::WA_DeleteOnClose, true);

	// costly, but not really a problem here
	int feedItemCount;
	bool fromTop = 	(ui->sortComboBox->itemData(ui->sortComboBox->currentIndex()).toInt() == Qt::AscendingOrder);

	while ((feedItemCount = ui->feedWidget->feedItemCount()) >= MAX_FEEDITEM_COUNT) {
		FeedItem *feedItem = ui->feedWidget->feedItem(fromTop ? 0 : feedItemCount - 1);
		if (!feedItem) {
			break;
		}

		ui->feedWidget->removeFeedItem(feedItem);
	}

	ui->feedWidget->addFeedItem(item, ROLE_RECEIVED, QDateTime::currentDateTime());
}

struct AddFeedItemIfUniqueData
{
	AddFeedItemIfUniqueData(FeedItem *feedItem, int type
	                        , const std::string& id1, const std::string& id2
	                        , const std::string& id3, const std::string& id4)
	  : mType(type), mId1(id1), mId2(id2), mId3(id3), mId4(id4)
	{
		mGxsCircleItem = dynamic_cast<GxsCircleItem*>(feedItem);
		mPeerItem = dynamic_cast<PeerItem*>(feedItem);
		mSecItem = dynamic_cast<SecurityItem*>(feedItem);
		mSecurityIpItem = dynamic_cast<SecurityIpItem*>(feedItem);
	}

	int mType;
	const std::string& mId1;
	const std::string& mId2;
	const std::string& mId3;
	const std::string& mId4;

	GxsCircleItem *mGxsCircleItem;
	PeerItem *mPeerItem;
	SecurityItem *mSecItem;
	SecurityIpItem *mSecurityIpItem;
};

static bool addFeedItemIfUniqueCallback(FeedItem *feedItem, void *data)
{
	AddFeedItemIfUniqueData *findData = (AddFeedItemIfUniqueData*) data;
	if (!findData || findData->mId1.empty()) {
		return false;
	}

	if (findData->mGxsCircleItem) {
		GxsCircleItem *gxsCircleItem = dynamic_cast<GxsCircleItem*>(feedItem);
		if (gxsCircleItem && gxsCircleItem->isSame(RsGxsCircleId(findData->mId1), RsGxsId(findData->mId2), findData->mType)) {
			return true;
		}
		return false;
	}

	if (findData->mPeerItem) {
		PeerItem *peerItem = dynamic_cast<PeerItem*>(feedItem);
		if (peerItem && peerItem->isSame(RsPeerId(findData->mId1), findData->mType)) {
			return true;
		}
		return false;
	}

	if (findData->mSecItem) {
		SecurityItem *secitem = dynamic_cast<SecurityItem*>(feedItem);
		if (secitem && secitem->isSame(RsPeerId(findData->mId1), findData->mType)) {
			return true;
		}
		return false;
	}

	if (findData->mSecurityIpItem) {
		SecurityIpItem *securityIpItem = dynamic_cast<SecurityIpItem*>(feedItem);
		if (securityIpItem && securityIpItem->isSame(RsPeerId(findData->mId1), findData->mId2, findData->mId3, findData->mType)) {
			return true;
		}
		return false;
	}

	return false;
}

void NewsFeed::addFeedItemIfUnique(FeedItem *item, int itemType, const std::string& id1, const std::string& id2, const std::string& id3, const std::string& id4, bool replace)
{
	AddFeedItemIfUniqueData data(item, itemType, id1, id2, id3, id4);
	FeedItem *feedItem = ui->feedWidget->findFeedItem(addFeedItemIfUniqueCallback, &data);

	if (feedItem) {
		if (!replace) {
			delete item;
			return;
		}

		ui->feedWidget->removeFeedItem(feedItem);
	}

	addFeedItem(item);
}

void NewsFeed::remUniqueFeedItem(FeedItem *item, int itemType, const std::string &id1, const std::string &id2, const std::string &id3, const std::string &id4)
{
	AddFeedItemIfUniqueData data(item, itemType, id1, id2, id3, id4);
	FeedItem *feedItem = ui->feedWidget->findFeedItem(addFeedItemIfUniqueCallback, &data);

	if (feedItem) {
		delete item;

		ui->feedWidget->removeFeedItem(feedItem);
	}
}

void NewsFeed::addFeedItemPeerConnect(const RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, RsPeerId(fi.mId1), PEER_TYPE_CONNECT, false);

	/* add to layout */
	addFeedItem(pi);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemPeerConnect()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemPeerDisconnect(const RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, RsPeerId(fi.mId1), PEER_TYPE_STD, false);

	/* add to layout */
	addFeedItem(pi);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemPeerDisconnect()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemPeerHello(const RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, RsPeerId(fi.mId1), PEER_TYPE_HELLO, false);

	/* add to layout */
	addFeedItem(pi);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemPeerHello()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemPeerNew(const RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, RsPeerId(fi.mId1), PEER_TYPE_NEW_FOF, false);

	/* add to layout */
	addFeedItem(pi);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemPeerNew()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemPeerOffset(const RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, RsPeerId(fi.mId1), PEER_TYPE_OFFSET, false);

	/* add to layout */
	addFeedItemIfUnique(pi, PEER_TYPE_OFFSET, fi.mId1, fi.mId2, fi.mId3, fi.mId4, false);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemPeerOffset()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemSecurityConnectAttempt(const RsFeedItem &fi)
{
	/* make new widget */
	SecurityItem *pi = new SecurityItem(this, NEWSFEED_SECLIST, RsPgpId(fi.mId1), RsPeerId(fi.mId2), fi.mId3, fi.mId4, fi.mType, false);

	/* add to layout */
	addFeedItemIfUnique(pi, fi.mType, fi.mId2, "", "", "", false);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemSecurityConnectAttempt()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemSecurityAuthDenied(const RsFeedItem &fi)
{
	/* make new widget */
	SecurityItem *pi = new SecurityItem(this, NEWSFEED_SECLIST, RsPgpId(fi.mId1), RsPeerId(fi.mId2), fi.mId3, fi.mId4, fi.mType, false);

	/* add to layout */
	addFeedItemIfUnique(pi, RS_FEED_ITEM_SEC_AUTH_DENIED, fi.mId2, "", "", "", false);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemSecurityAuthDenied()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemSecurityUnknownIn(const RsFeedItem &fi)
{
	/* make new widget */
	SecurityItem *pi = new SecurityItem(this, NEWSFEED_SECLIST, RsPgpId(fi.mId1), RsPeerId(fi.mId2), fi.mId3, fi.mId4, RS_FEED_ITEM_SEC_UNKNOWN_IN, false);

	/* add to layout */
	addFeedItemIfUnique(pi, RS_FEED_ITEM_SEC_UNKNOWN_IN, fi.mId2, "", "", "", false);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemSecurityUnknownIn()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemSecurityUnknownOut(const RsFeedItem &fi)
{
	/* make new widget */
	SecurityItem *pi = new SecurityItem(this, NEWSFEED_SECLIST, RsPgpId(fi.mId1), RsPeerId(fi.mId2), fi.mId3, fi.mId4, RS_FEED_ITEM_SEC_UNKNOWN_OUT, false);

	/* add to layout */
	addFeedItemIfUnique(pi, RS_FEED_ITEM_SEC_UNKNOWN_OUT, fi.mId2, "", "", "", false);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemSecurityUnknownOut()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemSecurityIpBlacklisted(const RsFeedItem &fi, bool isTest)
{
	/* make new widget */
	SecurityIpItem *pi = new SecurityIpItem(this, RsPeerId(fi.mId1), fi.mId2, fi.mResult1, RS_FEED_ITEM_SEC_IP_BLACKLISTED, isTest);

	/* add to layout */
	addFeedItemIfUnique(pi, RS_FEED_ITEM_SEC_IP_BLACKLISTED, fi.mId1, fi.mId2, fi.mId3, fi.mId4, false);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemSecurityIpBlacklisted()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemSecurityWrongExternalIpReported(const RsFeedItem &fi, bool isTest)
{
	/* make new widget */
	SecurityIpItem *pi = new SecurityIpItem(this, RsPeerId(fi.mId1), fi.mId2, fi.mId3, RS_FEED_ITEM_SEC_IP_WRONG_EXTERNAL_IP_REPORTED, isTest);

	/* add to layout */
	addFeedItemIfUnique(pi, RS_FEED_ITEM_SEC_IP_WRONG_EXTERNAL_IP_REPORTED, fi.mId1, fi.mId2, fi.mId3, fi.mId4, false);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemSecurityWrongExternalIpReported()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemChannelNew(const RsFeedItem &fi)
{
	RsGxsGroupId grpId(fi.mId1);

	if (grpId.isNull()) {
		return;
	}

	/* make new widget */
	GxsChannelGroupItem *item = new GxsChannelGroupItem(this, NEWSFEED_CHANNELNEWLIST, grpId, false, true);

	/* add to layout */
	addFeedItem(item);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemChanNew()";
	std::cerr << std::endl;
#endif
}

//void NewsFeed::addFeedItemChannelUpdate(const RsFeedItem &fi)
//{
//	/* make new widget */
//	ChanNewItem *cni = new ChanNewItem(this, NEWSFEED_CHANNEWLIST, fi.mId1, false, false);

//	/* add to layout */
//	addFeedItem(cni);

//#ifdef NEWS_DEBUG
//	std::cerr << "NewsFeed::addFeedItemChanUpdate()";
//	std::cerr << std::endl;
//#endif
//}

void NewsFeed::addFeedItemChannelMsg(const RsFeedItem &fi)
{
	RsGxsGroupId grpId(fi.mId1);
	RsGxsMessageId msgId(fi.mId2);

	if (grpId.isNull() || msgId.isNull()) {
		return;
	}

	/* make new widget */
	GxsChannelPostItem *item = new GxsChannelPostItem(this, NEWSFEED_CHANNELNEWLIST, grpId, msgId, false, true);

	/* add to layout */
	addFeedItem(item);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemChanMsg()";
	std::cerr << std::endl;
#endif
}
void NewsFeed::addFeedItemChannelPublishKey(const RsFeedItem &fi)
{
	RsGxsGroupId grpId(fi.mId1);

	if (grpId.isNull())
		return;

	/* make new widget */
	GxsChannelGroupItem *item = new GxsChannelGroupItem(this, NEWSFEED_CHANNELPUBKEYLIST, grpId, false, true);

	/* add to layout */
	addFeedItem(item);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemChanMsg()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemForumNew(const RsFeedItem &fi)
{
	RsGxsGroupId grpId(fi.mId1);

	if (grpId.isNull()) {
		return;
	}

	/* make new widget */
	GxsForumGroupItem *item = new GxsForumGroupItem(this, NEWSFEED_FORUMNEWLIST, grpId, false, true);

	/* add to layout */
	addFeedItem(item);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemForumNew()";
	std::cerr << std::endl;
#endif
}

//void NewsFeed::addFeedItemForumUpdate(const RsFeedItem &fi)
//{
//	/* make new widget */
//	ForumNewItem *fni = new ForumNewItem(this, NEWSFEED_FORUMNEWLIST, fi.mId1, false, false);

//	/* add to layout */
//	addFeedItem(fni);

//#ifdef NEWS_DEBUG
//	std::cerr << "NewsFeed::addFeedItemForumUpdate()";
//	std::cerr << std::endl;
//#endif
//}

void NewsFeed::addFeedItemForumMsg(const RsFeedItem &fi)
{
	RsGxsGroupId grpId(fi.mId1);
	RsGxsMessageId msgId(fi.mId2);

	if (grpId.isNull() || msgId.isNull()) {
		return;
	}

	/* make new widget */
	GxsForumMsgItem *item = new GxsForumMsgItem(this, NEWSFEED_FORUMMSGLIST, grpId, msgId, false, true);

	/* add to layout */
	addFeedItem(item);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemForumMsg()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemPostedNew(const RsFeedItem &fi)
{
	RsGxsGroupId grpId(fi.mId1);

	if (grpId.isNull()) {
		return;
	}

	/* make new widget */
	PostedGroupItem *item = new PostedGroupItem(this, NEWSFEED_POSTEDNEWLIST, grpId, false, true);

	/* add to layout */
	addFeedItem(item);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemPostedNew()";
	std::cerr << std::endl;
#endif
}

//void NewsFeed::addFeedItemPostedUpdate(const RsFeedItem &fi)
//{
//	/* make new widget */
//	GxsPostedGroupItem *item = new GxsPostedGroupItem(this, NEWSFEED_POSTEDNEWLIST, grpId, false, true);

//	/* add to layout */
//	addFeedItem(item);

//#ifdef NEWS_DEBUG
//	std::cerr << "NewsFeed::addFeedItemPostedUpdate()";
//	std::cerr << std::endl;
//#endif
//}

void NewsFeed::addFeedItemPostedMsg(const RsFeedItem &fi)
{
	RsGxsGroupId grpId(fi.mId1);
	RsGxsMessageId msgId(fi.mId2);

	if (grpId.isNull() || msgId.isNull()) {
		return;
	}

	/* make new widget */
	PostedItem *item = new PostedItem(this, NEWSFEED_POSTEDMSGLIST, grpId, msgId, false, true);

	/* add to layout */
	addFeedItem(item);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemPostedMsg()";
	std::cerr << std::endl;
#endif
}

#if 0
void NewsFeed::addFeedItemBlogNew(const RsFeedItem &fi)
{
	Q_UNUSED(fi);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemBlogNew()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemBlogMsg(const RsFeedItem &fi)
{
	Q_UNUSED(fi);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemBlogMsg()";
	std::cerr << std::endl;
#endif
}

#endif

void NewsFeed::addFeedItemChatNew(const RsFeedItem &fi, bool addWithoutCheck)
{
#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemChatNew()";
	std::cerr << std::endl;
#endif

	if (!addWithoutCheck && fi.mId1 == rsPeers->getOwnId().toStdString()) {
		/* chat message from myself */
		return;
	}

	/* make new widget */
	ChatMsgItem *cm = new ChatMsgItem(this, NEWSFEED_CHATMSGLIST, RsPeerId(fi.mId1), fi.mId2);

	/* add to layout */
	addFeedItem(cm);
}

void NewsFeed::addFeedItemMessage(const RsFeedItem &fi)
{
	/* make new widget */
	MsgItem *mi = new MsgItem(this, NEWSFEED_MESSAGELIST, fi.mId1, false);

	/* add to layout */
	addFeedItem(mi);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemMessage()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemFilesNew(const RsFeedItem &/*fi*/)
{
#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemFilesNew()";
	std::cerr << std::endl;
#endif
}

void NewsFeed::addFeedItemCircleMembReq(const RsFeedItem &fi)
{
	RsGxsCircleId circleId(fi.mId1);
	RsGxsId gxsId(fi.mId2);

	if (circleId.isNull() || gxsId.isNull()) {
		return;
	}

	/* make new widget */
	GxsCircleItem *item = new GxsCircleItem(this, NEWSFEED_CIRCLELIST, circleId, gxsId, RS_FEED_ITEM_CIRCLE_MEMB_REQ);

	/* add to layout */
	addFeedItemIfUnique(item, RS_FEED_ITEM_CIRCLE_MEMB_REQ, fi.mId1, fi.mId2, fi.mId3, fi.mId4, false);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemCircleMembReq()" << std::endl;
#endif
}

void NewsFeed::remFeedItemCircleMembReq(const RsFeedItem &fi)
{
	RsGxsCircleId circleId(fi.mId1);
	RsGxsId gxsId(fi.mId2);

	if (circleId.isNull() || gxsId.isNull()) {
		return;
	}

	/* make new widget */
	GxsCircleItem *item = new GxsCircleItem(this, NEWSFEED_CIRCLELIST, circleId, gxsId, RS_FEED_ITEM_CIRCLE_MEMB_REQ);

	/* add to layout */
	remUniqueFeedItem(item, RS_FEED_ITEM_CIRCLE_MEMB_REQ, fi.mId1, fi.mId2, fi.mId3, fi.mId4);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::remFeedItemCircleMembReq()" << std::endl;
#endif
}

void NewsFeed::addFeedItemCircleInvitRec(const RsFeedItem &fi)
{
	RsGxsCircleId circleId(fi.mId1);
	RsGxsId gxsId(fi.mId2);

	if (circleId.isNull()) {
		return;
	}

	/* make new widget */
	GxsCircleItem *item = new GxsCircleItem(this, NEWSFEED_CIRCLELIST, circleId, gxsId, RS_FEED_ITEM_CIRCLE_INVIT_REC);

	/* add to layout */
	addFeedItem(item);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemCircleInvitRec()" << std::endl;
#endif
}

/* FeedHolder Functions (for FeedItem functionality) */
QScrollArea *NewsFeed::getScrollArea()
{
	return NULL;
}

void NewsFeed::deleteFeedItem(QWidget *item, uint32_t /*type*/)
{
#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::deleteFeedItem()";
	std::cerr << std::endl;
#endif

	if (item) {
		item->close ();
	}
}

void NewsFeed::openChat(const RsPeerId &peerId)
{
#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::openChat()";
	std::cerr << std::endl;
#endif

    ChatDialog::chatFriend(ChatId(peerId));
}

void NewsFeed::openComments(uint32_t /*type*/, const RsGxsGroupId &/*groupId*/, const QVector<RsGxsMessageId> &/*versions*/,const RsGxsMessageId &/*msgId*/, const QString &/*title*/)
{
	std::cerr << "NewsFeed::openComments() Not Handled Yet";
	std::cerr << std::endl;
}

static void sendNewsFeedChangedCallback(FeedItem *feedItem, void *data)
{
	if (dynamic_cast<PeerItem*>(feedItem) == NULL) {
		/* don't count PeerItem's */
		++(*((int*) data));
	}
}

void NewsFeed::sendNewsFeedChanged()
{
	int count = 0;
	ui->feedWidget->withAll(sendNewsFeedChangedCallback, &count);

	emit newsFeedChanged(count);
}

void NewsFeed::feedoptions()
{
	SettingsPage::showYourself(this, SettingsPage::Notify);
}

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

#include <QDateTime>

#include "NewsFeed.h"
#include "ui_NewsFeed.h"

#include <retroshare/rsbanlist.h>
#include <retroshare/rsgxschannels.h>
#include <retroshare/rsgxsforums.h>
#include <retroshare/rschats.h>
#include <retroshare/rsmail.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsplugin.h>
#include <retroshare/rsposted.h>
#include <retroshare/rswire.h>

#include "util/misc.h"
#include "util/qtthreadsutils.h"
#include "feeds/BoardsCommentsItem.h"
#include "feeds/BoardsPostItem.h"
#include "feeds/ChatMsgItem.h"
#include "feeds/GxsCircleItem.h"
#include "feeds/ChannelsCommentsItem.h"
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
#include "feeds/WireNotifyGroupItem.h"
#include "feeds/WireNotifyPostItem.h"

#include "settings/rsettingswin.h"
#include "settings/rsharesettings.h"

#include "chat/ChatDialog.h"
#include "Posted/PostedItem.h"
#include "msgs/MessageComposer.h"
#include "msgs/MessageInterface.h"

#include "common/FeedNotify.h"

#define ROLE_RECEIVED FEED_TREEWIDGET_SORTROLE

#define TOKEN_TYPE_GROUP      1
#define TOKEN_TYPE_MESSAGE    2
#define TOKEN_TYPE_PUBLISHKEY 3

/*****
 * #define NEWS_DEBUG  1
 ****/

using namespace Rs::Mail;

static NewsFeed* instance = nullptr;

/** Constructor */
NewsFeed::NewsFeed(QWidget *parent) : MainPage(parent), ui(new Ui::NewsFeed),
    mEventTypes({
        RsEventType::AUTHSSL_CONNECTION_AUTENTICATION,
        RsEventType::FRIEND_LIST                     ,
        RsEventType::GXS_CIRCLES                     ,
        RsEventType::GXS_CHANNELS                    ,
        RsEventType::GXS_FORUMS                      ,
        RsEventType::GXS_POSTED                      ,
        RsEventType::MAIL_STATUS                     ,
        RsEventType::WIRE
    })
{
    /* Invoke the Qt Designer generated object setup routine */
    ui->setupUi(this);

    for(uint32_t i=0;i<mEventTypes.size();++i)
	{
		mEventHandlerIds.push_back(0); // needed to force intialization by registerEventsHandler()
		rsEvents->registerEventsHandler(
		            [this](std::shared_ptr<const RsEvent> event) { handleEvent(event); },
		            mEventHandlerIds.back(), mEventTypes[i] );
	}

	if (!instance) {
		instance = this;
	}

	ui->feedWidget->enableRemove(true);

	ui->sortComboBox->addItem(tr("Newest on top"), Qt::DescendingOrder);
	ui->sortComboBox->addItem(tr("Oldest on top"), Qt::AscendingOrder);

	connect(ui->sortComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(sortChanged(int)));

	connect(ui->removeAllButton, SIGNAL(clicked()), ui->feedWidget, SLOT(clear()));
	connect(ui->feedWidget, SIGNAL(feedCountChanged()), this, SLOT(sendNewsFeedChanged()),Qt::QueuedConnection);

	connect(ui->feedOptionsButton, SIGNAL(clicked()), this, SLOT(feedoptions()));
    ui->feedOptionsButton->hide();	// (csoler) Hidden until we repare the system to display a specific settings page.

	int H = misc::getFontSizeFactor("HelpButton").height();
	QString hlp_str = tr(
	    "<h1><img width=\"%1\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Activity Feed</h1>"
	    "<p>The Activity Feed displays the last events on your network, sorted by the time you received them."
	    "   This gives you a summary of the activity of your friends."
	    "   You can configure which events to show by pressing on <b>Options</b>. </p>"
	    "<p>The various events shown are:"
	    "   <ul>"
	    "    <li>Connection attempts (useful to make friends with new people and control who's trying to reach you)</li>"
	    "    <li>Channel, Forum and Board posts</li>"
	    "    <li>Circle membership requests and invites</li>"
	    "    <li>New Channels, Forums and Boards you can subscribe to</li>"
	    "    <li>Channel and Board comments</li>"
	    "    <li>New Mail messages</li>"
	    "    <li>Private messages from your friends</li>"
#ifdef RS_USE_WIRE
        "    <li>New Wire pulses and mentions</li>"
#endif	
	    "   </ul> </p>"
	                    ).arg(QString::number(2*H));

	registerHelpButton(ui->helpButton,hlp_str,"NewFeed") ;

	// load settings
	processSettings(true);
}

NewsFeed::~NewsFeed()
{
    for(uint32_t i=0;i<mEventHandlerIds.size();++i)
		rsEvents->unregisterEventsHandler(mEventHandlerIds[i]);

	// save settings
	processSettings(false);

	if (instance == this) {
		instance = NULL;
	}
}

UserNotify *NewsFeed::createUserNotify(QObject *parent)
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

// handler for the new notification system in libretroshare.

void NewsFeed::handleEvent(std::shared_ptr<const RsEvent> event)
{
	// /!\ Absolutely no access to Qt structures (such as Settings) should happen here!!!

	RsQThreadUtils::postToObject( [=]() { handleEvent_main_thread(event); }, this );
}

void NewsFeed::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    RsFeedTypeFlags flags = (RsFeedTypeFlags)Settings->getNewsFeedFlags();

    if(event->mType == RsEventType::AUTHSSL_CONNECTION_AUTENTICATION && (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_SECURITY)))
		handleSecurityEvent(event);

    if(event->mType == RsEventType::FRIEND_LIST && (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_PEER)))
		handleConnectionEvent(event);

    if(event->mType == RsEventType::GXS_CIRCLES && (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_CIRCLE)))
		handleCircleEvent(event);

    if(event->mType == RsEventType::GXS_CHANNELS && (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_CHANNEL)))
		handleChannelEvent(event);

    if(event->mType == RsEventType::GXS_FORUMS && (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_FORUM)))
		handleForumEvent(event);

    if(event->mType == RsEventType::GXS_POSTED && (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_POSTED)))
		handlePostedEvent(event);

    if(event->mType == RsEventType::MAIL_STATUS && (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_MSG)))
		handleMailEvent(event);

    if(event->mType == RsEventType::WIRE && (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_WIRE)))
		handleWireEvent(event);

}

void NewsFeed::handleMailEvent(std::shared_ptr<const RsEvent> event)
{
	const RsMailStatusEvent *pe =
	        dynamic_cast<const RsMailStatusEvent*>(event.get());
	if(!pe) return;


	switch(pe->mMailStatusEventCode)
	{
	case RsMailStatusEventCode::NEW_MESSAGE:
		for(auto msgid: pe->mChangedMsgIds)
			addFeedItem( new MsgItem(this, NEWSFEED_MESSAGELIST, msgid, false));
		break;
	default: break;
	}
}

void NewsFeed::handlePostedEvent(std::shared_ptr<const RsEvent> event)
{
	const RsGxsPostedEvent *pe =
	        dynamic_cast<const RsGxsPostedEvent*>(event.get());
	if(!pe) return;

	switch(pe->mPostedEventCode)
	{
	case RsPostedEventCode::UPDATED_POSTED_GROUP:
	case RsPostedEventCode::NEW_POSTED_GROUP:
		addFeedItem( new PostedGroupItem(this, NEWSFEED_POSTEDNEWLIST, pe->mPostedGroupId, false, true));
		break;
	case RsPostedEventCode::NEW_MESSAGE:
        addFeedItem( new BoardsPostItem(this, NEWSFEED_POSTEDMSGLIST, pe->mPostedGroupId, pe->mPostedMsgId, false, true));
		break;
	case RsPostedEventCode::NEW_COMMENT:
		addFeedItem( new BoardsCommentsItem(this, NEWSFEED_POSTEDMSGLIST, pe->mPostedGroupId, pe->mPostedMsgId, false, true));
		break;
	default: break;
	}
}

void NewsFeed::handleForumEvent(std::shared_ptr<const RsEvent> event)
{
	const RsGxsForumEvent *pe = dynamic_cast<const RsGxsForumEvent*>(event.get());
	if(!pe) return;

	switch(pe->mForumEventCode)
	{
	case RsForumEventCode::MODERATOR_LIST_CHANGED:
		addFeedItem(new GxsForumGroupItem(this, NEWSFEED_UPDATED_FORUM, pe->mForumGroupId,pe->mModeratorsAdded,pe->mModeratorsRemoved, false, true));
        break;

	case RsForumEventCode::UPDATED_FORUM:
	case RsForumEventCode::NEW_FORUM:
		addFeedItem(new GxsForumGroupItem(this, NEWSFEED_NEW_FORUM, pe->mForumGroupId, false, true));
		break;

	case RsForumEventCode::UPDATED_MESSAGE:
	case RsForumEventCode::NEW_MESSAGE:
			addFeedItem(new GxsForumMsgItem(
			                this, NEWSFEED_NEW_FORUM,
			                pe->mForumGroupId, pe->mForumMsgId,
			                false, true ));
		break;

	default: break;
	}
}

void NewsFeed::handleChannelEvent(std::shared_ptr<const RsEvent> event)
{
	const RsGxsChannelEvent* pe =
	        dynamic_cast<const RsGxsChannelEvent*>(event.get());
	if(!pe) return;

	switch(pe->mChannelEventCode)
	{
	case RsChannelEventCode::UPDATED_CHANNEL: // [[fallthrough]];
	case RsChannelEventCode::NEW_CHANNEL:
		addFeedItem(new GxsChannelGroupItem(this, NEWSFEED_CHANNELNEWLIST, pe->mChannelGroupId, false, true));
		break;
	case RsChannelEventCode::UPDATED_MESSAGE:  // [[fallthrough]];
	case RsChannelEventCode::NEW_MESSAGE:
		addFeedItem(new GxsChannelPostItem(this, NEWSFEED_CHANNELNEWLIST, pe->mChannelGroupId, pe->mChannelMsgId, false, true));
		break;
	case RsChannelEventCode::NEW_COMMENT:
        addFeedItem(new ChannelsCommentsItem(this, NEWSFEED_CHANNELNEWLIST, pe->mChannelGroupId, pe->mChannelMsgId,pe->mChannelThreadId, false, true));
		break;
	case RsChannelEventCode::RECEIVED_PUBLISH_KEY:
		addFeedItem(new GxsChannelGroupItem(this, NEWSFEED_CHANNELPUBKEYLIST, pe->mChannelGroupId, false, true));
		break;
	default: break;
	}
}

void NewsFeed::handleCircleEvent(std::shared_ptr<const RsEvent> event)
{
    // Gives the backend a few secs to load the cache data while not blocking the UI. This is not so nice, but there's no proper
    // other way to do that.

    RsThread::async( [event,this]()
    {
		const RsGxsCircleEvent *pe = dynamic_cast<const RsGxsCircleEvent*>(event.get());
		if(!pe)
			return;

		if(pe->mCircleId.isNull())	// probably an item for cache update
			return ;

		RsGxsCircleDetails details;
        bool loaded = false;

        for(int i=0;i<5 && !loaded;++i)
			if(rsGxsCircles->getCircleDetails(pe->mCircleId,details))
            {
                std::cerr << "Cache item loaded for circle " << pe->mCircleId << std::endl;
                loaded = true;
            }
			else
            {
                std::cerr << "Cache item for circle " << pe->mCircleId << " not loaded. Waiting " << i << "s" << std::endl;
                rstime::rs_usleep(1000*1000);
            }

        if(!loaded)
		{
			std::cerr << "(EE) Cannot get information about circle " << pe->mCircleId << ". Not in cache?" << std::endl;
			return;
		}

		if(!details.isGxsIdBased())	// not handled yet.
			return;

		// Check if the circle is one of which we belong to or we are an admin of.
		// If so, then notify in the GUI about other members leaving/subscribing, according
		// to the following rules. The names correspond to the RS_FEED_CIRCLE_* variables:
		//
		//  Message-based notifications:
		//
		//                       +---------------------------+----------------------------+
		//                       |  Membership request       |  Membership cancellation   |
		//                       +-------------+-------------+-------------+--------------+
		//                       |  Admin      |  Not admin  |    Admin    |   Not admin  |
		//  +--------------------+-------------+-------------+----------------------------+
		//  |    in invitee list |  MEMB_JOIN  |  MEMB_JOIN  |  MEMB_LEAVE |  MEMB_LEAVE  |
		//  +--------------------+-------------+-------------+-------------+--------------+
		//  |not in invitee list |  MEMB_REQ   |      X      |      X      |      X       |
		//  +--------------------+-------------+-------------+-------------+--------------+
		//
		//  Note: in this case, the GxsId never belongs to you, since you dont need to handle
		//        notifications for actions you took yourself (leave/join a circle)
		//
		//  GroupData-based notifications, the GxsId belongs to you:
		//
		//                       +---------------------------+----------------------------+
		//                       | GxsId joins invitee list  |  GxsId leaves invitee list |
		//                       +-------------+-------------+-------------+--------------+
		//                       |  Id is yours| Id is not   | Id is yours | Id is not    |
		//  +--------------------+-------------+-------------+-------------+--------------+
		//  | Has Member request | MEMB_ACCEPT | (MEMB_JOIN) | MEMB_REVOKED| (MEMB_LEAVE) |
		//  +--------------------+-------------+-------------+-------------+--------------+
		//  | No Member request  |  INVITE_REC |     X       |  INVITE_REM |     X        |
		//  +--------------------+-------------+-------------+-------------+--------------+
		//
		//  Note: In this case you're never an admin of the circle, since these notification
		//        would be a direct consequence of your own actions.

	RsQThreadUtils::postToObject( [event,details,this]()
	{
		const RsGxsCircleEvent *pe = static_cast<const RsGxsCircleEvent*>(event.get());

		switch(pe->mCircleEventType)
		{
		case RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_REQUEST:
			// only show membership requests if we're an admin of that circle
			if(details.isIdInInviteeList(pe->mGxsId))
                addFeedItemIfUnique(new GxsCircleItem(this, NEWSFEED_CIRCLELIST, pe->mCircleId, pe->mGxsId, RsFeedTypeFlags::RS_FEED_ITEM_CIRCLE_MEMB_JOIN),true);
			else if(details.mAmIAdmin)
                addFeedItemIfUnique(new GxsCircleItem(this, NEWSFEED_CIRCLELIST, pe->mCircleId, pe->mGxsId, RsFeedTypeFlags::RS_FEED_ITEM_CIRCLE_MEMB_REQ),true);

			break;

		case RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_LEAVE:

			if(details.isIdInInviteeList(pe->mGxsId))
                addFeedItemIfUnique(new GxsCircleItem(this, NEWSFEED_CIRCLELIST, pe->mCircleId, pe->mGxsId, RsFeedTypeFlags::RS_FEED_ITEM_CIRCLE_MEMB_LEAVE),true);
			break;

		case RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_ID_ADDED_TO_INVITEE_LIST:
			if(rsIdentity->isOwnId(pe->mGxsId))
			{
				if(details.isIdRequestingMembership(pe->mGxsId))
                    addFeedItemIfUnique(new GxsCircleItem(this, NEWSFEED_CIRCLELIST, pe->mCircleId, pe->mGxsId, RsFeedTypeFlags::RS_FEED_ITEM_CIRCLE_MEMB_ACCEPTED),true);
				else
                    addFeedItemIfUnique(new GxsCircleItem(this, NEWSFEED_CIRCLELIST, pe->mCircleId, pe->mGxsId, RsFeedTypeFlags::RS_FEED_ITEM_CIRCLE_INVITE_REC),true);
			}
			break;

		case RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_ID_REMOVED_FROM_INVITEE_LIST:
			if(rsIdentity->isOwnId(pe->mGxsId))
			{
				if(details.isIdRequestingMembership(pe->mGxsId))
                    addFeedItemIfUnique(new GxsCircleItem(this, NEWSFEED_CIRCLELIST, pe->mCircleId, pe->mGxsId, RsFeedTypeFlags::RS_FEED_ITEM_CIRCLE_MEMB_REVOKED),true);
				else
                    addFeedItemIfUnique(new GxsCircleItem(this, NEWSFEED_CIRCLELIST, pe->mCircleId, pe->mGxsId, RsFeedTypeFlags::RS_FEED_ITEM_CIRCLE_INVITE_CANCELLED),true);
			}
			break;

		default: break;
		}
	}, this ); }); // damn!
}

void NewsFeed::handleConnectionEvent(std::shared_ptr<const RsEvent> event)
{
    const RsFriendListEvent *pe = dynamic_cast<const RsFriendListEvent*>(event.get());
	if(!pe) return;

	auto& e(*pe);

    switch(e.mEventCode)
	{
    case RsFriendListEventCode::NODE_CONNECTED:
		addFeedItemIfUnique(new PeerItem(this, NEWSFEED_PEERLIST, e.mSslId, PEER_TYPE_CONNECT, false), true);
		break;
    case RsFriendListEventCode::NODE_DISCONNECTED: // not handled yet
		break;
    case RsFriendListEventCode::NODE_TIME_SHIFT:
		addFeedItemIfUnique(new PeerItem(this, NEWSFEED_PEERLIST, e.mSslId, PEER_TYPE_OFFSET, false),false);
		break;
    case RsFriendListEventCode::NODE_REPORTS_WRONG_IP:
		addFeedItemIfUnique(new SecurityIpItem(
		                        this, e.mSslId, e.mOwnLocator.toString(),
		                        e.mReportedLocator.toString(),
                                RsFeedTypeFlags::RS_FEED_ITEM_SEC_IP_WRONG_EXTERNAL_IP_REPORTED,
		                        false ), false);
		break;
	default: break;
	}
}

void NewsFeed::handleSecurityEvent(std::shared_ptr<const RsEvent> event)
{
 	const RsAuthSslConnectionAutenticationEvent *pe = dynamic_cast<const RsAuthSslConnectionAutenticationEvent*>(event.get());

    if(!pe)
        return;

    auto& e(*pe);
    RsFeedTypeFlags flags = (RsFeedTypeFlags)Settings->getNewsFeedFlags();

    if(e.mErrorCode == RsAuthSslError::PEER_REFUSED_CONNECTION && (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_SECURITY_IP)))
	{
		addFeedItemIfUnique(new PeerItem(this, NEWSFEED_PEERLIST, e.mSslId, PEER_TYPE_HELLO, false), true );
		return;
	}
	

    RsFeedTypeFlags FeedItemType(RsFeedTypeFlags::RS_FEED_TYPE_NONE);

	switch(e.mErrorCode)
	{
	case RsAuthSslError::NO_CERTIFICATE_SUPPLIED:      // fallthrough
	case RsAuthSslError::MISMATCHED_PGP_ID:            // fallthrough
	case RsAuthSslError::MISSING_AUTHENTICATION_INFO:
        FeedItemType = RsFeedTypeFlags::RS_FEED_ITEM_SEC_BAD_CERTIFICATE; break;
	case RsAuthSslError::PGP_SIGNATURE_VALIDATION_FAILED:
        FeedItemType = RsFeedTypeFlags::RS_FEED_ITEM_SEC_WRONG_SIGNATURE; break;
	case RsAuthSslError::NOT_A_FRIEND:
        FeedItemType = RsFeedTypeFlags::RS_FEED_ITEM_SEC_CONNECT_ATTEMPT; break;
	case RsAuthSslError::IP_IS_BLACKLISTED:
        FeedItemType = RsFeedTypeFlags::RS_FEED_ITEM_SEC_IP_BLACKLISTED; break;
	case RsAuthSslError::MISSING_CERTIFICATE:
        FeedItemType = RsFeedTypeFlags::RS_FEED_ITEM_SEC_MISSING_CERTIFICATE; break;
	default:
		return; // display nothing
	}

    RsPeerDetails det;
	rsPeers->getPeerDetails(e.mSslId,det) || rsPeers->getGPGDetails(e.mPgpId,det);

	addFeedItemIfUnique(new SecurityItem(this, NEWSFEED_SECLIST, e.mPgpId, e.mSslId, det.location, e.mLocator.toString(), FeedItemType, false), true );

    if (Settings->getMessageFlags() & RshareSettings::RS_MESSAGE_CONNECT_ATTEMPT)
		MessageComposer::addConnectAttemptMsg(e.mPgpId, e.mSslId, QString::fromStdString(det.name + "(" + det.location + ")"));
}

void NewsFeed::handleWireEvent(std::shared_ptr<const RsEvent> event)
{
    const RsWireEvent *pe =
            dynamic_cast<const RsWireEvent*>(event.get());
    if(!pe) return;

    switch(pe->mWireEventCode)
    {
    case RsWireEventCode::FOLLOW_STATUS_CHANGED:
        addFeedItem( new WireNotifyGroupItem(this, NEWSFEED_WIRELIST, pe->mWireGroupId, false, true, RsWireEventCode::FOLLOW_STATUS_CHANGED));
        break;
    case RsWireEventCode::NEW_WIRE:
        addFeedItem( new WireNotifyGroupItem(this, NEWSFEED_WIRELIST, pe->mWireGroupId, false, true, RsWireEventCode::NEW_WIRE));
        break;
    case RsWireEventCode::NEW_POST:
        addFeedItem( new WireNotifyPostItem(this, NEWSFEED_WIRELIST, pe->mWireGroupId, pe->mWireMsgId, false, true));
        break;
    case RsWireEventCode::NEW_REPLY:
        addFeedItem( new WireNotifyPostItem(this, NEWSFEED_WIRELIST, pe->mWireGroupId, pe->mWireMsgId, false, true));
        break;
    case RsWireEventCode::NEW_REPUBLISH:
        addFeedItem( new WireNotifyPostItem(this, NEWSFEED_WIRELIST, pe->mWireGroupId, pe->mWireMsgId, false, true));
        break;
    case RsWireEventCode::NEW_LIKE:
        addFeedItem( new WireNotifyPostItem(this, NEWSFEED_WIRELIST, pe->mWireGroupId, pe->mWireMsgId, false, true));
        break;
    case RsWireEventCode::WIRE_UPDATED:
        addFeedItem( new WireNotifyGroupItem(this, NEWSFEED_WIRELIST, pe->mWireGroupId, false, true, RsWireEventCode::WIRE_UPDATED));
        break;
//    case RsWireEventCode::POST_UPDATED:
//        addFeedItem( new WireNotifyGroupItem(this, NEWSFEED_WIRELIST, pe->mWireGroupId, false, true));
//        break;
    default: break;
    }
    sendNewsFeedChanged();
}


void NewsFeed::testFeeds(RsFeedTypeFlags /*notifyFlags*/)
{
    RsFeedTypeFlags flags = RsFeedTypeFlags(Settings->getNewsFeedFlags());

	//For test your feed add valid ID's for RsGxsGroupId & RsGxsMessageId, else test feed will be not displayed

    if (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_PEER))
		instance->addFeedItemIfUnique(new PeerItem(instance, NEWSFEED_PEERLIST, RsPeerId(""), PEER_TYPE_CONNECT, false), true);

    if (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_MSG))
		instance->addFeedItemIfUnique(new MsgItem(instance, NEWSFEED_MESSAGELIST, std::string(""), false), true);

    if (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_CHANNEL)){
		instance->addFeedItem(new GxsChannelGroupItem(instance, NEWSFEED_CHANNELNEWLIST, RsGxsGroupId(""), false, true));
		instance->addFeedItem(new GxsChannelPostItem(instance, NEWSFEED_CHANNELNEWLIST, RsGxsGroupId(""), RsGxsMessageId(""), false, true));
		instance->addFeedItem(new ChannelsCommentsItem(instance, NEWSFEED_CHANNELNEWLIST, RsGxsGroupId(""), RsGxsMessageId(""), RsGxsMessageId(""), false, true));
	}

    if(!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_FORUM)){
		instance->addFeedItem(new GxsForumGroupItem(instance, NEWSFEED_NEW_FORUM, RsGxsGroupId(""), false, true));
		instance->addFeedItem(new GxsForumMsgItem(instance, NEWSFEED_NEW_FORUM, RsGxsGroupId(""), RsGxsMessageId(""), false, true ));
	}

    if(!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_POSTED)){
		instance->addFeedItem( new PostedGroupItem(instance, NEWSFEED_POSTEDNEWLIST, RsGxsGroupId(""), false, true));
		instance->addFeedItem( new PostedItem(instance, NEWSFEED_POSTEDMSGLIST, RsGxsGroupId(""), RsGxsMessageId(""), false, true));
		instance->addFeedItem( new BoardsCommentsItem(instance, NEWSFEED_POSTEDMSGLIST, RsGxsGroupId(""), RsGxsMessageId(""), false, true));
	}

    if (!!(flags & RsFeedTypeFlags::RS_FEED_TYPE_CIRCLE))
        instance->addFeedItemIfUnique(new GxsCircleItem(instance, NEWSFEED_CIRCLELIST, RsGxsCircleId(""), RsGxsId(""), RsFeedTypeFlags::RS_FEED_ITEM_CIRCLE_MEMB_JOIN),true);;

    auto feedItem2 = new BoardsPostItem(instance,
                                           NEWSFEED_CHANNELNEWLIST,
                                           RsGxsGroupId  ("00000000000000000000000000000000"),
                                           RsGxsMessageId("0000000000000000000000000000000000000000")
                                           , false, true);

    instance->addFeedItem(feedItem2);

#ifdef TO_REMOVE
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
	}

	instance->ui->feedWidget->enableCountChangedSignal(true);

	instance->sendNewsFeedChanged();
#endif
    std::cerr << "(EE) testFeeds() is currently disabled" << std::endl;
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
	sendNewsFeedChanged();
}

void NewsFeed::addFeedItemIfUnique(FeedItem *item, bool replace)
{
	FeedItem *feedItem = ui->feedWidget->findFeedItem(item->uniqueIdentifier());

	if (feedItem)
    {
		if (!replace)
        {
			delete item;
			return;
		}

		ui->feedWidget->removeFeedItem(feedItem);
	}

	addFeedItem(item);
	//sendNewsFeedChanged(); //Already done by addFeedItem()
}

void NewsFeed::remUniqueFeedItem(FeedItem *item)
{
	//FeedItem *feedItem = ui->feedWidget->findFeedItem(item->uniqueIdentifier());

		ui->feedWidget->removeFeedItem(item);
		delete item;
}

/* FeedHolder Functions (for FeedItem functionality) */
QScrollArea *NewsFeed::getScrollArea()
{
	return NULL;
}

void NewsFeed::deleteFeedItem(FeedItem *item, uint32_t /*type*/)
{
#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::deleteFeedItem()";
	std::cerr << std::endl;
#endif

	if (item) {
		ui->feedWidget->removeFeedItem(item);
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

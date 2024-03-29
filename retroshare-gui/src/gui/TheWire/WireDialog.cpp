/*******************************************************************************
 * gui/TheWire/WireDialog.cpp                                                  *
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

#include "WireDialog.h"

#include "WireGroupDialog.h"
#include "WireGroupItem.h"
#include "gui/settings/rsharesettings.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/FilesDefs.h"
#include "gui/common/NotifyWidget.h"
#include "gui/gxs/GxsStatisticsProvider.h"
#include "gui/gxs/GxsGroupShareKey.h"

#include "PulseViewGroup.h"
#include "PulseReplySeperator.h"
#include "WireUserNotify.h"

#include "util/qtthreadsutils.h"
#include "util/misc.h"

#include <retroshare/rspeers.h>
#include <retroshare/rswire.h>

#include <iostream>
#include <sstream>

#include <QTimer>
#include <QMessageBox>

/****************************************************************
 * TheWire Display Widget.
 *
 */

#define GROUP_SET_ALL           (0)
#define GROUP_SET_OWN           (1)
#define GROUP_SET_SUBSCRIBED    (2)
#define GROUP_SET_OTHERS        (3)
// Future Extensions.
// #define GROUP_SET_AUTO          (4)
// #define GROUP_SET_RECOMMENDED   (5)


#define WIRE_TOKEN_TYPE_SUBSCRIBE_CHANGE 1
#define TOKEN_TYPE_GROUP_SUMMARY    1

/** Constructor */
WireDialog::WireDialog(QWidget *parent)
    : GxsStatisticsProvider(rsWire, settingsGroupName(),parent, true), mGroupSet(GROUP_SET_ALL)
    , mAddDialog(nullptr), mGroupSelected(nullptr), mWireQueue(nullptr)
    , mHistoryIndex(-1), mEventHandlerId(0)
{
	ui.setupUi(this);

	connect( ui.toolButton_createAccount, SIGNAL(clicked()), this, SLOT(createGroup()));
	connect( ui.toolButton_createPulse, SIGNAL(clicked()), this, SLOT(createPulse()));

	connect(ui.comboBox_groupSet, SIGNAL(currentIndexChanged(int)), this, SLOT(selectGroupSet(int)));
	connect(ui.comboBox_filterTime, SIGNAL(currentIndexChanged(int)), this, SLOT(selectFilterTime(int)));

	connect( ui.toolButton_back, SIGNAL(clicked()), this, SLOT(back()));
	connect( ui.toolButton_forward, SIGNAL(clicked()), this, SLOT(forward()));
	ui.toolButton_back->setEnabled(false);
	ui.toolButton_forward->setEnabled(false);

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);

	/* setup TokenQueue */
	mWireQueue = new TokenQueue(rsWire->getTokenService(), this);
    mCountChildMsgs = false;
    mShouldUpdateGroupStatistics = false;
    mDistSyncAllowed = true;

	requestGroupData();

	// just for testing
	postTestTwitterView();

	// load settings
	processSettings(true);

    // Needs to be asynced because this function is called by another thread!
    rsEvents->registerEventsHandler(
                [this](std::shared_ptr<const RsEvent> event)
    { RsQThreadUtils::postToObject([=]() { handleEvent_main_thread(event); }, this ); },
                mEventHandlerId, RsEventType::WIRE );
}

void WireDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    const RsWireEvent *e = dynamic_cast<const RsWireEvent*>(event.get());

    if(e)
    {

#ifdef GXSWIRE_DEBUG
                RsDbg() << " Refreshing the feed if there is a matching event. "<< std::endl;
#endif

        // The following switch statements refresh the wire feed whenever there is a new event

        std::cout<<"***************the wire event code is ************************* "<<std::endl;
        switch(e->mWireEventCode)
        {

        case RsWireEventCode::READ_STATUS_CHANGED:
            updateGroupStatisticsReal(e->mWireGroupId); // update the list immediately
            std::cout<<"1"<<std::endl;
            return;

        case RsWireEventCode::STATISTICS_CHANGED:
            std::cout<<"2"<<std::endl;

        case RsWireEventCode::NEW_POST:
            std::cout<<"3"<<std::endl;

        case RsWireEventCode::NEW_REPLY:
            std::cout<<"4"<<std::endl;

        case RsWireEventCode::NEW_LIKE:
            std::cout<<"5"<<std::endl;

        case RsWireEventCode::NEW_REPUBLISH:
            std::cout<<"6"<<std::endl;

        case RsWireEventCode::POST_UPDATED:
            std::cout<<"7"<<std::endl;

        case RsWireEventCode::FOLLOW_STATUS_CHANGED:
            std::cout<<"8"<<std::endl;

        case RsWireEventCode::NEW_WIRE:
            std::cout<<"9"<<std::endl;
            updateGroupStatisticsReal(e->mWireGroupId);
            break;

        default:
#ifdef GXSWIRE_DEBUG
                RsDbg() << " Unknown event occured: "<< e->mWireEventCode << std::endl;
#endif
            break;

        }
        refreshGroups();
    }
}

WireDialog::~WireDialog()
{
	// save settings
	processSettings(false);
	
	clearTwitterView();
    delete(mWireQueue);

    rsEvents->unregisterEventsHandler(mEventHandlerId);
}

UserNotify *WireDialog::createUserNotify(QObject *parent)
{
    return new WireUserNotify(rsWire, this, parent);
}

//QString WireDialog::getHelpString() const
//{
//    int H = misc::getFontSizeFactor("HelpButton").height();

//    QString hlp_str = tr(
//        "<h1><img width=\"%1\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Boards</h1>"
//        "<p>The wire service is a day to day social network service.</p>"
//        "<p>The Wire service allows you to share your pulses, republish someone else's pulses, like "
//        "a pulse.</p>"
//        "<p>A pulse is like a channel or board post. It can be a image or a text or both. You can "
//        "only see the pulses of the people you follow.</p>"
//        "<p>Wire posts are kept for %2 days, and sync-ed over the last %3 days, unless you change this.</p>"
//                        ).arg(  QString::number(2*H)
//                              , QString::number(rsWire->getDefaultStoragePeriod()/86400)
//                              , QString::number(rsWire->getDefaultSyncPeriod()/86400));

//    return hlp_str ;
//}

//QString WireDialog::text(TextType type)
//{
//    switch (type) {
//    case TEXT_NAME:
//        return tr("Wire");
//    case TEXT_NEW:
//        return tr("Create Wire");

//// Dont know what this does
////	case TEXT_TODO:
////		return "<b>Open points:</b><ul>"
////		       "<li>Subreddits/tag to posts support"
////		       "<li>Picture Support"
////		       "<li>Navigate channel link"
////		       "</ul>";

//    case TEXT_YOUR_GROUP:
//        return tr("My Wire");
//    case TEXT_SUBSCRIBED_GROUP:
//        return tr("Followed Wires");
//    case TEXT_POPULAR_GROUP:
//        return tr("Popular Wires");
//    case TEXT_OTHER_GROUP:
//        return tr("Other Wires");
//    }

//    return "";
//}

//QString WireDialog::icon(IconType type)
//{
//    switch (type) {
//    case ICON_NAME:
//        return ":/icons/png/postedlinks.png";
//    case ICON_NEW:
//        return ":/icons/png/add.png";
//    case ICON_YOUR_GROUP:
//        return "";
//    case ICON_SUBSCRIBED_GROUP:
//        return "";
//    case ICON_POPULAR_GROUP:
//        return "";
//    case ICON_OTHER_GROUP:
//        return "";
//    case ICON_SEARCH:
//        return ":/images/find.png";
//    case ICON_DEFAULT:
//        return ":/icons/png/posted.png";
//    }

//    return "";
//}

//bool WireDialog::getGroupData(std::list<RsGxsGenericGroupData*>& groupInfo)
//{
//    std::vector<RsWireGroup> groups;

//    // request all group infos at once

//    if(! rsWire->getGroups(std::list<RsGxsGroupId>(),groups))
//        return false;

//    /* Save groups to fill icons and description */

//    for (auto& group: groups)
//       groupInfo.push_back(new RsWireGroup(group));

//    return true;
//}

bool WireDialog::getGroupStatistics(const RsGxsGroupId& groupId,GxsGroupStatistic& stat)
{
    // What follows is a hack to replace the GXS group statistics by the actual count of unread messages in wire,
    // which should take into account old post versions, discard replies and likes, etc.

    RsWireStatistics s;
    bool res = rsWire->getWireStatistics(groupId,s);

    if(!res)
        return false;

    stat.mGrpId = groupId;
    stat.mNumMsgs = s.mNumberOfPulses;

    stat.mTotalSizeOfMsgs = 0;	// hopefuly unused. Required the loading of the full channel data, so not very convenient.
    stat.mNumThreadMsgsNew = s.mNumberOfNewPulses;
    stat.mNumThreadMsgsUnread = s.mNumberOfUnreadPulses;
    stat.mNumChildMsgsNew = 0;
    stat.mNumChildMsgsUnread = 0;

    return true;
}

//GxsGroupDialog *WireDialog::createNewGroupDialog()
//{
//    return new WireGroupDialog(this);
//}

//GxsGroupDialog *WireDialog::createGroupDialog(GxsGroupDialog::Mode mode, RsGxsGroupId groupId)
//{
//    return new WireGroupDialog(mode, groupId, this);
//}

//int WireDialog::shareKeyType()
//{
//    return GroupShareKey::NO_KEY_SHARE;
//}

//GxsMessageFrameWidget *WireDialog::createMessageFrameWidget(const RsGxsGroupId &groupId)
//{
//    // to do
////	return new WireListWidgetWithModel(groupId);
//}

void WireDialog::processSettings(bool load)
{
	Settings->beginGroup("WireDialog");

	if (load) {
		// load settings

		// state of splitter
		ui.splitter->restoreState(Settings->value("SplitterWire").toByteArray());

		// state of filter combobox
		int index = Settings->value("ShowGroup", 0).toInt();
		ui.comboBox_groupSet->setCurrentIndex(index);
	} else {
		// save settings

		// state of filter combobox
		Settings->setValue("ShowGroup", ui.comboBox_groupSet->currentIndex());

		// state of splitter
		Settings->setValue("SplitterWire", ui.splitter->saveState());
	}

	Settings->endGroup();
}

void WireDialog::refreshGroups()
{
	requestGroupData();
}

void WireDialog::addGroup(QWidget *item)
{
	QLayout *alayout = ui.groupsWidget->layout();
	alayout->addWidget(item);
}

bool WireDialog::setupPulseAddDialog()
{
	std::cerr << "WireDialog::setupPulseAddDialog()";
	std::cerr << std::endl;

	if (!mAddDialog)
	{
		mAddDialog = new PulseAddDialog(NULL);
		mAddDialog->hide();
	}

	mAddDialog->cleanup();

	int idx = ui.groupChooser->currentIndex();
	if (idx < 0) {
		std::cerr << "WireDialog::setupPulseAddDialog() ERROR GETTING AuthorId!";
		std::cerr << std::endl;

		QMessageBox::warning(this, tr("RetroShare"),tr("Please create or choose Wire Groupd first"), QMessageBox::Ok, QMessageBox::Ok);
		return false;
	}

	// publishing group.
	RsWireGroup group = mOwnGroups[idx];
	mAddDialog->setGroup(group.mMeta.mGroupId);

	return true;
}


void WireDialog::subscribe(RsGxsGroupId &groupId)
{
	uint32_t token;
	rsWire->subscribeToGroup(token, groupId, true);
	mWireQueue->queueRequest(token, TOKENREQ_GROUPINFO, RS_TOKREQ_ANSTYPE_ACK, WIRE_TOKEN_TYPE_SUBSCRIBE_CHANGE);
}

void WireDialog::unsubscribe(RsGxsGroupId &groupId)
{
	uint32_t token;
	rsWire->subscribeToGroup(token, groupId, false);
	mWireQueue->queueRequest(token, TOKENREQ_GROUPINFO, RS_TOKREQ_ANSTYPE_ACK, WIRE_TOKEN_TYPE_SUBSCRIBE_CHANGE);
}

void WireDialog::notifyGroupSelection(WireGroupItem *item)
{
	std::cerr << "WireDialog::notifyGroupSelection() from : " << item;
	std::cerr << std::endl;

	bool doSelection = true;
	if (mGroupSelected)
	{
		std::cerr << "WireDialog::notifyGroupSelection() unselecting old one : " << mGroupSelected;
		std::cerr << std::endl;

		mGroupSelected->setSelected(false);
		if (mGroupSelected == item)
		{
			std::cerr << "WireDialog::notifyGroupSelection() current -> unselect";
			std::cerr << std::endl;
			/* de-selection of current item */
			mGroupSelected = NULL;
			doSelection = false;
		}
	}

	if (doSelection)
	{
		item->setSelected(true);
		mGroupSelected = item;
	}

	/* update display */
	showSelectedGroups();
}


void WireDialog::checkUpdate()
{
#if 0
	/* update */
	if (!rsWire)
		return;

	if (rsWire->updated())
	{
		insertAlbums();
	}
#endif
	return;
}

void WireDialog::createGroup()
{
	WireGroupDialog wireCreate(this);
	wireCreate.exec();
}

void WireDialog::createPulse()
{
	if (!mAddDialog)
	{
		mAddDialog = new PulseAddDialog(NULL);
		mAddDialog->hide();
	}

	int idx = ui.groupChooser->currentIndex();
	if (idx < 0) {
		std::cerr << "WireDialog::createPulse() ERROR GETTING AuthorId!";
		std::cerr << std::endl;

		QMessageBox::warning(this, tr("RetroShare"),tr("Please create or choose Wire Groupd first"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	RsWireGroup group = mOwnGroups[idx];

	mAddDialog->cleanup();
	mAddDialog->setGroup(group.mMeta.mGroupId);
	mAddDialog->show();
}

void WireDialog::addGroup(const RsWireGroup &group)
{
	std::cerr << "WireDialog::addGroup() GroupId : " << group.mMeta.mGroupId;
	std::cerr << std::endl;

	addGroup(new WireGroupItem(this, group));
}

void WireDialog::deleteGroups()
{
	std::cerr << "WireDialog::deleteGroups()";
	std::cerr << std::endl;

	mGroupSelected = NULL;

	QLayout *alayout = ui.groupsWidget->layout();
	QLayoutItem *item;
	int i = 0;
	while (i < alayout->count())
	{
		item = alayout->itemAt(i);
		QWidget *widget = item->widget();
		if (NULL != dynamic_cast<WireGroupItem *>(widget))
		{
			std::cerr << "WireDialog::deleteGroups() Removing Item at: " << i;
			std::cerr << std::endl;

			item = alayout->takeAt(i);
			delete item->widget();
			delete item;
		}
		else
		{
			std::cerr << "WireDialog::deleteGroups() Leaving Item at: " << i;
			std::cerr << std::endl;

			i++;
		}
	}
}

void WireDialog::updateGroups(std::vector<RsWireGroup>& groups)
{
	mAllGroups.clear();
	mOwnGroups.clear();
	ui.groupChooser->clear();

	for(auto &it : groups) {
		// save list of all groups.
		mAllGroups[it.mMeta.mGroupId] = it;

		if (it.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
		{
			// grab own groups.
			// setup Chooser too.
			mOwnGroups.push_back(it); 
			QPixmap pixmap;
			if (it.mHeadshot.mData)
			{
				if (GxsIdDetails::loadPixmapFromData( it.mHeadshot.mData,it.mHeadshot.mSize,pixmap,GxsIdDetails::ORIGINAL))
					pixmap = pixmap.scaled(32,32);
			} 
			else 
			{
				// default.
				pixmap = FilesDefs::getPixmapFromQtResourcePath(":/icons/wire.png").scaled(32,32);
			}

			ui.groupChooser->addItem(QPixmap(pixmap),QString::fromStdString(it.mMeta.mGroupName));
		}
	}
}


void WireDialog::selectGroupSet(int index)
{
	std::cerr << "WireDialog::selectGroupSet(" << index << ")";
	std::cerr << std::endl;

	mGroupSet = index;
	if (mGroupSet < 0) {
		mGroupSet = GROUP_SET_ALL;
	}
	showGroups();
}

void WireDialog::selectFilterTime(int index)
{
	std::cerr << "WireDialog::selectFilterTime(" << index << ")";
	std::cerr << std::endl;

	showSelectedGroups();
}

void WireDialog::showSelectedGroups()
{
	ui.comboBox_filterTime->setEnabled(false);
	if (mGroupSelected)
	{
		// request data.
		std::list<RsGxsGroupId> grpIds;
		grpIds.push_back(mGroupSelected->groupId());

		// show GroupFocus.
		requestGroupFocus(mGroupSelected->groupId());
	}
	else
	{
		showGroups();
	}
}

void WireDialog::showGroups()
{
	ui.comboBox_filterTime->setEnabled(false);
	deleteGroups();

	std::list<RsGxsGroupId> allGroupIds;

	/* depends on the comboBox */
	for (auto &it : mAllGroups)
	{
		bool add = (mGroupSet == GROUP_SET_ALL);
		if (it.second.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) {
			if (mGroupSet == GROUP_SET_OWN) {
				add = true;
			}
		}
		else if (it.second.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED) {
			if (mGroupSet == GROUP_SET_SUBSCRIBED) {
				add = true;
			}
		}
		else {
			if (mGroupSet == GROUP_SET_OTHERS) {
				add = true;
			}
		}

		if (add) {
			addGroup(it.second);
			// request data.
			std::list<RsGxsGroupId> grpIds;
			grpIds.push_back(it.second.mMeta.mGroupId);
			allGroupIds.push_back(it.second.mMeta.mGroupId);
		}
	}

	requestGroupsPulses(allGroupIds);
}


// LOAD DATA...............................................

void WireDialog::requestGroupData()
{
	std::cerr << "WireDialog::requestGroupData()";
	std::cerr << std::endl;

	RsTokReqOptions opts;
	uint32_t token;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	mWireQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, 0);
}

bool WireDialog::loadGroupData(const uint32_t &token)
{
	std::cerr << "WireDialog::loadGroupData()";
	std::cerr << std::endl;

	std::vector<RsWireGroup> groups;
	rsWire->getGroupData(token, groups);

	// save list of groups.
	updateGroups(groups);
	showGroups();
	return true;
}

rstime_t WireDialog::getFilterTimestamp()
{
	rstime_t filterTimestamp = time(NULL);
	switch(ui.comboBox_filterTime->currentIndex())
	{
		case 1: // Last 24 Hours.
			filterTimestamp -= (3600 * 24);
			break;
		case 2: // Last 7 Days.
			filterTimestamp -= (3600 * 24 * 7);
			break;
		case 3: // Last 30 Days.
			filterTimestamp -= (3600 * 24 * 30);
			break;
		case 0: // All Time.
		case -1: // no index.
		default:
			filterTimestamp = 0; // back to Epoch! effectively all.
			break;
	}
	return filterTimestamp;
}

void WireDialog::acknowledgeGroup(const uint32_t &token, const uint32_t &userType)
{
	/* reload groups */
	std::cerr << "WireDialog::acknowledgeGroup(usertype: " << userType << ")";
	std::cerr << std::endl;

	RsGxsGroupId grpId;
	rsWire->acknowledgeGrp(token, grpId);

	refreshGroups();
}


/**************************** Request / Response Filling of Data ************************/

void WireDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "WireDialog::loadRequest()";
	std::cerr << std::endl;

	if (queue == mWireQueue)
	{
		/* now switch on req */
		switch(req.mType)
		{
			case TOKENREQ_GROUPINFO:
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_DATA:
						loadGroupData(req.mToken);
						break;
					case RS_TOKREQ_ANSTYPE_ACK:
						acknowledgeGroup(req.mToken, req.mUserType);
						break;
					default:
						std::cerr << "WireDialog::loadRequest() ERROR: GROUP: INVALID ANS TYPE";
						std::cerr << std::endl;
						break;
				}
				break;
			default:
				std::cerr << "WireDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}
}


/**************************** Request / Response Filling of Data ************************/





/****************************************************************************************/
// TWITTER VIEW.
/****************************************************************************************/

// TODO
// - Handle Groups
//	 - Add GroupPtr to WirePulseSPtrs (DONE)
//   - Add HeadShot to WireGroup
//   - Add GxsIdLabel to Pulse UI elements.
//
// - Create Groups.
//   - Add HeadShot
//
// - Create Pulse.
//   - Add Images.
//
// - Link up Reply / Republish / Like.
//
// - showGroupFocus
//   - TODO
//
// - showPulseFocus
//   - Basics (DONE).
//   - MoreReplies
//   - Show Actual Message.

//
// - showReplyFocus
//   - TODO


// PulseDataItem interface
// Actions.
void WireDialog::PVHreply(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId)
{
	std::cerr << "WireDialog::PVHreply() GroupId: " << groupId;
	std::cerr << "MsgId: " << msgId;
	std::cerr << std::endl;

	if (setupPulseAddDialog())
	{
		mAddDialog->setReplyTo(groupId, msgId, WIRE_PULSE_TYPE_REPLY);
		mAddDialog->show();
	}
}

void WireDialog::PVHrepublish(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId)
{
	std::cerr << "WireDialog::PVHrepublish() GroupId: " << groupId;
	std::cerr << "MsgId: " << msgId;
	std::cerr << std::endl;

	if (setupPulseAddDialog())
	{
		mAddDialog->setReplyTo(groupId, msgId, WIRE_PULSE_TYPE_REPUBLISH);
		mAddDialog->show();
	}
}

void WireDialog::PVHlike(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId)
{
	std::cerr << "WireDialog::PVHlike() GroupId: " << groupId;
	std::cerr << "MsgId: " << msgId;
	std::cerr << std::endl;

	if (setupPulseAddDialog())
	{
		mAddDialog->setReplyTo(groupId, msgId, WIRE_PULSE_TYPE_LIKE);
		mAddDialog->show();
	}
}

void WireDialog::PVHviewGroup(const RsGxsGroupId &groupId)
{
	std::cerr << "WireDialog::PVHviewGroup(";
	std::cerr << groupId.toStdString();
	std::cerr << ")";
	std::cerr << std::endl;

	requestGroupFocus(groupId);
}

void WireDialog::PVHviewPulse(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId)
{
	std::cerr << "WireDialog::PVHviewPulse(";
	std::cerr << groupId.toStdString() << ",";
	std::cerr << msgId.toStdString();
	std::cerr << ")";
	std::cerr << std::endl;

	requestPulseFocus(groupId, msgId);
}

void WireDialog::PVHviewReply(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId)
{
	std::cerr << "WireDialog::PVHviewReply(";
	std::cerr << groupId.toStdString() << ",";
	std::cerr << msgId.toStdString();
	std::cerr << ")";
	std::cerr << std::endl;

	// requestPulseFocus(groupId, msgId);
}

void WireDialog::PVHfollow(const RsGxsGroupId &groupId)
{
	std::cerr << "WireDialog::PVHfollow(";
	std::cerr << groupId.toStdString();
	std::cerr << ")";
	std::cerr << std::endl;

	uint32_t token;
	rsWire->subscribeToGroup(token, groupId, true);
	mWireQueue->queueRequest(token, TOKENREQ_GROUPINFO, RS_TOKREQ_ANSTYPE_ACK, WIRE_TOKEN_TYPE_SUBSCRIBE_CHANGE);
}

void WireDialog::PVHrate(const RsGxsId &authorId)
{
	std::cerr << "WireDialog::PVHrate(";
	std::cerr << authorId.toStdString();
	std::cerr << ") TODO";
	std::cerr << std::endl;
}

void WireDialog::postTestTwitterView()
{
	clearTwitterView();

	addTwitterView(new PulseTopLevel(NULL,RsWirePulseSPtr())); 
	addTwitterView(new PulseReply(NULL,RsWirePulseSPtr()));
	addTwitterView(new PulseReply(NULL,RsWirePulseSPtr()));
	addTwitterView(new PulseReply(NULL,RsWirePulseSPtr()));
	addTwitterView(new PulseReply(NULL,RsWirePulseSPtr()));
	addTwitterView(new PulseReply(NULL,RsWirePulseSPtr()));
}

void WireDialog::clearTwitterView()
{
	std::cerr << "WireDialog::clearTwitterView()";
	std::cerr << std::endl;

	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	QLayoutItem *item;
	int i = 0;
	while (i < alayout->count())
	{
		item = alayout->itemAt(i);
		QWidget *widget = item->widget();
		if (NULL != dynamic_cast<PulseViewItem *>(widget))
		{
			std::cerr << "WireDialog::clearTwitterView() Removing Item at: " << i;
			std::cerr << std::endl;

			item = alayout->takeAt(i);
			delete item->widget();
			delete item;
		}
		else
		{
			std::cerr << "WireDialog::clearTwitterView() Leaving Item at: " << i;
			std::cerr << std::endl;

			i++;
		}
	}
}

void WireDialog::addTwitterView(PulseViewItem *item)
{
	std::cerr << "WireDialog::addTwitterView()";
	std::cerr << std::endl;

	/* ensure its a boxlayout */
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	QBoxLayout *boxlayout = dynamic_cast<QBoxLayout *>(alayout);
	if (boxlayout == NULL) {
		std::cerr << "WireDialog::addTwitterView() ERROR not boxlayout, not Inserting";
		std::cerr << std::endl;
		return;
	}

	// inserting as last item.
	std::cerr << "WireDialog::addTwitterView() Inserting at end";
	std::cerr << std::endl;
	boxlayout->addWidget(item);
}

// HISTORY -------------------------------------------------------------------------------

void printWireViewHistory(const WireViewHistory &view)
{
	std::cerr << "WireViewHistory(" << (int) view.viewType << "): ";
	switch(view.viewType) {
		case WireViewType::PULSE_FOCUS:
			std::cerr << " PULSE_FOCUS: grpId: " << view.groupId;
			std::cerr << " msgId: " << view.msgId;
			std::cerr << std::endl;
			break;
		case WireViewType::GROUP_FOCUS:
			std::cerr << " GROUP_FOCUS: grpId: " << view.groupId;
			std::cerr << std::endl;
			break;
		case WireViewType::GROUPS:
			std::cerr << " GROUPS_PULSES: grpIds: TBD";
			std::cerr << std::endl;
			break;
		default:
			break;
	}
}

void WireDialog::AddToHistory(const WireViewHistory &view)
{
	std::cerr << "AddToHistory():";
	printWireViewHistory(view);

	/* clear future history */
	mHistory.resize(mHistoryIndex + 1);

	mHistory.push_back(view);
	mHistoryIndex = mHistory.size() - 1;

	// at end of history.
	// enable back, disable forward.
	ui.toolButton_back->setEnabled(mHistoryIndex > 0);
	ui.toolButton_forward->setEnabled(false);
}

void WireDialog::back()
{
	LoadHistory(mHistoryIndex-1);
}

void WireDialog::forward()
{
	LoadHistory(mHistoryIndex+1);
}

void WireDialog::LoadHistory(uint32_t index)
{
	if (index >= mHistory.size()) {
		return;
	}

	mHistoryIndex = index;
	WireViewHistory view = mHistory[index];

	std::cerr << "LoadHistory:";
	printWireViewHistory(view);

	switch(view.viewType) {
		case WireViewType::PULSE_FOCUS:
			showPulseFocus(view.groupId, view.msgId);
			break;
		case WireViewType::GROUP_FOCUS:
			showGroupFocus(view.groupId);
			break;
		case WireViewType::GROUPS:
			showGroupsPulses(view.groupIds);
			break;
		default:
			break;
	}

	ui.toolButton_back->setEnabled(index > 0);
	ui.toolButton_forward->setEnabled(index + 1 < mHistory.size());
}
// HISTORY -------------------------------------------------------------------------------

void WireDialog::requestPulseFocus(const RsGxsGroupId groupId, const RsGxsMessageId msgId)
{
	WireViewHistory view;
	view.viewType = WireViewType::PULSE_FOCUS;
	view.groupId = groupId;
	view.msgId = msgId;

	AddToHistory(view);
	showPulseFocus(groupId, msgId);
}

void WireDialog::showPulseFocus(const RsGxsGroupId groupId, const RsGxsMessageId msgId)
{
	clearTwitterView();

	// background thread for loading.
	RsThread::async([this, groupId, msgId]()
	{
		// fetch data from backend.
		RsWirePulseSPtr pPulse;
		int type = 0;
		if(rsWire->getPulseFocus(groupId, msgId, type, pPulse))
		{
			// sleep(2);

			/* now insert the pulse + children into the layput */
			RsQThreadUtils::postToObject([pPulse,this]()
			{
				/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

				postPulseFocus(pPulse);

			}, this);
		}
	});
}


void WireDialog::postPulseFocus(RsWirePulseSPtr pPulse)
{
	clearTwitterView();
	if (!pPulse)
	{
		std::cerr << "WireDialog::postPulseFocus() Invalid pulse";
		std::cerr << std::endl;
		return;
	}

	ui.label_viewMode->setText("Pulse Focus");

	addTwitterView(new PulseTopLevel(this, pPulse));

	for(auto &it : pPulse->mReplies)
	{
		RsWirePulseSPtr reply = it;
		PulseReply *firstReply = new PulseReply(this, reply);
		addTwitterView(firstReply);

		if (reply->mReplies.size() > 0)
		{
			PulseReply *secondReply = new PulseReply(this, reply->mReplies.front());
			addTwitterView(secondReply);
			firstReply->showReplyLine(true);
			secondReply->showReplyLine(false);
		}
		else
		{
			firstReply->showReplyLine(false);
		}


		if (reply->mReplies.size() > 1)
		{
			// addTwitterView(new PulseMoreReplies(NULL, reply));
		}

		addTwitterView(new PulseReplySeperator());
	}

	// Add big separator, and republishes.
	if (pPulse->mReplies.size() > 0 && pPulse->mRepublishes.size() > 0)
	{
		addTwitterView(new PulseReplySeperator());
		addTwitterView(new PulseReplySeperator());
	}

	for(auto &it : pPulse->mRepublishes)
	{
		RsWirePulseSPtr repub = it;

		PulseReply *firstRepub = new PulseReply(this, repub);
		firstRepub->showReplyLine(false);

		addTwitterView(firstRepub);
		addTwitterView(new PulseReplySeperator());
	}


}

void WireDialog::requestGroupFocus(const RsGxsGroupId groupId)
{
	WireViewHistory view;
	view.viewType = WireViewType::GROUP_FOCUS;
	view.groupId = groupId;

	AddToHistory(view);
	showGroupFocus(groupId);
}

void WireDialog::showGroupFocus(const RsGxsGroupId groupId)
{
	clearTwitterView();

	// background thread for loading.
	RsThread::async([this, groupId]()
	{
		// fetch data from backend.
		RsWireGroupSPtr grp;
		std::list<RsWirePulseSPtr> pulses;

		bool success = rsWire->getWireGroup(groupId, grp);
		std::list<RsGxsGroupId> groupIds = { groupId };
		success = rsWire->getPulsesForGroups(groupIds, pulses);

		// sleep(2);

		/* now insert the pulse + children into the layput */
		RsQThreadUtils::postToObject([grp, pulses,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

			postGroupFocus(grp, pulses);

		}, this);
	});
}


void WireDialog::postGroupFocus(RsWireGroupSPtr group, std::list<RsWirePulseSPtr> pulses)
{
    clearTwitterView();

	std::cerr << "WireDialog::postGroupFocus()";
	std::cerr << std::endl;

	if (!group)
	{
		std::cerr << "WireDialog::postGroupFocus() group is INVALID";
		std::cerr << std::endl;
		return;
	}

	ui.label_viewMode->setText("Group Focus");

	addTwitterView(new PulseViewGroup(this, group));

	for(auto &it : pulses)
	{
		RsWirePulseSPtr reply = it;

		// don't show likes
		if (reply->mPulseType & WIRE_PULSE_TYPE_LIKE) {
			std::cerr << "WireDialog::postGroupFocus() Not showing LIKE";
			std::cerr << std::endl;
			continue;
		}

		PulseReply *firstReply = new PulseReply(this, reply);
		addTwitterView(firstReply);
		firstReply->showReplyLine(false);

		addTwitterView(new PulseReplySeperator());

	}
}

void WireDialog::requestGroupsPulses(const std::list<RsGxsGroupId>& groupIds)
{
	WireViewHistory view;
	view.viewType = WireViewType::GROUPS;
	view.groupIds = groupIds;

	AddToHistory(view);
	showGroupsPulses(groupIds);
}

void WireDialog::showGroupsPulses(const std::list<RsGxsGroupId>& groupIds)
{
	clearTwitterView();

	// background thread for loading.
	RsThread::async([this, groupIds]()
	{
		// fetch data from backend.
		std::list<RsWirePulseSPtr> pulses;
		if(rsWire->getPulsesForGroups(groupIds, pulses))
		{
			// sleep(2);

			/* now insert the pulse + children into the layput */
			RsQThreadUtils::postToObject([pulses,this]()
			{
				/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

				postGroupsPulses(pulses);

			}, this);
		}
	});
}

void WireDialog::postGroupsPulses(std::list<RsWirePulseSPtr> pulses)
{
    clearTwitterView();
	std::cerr << "WireDialog::postGroupsPulses()";
	std::cerr << std::endl;

	ui.label_viewMode->setText("Groups Pulses");

	for(auto &it : pulses)
	{
		RsWirePulseSPtr reply = it;
		// don't show likes
		if (reply->mPulseType & WIRE_PULSE_TYPE_LIKE) {
			std::cerr << "WireDialog::postGroupsPulses() Not showing LIKE";
			std::cerr << std::endl;
			continue;
		}

		PulseReply *firstReply = new PulseReply(this, reply);
		addTwitterView(firstReply);
		firstReply->showReplyLine(false);

		addTwitterView(new PulseReplySeperator());

	}
}

void WireDialog::getServiceStatistics(GxsServiceStatistic& stats) const
{
    std::cout<<"inside the getServiceStatics *********"<<std::endl;
    if(!mCachedGroupStats.empty())
    {
        stats = GxsServiceStatistic(); // clears everything

        for(auto& it:  mCachedGroupStats)
        {
            const GxsGroupStatistic& s(it.second);

            stats.mNumMsgs             += s.mNumMsgs;
            stats.mNumGrps             += 1;
            stats.mSizeOfMsgs          += s.mTotalSizeOfMsgs;
            stats.mNumThreadMsgsNew    += s.mNumThreadMsgsNew;
            stats.mNumThreadMsgsUnread += s.mNumThreadMsgsUnread;
            stats.mNumChildMsgsNew     += s.mNumChildMsgsNew ;
            stats.mNumChildMsgsUnread  += s.mNumChildMsgsUnread ;
            std::cout<<stats.mNumMsgs  <<std::endl;
            std::cout<<stats.mNumGrps  <<std::endl;
            std::cout<<stats.mSizeOfMsgs  <<std::endl;
            std::cout<<stats.mNumThreadMsgsNew  <<std::endl;
            std::cout<<stats.mNumThreadMsgsUnread  <<std::endl;
            std::cout<<stats.mNumChildMsgsNew  <<std::endl;
            std::cout<<stats.mNumChildMsgsUnread   <<std::endl;
        }

        // Also save the service statistics in conf file, so that we can display it right away at start.

        Settings->beginGroup(mSettingsName);
        Settings->setValue("NumMsgs",            stats.mNumMsgs            );
        Settings->setValue("NumGrps",            stats.mNumGrps            );
        Settings->setValue("SizeOfMessages",     stats.mSizeOfMsgs         );
        Settings->setValue("NumThreadMsgsNew",   stats.mNumThreadMsgsNew   );
        Settings->setValue("NumThreadMsgsUnread",stats.mNumThreadMsgsUnread);
        Settings->setValue("NumChildMsgsNew",    stats.mNumChildMsgsNew    );
        Settings->setValue("NumChildMsgsUnread", stats.mNumChildMsgsUnread );
        Settings->endGroup();
    }
    else	// Get statistics from settings if no cache is already present: allows to display at start.
    {
        Settings->beginGroup(mSettingsName);

        stats.mNumMsgs             = Settings->value("NumMsgs",QVariant(0)).toInt();
        stats.mNumGrps             = Settings->value("NumGrps",QVariant(0)).toInt();
        stats.mSizeOfMsgs          = Settings->value("SizeOfMessages",QVariant(0)).toInt();
        stats.mNumThreadMsgsNew    = Settings->value("NumThreadMsgsNew",QVariant(0)).toInt();
        stats.mNumThreadMsgsUnread = Settings->value("NumThreadMsgsUnread",QVariant(0)).toInt();
        stats.mNumChildMsgsNew     = Settings->value("NumChildMsgsNew",QVariant(0)).toInt();
        stats.mNumChildMsgsUnread  = Settings->value("NumChildMsgsUnread",QVariant(0)).toInt();

        Settings->endGroup();
    }
}

void WireDialog::updateGroupStatistics(const RsGxsGroupId &groupId)
{
    mGroupStatisticsToUpdate.insert(groupId);
    mShouldUpdateGroupStatistics = true;
}

void WireDialog::updateGroupStatisticsReal(const RsGxsGroupId &groupId)
{
    RsThread::async([this,groupId]()
    {
        GxsGroupStatistic stats;

        if(! getGroupStatistics(groupId, stats))
        {
            std::cerr << __PRETTY_FUNCTION__ << " failed to collect group statistics for group " << groupId << std::endl;
            return;
        }

        RsQThreadUtils::postToObject( [this,stats, groupId]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete, note that
             * Qt::QueuedConnection is important!
             */

//            QTreeWidgetItem *item = ui.scrollAreaWidgetContents->getItemFromId(QString::fromStdString(stats.mGrpId.toStdString()));

//            if (item)
//                ui.scrollAreaWidgetContents->setUnreadCount(item, mCountChildMsgs ? (stats.mNumThreadMsgsUnread + stats.mNumChildMsgsUnread) : stats.mNumThreadMsgsUnread);

            mCachedGroupStats[groupId] = stats;

            getUserNotify()->updateIcon();

        }, this );
    });
}

bool WireDialog::navigate(const RsGxsGroupId &groupId, const RsGxsMessageId& msgId)
{
    if (groupId.isNull()) {
        return false;
    }

//    if (mStateHelper->isLoading(TOKEN_TYPE_GROUP_SUMMARY)) {
//        mNavigatePendingGroupId = groupId;
//        mNavigatePendingMsgId = msgId;

//        /* No information if group is available */
//        return true;
//    }

//    QString groupIdString = QString::fromStdString(groupId.toStdString());
//    if (ui.groupTreeWidget->activateId(groupIdString, msgId.isNull()) == NULL) {
//        return false;
//    }

//    changedCurrentGroup(groupIdString);

    /* search exisiting tab */
//    GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId);
//    if (!msgWidget) {
//        return false;
//    }

//    if (msgId.isNull()) {
//        return true;
//    }

//    return msgWidget->navigate(msgId);
    return true;
}

GxsMessageFrameWidget *WireDialog::messageWidget(const RsGxsGroupId &groupId)
{
//    int tabCount = ui.tabWidget->count();

//    for (int index = 0; index < tabCount; ++index)
//    {
//        GxsMessageFrameWidget *childWidget = dynamic_cast<GxsMessageFrameWidget*>(ui.tabWidget->widget(index));

//        if (childWidget && childWidget->groupId() == groupId)
//            return childWidget;
//    }

    return NULL;
}

//void GxsGroupFrameDialog::changedCurrentGroup(const QString& groupId)
//{
//	if (mInFill) {
//		return;
//	}

//	if (groupId.isEmpty())
//    {
//        auto w = currentWidget();

//        if(w)
//			w->setGroupId(RsGxsGroupId());

//		return;
//	}

//	mGroupId = RsGxsGroupId(groupId.toStdString());

//	if (mGroupId.isNull())
//		return;

//	/* search exisiting tab */
//	GxsMessageFrameWidget *msgWidget = messageWidget(mGroupId);

//    // check that we have at least one tab

//	if(msgWidget)
//		ui.messageTabWidget->setCurrentWidget(msgWidget);
//    else
//    {
//        if(useTabs() || ui.messageTabWidget->count()==0)
//        {
//			msgWidget = createMessageWidget(RsGxsGroupId(groupId.toStdString()));
//			ui.messageTabWidget->setCurrentWidget(msgWidget);
//		}
//        else
//			currentWidget()->setGroupId(mGroupId);
//	}
//}

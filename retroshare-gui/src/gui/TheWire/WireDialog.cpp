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

#include "PulseViewGroup.h"
#include "PulseReplySeperator.h"

#include "util/qtthreadsutils.h"

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


/** Constructor */
WireDialog::WireDialog(QWidget *parent)
    : MainPage(parent), mGroupSet(GROUP_SET_ALL)
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
        switch(e->mWireEventCode)
        {
        case RsWireEventCode::NEW_POST:
            refreshGroups();
            break;
        case RsWireEventCode::NEW_REPLY:
            refreshGroups();
            break;
        case RsWireEventCode::NEW_LIKE:
            refreshGroups();
            break;
        case RsWireEventCode::NEW_REPUBLISH:
            refreshGroups();
            break;
        case RsWireEventCode::POST_UPDATED:
            refreshGroups();
            break;
        case RsWireEventCode::NEW_WIRE:
            refreshGroups();
            break;
        default:
            break;

        }
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

void WireDialog::processSettings(bool load)
{
	Settings->beginGroup("WireDialog");

	if (load) {
		// load settings

		// state of splitter
		ui.splitter->restoreState(Settings->value("SplitterWire").toByteArray());
	} else {
		// save settings

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
        std::cerr << "This is mMeta.mMsgId : " << reply->mMeta.mMsgId;
        std::cerr << std::endl;
        std::cerr << "This is mMeta.mMsgName : " << reply->mMeta.mMsgName;
        std::cerr << std::endl;

		PulseReply *firstReply = new PulseReply(this, reply);
        addTwitterView(firstReply);
        firstReply->showReplyLine(false);

        addTwitterView(new PulseReplySeperator());

	}
}


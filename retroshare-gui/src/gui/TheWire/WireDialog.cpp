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
{
	ui.setupUi(this);

	mAddDialog = NULL;
	mPulseSelected = NULL;
	mGroupSelected = NULL;

	connect( ui.toolButton_createAccount, SIGNAL(clicked()), this, SLOT(createGroup()));
	connect( ui.toolButton_createPulse, SIGNAL(clicked()), this, SLOT(createPulse()));
	connect( ui.toolButton_refresh, SIGNAL(clicked()), this, SLOT(refreshGroups()));

	connect(ui.comboBox_groupSet, SIGNAL(currentIndexChanged(int)), this, SLOT(selectGroupSet(int)));
	connect(ui.comboBox_filterTime, SIGNAL(currentIndexChanged(int)), this, SLOT(selectFilterTime(int)));

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);

	/* setup TokenQueue */
	mWireQueue = new TokenQueue(rsWire->getTokenService(), this);

	requestGroupData();
}

void WireDialog::refreshGroups()
{
	requestGroupData();
}


void WireDialog::addGroup(QWidget *item)
{
	QLayout *alayout = ui.scrollAreaWidgetContents_groups->layout();
	alayout->addWidget(item);
}

// PulseHolder interface.
void WireDialog::deletePulseItem(PulseItem * /* item */, uint32_t /* type */)
{
	return;
}


	// Actions from PulseHolder.
void WireDialog::follow(RsGxsGroupId &groupId)
{
	std::cerr << "WireDialog::follow(";
	std::cerr << groupId.toStdString();
	std::cerr << ")";
	std::cerr << std::endl;
}

void WireDialog::rate(RsGxsId &authorId)
{
	std::cerr << "WireDialog::rate(";
	std::cerr << authorId.toStdString();
	std::cerr << ")";
	std::cerr << std::endl;
}

void WireDialog::reply(RsWirePulse &pulse, std::string &groupName)
{
	std::cerr << "WireDialog::reply(";
	std::cerr << pulse.mMeta.mGroupId.toStdString();
	std::cerr << ",";
	std::cerr << pulse.mMeta.mOrigMsgId.toStdString();
	std::cerr << ")";
	std::cerr << std::endl;

	if (!mAddDialog)
	{
		mAddDialog = new PulseAddDialog(NULL);
		mAddDialog->hide();
	}

	int idx = ui.groupChooser->currentIndex();
	if (idx < 0) {
		std::cerr << "WireDialog::reply() ERROR GETTING AuthorId!";
		std::cerr << std::endl;

		QMessageBox::warning(this, tr("RetroShare"),tr("Please create or choose Wire Groupd first"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	// publishing group.
	RsWireGroup group = mOwnGroups[idx];
	mAddDialog->cleanup();
	mAddDialog->setGroup(group);

	// establish replyTo.
	mAddDialog->setReplyTo(pulse, groupName);

	mAddDialog->show();
}

void WireDialog::notifyPulseSelection(PulseItem *item)
{
	if (mPulseSelected)
	{
		std::cerr << "WireDialog::notifyPulseSelection() unselecting old one : " << mPulseSelected;
		std::cerr << std::endl;
	
		mPulseSelected->setSelected(false);
	}
	mPulseSelected = item;
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
	mAddDialog->setGroup(group);
	mAddDialog->show();
}

void WireDialog::addPulse(RsWirePulse *pulse, RsWireGroup *group,
						std::map<rstime_t, RsWirePulse *> replies)
{
	std::cerr << "WireDialog::addPulse() GroupId : " << pulse->mMeta.mGroupId;
	std::cerr << " OrigMsgId : " << pulse->mMeta.mOrigMsgId;
	std::cerr << " Replies : " << replies.size();
	std::cerr << std::endl;

	PulseItem *pulseItem = new PulseItem(this, pulse, group, replies);

	/* ensure its a boxlayout */
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	QBoxLayout *boxlayout = dynamic_cast<QBoxLayout *>(alayout);
	if (boxlayout == NULL) {
		std::cerr << "WireDialog::addPulse() ERROR not boxlayout, inserting at end";
		std::cerr << std::endl;
		alayout->addWidget(pulseItem);
		return;
	}

	/* iterate through layout, and insert at the correct time */
	for(int i = 0; i < alayout->count(); i++)
	{
		QLayoutItem *layoutItem = boxlayout->itemAt(i);
		PulseItem *pitem = dynamic_cast<PulseItem *>(layoutItem->widget());
		if (pitem != NULL)
		{
			if (pitem->publishTs() < pulseItem->publishTs())
			{
				std::cerr << "WireDialog::addPulse() Inserting at index: " << i;
				std::cerr << std::endl;
				/* insert at this index */
				boxlayout->insertWidget(i, pulseItem);
				return;
			}
		}
	}
	// last item.
	std::cerr << "WireDialog::addPulse() Inserting at end";
	std::cerr << std::endl;
	boxlayout->addWidget(pulseItem);
}

void WireDialog::addGroup(const RsWireGroup &group)
{
	std::cerr << "WireDialog::addGroup() GroupId : " << group.mMeta.mGroupId;
	std::cerr << std::endl;

	addGroup(new WireGroupItem(this, group));
}

void WireDialog::deletePulses()
{
	std::cerr << "WireDialog::deletePulses()";
	std::cerr << std::endl;

	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	QLayoutItem *item;
	int i = 0;
	while (i < alayout->count())
	{
		item = alayout->itemAt(i);
		QWidget *widget = item->widget();
		if (NULL != dynamic_cast<PulseItem *>(widget))
		{
			std::cerr << "WireDialog::deletePulses() Removing Item at: " << i;
			std::cerr << std::endl;

			item = alayout->takeAt(i);
			delete item->widget();
			delete item;
		}
		else
		{
			std::cerr << "WireDialog::deletePulses() Leaving Item at: " << i;
			std::cerr << std::endl;

			i++;
		}
	}
}
	
void WireDialog::deleteGroups()
{
	std::cerr << "WireDialog::deleteGroups()";
	std::cerr << std::endl;

	mGroupSelected = NULL;

	QLayout *alayout = ui.scrollAreaWidgetContents_groups->layout();
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

	std::vector<RsWireGroup>::const_iterator it;
	for(it = groups.begin(); it != groups.end(); it++) {
		// save list of all groups.
		mAllGroups[it->mMeta.mGroupId] = *it;

		if (it->mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
		{
			// grab own groups.
			// setup Chooser too.
			mOwnGroups.push_back(*it);
			ui.groupChooser->addItem(QString::fromStdString(it->mMeta.mGroupName));
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
		deletePulses();
		// request data.
		std::list<RsGxsGroupId> grpIds;
		grpIds.push_back(mGroupSelected->groupId());
		requestPulseData(grpIds);
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
	deletePulses();



	/* depends on the comboBox */
	std::map<RsGxsGroupId, RsWireGroup>::const_iterator it;
	for (it = mAllGroups.begin(); it != mAllGroups.end(); it++)
	{
		bool add = (mGroupSet == GROUP_SET_ALL);
		if (it->second.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) {
			if (mGroupSet == GROUP_SET_OWN) {
				add = true;
			}
		}
		else if (it->second.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED) {
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
			addGroup(it->second);
			// request data.
			std::list<RsGxsGroupId> grpIds;
			grpIds.push_back(it->second.mMeta.mGroupId);
			requestPulseData(grpIds);
		}
	}
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

void WireDialog::requestPulseData(const std::list<RsGxsGroupId>& grpIds)
{
	std::cerr << "WireDialog::requestPulseData()";
	std::cerr << std::endl;

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_LATEST;
	uint32_t token;
	mWireQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds, 0);
}


/* LoadPulseData...
 *
 * group into threads, using std::map<RsGxsMessageId, PulseReplySet>
 * then sort by publishTS, using std::map<time, PulseOrderedReply>
 * then add into gui.
 * - use pointers to avoid copying everywhere.
 *
 * should we mutex Groups, or copy so we don't lose a pointer?
 * should be fine, as mAllGroups only modified from loadData calls.
 ******
 *
 * NB: Potentially, this should be changed to use GXS to do the bulk of the work.
 * - request Top-Level Msgs, sorted by PublishTS.
 *   - Insert into GUI.
 *   - for each request children Msg, and fill in "replies"
 *
 * This needs sorted option on GXS Data fetch.
 */

class PulseReplySet
{
public:
    PulseReplySet() : group(NULL), msg(NULL) {}
    PulseReplySet(RsWirePulse *m, RsWireGroup *g)
	: group(g), msg(m) {}

	RsWireGroup *group;
	RsWirePulse *msg;
	std::map<RsGxsMessageId, RsWirePulse *> replies; // orig ID -> replies.
};

class PulseOrderedReply
{
public:
    PulseOrderedReply() : group(NULL), msg(NULL) {}
    PulseOrderedReply(RsWirePulse *m, RsWireGroup *g)
	: group(g), msg(m) {}

	RsWireGroup *group;
	RsWirePulse *msg;
	std::map<rstime_t, RsWirePulse *> replies; // publish -> replies.
};

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

bool WireDialog::loadPulseData(const uint32_t &token)
{
	std::cerr << "WireDialog::loadPulseData()";
	std::cerr << std::endl;

	std::vector<RsWirePulse> pulses;
	rsWire->getPulseData(token, pulses);

	std::list<RsWirePulse *> references;
	std::map<RsGxsMessageId, PulseReplySet> pulseGrouping;

	// setup time filtering.
	uint32_t filterTimestamp;
	bool filterTime = (ui.comboBox_filterTime->currentIndex() > 0);
	if (filterTime) 
	{
		filterTimestamp = getFilterTimestamp();
	}

	std::vector<RsWirePulse>::iterator vit = pulses.begin();
	for(; vit != pulses.end(); vit++)
	{
		RsWirePulse& pulse = *vit;
		if (pulse.mPulseType & WIRE_PULSE_TYPE_REPLY_REFERENCE)
		{
			// store references to add in later.
			std::cerr << "WireDialog::loadPulseData() REF: GroupId: " << pulse.mMeta.mGroupId;
			std::cerr << " PulseId: " << pulse.mMeta.mMsgId;
			std::cerr << std::endl;
			references.push_back(&pulse);
		}
		else
		{
			// Filter timestamp now. (as soon as possible).
			if (filterTime && (pulse.mMeta.mPublishTs < filterTimestamp))
			{
				std::cerr << "WireDialog::loadPulseData() SKipping OLD MSG: GroupId: " << pulse.mMeta.mGroupId;
				std::cerr << " PulseId: " << pulse.mMeta.mMsgId;
				std::cerr << std::endl;
				continue;
			}

			RsGxsGroupId &gid = pulse.mMeta.mGroupId;
			std::map<RsGxsGroupId, RsWireGroup>::iterator git = mAllGroups.find(gid);
			if (git != mAllGroups.end())
			{
				RsWireGroup &group = git->second;
				std::cerr << "WireDialog::loadPulseData() MSG: GroupId: " << pulse.mMeta.mGroupId;
				std::cerr << " PulseId: " << pulse.mMeta.mMsgId;
				std::cerr << std::endl;

				// install into pulseGrouping.
				pulseGrouping[pulse.mMeta.mOrigMsgId] = PulseReplySet(&pulse, &group);
			}
			else
			{
				std::cerr << "WireDialog::loadPulseData() ERROR Missing GroupId: " << pulse.mMeta.mGroupId;
				std::cerr << " PulseId: " << pulse.mMeta.mMsgId;
				std::cerr << std::endl;
			}
		}
	}

	// add references.
	std::list<RsWirePulse *>::iterator lrit;
	for(lrit = references.begin(); lrit != references.end(); lrit++)
	{
		std::map<RsGxsMessageId, PulseReplySet>::iterator pgit;
		pgit = pulseGrouping.find((*lrit)->mMeta.mThreadId);
		if (pgit != pulseGrouping.end())
		{
			// install into reply map.
			// TODO handle Edits / Latest MSGS.
			std::map<RsGxsMessageId, RsWirePulse *>::iterator rmit;
			rmit = pgit->second.replies.find((*lrit)->mMeta.mOrigMsgId);
			if (rmit == pgit->second.replies.end())
			{
				std::cerr << "WireDialog::loadPulseData() Installing REF: " << (*lrit)->mMeta.mOrigMsgId;
				std::cerr << " to threadId: " << (*lrit)->mMeta.mThreadId;
				std::cerr << std::endl;
				pgit->second.replies[(*lrit)->mMeta.mOrigMsgId] = (*lrit);
			}
			else
			{
				std::cerr << "WireDialog::loadPulseData() ERROR Duplicate reply REF: " << (*lrit)->mMeta.mOrigMsgId;
				std::cerr << std::endl;
			}
		}
		else
		{
			// no original msg for REF.
			std::cerr << "WireDialog::loadPulseData() ERROR No matching ThreadId REF: " << (*lrit)->mMeta.mThreadId;
			std::cerr << std::endl;
		}
	}
	references.clear();

	// sort by publish time.
	std::map<rstime_t, PulseOrderedReply> pulseOrdering;
	std::map<RsGxsMessageId, PulseReplySet>::iterator pgit;
	for(pgit = pulseGrouping.begin(); pgit != pulseGrouping.end(); pgit++)
	{

		PulseOrderedReply &msg = pulseOrdering[pgit->second.msg->mMeta.mPublishTs] = 
			PulseOrderedReply(pgit->second.msg, pgit->second.group);
		std::map<RsGxsMessageId, RsWirePulse *>::iterator rmit;
		for(rmit = pgit->second.replies.begin();
					rmit != pgit->second.replies.end(); rmit++)
		{
			msg.replies[rmit->second->mMeta.mPublishTs] = rmit->second;
		}
	}

	// now add to the GUI.
	std::map<rstime_t, PulseOrderedReply>::reverse_iterator poit;
	for (poit = pulseOrdering.rbegin(); poit != pulseOrdering.rend(); poit++)
	{
		// add into GUI should insert at correct time point, amongst all other ones.
		addPulse(poit->second.msg, poit->second.group, poit->second.replies);
	}

    // allow filterTime to be changed again
	ui.comboBox_filterTime->setEnabled(true);
	return true;
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
					// case RS_TOKREQ_ANSTYPE_LIST:
					//	loadGroupList(req.mToken);
					//	break;
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
			case TOKENREQ_MSGINFO:
				switch(req.mAnsType)
				{
#if 0
					case RS_TOKREQ_ANSTYPE_LIST:
						loadPhotoList(req.mToken);
						break;
					case RS_TOKREQ_ANSTYPE_ACK:
						acknowledgeMessage(req.mToken);
						break;
#endif
					case RS_TOKREQ_ANSTYPE_DATA:
						loadPulseData(req.mToken);
						break;
					default:
						std::cerr << "WireDialog::loadRequest() ERROR: MSG: INVALID ANS TYPE";
						std::cerr << std::endl;
						break;
				}
				break;
#if 0
			case TOKENREQ_MSGRELATEDINFO:
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_DATA:
						loadPhotoData(req.mToken);
						break;
					default:
						std::cerr << "WireDialog::loadRequest() ERROR: MSG: INVALID ANS TYPE";
						std::cerr << std::endl;
						break;
				}
				break;
#endif
			default:
				std::cerr << "WireDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}
}


/**************************** Request / Response Filling of Data ************************/


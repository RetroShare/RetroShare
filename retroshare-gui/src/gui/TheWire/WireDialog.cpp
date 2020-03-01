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


/** Constructor */
WireDialog::WireDialog(QWidget *parent)
: MainPage(parent)
{
	ui.setupUi(this);

	mAddDialog = NULL;
	mPulseSelected = NULL;

	connect( ui.toolButton_createAccount, SIGNAL(clicked()), this, SLOT(createGroup()));
	connect( ui.toolButton_createPulse, SIGNAL(clicked()), this, SLOT(createPulse()));
	connect( ui.pushButton_Post, SIGNAL(clicked()), this, SLOT(createPulse()));
	connect( ui.toolButton_refresh, SIGNAL(clicked()), this, SLOT(refreshGroups()));

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

void WireDialog::addItem(QWidget *item)
{
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	alayout->addWidget(item);
}


void WireDialog::addGroup(QWidget *item)
{
	QLayout *alayout = ui.scrollAreaWidgetContents_groups->layout();
	alayout->addWidget(item);
}

// PulseHolder interface.
void WireDialog::deletePulseItem(PulseItem *item, uint32_t type)
{
	return;
}

void WireDialog::notifySelection(PulseItem *item, int ptype)
{
	std::cerr << "WireDialog::notifySelection() from : " << ptype << " " << item;
	std::cerr << std::endl;

	notifyPulseSelection(item);
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
	mAddDialog->setGroup(group);

	// establish replyTo.
	mAddDialog->setReplyTo(pulse, groupName);

	mAddDialog->show();
}

void WireDialog::notifyPulseSelection(PulseItem *item)
{
	std::cerr << "WireDialog::notifyPulseSelection() from : " << item;
	std::cerr << std::endl;
	
	if (mPulseSelected)
	{
		std::cerr << "WireDialog::notifyPulseSelection() unselecting old one : " << mPulseSelected;
		std::cerr << std::endl;
	
		mPulseSelected->setSelected(false);
	}
	
	mPulseSelected = item;
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
	WireGroupDialog wireCreate(mWireQueue, this);
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

	mAddDialog->setGroup(group);
	mAddDialog->show();
}

void WireDialog::addPulse(RsWirePulse &pulse, RsWireGroup &group)
{
	std::cerr << "WireDialog::addPulse() GroupId : " << pulse.mMeta.mGroupId;
	std::cerr << " MsgId : " << pulse.mMeta.mGroupId;
	std::cerr << std::endl;

	QWidget *item = new PulseItem(this, pulse, group);
	addItem(item);
}

void WireDialog::addGroup(RsWireGroup &group)
{
	std::cerr << "WireDialog::addGroup() GroupId : " << group.mMeta.mGroupId;
	std::cerr << std::endl;

	addGroup(new WireGroupItem(group));
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

	mAllGroups.clear();
	mOwnGroups.clear();
	ui.groupChooser->clear();


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

	deleteGroups();
	deletePulses();

	std::vector<RsWireGroup> groups;
	rsWire->getGroupData(token, groups);

	std::vector<RsWireGroup>::iterator vit = groups.begin();

	for(; vit != groups.end(); ++vit)
	{
		RsWireGroup& group = *vit;

		std::cerr << " WireDialog::addGroup() GroupId: " << group.mMeta.mGroupId << std::endl;

		addGroup(group);

		std::list<RsGxsGroupId> grpIds;
		grpIds.push_back(group.mMeta.mGroupId);
		requestPulseData(grpIds);
	}

	// save list of groups.
	updateGroups(groups);
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

bool WireDialog::loadPulseData(const uint32_t &token)
{
	std::cerr << "WireDialog::loadPulseData()";
	std::cerr << std::endl;

	// clearPulses();

	std::vector<RsWirePulse> pulses;
	rsWire->getPulseData(token, pulses);

	std::vector<RsWirePulse>::iterator vit = pulses.begin();
	for(; vit != pulses.end(); vit++)
	{
		RsWirePulse& pulse = *vit;

		RsGxsGroupId &gid = pulse.mMeta.mGroupId;
		std::map<RsGxsGroupId, RsWireGroup>::iterator mit = mAllGroups.find(gid);
		if (mit != mAllGroups.end())
		{
			RsWireGroup &group = mit->second;
			addPulse(pulse, group);
			std::cerr << "WireDialog::loadPulseData() GroupId: " << pulse.mMeta.mGroupId;
			std::cerr << " PulseId: " << pulse.mMeta.mMsgId;
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "WireDialog::loadPulseData() ERROR Missing GroupId: " << pulse.mMeta.mGroupId;
			std::cerr << " PulseId: " << pulse.mMeta.mMsgId;
			std::cerr << std::endl;
		}
	}

	// updatePulses();
	return true;
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
					// case RS_TOKREQ_ANSTYPE_ACK:
					//	acknowledgeGroup(req.mToken);
					//	break;
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


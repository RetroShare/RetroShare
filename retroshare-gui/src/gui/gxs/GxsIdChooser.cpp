/*
 * Retroshare Gxs Support
 *
 * Copyright 2012-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "GxsIdChooser.h"
#include "GxsIdDetails.h"
#include "RsGxsUpdateBroadcastBase.h"

#include <QTimer>
#include <QSortFilterProxyModel>
#include <algorithm>

#include <retroshare/rspeers.h>

#include <iostream>

#define MAX_TRY  10 // 5 seconds

#define ROLE_SORT Qt::UserRole + 1 // Qt::UserRole is reserved for data
#define ROLE_TYPE Qt::UserRole + 2 //

#define TYPE_NO_ID       1
#define TYPE_FOUND_ID    2
#define TYPE_UNKNOWN_ID  3

#define IDCHOOSER_REFRESH  1

/** Constructor */
GxsIdChooser::GxsIdChooser(QWidget *parent)
: QComboBox(parent), mFlags(IDCHOOSER_ANON_DEFAULT)
{
	mBase = new RsGxsUpdateBroadcastBase(rsIdentity, this);
	connect(mBase, SIGNAL(fillDisplay(bool)), this, SLOT(fillDisplay(bool)));

	mIdQueue = NULL;
	mFirstLoad=true;

	mDefaultId.clear() ;
	mDefaultIdName.clear();
	mTimer = NULL;
	mTimerCount = 0;

	/* Enable sort with own role */
	QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
	proxy->setSourceModel(model());
	model()->setParent(proxy);
	setModel(proxy);

	proxy->setSortRole(ROLE_SORT);
	connect(this, SIGNAL(currentIndexChanged(int)),this,SLOT(myCurrentIndexChanged(int)));

	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);
}

void GxsIdChooser::setFlags(uint32_t flags)
{
    mFlags = flags ;
    updateDisplay(true);
}
GxsIdChooser::~GxsIdChooser()
{
	if (mIdQueue) {
		delete(mIdQueue);
		mIdQueue = NULL;
	}
}

void GxsIdChooser::setUpdateWhenInvisible(bool update)
{
	mBase->setUpdateWhenInvisible(update);
}

const std::list<RsGxsGroupId> &GxsIdChooser::getGrpIds()
{
	return mBase->getGrpIds();
}

const std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &GxsIdChooser::getMsgIds()
{
	return mBase->getMsgIds();
}

void GxsIdChooser::fillDisplay(bool complete)
{
	updateDisplay(complete);
	update(); // Qt flush
}

void GxsIdChooser::showEvent(QShowEvent *event)
{
	mBase->showEvent(event);
	QComboBox::showEvent(event);
}


void GxsIdChooser::loadIds(uint32_t chooserFlags, RsGxsId defId)
{
	mFlags = chooserFlags;
	mDefaultId = defId;
	clear();
	mFirstLoad = true;
}

bool GxsIdChooser::makeIdDesc(const RsGxsId &gxsId, QString &desc)
{
	std::list<QIcon> icons;
	QString comment ;
	if (!GxsIdDetails::MakeIdDesc(gxsId, false, desc, icons,comment)) {
		if (mTimerCount > MAX_TRY) {
			desc = QString("%1 ... [").arg(tr("Not found"));
			desc += QString::fromStdString(gxsId.toStdString().substr(0,5));
			desc += "...]";
        }
		return false;
    }
	return true;
}

void GxsIdChooser::addPrivateId(const RsGxsId &gxsId, bool replace)
{
	QString str;
	bool found = makeIdDesc(gxsId, str);
	if (!found) {
		/* Add to pending id's */
		mPendingId.push_back(gxsId);
		if (replace && mTimerCount <= MAX_TRY) {
			/* Retry */
			return;
        }
    }

    QString id = QString::fromStdString(gxsId.toStdString());

	if (replace) {
		/* Find and replace text of exisiting item */
		int index = findData(id);
		if (index >= 0) {
			setItemText(index, str);
			setItemData(index, QString("%1_%2").arg(found ? "1" : "2").arg(str), ROLE_SORT);
			setItemData(index, found ? TYPE_FOUND_ID : TYPE_UNKNOWN_ID, ROLE_TYPE);
			model()->sort(0);
		return;
        }
		//If not found create a new item.
    }

	/* Add new item */
	addItem(str, id);
	setItemData(count() - 1, QString("%1_%2").arg(found ? "1" : "2").arg(str), ROLE_SORT);
	setItemData(count() - 1, found ? TYPE_FOUND_ID : TYPE_UNKNOWN_ID, ROLE_TYPE);
	model()->sort(0);
}

void GxsIdChooser::loadPrivateIds(uint32_t token)
{
	mPendingId.clear();
	if (mFirstLoad)	{	clear();}
	mTimerCount = 0;
	if (mTimer) {
		delete(mTimer);
    }

	std::list<RsGxsId> ids;
	//rsIdentity->getOwnIds(ids);
	std::vector<RsGxsIdGroup> datavector;
	if (!rsIdentity->getGroupData(token, datavector)) {
		std::cerr << "GxsIdChooser::loadPrivateIds() Error getting GroupData";
		std::cerr << std::endl;
		return;
    }

	for (std::vector<RsGxsIdGroup>::iterator vit = datavector.begin();
	     vit != datavector.end(); ++vit) {
		RsGxsIdGroup data = (*vit);
		if (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) {
			ids.push_back((RsGxsId) data.mMeta.mGroupId);
        }

		if (mDefaultIdName == data.mMeta.mGroupName) {
			mDefaultId=(RsGxsId) data.mMeta.mGroupId;
        }
    }

	//rsIdentity->getDefaultId(defId);
	// Prefer to use an application specific default???

	if (mFirstLoad) {
		if (!(mFlags & IDCHOOSER_ID_REQUIRED)) {
		/* add No Signature option */
		QString str = tr("No Signature");
		QString id = "";

		addItem(str, id);
		setItemData(count() - 1, QString("0_%2").arg(str), ROLE_SORT);
			setItemData(count() - 1, TYPE_NO_ID, ROLE_TYPE);
        }
    }

	if (!mFirstLoad) {
		for (int idx=0; idx < count(); ++idx) {
			QVariant type = itemData(idx,ROLE_TYPE);
			switch (type.toInt()) {
				case TYPE_NO_ID:
				break;
				case TYPE_FOUND_ID:
				case TYPE_UNKNOWN_ID:
				default: {
					QVariant var = itemData(idx);
					RsGxsId gxsId = RsGxsId(var.toString().toStdString());
					std::list<RsGxsId>::iterator lit = std::find( ids.begin(), ids.end(), gxsId);
					if (lit == ids.end()) {
						removeItem(idx);
						idx--;
                    }
                }
            }
        }
    }

	if (ids.empty()) {
		std::cerr << "GxsIdChooser::loadPrivateIds() ERROR no ids";
		std::cerr << std::endl;
		mFirstLoad = false;
		return;
    }

	for(std::list<RsGxsId>::iterator it = ids.begin(); it != ids.end(); ++it) {
		/* add to Chooser */
		addPrivateId(*it, !mFirstLoad);
    }

	if (!mPendingId.empty()) {
		/* Create and start timer to load pending id's */
		mTimerCount = 0;
		mTimer = new QTimer();
		mTimer->setSingleShot(true);
		mTimer->setInterval(500);
		connect(mTimer, SIGNAL(timeout()), this, SLOT(timer()));
		mTimer->start();
    }

	setDefaultItem();

	mFirstLoad=false;
}

void GxsIdChooser::setDefaultItem()
{
	int def = -1;

	if ((mFlags & IDCHOOSER_ANON_DEFAULT) && !(mFlags & IDCHOOSER_ID_REQUIRED)) {
		def = findData(TYPE_NO_ID, ROLE_TYPE);
	} else {
		QString id = QString::fromStdString(mDefaultId.toStdString());
		def = findData(id);
    }

	if (def >= 0) {
		setCurrentIndex(def);
    }

}

bool GxsIdChooser::setChosenId(RsGxsId &gxsId)
	{
	QString id = QString::fromStdString(gxsId.toStdString());

	/* Find text of exisiting item */
	int index = findData(id);
	if (index >= 0) {
		setCurrentIndex(index);
		return true;
    }
		return false;
	}

GxsIdChooser::ChosenId_Ret GxsIdChooser::getChosenId(RsGxsId &gxsId)
{
	if (count() < 1) {
		return None;
    }

	int idx = currentIndex();

	QVariant var = itemData(idx);
	gxsId = RsGxsId(var.toString().toStdString());
	QVariant type = itemData(idx,ROLE_TYPE);
	switch (type.toInt()) {
		case TYPE_NO_ID:
		return NoId;
		case TYPE_FOUND_ID:
		return KnowId;
		case TYPE_UNKNOWN_ID:
		return UnKnowId;
    }

	return None;
}

void GxsIdChooser::myCurrentIndexChanged(int index)
{
	Q_UNUSED(index)
	QFontMetrics fm = QFontMetrics(font());
	QString text = currentText();
	if (width()<fm.boundingRect(text).width()) {
		setToolTip(text);
	} else {
		setToolTip("");
    }
}
		
void GxsIdChooser::timer()
{
	++mTimerCount;

	QList<RsGxsId> pendingId = mPendingId;
	mPendingId.clear();

	/* Process pending id's */
	while (!pendingId.empty()) {
		RsGxsId id = pendingId.front();
		pendingId.pop_front();

		addPrivateId(id, true);
    }

	setDefaultItem();

	if (mPendingId.empty()) {
		/* All pending id's processed */
		delete(mTimer);
		mTimer = NULL;
		mTimerCount = 0;
	} else {//if (mPendingId.empty())
		if (mTimerCount <= MAX_TRY) {
			/* Restart timer */
			mTimer->start();
		} else {//if (mTimerCount <= MAX_TRY)
			delete(mTimer);
			mTimer = NULL;
			mTimerCount = 0;
			mPendingId.clear();
        }
    }
		}

void GxsIdChooser::updateDisplay(bool complete)
{
	Q_UNUSED(complete)
	/* Update identity list */
	requestIdList();
	}

void GxsIdChooser::requestIdList()
{
	if (!mIdQueue) return;

	mIdQueue->cancelActiveRequestTokens(IDCHOOSER_REFRESH);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;

	mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, IDCHOOSER_REFRESH);
}

void GxsIdChooser::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	Q_UNUSED(queue)
	std::cerr << "IdDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

	switch(req.mUserType) {
		case IDCHOOSER_REFRESH:
			insertIdList(req.mToken);
		break;
		default:
			std::cerr << "IdDialog::loadRequest() ERROR";
			std::cerr << std::endl;
		break;
    }
}

void GxsIdChooser::insertIdList(uint32_t token)
{
	loadPrivateIds(token);
}

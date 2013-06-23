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

#include <QTimer>
#include <QSortFilterProxyModel>
#include <algorithm>

#include <retroshare/rspeers.h>

#include <iostream>

#define MAX_TRY  10 // 5 seconds

#define ROLE_SORT Qt::UserRole + 1 // Qt::UserRole is reserved for data

/** Constructor */
GxsIdChooser::GxsIdChooser(QWidget *parent)
: QComboBox(parent), mFlags(IDCHOOSER_ANON_DEFAULT), mDefaultId("")
{
	mTimer = NULL;
	mTimerCount = 0;

	/* Enable sort with own role */
	QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
	proxy->setSourceModel(model());
	model()->setParent(proxy);
	setModel(proxy);

	proxy->setSortRole(ROLE_SORT);
}

void GxsIdChooser::loadIds(uint32_t chooserFlags, RsGxsId defId)
{
	mFlags = chooserFlags;
	mDefaultId = defId;
	clear();
	loadPrivateIds();
}

bool GxsIdChooser::MakeIdDesc(const RsGxsId &id, QString &desc)
{
	RsIdentityDetails details;

	bool found = rsIdentity->getIdDetails(id, details);
	if (found)
	{
		desc = QString::fromUtf8(details.mNickname.c_str());
		if (details.mPgpLinked)
		{
			desc += " (PGP) [";
		}
		else
		{
			desc += " (Anon) [";
		}
	} else {
		if (mTimerCount <= MAX_TRY) {
			desc = QString("%1 ... [").arg(tr("Loading"));
		} else {
			desc = QString("%1 ... [").arg(tr("Not found"));
		}
	}

	desc += QString::fromStdString(id.substr(0,5));
	desc += "...]";

	return found;
}

void GxsIdChooser::addPrivateId(const RsGxsId &gxsId, bool replace)
{
	QString str;
	bool found = MakeIdDesc(gxsId, str);
	if (!found)
	{
		/* Add to pending id's */
		mPendingId.push_back(gxsId);
		if (replace && mTimerCount <= MAX_TRY) {
			/* Retry */
			return;
		}
	}

	QString id = QString::fromStdString(gxsId);

	if (replace) {
		/* Find and replace text of exisiting item */
		int index = findData(id);
		if (index >= 0) {
			setItemText(index, str);
			setItemData(index, QString("%1_%2").arg(found ? "1" : "2").arg(str), ROLE_SORT);
			model()->sort(0);
		}
		return;
	}

	/* Add new item */
	addItem(str, id);
	setItemData(count() - 1, QString("%1_%2").arg(found ? "1" : "2").arg(str), ROLE_SORT);
	model()->sort(0);
}

void GxsIdChooser::loadPrivateIds()
{
	mPendingId.clear();
	mTimerCount = 0;
	if (mTimer) {
		delete(mTimer);
	}

	std::list<RsGxsId> ids;
	rsIdentity->getOwnIds(ids);

	if (ids.empty())
	{
		std::cerr << "GxsIdChooser::loadPrivateIds() ERROR no ids";
		std::cerr << std::endl;
		return;
	}	

	//rsIdentity->getDefaultId(defId);
	// Prefer to use an application specific default???
	int def = -1;

	if (!(mFlags & IDCHOOSER_ID_REQUIRED))
	{
		/* add No Signature option */
		QString str = tr("No Signature");
		QString id = "";

		addItem(str, id);
		setItemData(count() - 1, QString("0_%2").arg(str), ROLE_SORT);
		if (mFlags & IDCHOOSER_ANON_DEFAULT)
		{
			def = 0;
		}
	}
	
	int i = 1;	
	std::list<RsGxsId>::iterator it;
	for(it = ids.begin(); it != ids.end(); it++, i++)
	{
		/* add to Chooser */
		addPrivateId(*it, false);

		if (mDefaultId == *it)
		{
			def = i;
		}
	}

	if (def >= 0)
	{
		setCurrentIndex(def);
		//ui.comboBox->setCurrentIndex(def);
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
}

bool GxsIdChooser::getChosenId(RsGxsId &id)
{
	if (count() < 1)
	{
		return false;
	}

	int idx = currentIndex();

	QVariant var = itemData(idx);
	id = var.toString().toStdString();

	return true;
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

	if (mPendingId.empty()) {
		/* All pending id's processed */
		delete(mTimer);
		mTimer = NULL;
		mTimerCount = 0;
	} else {
		if (mTimerCount <= MAX_TRY) {
			/* Restart timer */
			mTimer->start();
		} else {
			delete(mTimer);
			mTimer = NULL;
			mTimerCount = 0;
			mPendingId.clear();
		}
	}
}

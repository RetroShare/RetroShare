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

#include "GxsIdTreeWidgetItem.h"
#include "rshare.h"

#include <retroshare/rspeers.h>

#include <iostream>

#define MAX_ATTEMPTS 5

static bool MakeIdDesc(const RsGxsId &id, QString &str)
{
	RsIdentityDetails details;
	
	if (!rsIdentity->getIdDetails(id, details))
	{
		std::cerr << "GxsIdRSTreeWidgetItem::MakeIdDesc() FAILED TO GET ID";
		std::cerr << std::endl;

		str = "Loading... " + QString::fromStdString(id.substr(0,5));
		return false;
	}

	str = QString::fromUtf8(details.mNickname.c_str());

	std::list<RsRecognTag>::iterator it;
	for(it = details.mRecognTags.begin(); it != details.mRecognTags.end(); it++)
	{
		str += " (";
		str += QString::number(it->tag_class);
		str += ":";
		str += QString::number(it->tag_type);
		str += ")";
	}


	bool addCode = true;
	if (details.mPgpLinked)
	{
		str += " (PGP) [";
		if (details.mPgpKnown)
		{
			/* look up real name */
			std::string authorName = rsPeers->getPeerName(details.mPgpId);
			str += QString::fromUtf8(authorName.c_str());
			str += "]";

			addCode = false;
		}
	}
	else
	{
		str += " (Anon) [";
	}

	if (addCode)
	{
		str += QString::fromStdString(id.substr(0,5));
		str += "...]";
	}

	std::cerr << "GxsIdRSTreeWidgetItem::MakeIdDesc() ID Ok";
	std::cerr << std::endl;

	return true;
}

/** Constructor */
GxsIdRSTreeWidgetItem::GxsIdRSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidget *parent)
: QObject(NULL), RSTreeWidgetItem(compareRole, parent), mCount(0), mColumn(0)
{
	init();
}

GxsIdRSTreeWidgetItem::GxsIdRSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidgetItem *parent)
: QObject(NULL), RSTreeWidgetItem(compareRole, parent), mCount(0), mColumn(0)
{
	init();
}

void GxsIdRSTreeWidgetItem::init()
{
}

void GxsIdRSTreeWidgetItem::setId(const RsGxsId &id, int column)
{
	std::cerr << " GxsIdRSTreeWidgetItem::setId(" << id << "," << column << ")";
	std::cerr << std::endl;

	mId = id;
	mColumn = column;
	if (mId == "")
	{
		setText(mColumn, "No Signature");
	}
	else
	{
		loadId();
	}
}

bool GxsIdRSTreeWidgetItem::getId(RsGxsId &id)
{
	id = mId;
	return true;
}

void GxsIdRSTreeWidgetItem::loadId()
{
	disconnect(rApp, SIGNAL(secondTick()), this, SLOT(loadId()));

	std::cerr << " GxsIdRSTreeWidgetItem::loadId() Id: " << mId << ", mCount: " << mCount;
	std::cerr << std::endl;

	mCount++;

	/* try and get details - if not there ... set callback */
	QString desc;
	bool loaded = MakeIdDesc(mId, desc);

	setText(mColumn, desc);

	if (loaded)
	{
		std::cerr << " GxsIdRSTreeWidgetItem::loadId() Loaded Okay";
		std::cerr << std::endl;
		return;
	}

	if (mCount < MAX_ATTEMPTS)
	{
		std::cerr << " GxsIdRSTreeWidgetItem::loadId() Starting Timer for re-try";
		std::cerr << std::endl;

		/* timer event to try again */
		connect(rApp, SIGNAL(secondTick()), this, SLOT(loadId()));
	}
}

/** Constructor */
GxsIdTreeWidgetItem::GxsIdTreeWidgetItem(QTreeWidget *parent)
: QObject(NULL), QTreeWidgetItem(parent), mCount(0), mColumn(0)
{
	init();
}

GxsIdTreeWidgetItem::GxsIdTreeWidgetItem(QTreeWidgetItem *parent)
: QObject(NULL), QTreeWidgetItem(parent), mCount(0), mColumn(0)
{
	init();
}

void GxsIdTreeWidgetItem::init()
{
}

void GxsIdTreeWidgetItem::setId(const RsGxsId &id, int column)
{
	std::cerr << " GxsIdTreeWidgetItem::setId(" << id << "," << column << ")";
	std::cerr << std::endl;

	mId = id;
	mColumn = column;
	if (mId == "")
	{
		setText(mColumn, "No Signature");
	}
	else
	{
		loadId();
	}
}

bool GxsIdTreeWidgetItem::getId(RsGxsId &id)
{
	id = mId;
	return true;
}

void GxsIdTreeWidgetItem::loadId()
{
	disconnect(rApp, SIGNAL(secondTick()), this, SLOT(loadId()));

	std::cerr << " GxsIdTreeWidgetItem::loadId() Id: " << mId << ", mCount: " << mCount;
	std::cerr << std::endl;

	mCount++;

	/* try and get details - if not there ... set callback */
	QString desc;
	bool loaded = MakeIdDesc(mId, desc);

	setText(mColumn, desc);

	if (loaded)
	{
		std::cerr << " GxsIdTreeWidgetItem::loadId() Loaded Okay";
		std::cerr << std::endl;
		return;
	}

	if (mCount < MAX_ATTEMPTS)
	{
		std::cerr << " GxsIdTreeWidgetItem::loadId() Starting Timer for re-try";
		std::cerr << std::endl;

		/* timer event to try again */
		connect(rApp, SIGNAL(secondTick()), this, SLOT(loadId()));
	}
}

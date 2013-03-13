/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License
 *  as published by the Free Software Foundation; either version 2.1
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

#include "GxsIdTreeWidgetItem.h"

#include <retroshare/rspeers.h>

#include <iostream>

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
:RSTreeWidgetItem(compareRole, parent), QObject(NULL), mTimer(NULL), mCount(0), mColumn(0)
{
	init();
}

GxsIdRSTreeWidgetItem::GxsIdRSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidgetItem *parent)
:RSTreeWidgetItem(compareRole, parent), QObject(NULL), mTimer(NULL), mCount(0), mColumn(0)
{
	init();
}

void GxsIdRSTreeWidgetItem::init()
{
	mTimer = new QTimer(this);
	mTimer->setSingleShot(true);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(loadId()));
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


#define MAX_ATTEMPTS 5

void GxsIdRSTreeWidgetItem::loadId()
{
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
		mTimer->setInterval(mCount * 1000);
		mTimer->start();
	}
}


/** Constructor */
GxsIdTreeWidgetItem::GxsIdTreeWidgetItem(QTreeWidget *parent)
:QTreeWidgetItem(parent), QObject(NULL), mTimer(NULL), mCount(0), mColumn(0)
{
	init();
}


GxsIdTreeWidgetItem::GxsIdTreeWidgetItem(QTreeWidgetItem *parent)
:QTreeWidgetItem(parent), QObject(NULL), mTimer(NULL), mCount(0), mColumn(0)
{
	init();
}

void GxsIdTreeWidgetItem::init()
{
	mTimer = new QTimer(this);
	mTimer->setSingleShot(true);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(loadId()));
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
		mTimer->setInterval(mCount * 1000);
		mTimer->start();
	}
}

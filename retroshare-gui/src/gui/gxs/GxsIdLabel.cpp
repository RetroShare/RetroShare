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

#include "GxsIdLabel.h"
#include "GxsIdDetails.h"

#include <algorithm>

#include <retroshare/rspeers.h>

#include <iostream>

/** Constructor */
GxsIdLabel::GxsIdLabel(QWidget *parent)
:QLabel(parent), mTimer(NULL), mCount(0)
{
	mTimer = new QTimer(this);
	mTimer->setSingleShot(true);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(loadId()));

	return;
}

void GxsIdLabel::setId(const RsGxsId &id)
{
	mId = id;
	if (mId == "")
	{
		setText("No Signature");
	}
	else
	{
		loadId();
	}
}

bool GxsIdLabel::getId(RsGxsId &id)
{
	id = mId;
	return true;
}

#define MAX_ATTEMPTS 3

void GxsIdLabel::loadId()
{
	mCount++;

	/* try and get details - if not there ... set callback */
	QString desc;
	std::list<QIcon> icons;
	bool loaded = GxsIdDetails::MakeIdDesc(mId, false, desc, icons);

	setText(desc);

	if (loaded)
	{
		return;
	}

	if (mCount < MAX_ATTEMPTS)
	{
		/* timer event to try again */
		mTimer->setInterval(mCount * 1000);
     		mTimer->start();
	}
}


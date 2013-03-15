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

#include "GxsCircleLabel.h"

#include <algorithm>

#include <retroshare/rspeers.h>

#include <iostream>

/** Constructor */
GxsCircleLabel::GxsCircleLabel(QWidget *parent)
:QLabel(parent), mTimer(NULL), mCount(0)
{
	mTimer = new QTimer(this);
	mTimer->setSingleShot(true);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(loadId()));

	return;
}

void GxsCircleLabel::setCircleId(const RsGxsCircleId &id)
{
	mId = id;
	if (mId == "")
	{
		setText("No Circle");
	}
	else
	{
		loadGxsCircle();
	}
}

bool GxsCircleLabel::getCircleId(RsGxsCircleId &id)
{
	id = mId;
	return true;
}

static bool MakeCircleDesc(const RsGxsCircleId &id, QString &str)
{
	RsGxsCircleDetails details;
	
	if (!rsGxsCircles->getCircleDetails(id, details))
	{
		str = "Loading... " + QString::fromStdString(id.substr(0,5));
		return false;
	}

	str = QString::fromUtf8(details.mCircleName.c_str());

        str += " (Ext) [";
        str += QString::fromStdString(id.substr(0,5));
        str += "...]";

	return true;
}

#define MAX_ATTEMPTS 3

void GxsCircleLabel::loadGxsCircle()
{
	mCount++;

	/* try and get details - if not there ... set callback */
	QString desc;
	bool loaded = MakeCircleDesc(mId, desc);

	setText(desc);

	if (loaded)
	{
		return;
	}

	if (mCount < MAX_ATTEMPTS)
	{
		/* timer event to try again (circles take longer) */
		mTimer->setInterval(mCount * 3000);
     		mTimer->start();
	}
}


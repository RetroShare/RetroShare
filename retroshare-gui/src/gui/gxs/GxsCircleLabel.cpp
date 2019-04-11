/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsCircleLabel.cpp                               *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie   <retroshare.project@gmail.com>       *
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
    if (mId.isNull())
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
        str = "Loading... " + QString::fromStdString(id.toStdString().substr(0,5));
		return false;
	}

	str = QString::fromUtf8(details.mCircleName.c_str());

        str += " (Ext) [";
        str += QString::fromStdString(id.toStdString().substr(0,5));
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


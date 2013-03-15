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

#include "GxsCircleChooser.h"

#include <algorithm>

#include <retroshare/rspeers.h>

#include <iostream>

/** Constructor */
GxsCircleChooser::GxsCircleChooser(QWidget *parent)
: QComboBox(parent), mFlags(0)
{
	return;
}

void GxsCircleChooser::loadCircles(uint32_t chooserFlags)
{
	mFlags = chooserFlags;
	loadGxsCircles();
}


bool MakeGxsCircleDesc(const RsGxsCircleId &id, QString &desc)
{
	RsGxsCircleDetails details;
	
	if (rsGxsCircles->getCircleDetails(id, details))
	{
		desc = QString::fromUtf8(details.mCircleName.c_str());
	}
	else
	{
		desc += "Unknown";
	}

	desc += " (Ext) [";
	desc += QString::fromStdString(id.substr(0,5));
	desc += "...]";

	return true;
}


void GxsCircleChooser::loadGxsCircles()
{
	std::list<RsGxsCircleId> ids;
	rsGxsCircles->getCircleIdList(ids);

	if (ids.empty())
	{
		std::cerr << "GxsCircleChooser::loadGxsCircles() ERROR no ids";
		std::cerr << std::endl;
		return;
	}	

	std::list<RsGxsCircleId>::iterator it;
	for(it = ids.begin(); it != ids.end(); it++)
	{
		/* add to Chooser */
		QString str;
		if (!MakeGxsCircleDesc(*it, str))
		{
			std::cerr << "GxsCircleChooser::loadGxsCircles() ERROR Desc for Id: " << *it;
			std::cerr << std::endl;
			continue;
		}
		QString id = QString::fromStdString(*it);

		addItem(str, id);
	}
}

bool GxsCircleChooser::getChosenCircle(RsGxsCircleId &id)
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
		

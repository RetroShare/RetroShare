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
		

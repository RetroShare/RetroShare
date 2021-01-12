/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsCircleChooser.cpp                             *
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

#include "GxsCircleChooser.h"

#include <algorithm>

#include <retroshare/rspeers.h>

#include <iostream>

/** Constructor */
GxsCircleChooser::GxsCircleChooser(QWidget *parent)
: QComboBox(parent)
{
	return;
}

void GxsCircleChooser::loadCircles(const RsGxsCircleId &defaultId)
{
	mDefaultCircleId = defaultId;
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
    desc += QString::fromStdString(id.toStdString().substr(0,5));
	desc += "...]";

	return true;
}


void GxsCircleChooser::loadGxsCircles()
{
    std::set<RsGxsCircleId> ids;
    rsGxsCircles->getCircleExternalIdList(ids);

	if (ids.empty())
	{
		std::cerr << "GxsCircleChooser::loadGxsCircles() ERROR no ids";
		std::cerr << std::endl;
		return;
	}	

	int i = 0; 
	int def = -1;
    for(auto it(ids.begin()); it != ids.end(); ++it, ++i)
	{
		/* add to Chooser */
		QString str;
		if (!MakeGxsCircleDesc(*it, str))
		{
			std::cerr << "GxsCircleChooser::loadGxsCircles() ERROR Desc for Id: " << *it;
			std::cerr << std::endl;
			continue;
		}
        QString id = QString::fromStdString((*it).toStdString());

		addItem(str, id);

		if (mDefaultCircleId == *it)
			def = i;
	}

	if (def >= 0)
		setCurrentIndex(def);
}

bool GxsCircleChooser::getChosenCircle(RsGxsCircleId &id)
{
	if (count() < 1)
	{
		return false;
	}

	int idx = currentIndex();

	QVariant var = itemData(idx);
	id = RsGxsCircleId(var.toString().toStdString());

	return true;
}
		

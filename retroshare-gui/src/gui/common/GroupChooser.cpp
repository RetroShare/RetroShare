/*******************************************************************************
 * gui/common/GroupChooser.cpp                                                 *
 *                                                                             *
 * Copyright (C) 2013, Robert Fernie   <retroshare.project@gmail.com>          *
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

#include "GroupChooser.h"

#include <gui/common/GroupDefs.h>

#include <retroshare/rspeers.h>

#include <algorithm>
#include <iostream>

/** Constructor */
GroupChooser::GroupChooser(QWidget *parent)
    : RSComboBox(parent), mFlags(0)
{
	return;
}

void GroupChooser::loadGroups(uint32_t chooserFlags, const RsNodeGroupId& defaultId)
{
	mFlags = chooserFlags;
    mDefaultGroupId = defaultId;

    loadGroups();
}


bool GroupChooser::makeNodeGroupDesc(const RsGroupInfo& info, QString &desc)
{
    desc.clear();

    if(info.name.empty())
        desc = tr("[Unknown]") ;
    else
        desc = "\"" + GroupDefs::name(info) + "\"";

    desc += " [" ;
    desc += QString::fromStdString(info.id.toStdString().substr(0,3));
    desc += "...";
    desc += QString::fromStdString(info.id.toStdString().substr(info.id.toStdString().length()-2,2));
    desc += "]";

	return true;
}


void GroupChooser::loadGroups()
{
    std::list<RsGroupInfo> ids;

    rsPeers->getGroupInfoList(ids);

	if (ids.empty())
	{
        std::cerr << "GroupChooser::loadGroups() ERROR no ids";
		std::cerr << std::endl;
		return;
	}	

	int i = 0; 
	int def = -1;
    for( std::list<RsGroupInfo>::iterator it = ids.begin(); it != ids.end(); ++it, ++i)
	{
		/* add to Chooser */
		QString str;
        if (!makeNodeGroupDesc(*it, str))
		{
            std::cerr << "GroupChooser::loadGroups() ERROR Desc for Id: " << it->id;
			std::cerr << std::endl;
			continue;
		}
		QString id = QString::fromStdString((*it).id.toStdString()) ;

		addItem(str, id);

        if (mDefaultGroupId == it->id)
			def = i;
	}

	if (def >= 0)
	{
		setCurrentIndex(def);
	}
}

bool GroupChooser::getChosenGroup(RsNodeGroupId& id)
{
	if (count() < 1)
		return false;

	int idx = currentIndex();

	QVariant var = itemData(idx);
    id = RsNodeGroupId(var.toString().toStdString());

	return true;
}
		

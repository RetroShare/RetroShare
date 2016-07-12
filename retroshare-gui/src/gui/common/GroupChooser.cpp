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

#include "GroupChooser.h"

#include <algorithm>
#include <iostream>

#include <retroshare/rspeers.h>

/** Constructor */
GroupChooser::GroupChooser(QWidget *parent)
: QComboBox(parent), mFlags(0)
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
        desc = "\"" + QString::fromUtf8(info.name.c_str()) + "\"";

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
        QString id = QString::fromStdString(it->id.toStdString());

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
		

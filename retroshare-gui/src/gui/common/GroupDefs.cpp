/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
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

#include <QCoreApplication>
#include <retroshare/rspeers.h>

#include <iostream>
#include <algorithm>

#include "GroupDefs.h"
#include "util/misc.h"

const QString GroupDefs::name(const RsGroupInfo &groupInfo)
{
    if ((groupInfo.flag & RS_GROUP_FLAG_STANDARD) == 0) {
        /* no need to be translated */
        return misc::removeNewLine(groupInfo.name);
    }

    if (groupInfo.id == RS_GROUP_ID_FRIENDS) {
        return qApp->translate("GroupDefs", "Friends");
    }
    if (groupInfo.id == RS_GROUP_ID_FAMILY) {
        return qApp->translate("GroupDefs", "Family");
    }
    if (groupInfo.id == RS_GROUP_ID_COWORKERS) {
        return qApp->translate("GroupDefs", "Co-Workers");
    }
    if (groupInfo.id == RS_GROUP_ID_OTHERS) {
        return qApp->translate("GroupDefs", "Other Contacts");
    }
    if (groupInfo.id == RS_GROUP_ID_FAVORITES) {
        return qApp->translate("GroupDefs", "Favorites");
    }

    std::cerr << "GroupDefs::name: Unknown group id requested " << groupInfo.id;
    return "";
}

static bool sortGroupInfo(const RsGroupInfo &groupInfo1, const RsGroupInfo &groupInfo2)
{
    return (GroupDefs::name(groupInfo1).compare(GroupDefs::name(groupInfo2), Qt::CaseInsensitive) < 0);
}

void GroupDefs::sortByName(std::list<RsGroupInfo> &groupInfoList)
{
    groupInfoList.sort(sortGroupInfo);
}

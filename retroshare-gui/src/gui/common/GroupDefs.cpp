/*******************************************************************************
 * gui/common/GroupDefs.cpp                                                    *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
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

    std::cerr << "GroupDefs::name: Unknown group id requested " << groupInfo.id << std::endl;
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

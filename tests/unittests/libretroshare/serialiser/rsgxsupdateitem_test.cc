/*******************************************************************************
 * unittests/libretroshare/serialiser/rsgxsupdateitem_test.cc                  *
 *                                                                             *
 * Copyright 2013 by Crispy <retroshare.project@gmail.com>                     *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#include <gtest/gtest.h>

#include "support.h"
#include "rsitems/rsgxsupdateitems.h"
#define RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM	0x0010

void init_item(RsGxsGrpUpdateItem& i)
{
	i.clear();
	i.grpUpdateTS = rand()%2424;
    i.peerID = RsPeerId::random();
}

void init_item(RsGxsMsgUpdateItem& i)
{
	i.clear();
    i.peerID = RsPeerId::random();
	int numUpdates = rand()%123;

    i.peerID = RsPeerId::random();
	for(int j=0; j < numUpdates; j++)
	{
        struct RsGxsMsgUpdateItem::MsgUpdateInfo info;
        info.message_count = rand();
        info.time_stamp = rand()%45;
        i.msgUpdateInfos[RsGxsGroupId::random()] = info;
	}
}

void init_item(RsGxsServerGrpUpdateItem& i)
{
	i.clear();
	i.grpUpdateTS = rand()%2424;
}

void init_item(RsGxsServerMsgUpdateItem& i)
{
	i.clear();
    i.grpId = RsGxsGroupId::random();
	i.msgUpdateTS = rand()%4252;
}

bool operator ==(const RsGxsGrpUpdateItem& l, const RsGxsGrpUpdateItem& r)
{
	bool ok = l.grpUpdateTS == r.grpUpdateTS;
	ok &= l.peerID == r.peerID;

	return ok;
}

bool operator ==(const RsGxsMsgUpdateItem::MsgUpdateInfo& l, const RsGxsMsgUpdateItem::MsgUpdateInfo& r)
{
    return (l.message_count == r.message_count) && (l.time_stamp == r.time_stamp);
}

bool operator ==(const RsGxsMsgUpdateItem& l, const RsGxsMsgUpdateItem& r)
{
	bool ok = l.peerID == r.peerID;

    const std::map<RsGxsGroupId, RsGxsMsgUpdateItem::MsgUpdateInfo>& lUp = l.msgUpdateInfos, rUp = r.msgUpdateInfos;

	ok &= lUp.size() == rUp.size();

    std::map<RsGxsGroupId, RsGxsMsgUpdateItem::MsgUpdateInfo>::const_iterator lit = lUp.begin(), rit;

	for(; lit != lUp.end(); lit++)
	{
		RsGxsGroupId key = lit->first;
		if((rit = rUp.find(key)) != rUp.end())
			ok &= lit->second == rit->second;
		else
			return false;
	}

	return ok;
}

bool operator ==(const RsGxsServerGrpUpdateItem& l,
		const RsGxsServerGrpUpdateItem& r)
{
	return l.grpUpdateTS == r.grpUpdateTS;
}

bool operator ==(const RsGxsServerMsgUpdateItem& l,
		const RsGxsServerMsgUpdateItem& r)
{
	bool ok = l.grpId == r.grpId;
	ok &= l.msgUpdateTS == r.msgUpdateTS;
	return ok;
}


TEST(libretroshare_serialiser, RsGxsGrpUpateItem)
{
    test_RsItem<RsGxsGrpUpdateItem,RsGxsUpdateSerialiser>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsGxsMsgUpdateItem,RsGxsUpdateSerialiser>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsGxsServerGrpUpdateItem,RsGxsUpdateSerialiser>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsGxsServerMsgUpdateItem,RsGxsUpdateSerialiser>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

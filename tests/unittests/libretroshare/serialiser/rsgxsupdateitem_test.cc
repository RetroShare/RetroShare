/*
 * rsgxsupdateitem_test.cc
 *
 *  Created on: 9 Dec 2013
 *      Author: crispy
 */

#include <gtest/gtest.h>

#include "support.h"
#include "rsitems/rsgxsupdateitems.h"
#define RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM	0x0010

RsSerialType* init_item(RsGxsGrpUpdateItem& i)
{
	i.clear();
	i.grpUpdateTS = rand()%2424;
    i.peerID = RsPeerId::random();
	return new RsGxsUpdateSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsGxsMsgUpdateItem& i)
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

	return new RsGxsUpdateSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsGxsServerGrpUpdateItem& i)
{
	i.clear();
	i.grpUpdateTS = rand()%2424;

	return new RsGxsUpdateSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsGxsServerMsgUpdateItem& i)
{
	i.clear();
    i.grpId = RsGxsGroupId::random();
	i.msgUpdateTS = rand()%4252;
	return new RsGxsUpdateSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
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
    test_RsItem<RsGxsGrpUpdateItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsGxsMsgUpdateItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsGxsServerGrpUpdateItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsGxsServerMsgUpdateItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

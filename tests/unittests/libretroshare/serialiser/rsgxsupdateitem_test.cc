/*
 * rsgxsupdateitem_test.cc
 *
 *  Created on: 9 Dec 2013
 *      Author: crispy
 */

#include <gtest/gtest.h>

#include "support.h"
#include "serialiser/rsgxsupdateitems.h"
#define RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM	0x0010

RsSerialType* init_item(RsGxsGrpUpdateItem& i)
{
	i.clear();
	i.grpUpdateTS = rand()%2424;
	i.peerId.random();
	return new RsGxsUpdateSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsGxsMsgUpdateItem& i)
{
	i.clear();
	i.peerId.random();
	int numUpdates = rand()%123;

	RsPeerId peer;
	peer.random();
	for(int j=0; j < numUpdates; j++)
	{
		i.msgUpdateTS.insert(std::make_pair(peer, rand()%45));
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
	i.grpId.random();
	i.msgUpdateTS = rand()%4252;
	return new RsGxsUpdateSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

bool operator ==(const RsGxsGrpUpdateItem& l, const RsGxsGrpUpdateItem& r)
{
	bool ok = l.grpUpdateTS == r.grpUpdateTS;
	ok &= l.peerId == r.peerId;

	return ok;
}

bool operator ==(const RsGxsMsgUpdateItem& l, const RsGxsMsgUpdateItem& r)
{
	bool ok = l.peerId == r.peerId;

	const std::map<RsGxsGroupId, uint32_t>& lUp = l.msgUpdateTS, rUp = r.msgUpdateTS;

	ok &= lUp.size() == rUp.size();

	std::map<RsGxsGroupId, uint32_t>::const_iterator lit = lUp.begin(), rit;

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

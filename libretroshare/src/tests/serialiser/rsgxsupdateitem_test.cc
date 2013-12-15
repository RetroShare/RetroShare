/*
 * rsgxsupdateitem_test.cc
 *
 *  Created on: 9 Dec 2013
 *      Author: crispy
 */

#include "support.h"
#include "rsgxsupdateitem_test.h"

INITTEST();

RsSerialType* init_item(RsGxsGrpUpdateItem& i)
{
	i.clear();
	i.grpUpdateTS = rand()%2424;
	randString(SHORT_STR, i.peerId);
	return new RsGxsUpdateSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

RsSerialType* init_item(RsGxsMsgUpdateItem& i)
{
	i.clear();
	randString(SHORT_STR, i.peerId);
	int numUpdates = rand()%123;

	std::string peer;
	for(int j=0; j < numUpdates; j++)
	{
		randString(SHORT_STR, peer);
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
	randString(SHORT_STR, i.grpId);
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

	const std::map<std::string, uint32_t>& lUp = l.msgUpdateTS, rUp = r.msgUpdateTS;

	ok &= lUp.size() == rUp.size();

	std::map<std::string, uint32_t>::const_iterator lit = lUp.begin(), rit;

	for(; lit != lUp.end(); lit++)
	{
		std::string key = lit->first;
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


int main()
{
    std::cerr << "RsGxsUpdateItem Tests" << std::endl;

    test_RsItem<RsGxsGrpUpdateItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsGxsGrpUpdateItem");
    test_RsItem<RsGxsMsgUpdateItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsGxsMsgUpdateItem");
    test_RsItem<RsGxsServerGrpUpdateItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsGxsServerGrpUpdateItem");
    test_RsItem<RsGxsServerMsgUpdateItem>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM); REPORT("Serialise/Deserialise RsGxsServerMsgUpdateItem");

    FINALREPORT("RsGxsUpdateItem Tests");

    return TESTRESULT();
}

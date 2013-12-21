/*
 * rsgxsupdateitem_test.h
 *
 *  Created on: 9 Dec 2013
 *      Author: crispy
 */

#ifndef RSGXSUPDATEITEM_TEST_H_
#define RSGXSUPDATEITEM_TEST_H_

#include "serialiser/rsgxsupdateitems.h"
#include "support.h"


RsSerialType* init_item(RsGxsGrpUpdateItem& i);
RsSerialType* init_item(RsGxsMsgUpdateItem& i);
RsSerialType* init_item(RsGxsServerGrpUpdateItem& i);
RsSerialType* init_item(RsGxsServerMsgUpdateItem& i);

bool operator==(const RsGxsGrpUpdateItem& l, const RsGxsGrpUpdateItem& r);
bool operator==(const RsGxsMsgUpdateItem& l, const RsGxsMsgUpdateItem& r);
bool operator==(const RsGxsServerGrpUpdateItem& l, const RsGxsServerGrpUpdateItem& r);
bool operator==(const RsGxsServerMsgUpdateItem& l, const RsGxsServerMsgUpdateItem& r);

#endif /* RSGXSUPDATEITEM_TEST_H_ */

/*
 * libretroshare/src/tests/serialiser: msgitem_test.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2010 by Christopher Evi-Parker.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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

#include <iostream>

#include "serialiser/rsmsgitems.h"
#include "serialiser/rstlvutil.h"
#include "util/utest.h"
#include "support.h"
#include "rsmsgitem_test.h"

INITTEST();

RsSerialType* init_item(RsChatMsgItem& cmi)
{
	cmi.chatFlags = rand()%34;
	cmi.sendTime = rand()%422224;
	randString(LARGE_STR, cmi.message);

	return new RsChatSerialiser();
}

RsSerialType* init_item(RsChatStatusItem& csi)
{

	randString(SHORT_STR, csi.status_string);
	csi.flags = rand()%232;

	return new RsChatSerialiser();

}

RsSerialType* init_item(RsChatAvatarItem& cai)
{
	std::string image_data;
	randString(LARGE_STR, image_data);
	cai.image_data = new unsigned char[image_data.size()];

	memcpy(cai.image_data, image_data.c_str(), image_data.size());
	cai.image_size = image_data.size();

	return new RsChatSerialiser();
}

RsSerialType* init_item(RsMsgItem& mi)
{
	init_item(mi.attachment);
	init_item(mi.msgbcc);
	init_item(mi.msgcc);
	init_item(mi.msgto);

	randString(LARGE_STR, mi.message);
	randString(SHORT_STR, mi.subject);

	mi.msgId = rand()%324232;
	mi.recvTime = rand()%44252;
	mi.sendTime = mi.recvTime;
	mi.msgFlags = mi.recvTime;

	return new RsMsgSerialiser(true);
}

RsSerialType* init_item(RsMsgTagType& mtt)
{
	mtt.rgb_color = rand()%5353;
	mtt.tagId = rand()%24242;
	randString(SHORT_STR, mtt.text);

	return new RsMsgSerialiser();
}


RsSerialType* init_item(RsMsgTags& mt)
{
	mt.msgId = rand()%3334;

	int i;
	for (i = 0; i < 10; i++) {
		mt.tagIds.push_back(rand()%21341);
	}

	return new RsMsgSerialiser();
}


bool operator ==(const RsChatMsgItem& cmiLeft,const  RsChatMsgItem& cmiRight)
{

	if(cmiLeft.chatFlags != cmiRight.chatFlags) return false;
	if(cmiLeft.message != cmiRight.message) return false;
	if(cmiLeft.sendTime != cmiRight.sendTime) return false;

	return true;
}

bool operator ==(const RsChatStatusItem& csiLeft, const RsChatStatusItem& csiRight)
{
	if(csiLeft.flags != csiRight.flags) return false;
	if(csiLeft.status_string != csiRight.status_string) return false;

	return true;
}



bool operator ==(const RsChatAvatarItem& caiLeft, const RsChatAvatarItem& caiRight)
{
	unsigned char* image_dataLeft = (unsigned char*)caiLeft.image_data;
	unsigned char* image_dataRight = (unsigned char*)caiRight.image_data;

	// make image sizes are the same to prevent dereferencing garbage
	if(caiLeft.image_size == caiRight.image_size)
	{
		image_dataLeft = (unsigned char*)caiLeft.image_data;
		image_dataRight = (unsigned char*)caiRight.image_data;
	}
	else
	{
		return false;
	}

	for(uint32_t i = 0; i < caiLeft.image_size; i++)
		if(image_dataLeft[i] != image_dataRight[i]) return false;

	return true;

}

bool operator ==(const RsMsgItem& miLeft, const RsMsgItem& miRight)
{
	if(miLeft.message != miRight.message) return false;
	if(miLeft.msgFlags != miRight.msgFlags) return false;
	if(miLeft.recvTime != miRight.recvTime) return false;
	if(miLeft.sendTime != miRight.sendTime) return false;
	if(miLeft.subject != miRight.subject) return false;
	if(miLeft.msgId != miRight.msgId) return false;

	if(!(miLeft.attachment == miRight.attachment)) return false;
	if(!(miLeft.msgbcc == miRight.msgbcc)) return false;
	if(!(miLeft.msgcc == miRight.msgcc)) return false;
	if(!(miLeft.msgto == miRight.msgto)) return false;

	return true;
}

bool operator ==(const RsMsgTagType& mttLeft, const RsMsgTagType& mttRight)
{
	if(mttLeft.rgb_color != mttRight.rgb_color) return false;
	if(mttLeft.tagId != mttRight.tagId) return false;
	if(mttLeft.text != mttRight.text) return false;

	return true;
}

bool operator ==(const RsMsgTags& mtLeft, const RsMsgTags& mtRight)
{
	if(mtLeft.msgId != mtRight.msgId) return false;
	if(mtLeft.tagIds != mtRight.tagIds) return false;

	return true;
}

int main()
{
	test_RsItem<RsChatMsgItem >(); REPORT("Serialise/Deserialise RsChatMsgItem");
	test_RsItem<RsChatStatusItem >(); REPORT("Serialise/Deserialise RsChatStatusItem");
	test_RsItem<RsChatAvatarItem >(); REPORT("Serialise/Deserialise RsChatAvatarItem");
	test_RsItem<RsMsgItem >(); REPORT("Serialise/Deserialise RsMsgItem");
	test_RsItem<RsMsgTagType>(); REPORT("Serialise/Deserialise RsMsgTagType");
	test_RsItem<RsMsgTags>(); REPORT("Serialise/Deserialise RsMsgTags");

	std::cerr << std::endl;

	FINALREPORT("RsMsgItem Tests");

	return TESTRESULT();
}






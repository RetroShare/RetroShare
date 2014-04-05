
/*
 * libretroshare/src/serialiser: rsturtleitems_test.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Cyril Soler
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

/******************************************************************
 */
#include <gtest/gtest.h>

#include "serialiser/rsgxsiditems.h"
#include "support.h"


bool operator==(const RsGxsIdGroupItem& it1,const RsGxsIdGroupItem& it2)
{
	if(it1.group.mPgpIdSign != it2.group.mPgpIdSign) return false ;

	return true ;
}
RsSerialType* init_item(RsGxsIdGroupItem& item)
{
	item.group.mPgpIdSign = "hello";
	item.group.mPgpKnown = false;

	return new RsGxsIdSerialiser();
}


TEST(libretroshare_serialiser, RsGxsIdItem)
{
	for(uint32_t i=0;i<20;++i)
	{
		test_RsItem< RsGxsIdGroupItem >();
	}
}



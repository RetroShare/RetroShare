/*******************************************************************************
 * unittests/libretroshare/serialiser/rsgxsiditem_test.cc                      *
 *                                                                             *
 * Copyright 2007-2008 by Cyril Soler   <retroshare.project@gmail.com>         *
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

#include "rsitems/rsgxsiditems.h"
#include "support.h"


bool operator==(const RsGxsIdGroupItem& it1,const RsGxsIdGroupItem& it2)
{
    if(it1.mPgpIdSign != it2.mPgpIdSign) return false ;

	return true ;
}
void init_item(RsGxsIdGroupItem& item)
{
    item.mPgpIdSign = "hello";
}


TEST(libretroshare_serialiser, RsGxsIdItem)
{
	for(uint32_t i=0;i<20;++i)
	{
		test_RsItem< RsGxsIdGroupItem,RsGxsIdSerialiser >();
	}
}



/*******************************************************************************
 * unittests/libretroshare/serialiser/rsstatusitem_test.cc                     *
 *                                                                             *
 * Copyright 2010 by Christopher Evi-Parker <retroshare.project@gmail.com>     *
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

#include "support.h"
#include "rsitems/rsstatusitems.h"

void init_item(RsStatusItem& rsi)
{

	rsi.sendTime = rand()%5353;
	rsi.status = rand()%2032;
}

bool operator ==(RsStatusItem& rsi1, RsStatusItem& rsi2)
{
	// note: recv time is not serialised

	if(rsi1.sendTime != rsi2.sendTime) return false;
	if(rsi1.status != rsi2.status) return false;

	return true;
}


TEST(libretroshare_serialiser, test_RsStatusItem)
{
	test_RsItem<RsStatusItem,RsStatusSerialiser >();
}

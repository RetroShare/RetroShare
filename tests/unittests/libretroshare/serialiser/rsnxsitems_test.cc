/*******************************************************************************
 * unittests/libretroshare/serialiser/rsnxsitems_test.cc                       *
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
#include "libretroshare/gxs/common/data_support.h"
#include "rsitems/rsnxsitems.h"


#define NUM_BIN_OBJECTS 5
#define NUM_SYNC_MSGS 8
#define NUM_SYNC_GRPS 5

TEST(libretroshare_serialiser, RsNxsItem)
{
    test_RsItem<RsNxsGrp,RsNxsSerialiser>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsMsg,RsNxsSerialiser>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsSyncGrpItem,RsNxsSerialiser>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsSyncMsgItem,RsNxsSerialiser>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsSyncGrpItem,RsNxsSerialiser>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsSyncMsgItem,RsNxsSerialiser>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    test_RsItem<RsNxsTransacItem,RsNxsSerialiser>(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

/*******************************************************************************
 * unittests/libretroshare/gxs/common/data_support.h                           *
 *                                                                             *
 * Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>              *
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

#pragma once

#include "rsitems/rsnxsitems.h"
#include "gxs/rsgxsdata.h"

#define RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM 0x012

bool operator==(const RsNxsGrp&, const RsNxsGrp&);
bool operator==(const RsNxsMsg&, const RsNxsMsg&);
bool operator==(const RsGxsGrpMetaData& l, const RsGxsGrpMetaData& r);
bool operator==(const RsGxsMsgMetaData& l, const RsGxsMsgMetaData& r);
bool operator==(const RsNxsSyncGrpItem& l, const RsNxsSyncGrpItem& r);
bool operator==(const RsNxsSyncMsgItem& l, const RsNxsSyncMsgItem& r);
bool operator==(const RsNxsSyncGrpItem& l, const RsNxsSyncGrpItem& r);
bool operator==(const RsNxsSyncMsgItem& l, const RsNxsSyncMsgItem& r);
bool operator==(const RsNxsTransacItem& l, const RsNxsTransacItem& r);

//void init_item(RsNxsGrp& nxg);
//void init_item(RsNxsMsg& nxm);
void init_item(RsGxsGrpMetaData* metaGrp);
void init_item(RsGxsMsgMetaData* metaMsg);


void init_item(RsNxsGrp& nxg            ,RsSerialType ** = NULL);
void init_item(RsNxsMsg& nxm            ,RsSerialType ** = NULL);
void init_item(RsNxsSyncGrpReqItem &rsg ,RsSerialType ** = NULL);
void init_item(RsNxsSyncMsgReqItem &rsgm,RsSerialType ** = NULL);
void init_item(RsNxsSyncGrpItem& rsgl   ,RsSerialType ** = NULL);
void init_item(RsNxsSyncMsgItem& rsgml  ,RsSerialType ** = NULL);
void init_item(RsNxsTransacItem& rstx   ,RsSerialType ** = NULL);

template<typename T>
void copy_all_but(T& ex, const std::list<T>& s, std::list<T>& d)
{
	typename std::list<T>::const_iterator cit = s.begin();
	for(; cit != s.end(); cit++)
		if(*cit != ex)
			d.push_back(*cit);
}

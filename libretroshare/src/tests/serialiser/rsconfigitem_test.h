/*
 * libretroshare/src/serialiser: rsconfigitem_test.h
 *
 * RetroShare Serialiser tests.
 *
 * Copyright 2011 by Christopher Evi-Parker.
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

#ifndef RSCONFIGITEM_TEST_H_
#define RSCONFIGITEM_TEST_H_

#include "serialiser/rsconfigitems.h"
#include "turtle/rsturtleitem.h"

RsSerialType* init_item(CompressedChunkMap& map);
RsSerialType* init_item(RsPeerNetItem& );
RsSerialType* init_item(RsPeerOldNetItem& );
RsSerialType* init_item(RsPeerGroupItem& );
RsSerialType* init_item(RsPeerStunItem& );
RsSerialType* init_item(RsCacheConfig& );
RsSerialType* init_item(RsFileTransfer& );

bool operator==(const RsPeerNetItem&, const RsPeerNetItem& );
bool operator==(const RsPeerOldNetItem&, const RsPeerOldNetItem& );
bool operator==(const RsPeerGroupItem&, const RsPeerGroupItem& );
bool operator==(const RsPeerStunItem&, const RsPeerStunItem& );
bool operator==(const RsCacheConfig&, const RsCacheConfig& );
bool operator==(const RsFileTransfer&, const RsFileTransfer& );

bool operator==(const std::list<RsTlvIpAddressInfo>&,
		const std::list<RsTlvIpAddressInfo>&);

bool operator!=(const sockaddr_in&, const sockaddr_in&);
bool operator!=(const RsTlvIpAddressInfo&, const RsTlvIpAddressInfo& );
bool operator!=(const std::list<std::string>& left,
		const std::list<std::string>& right);
//bool operator==(const RsTlvPeerIdSet& left, const RsTlvPeerIdSet& right);
bool operator==(const RsTlvFileItem&, const RsTlvFileItem& );



#endif /* RSCONFIGITEM_TEST_H_ */

/*
 * libretroshare/src/gxs: rgxsrequesttypes.cc
 *
 * Type introspect request types for data access request implementation
 *
 * Copyright 2012-2012 by Christopher Evi-Parker
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

#include "rsgxsrequesttypes.h"
#include "util/rsstd.h"

GroupMetaReq::~GroupMetaReq()
{
	rsstd::delete_all(mGroupMetaData.begin(), mGroupMetaData.end());
}

GroupDataReq::~GroupDataReq()
{
	rsstd::delete_all(mGroupData.begin(), mGroupData.end());
}

MsgMetaReq::~MsgMetaReq()
{
	for (GxsMsgMetaResult::iterator it = mMsgMetaData.begin(); it != mMsgMetaData.end(); ++it) {
		rsstd::delete_all(it->second.begin(), it->second.end());
	}
}

MsgDataReq::~MsgDataReq()
{
	for (NxsMsgDataResult::iterator it = mMsgData.begin(); it != mMsgData.end(); ++it) {
		rsstd::delete_all(it->second.begin(), it->second.end());
	}
}

MsgRelatedInfoReq::~MsgRelatedInfoReq()
{
	for (MsgRelatedMetaResult::iterator metaIt = mMsgMetaResult.begin(); metaIt != mMsgMetaResult.end(); ++metaIt) {
		rsstd::delete_all(metaIt->second.begin(), metaIt->second.end());
	}
	for (NxsMsgRelatedDataResult::iterator dataIt = mMsgDataResult.begin(); dataIt != mMsgDataResult.end(); ++dataIt) {
		rsstd::delete_all(dataIt->second.begin(), dataIt->second.end());
	}
}

/*
 * libretroshare/src/gxs: rsgxsutil.h
 *
 * RetroShare C++ Interface. Generic routines that are useful in GXS
 *
 * Copyright 2013-2013 by Christopher Evi-Parker
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

#ifndef GXSUTIL_H_
#define GXSUTIL_H_

#include <vector>

/*!
 * Handy function for cleaning out meta result containers
 * @param container
 */
template <class Container, class Item>
void freeAndClearContainerResource(Container container)
{
	typename Container::iterator meta_it = container.begin();

	for(; meta_it != container.end(); meta_it++)
	{
		delete meta_it->second;

	}
	container.clear();
}

inline RsGxsGrpMsgIdPair getMsgIdPair(RsNxsMsg& msg)
{
	return RsGxsGrpMsgIdPair(std::make_pair(msg.grpId, msg.msgId));
}

inline RsGxsGrpMsgIdPair getMsgIdPair(RsGxsMsgItem& msg)
{
	return RsGxsGrpMsgIdPair(std::make_pair(msg.meta.mGroupId, msg.meta.mMsgId));
}


#endif /* GXSUTIL_H_ */

/*
 * libretroshare/src/services p3gxscommon.cc
 *
 * GxsChannels interface for RetroShare.
 *
 * Copyright 2012-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "retroshare/rsgxscommon.h"
#include "gxs/rsgenexchange.h"
#include <stdio.h>


/****
 * This mirrors rsGxsCommentService, with slightly different names, 
 * provides the implementation for any services requiring Comments.
 */

class p3GxsCommentService
{
	public:

	p3GxsCommentService(RsGenExchange *exchange, uint32_t service_type);

	bool getGxsCommentData(const uint32_t &token, std::vector<RsGxsComment> &msgs);
	bool getGxsRelatedComments(const uint32_t &token, std::vector<RsGxsComment> &msgs);

	bool createGxsComment(uint32_t &token, RsGxsComment &msg);
	bool createGxsVote(uint32_t &token, RsGxsVote &msg);

#if 0
	void setGxsMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read);
#endif

	private:

	RsGenExchange 	*mExchange;
	uint32_t	mServiceType;
};




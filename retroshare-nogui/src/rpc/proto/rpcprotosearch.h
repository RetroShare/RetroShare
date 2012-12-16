/*
 * RetroShare External Interface.
 *
 * Copyright 2012-2012 by Robert Fernie.
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


#ifndef RS_RPC_PROTO_SEARCH_H
#define RS_RPC_PROTO_SEARCH_H

#include "rpc/rpcserver.h"

class NotifyTxt;

class RpcProtoSearch: public RpcQueueService
{
public:
	RpcProtoSearch(uint32_t serviceId, NotifyTxt *notify);
        virtual void reset(uint32_t chan_id);

	virtual int processMsg(uint32_t chan_id, uint32_t msgId, uint32_t req_id, const std::string &msg);

protected:

	int processReqBasicSearch(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);
	int processReqCloseSearch(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);
	int processReqListSearches(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);
	int processReqSearchResults(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);


private:
	// These private functions use Mutex below and manipulate mActiveSearches.
	int get_search_list(uint32_t chan_id, std::list<uint32_t> &search_ids);
	int add_search(uint32_t chan_id, uint32_t search_id);
	int remove_search(uint32_t chan_id, uint32_t search_id);
	int clear_searches(uint32_t chan_id);

	NotifyTxt *mNotify;

	RsMutex searchMtx; 

	/* must store list of active searches per channel */
	std::map<uint32_t, std::list<uint32_t> > mActiveSearches;

};

#endif /* RS_PROTO_SEARCH_H */

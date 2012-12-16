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


#ifndef RS_RPC_PROTO_FILES_H
#define RS_RPC_PROTO_FILES_H

#include "rpc/rpcserver.h"

class RpcProtoFiles: public RpcQueueService
{
public:
	RpcProtoFiles(uint32_t serviceId);

	virtual int processMsg(uint32_t chan_id, uint32_t msgId, uint32_t req_id, const std::string &msg);

protected:

	int processReqTransferList(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);
	int processReqControlDownload(uint32_t chan_id, uint32_t msg_id, uint32_t req_id, const std::string &msg);

};

#endif /* RS_PROTO_FILES_H */

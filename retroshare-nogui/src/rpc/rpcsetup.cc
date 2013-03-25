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

#include "rpc/rpcsetup.h"
#include "rpc/rpcserver.h"

#include "rpc/proto/rpcprotopeers.h"
#include "rpc/proto/rpcprotosystem.h"
#include "rpc/proto/rpcprotochat.h"
#include "rpc/proto/rpcprotosearch.h"
#include "rpc/proto/rpcprotofiles.h"
#include "rpc/proto/rpcprotostream.h"

#include "rpc/rpcecho.h"

RpcMediator *CreateRpcSystem(RpcComms *comms, NotifyTxt *notify)
{
	RpcMediator *med = new RpcMediator(comms);
	RpcServer *server = new RpcServer(med);

	/* add services */
	RpcProtoPeers *peers = new RpcProtoPeers(1);
	server->addService(peers);

	RpcProtoSystem *system = new RpcProtoSystem(1);
	server->addService(system);

	RpcProtoChat *chat = new RpcProtoChat(1);
	server->addService(chat);

	RpcProtoSearch *search = new RpcProtoSearch(1, notify);
	server->addService(search);

	RpcProtoFiles *files = new RpcProtoFiles(1);
	server->addService(files);

	RpcProtoStream *streamer = new RpcProtoStream(1);
	server->addService(streamer);

	/* Finally an Echo Service - which will echo back any unprocesses commands. */
	RpcEcho *echo = new RpcEcho(1);
	server->addService(echo);

	med->setRpcServer(server);

	return med;
}


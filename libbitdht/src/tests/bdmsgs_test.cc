
/*
 * bitdht/bdmsgs_test.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */


#include "bitdht/bdmsgs.h"
#include "bitdht/bdstddht.h"
#include <string.h>

/*******************************************************************
 * Test of bencode message creation functions in bdmsgs.cc
 *
 * Create a couple of each type.
 */

#define MAX_MESSAGE_LEN	10240

int main(int argc, char **argv)
{
	/***** create messages *****/
	char msg[MAX_MESSAGE_LEN];
	int avail = MAX_MESSAGE_LEN -1;

	bdToken tid;
	bdToken vid;
	bdToken token;


	bdNodeId ownId;
	bdNodeId peerId;
	bdNodeId target;
	bdNodeId info_hash;

	bdStdRandomNodeId(&ownId);
	bdStdRandomNodeId(&peerId);
	bdStdRandomNodeId(&target);
	bdStdRandomNodeId(&info_hash);

	std::list<bdId> nodes;
	std::list<std::string> values;

	/* setup tokens */
	strncpy((char*)tid.data, "tid", 4);
	strncpy((char*)vid.data, "RS50", 5);
	strncpy((char*)token.data, "ToKEn", 6);

	tid.len = 3;
	vid.len = 4;
	token.len = 5;

	/* setup lists */
	for(int i = 0; i < 8; i++)
	{
		bdId rndId;
		bdStdRandomId(&rndId);

		nodes.push_back(rndId);
		values.push_back("values");
	}

        uint32_t port = 1234;

	bitdht_create_ping_msg(&tid, &ownId, msg, avail);
	bitdht_response_ping_msg(&tid, &ownId, &vid, msg, avail);

	bitdht_find_node_msg(&tid, &ownId, &target, msg, avail);
	bitdht_resp_node_msg(&tid, &ownId, nodes, msg, avail);

	bitdht_get_peers_msg(&tid, &ownId, &info_hash, msg, avail);
	bitdht_peers_reply_hash_msg(&tid, &ownId, &token, values, msg, avail);
	bitdht_peers_reply_closest_msg(&tid, &ownId, &token, nodes, msg, avail);

	bitdht_announce_peers_msg(&tid, &ownId, &info_hash, port, &token, msg, avail);
	bitdht_reply_announce_msg(&tid, &ownId, msg, avail);


	return 1;
}



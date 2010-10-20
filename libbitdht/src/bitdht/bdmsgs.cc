/*
 * bitdht/bdmsgs.cc
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

#include <stdio.h>
#include <string.h>
#include "bitdht/bencode.h"
#include "bitdht/bdmsgs.h"


int create_ping_message();
int response_ping_message();

int find_node_message();
int response_node_message();

int get_peers_message();
int response_peers_message();
int response_closestnodes_message();

	/*	
ping Query = {"t":"aa", "y":"q", "q":"ping", "a":{"id":"abcdefghij0123456789"}}
bencoded = d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe
	*/

/****
 * #define DEBUG_MSG_DUMP 	1
 * #define DEBUG_MSG_TYPE 	1
 * #define DEBUG_MSGS 		1
 ****/


int bitdht_create_ping_msg(bdToken *tid, bdNodeId *id, char *msg, int avail)
{
#ifdef DEBUG_MSGS 
	fprintf(stderr, "bitdht_create_ping_msg()\n");
#endif

	be_node *dict = be_create_dict();
	be_node *iddict = be_create_dict();
	be_node *idnode = be_create_str_wlen((char *) id->data, BITDHT_KEY_LEN);
	
	be_add_keypair(iddict, "id", idnode);

	be_node *pingnode = be_create_str("ping");
	be_node *tidnode = be_create_str_wlen((char *) tid->data, tid->len);
	be_node *qynode = be_create_str("q");

	be_add_keypair(dict, "a", iddict);

	be_add_keypair(dict, "q", pingnode);
	be_add_keypair(dict, "t", tidnode);
	be_add_keypair(dict, "y", qynode);

#ifdef DEBUG_MSG_DUMP
	/* dump answer */
	be_dump(dict);
#endif

	int blen = be_encode(dict, msg, avail);
	be_free(dict);

	return blen;
	

}


	/*
Response = {"t":"aa", "y":"r", "r": {"id":"mnopqrstuvwxyz123456"}}
bencoded = d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re
	*/

int bitdht_response_ping_msg(bdToken *tid, bdNodeId *id, bdToken *vid, 
					char *msg, int avail)
{
#ifdef DEBUG_MSGS 
	fprintf(stderr, "bitdht_response_ping_msg()\n");
#endif

        be_node *dict = be_create_dict();

        be_node *iddict = be_create_dict();
        be_node *idnode = be_create_str_wlen((char *) id->data, BITDHT_KEY_LEN);

        be_node *tidnode = be_create_str_wlen((char *) tid->data, tid->len);
        be_node *yqrnode = be_create_str("r");

        be_node *vnode = be_create_str_wlen((char *) vid->data, vid->len);

        be_add_keypair(iddict, "id", idnode);
        be_add_keypair(dict, "r", iddict);

        be_add_keypair(dict, "t", tidnode);
        be_add_keypair(dict, "y", yqrnode);
        be_add_keypair(dict, "v", vnode);

#ifdef DEBUG_MSG_DUMP
        /* dump answer */
	be_dump(dict);
#endif

	int blen = be_encode(dict, msg, avail);
	be_free(dict);

	return blen;
}


	/*
find_node Query = {"t":"aa", "y":"q", "q":"find_node", "a": {"id":"abcdefghij0123456789", "target":"mnopqrstuvwxyz123456"}}
bencoded = d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe
	*/


int bitdht_find_node_msg(bdToken *tid, bdNodeId *id, bdNodeId *target, 
					char *msg, int avail)
{
#ifdef DEBUG_MSGS 
	fprintf(stderr, "bitdht_find_node_msg()\n");
#endif

        be_node *dict = be_create_dict();

        be_node *iddict = be_create_dict();
        be_node *idnode = be_create_str_wlen((char *) id->data, BITDHT_KEY_LEN);
        be_node *targetnode = be_create_str_wlen((char *) target->data, BITDHT_KEY_LEN);

        be_node *tidnode = be_create_str_wlen((char *) tid->data, tid->len);
        be_node *yqrnode = be_create_str("q");
        be_node *findnode = be_create_str("find_node");

        be_add_keypair(iddict, "id", idnode);
        be_add_keypair(iddict, "target", targetnode);
        be_add_keypair(dict, "a", iddict);

        be_add_keypair(dict, "t", tidnode);
        be_add_keypair(dict, "y", yqrnode);
        be_add_keypair(dict, "q", findnode);

#ifdef DEBUG_MSG_DUMP
        /* dump answer */
	be_dump(dict);
#endif

	int blen = be_encode(dict, msg, avail);
	be_free(dict);

	return blen;
}



	/*
Response = {"t":"aa", "y":"r", "r": {"id":"0123456789abcdefghij", "nodes": "def456..."}}
bencoded = d1:rd2:id20:0123456789abcdefghij5:nodes9:def456...e1:t2:aa1:y1:re
	*/

int bitdht_resp_node_msg(bdToken *tid, bdNodeId *id, std::list<bdId> &nodes, 
					char *msg, int avail)
{
#ifdef DEBUG_MSGS 
	fprintf(stderr, "bitdht_resp_node_msg()\n");
#endif

        be_node *dict = be_create_dict();

        be_node *replydict = be_create_dict();
        be_node *idnode = be_create_str_wlen((char *) id->data, BITDHT_KEY_LEN);
        be_node *tidnode = be_create_str_wlen((char *) tid->data, tid->len);

	be_node *peersnode = makeCompactNodeIdString(nodes);

        be_node *yqrnode = be_create_str("r");

        be_add_keypair(replydict, "id", idnode);
        be_add_keypair(replydict, "nodes", peersnode);

        be_add_keypair(dict, "t", tidnode);
        be_add_keypair(dict, "y", yqrnode);
        be_add_keypair(dict, "r", replydict);

#ifdef DEBUG_MSG_DUMP
        /* dump answer */
	be_dump(dict);
#endif

	int blen = be_encode(dict, msg, avail);
	be_free(dict);

#ifdef DEBUG_MSG_DUMP
	fprintf(stderr, "bitdht_resp_node_msg() len = %d / %d\n", blen, avail);
#endif

	return blen;
}


	/*
get_peers Query = {"t":"aa", "y":"q", "q":"get_peers", "a": {"id":"abcdefghij0123456789", "info_hash":"mnopqrstuvwxyz123456"}}
bencoded = d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe
	*/

int bitdht_get_peers_msg(bdToken *tid, bdNodeId *id, bdNodeId *info_hash, 
					char *msg, int avail)
{
#ifdef DEBUG_MSGS 
	fprintf(stderr, "bitdht_get_peers_msg()\n");
#endif

        be_node *dict = be_create_dict();

        be_node *iddict = be_create_dict();
        be_node *idnode = be_create_str_wlen((char *) id->data, BITDHT_KEY_LEN);
        be_node *hashnode = be_create_str_wlen((char *) info_hash->data, BITDHT_KEY_LEN);

        be_node *tidnode = be_create_str_wlen((char *) tid->data, tid->len);
        be_node *yqrnode = be_create_str("q");
        be_node *findnode = be_create_str("get_peers");

        be_add_keypair(iddict, "id", idnode);
        be_add_keypair(iddict, "info_hash", hashnode);
        be_add_keypair(dict, "a", iddict);

        be_add_keypair(dict, "t", tidnode);
        be_add_keypair(dict, "y", yqrnode);
        be_add_keypair(dict, "q", findnode);

#ifdef DEBUG_MSG_DUMP
        /* dump answer */
	be_dump(dict);
#endif

	int blen = be_encode(dict, msg, avail);
	be_free(dict);

	return blen;
}


	/*
Response with peers = {"t":"aa", "y":"r", "r": {"id":"abcdefghij0123456789", "token":"aoeusnth", "values": ["axje.u", "idhtnm"]}}
bencoded = d1:rd2:id20:abcdefghij01234567895:token8:aoeusnth6:valuesl6:axje.u6:idhtnmee1:t2:aa1:y1:re
	*/

int bitdht_peers_reply_hash_msg(bdToken *tid, bdNodeId *id, 
		bdToken *token, std::list<std::string> &values,
					char *msg, int avail)
{
#ifdef DEBUG_MSGS 
	fprintf(stderr, "bitdht_peers_reply_hash_msg()\n");
#endif

        be_node *dict = be_create_dict();

        be_node *replydict = be_create_dict();
        be_node *idnode = be_create_str_wlen((char *) id->data, BITDHT_KEY_LEN);

        be_node *tidnode = be_create_str_wlen((char *) tid->data, tid->len);
        be_node *tokennode = be_create_str_wlen((char *) token->data, token->len);
	be_node *valuesnode = makeCompactPeerIds(values);

        be_node *yqrnode = be_create_str("r");

        be_add_keypair(replydict, "id", idnode);
        be_add_keypair(replydict, "token", tokennode);
        be_add_keypair(replydict, "values", valuesnode);

        be_add_keypair(dict, "t", tidnode);
        be_add_keypair(dict, "y", yqrnode);
        be_add_keypair(dict, "r", replydict);

#ifdef DEBUG_MSG_DUMP
        /* dump answer */
	be_dump(dict);
#endif

	int blen = be_encode(dict, msg, avail);
	be_free(dict);

	return blen;
}

	/**

Response with closest nodes = {"t":"aa", "y":"r", "r": {"id":"abcdefghij0123456789", "token":"aoeusnth", "nodes": "def456..."}}
bencoded = d1:rd2:id20:abcdefghij01234567895:nodes9:def456...5:token8:aoeusnthe1:t2:aa1:y1:re

	**/


int bitdht_peers_reply_closest_msg(bdToken *tid, bdNodeId *id, 
				bdToken *token, std::list<bdId> &nodes, 
					char *msg, int avail)
{
#ifdef DEBUG_MSGS 
	fprintf(stderr, "bitdht_peers_reply_closest_msg()\n");
#endif

        be_node *dict = be_create_dict();

        be_node *replydict = be_create_dict();
        be_node *idnode = be_create_str_wlen((char *) id->data, BITDHT_KEY_LEN);
        be_node *tidnode = be_create_str_wlen((char *) tid->data, tid->len);
        be_node *tokennode = be_create_str_wlen((char *) token->data, token->len);

	be_node *peersnode = makeCompactNodeIdString(nodes);

        be_node *yqrnode = be_create_str("r");

        be_add_keypair(replydict, "id", idnode);
        be_add_keypair(replydict, "token", tokennode);
        be_add_keypair(replydict, "nodes", peersnode);

        be_add_keypair(dict, "t", tidnode);
        be_add_keypair(dict, "y", yqrnode);
        be_add_keypair(dict, "r", replydict);

#ifdef DEBUG_MSG_DUMP
        /* dump answer */
	be_dump(dict);
#endif

	int blen = be_encode(dict, msg, avail);
	be_free(dict);

	return blen;
}



/**** FINAL TWO MESSAGES! ***/

/****
announce_peers Query = {"t":"aa", "y":"q", "q":"announce_peer", "a": {"id":"abcdefghij0123456789", "info_hash":"mnopqrstuvwxyz123456", "port": 6881, "token": "aoeusnth"}}
bencoded = d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe

****/

int bitdht_announce_peers_msg(bdToken *tid, bdNodeId *id, bdNodeId *info_hash, uint32_t port, bdToken *token, char *msg, int avail)
{
#ifdef DEBUG_MSGS 
	fprintf(stderr, "bitdht_announce_peers_msg()\n");
#endif

	be_node *dict = be_create_dict();
	be_node *iddict = be_create_dict();

	be_node *idnode = be_create_str_wlen((char *) id->data, BITDHT_KEY_LEN);

	be_node *hashnode = be_create_str_wlen((char *) info_hash->data, BITDHT_KEY_LEN);
	be_node *portnode = be_create_int(port);
	be_node *tokennode = be_create_str_wlen((char *) token->data, token->len);
	
	be_add_keypair(iddict, "id", idnode);
	be_add_keypair(iddict, "info_hash", hashnode);
	be_add_keypair(iddict, "port", portnode);
	be_add_keypair(iddict, "token", tokennode);

	be_node *announcenode = be_create_str("announce_peer");
	be_node *tidnode = be_create_str_wlen((char *) tid->data, tid->len);
	be_node *qynode = be_create_str("q");

	be_add_keypair(dict, "a", iddict);

	be_add_keypair(dict, "q", announcenode);
	be_add_keypair(dict, "t", tidnode);
	be_add_keypair(dict, "y", qynode);

#ifdef DEBUG_MSG_DUMP
	/* dump answer */
	be_dump(dict);
#endif

	int blen = be_encode(dict, msg, avail);
	be_free(dict);

	return blen;

}


/*****
Response to Announce Peers = {"t":"aa", "y":"r", "r": {"id":"mnopqrstuvwxyz123456"}}
bencoded = d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re
****/

/****
 * NB: This is the same as a PONG msg!
 ***/

int bitdht_reply_announce_msg(bdToken *tid, bdNodeId *id, 
					char *msg, int avail)
{
#ifdef DEBUG_MSGS 
	fprintf(stderr, "bitdht_response_ping_msg()\n");
#endif

        be_node *dict = be_create_dict();

        be_node *iddict = be_create_dict();
        be_node *idnode = be_create_str_wlen((char *) id->data, BITDHT_KEY_LEN);

        be_node *tidnode = be_create_str_wlen((char *) tid->data, tid->len);
        be_node *yqrnode = be_create_str("r");

        be_add_keypair(iddict, "id", idnode);
        be_add_keypair(dict, "r", iddict);

        be_add_keypair(dict, "t", tidnode);
        be_add_keypair(dict, "y", yqrnode);

#ifdef DEBUG_MSG_DUMP
        /* dump answer */
	be_dump(dict);
#endif

	int blen = be_encode(dict, msg, avail);
	be_free(dict);

	return blen;
}



/************************ Parsing Messages *********************
 *
 */

be_node *beMsgGetDictNode(be_node *node, const char *key)
{
	/* make sure its a dictionary */
	if (node->type != BE_DICT)
	{
		return NULL;
	}

	/* get dictionary entry 'y' */
	int i;
	for(i = 0; node->val.d[i].val; i++)
	{
		if (0 == strcmp(key, node->val.d[i].key))
		{
			return node->val.d[i].val;
		}
	}
	return NULL;
}



int beMsgMatchString(be_node *n, const char *str, int len)
{
	if (n->type != BE_STR)	
	{
		return 0;
	}
	if (len != be_str_len(n))
	{
		return 0;
	}

	int i;
	for(i = 0; i < len; i++)
	{
		if (n->val.s[i] != str[i])
			return 0;
	}
	return 1;
}


uint32_t beMsgGetY(be_node *n)
{
	be_node *val = beMsgGetDictNode(n, "y");
	if (val->type != BE_STR)	
	{
		return BE_Y_UNKNOWN;
	}
		
	if (val->val.s[0] == 'q')
	{
		return BE_Y_Q;
	}
	else if (val->val.s[0] == 'r')
	{
		return BE_Y_R;
	}

	return BE_Y_UNKNOWN;
}



uint32_t beMsgType(be_node *n)
{
	/* check for 
	 * y: q or r
	 */
	uint32_t beY = beMsgGetY(n);

#ifdef DEBUG_MSG_TYPE 
	std::cerr << "bsMsgType() beY: " << beY << std::endl;
#endif

	if (beY == BE_Y_UNKNOWN)
	{
#ifdef DEBUG_MSG_TYPE 
		std::cerr << "bsMsgType() UNKNOWN MSG TYPE" << std::endl;
#endif

		return BITDHT_MSG_TYPE_UNKNOWN;
	}

	if (beY == BE_Y_Q) /* query */
	{
#ifdef DEBUG_MSG_TYPE 
		std::cerr << "bsMsgType() QUERY MSG TYPE" << std::endl;
#endif
	        be_node *query = beMsgGetDictNode(n, "q");

		if (beMsgMatchString(query, "ping", 4))
		{
#ifdef DEBUG_MSG_TYPE 
			std::cerr << "bsMsgType() QUERY:ping MSG TYPE" << std::endl;
#endif
			return BITDHT_MSG_TYPE_PING;
		}
		else if (beMsgMatchString(query, "find_node", 9))
		{
#ifdef DEBUG_MSG_TYPE 
			std::cerr << "bsMsgType() QUERY:find_node MSG TYPE" << std::endl;
#endif
			return BITDHT_MSG_TYPE_FIND_NODE;
		}
		else if (beMsgMatchString(query, "get_peers", 9))
		{
#ifdef DEBUG_MSG_TYPE 
			std::cerr << "bsMsgType() QUERY:get_peers MSG TYPE" << std::endl;
#endif
			return BITDHT_MSG_TYPE_GET_HASH;
		}
		else if (beMsgMatchString(query, "announce_peer", 13))
		{
#ifdef DEBUG_MSG_TYPE 
			std::cerr << "bsMsgType() QUERY:announce_peer MSG TYPE" << std::endl;
#endif
			return BITDHT_MSG_TYPE_POST_HASH;
		}
#ifdef DEBUG_MSG_TYPE 
		std::cerr << "bsMsgType() QUERY:UNKNOWN MSG TYPE, dumping dict" << std::endl;
        	/* dump answer */
        	be_dump(n);
#endif
		return BITDHT_MSG_TYPE_UNKNOWN;
	}

	if (beY != BE_Y_R)
	{
#ifdef DEBUG_MSG_TYPE 
		std::cerr << "bsMsgType() UNKNOWN2 MSG TYPE" << std::endl;
#endif
		return BITDHT_MSG_TYPE_UNKNOWN;
	}

#ifdef DEBUG_MSG_TYPE 
		std::cerr << "bsMsgType() REPLY MSG TYPE" << std::endl;
#endif

	/* otherwise a reply or - invalid 
	pong {"id":"mnopqrstuvwxyz123456"}
	reply_neigh { "id":"0123456789abcdefghij", "nodes": "def456..."}}
	reply_hash { "id":"abcdefghij0123456789", "token":"aoeusnth", "values": ["axje.u", "idhtnm"]}}
	reply_near { "id":"abcdefghij0123456789", "token":"aoeusnth", "nodes": "def456..."}
	*/

	be_node *reply = beMsgGetDictNode(n, "r");
	if (!reply)
	{
		return BITDHT_MSG_TYPE_UNKNOWN;
	}
	
	be_node *id = beMsgGetDictNode(reply, "id");
	be_node *token = beMsgGetDictNode(reply, "token");
	be_node *values = beMsgGetDictNode(reply, "values");
	be_node *nodes = beMsgGetDictNode(reply, "nodes");

	if (!id)
	{
		return BITDHT_MSG_TYPE_UNKNOWN;
	}
	
	if (token && values)
	{
		/* reply hash */
		return BITDHT_MSG_TYPE_REPLY_HASH;
	}
	else if (token && nodes)
	{
		/* reply near */
		return BITDHT_MSG_TYPE_REPLY_NEAR;
	}
	else if (nodes)
	{
		/* reply neigh */
		return BITDHT_MSG_TYPE_REPLY_NODE;
	}
	else
	{
		/* pong */
		return BITDHT_MSG_TYPE_PONG;
	}
	/* TODO reply_post */
	//return BITDHT_MSG_TYPE_REPLY_POST;
	/* can't get here! */
	return BITDHT_MSG_TYPE_UNKNOWN;
}

/* extract specific types here */

int beMsgGetToken(be_node *n, bdToken &token)
{
	if (n->type != BE_STR)	
	{
		return 0;
	}
        int len = be_str_len(n);
	for(int i = 0; i < len; i++)
	{
		token.data[i] = n->val.s[i];
	}
	token.len = len;
	return 1;
}

int beMsgGetNodeId(be_node *n, bdNodeId &nodeId)
{
	if (n->type != BE_STR)	
	{
		return 0;
	}
        int len = be_str_len(n);
	if (len != BITDHT_KEY_LEN)
	{
		return 0;
	}
	for(int i = 0; i < len; i++)
	{
		nodeId.data[i] = n->val.s[i];
	}
	return 1;
}

be_node *makeCompactNodeIdString(std::list<bdId> &nodes)
{
	int len = BITDHT_COMPACTNODEID_LEN * nodes.size();
	std::string cni;
	std::list<bdId>::iterator it;
	for(it = nodes.begin(); it != nodes.end(); it++)
	{
		cni += encodeCompactNodeId(&(*it));
	}
		

	be_node *cninode = be_create_str_wlen((char *) cni.c_str(), len);
	return cninode;
}

be_node *makeCompactPeerIds(std::list<std::string> &values)
{
        be_node *valuesnode = be_create_list();
	std::list<std::string>::iterator it;
	for(it = values.begin(); it != values.end(); it++)
	{
        	be_node *val1 = be_create_str_wlen((char *) it->c_str(), it->length());
		be_add_list(valuesnode, val1);
	}
	return valuesnode;
}


int beMsgGetListBdIds(be_node *n, std::list<bdId> &nodes)
{
	/* extract the string pointer, and size */
	/* split into parts */

        if (n->type != BE_STR)
        {
                return 0;
        }

	int len = be_str_len(n);
	int count = len / BITDHT_COMPACTNODEID_LEN;
	for(int i = 0; i < count; i++)
	{
		bdId id;
		if (decodeCompactNodeId(&id, &(n->val.s[i*BITDHT_COMPACTNODEID_LEN]), BITDHT_COMPACTNODEID_LEN))
		{
			nodes.push_back(id);
		}
	}
        return 1;
}

std::string encodeCompactNodeId(bdId *id)
{
	std::string enc;
	for(int i = 0; i < BITDHT_KEY_LEN; i++)
	{
		enc += id->id.data[i];
	}
	/* convert ip address (already in network order) */
	enc += encodeCompactPeerId(&(id->addr));
	return enc;
}

int decodeCompactNodeId(bdId *id, char *enc, int len)
{
	if (len < BITDHT_COMPACTNODEID_LEN)
	{
		return 0;
	}

	for(int i = 0; i < BITDHT_KEY_LEN; i++)
	{
		id->id.data[i] = enc[i];
	}

	char *ipenc = &(enc[BITDHT_COMPACTNODEID_LEN - BITDHT_COMPACTPEERID_LEN]);
	if (!decodeCompactPeerId(&(id->addr), ipenc, BITDHT_COMPACTPEERID_LEN))
	{
		return 0;
	}

	return 1;
}

std::string encodeCompactPeerId(struct sockaddr_in *addr)
{
	std::string encstr;
	char enc[BITDHT_COMPACTPEERID_LEN];
	uint32_t *ip = (uint32_t *) (enc);
	uint16_t *port = (uint16_t *) (&enc[4]);
	(*ip) = addr->sin_addr.s_addr;
	(*port) = addr->sin_port;

	encstr.append(enc, BITDHT_COMPACTPEERID_LEN);
	return encstr;
}

int decodeCompactPeerId(struct sockaddr_in *addr, char *enc, int len)
{
	if (len < BITDHT_COMPACTPEERID_LEN)
		return 0;

	memset(addr, 0, sizeof(struct sockaddr_in));

	uint32_t *ip = (uint32_t *) (enc);
	uint16_t *port = (uint16_t *) (&enc[4]);
	addr->sin_addr.s_addr = (*ip); 
	addr->sin_port = (*port); 
	addr->sin_family = AF_INET;

	return 1;
}


int beMsgGetListStrings(be_node *n, std::list<std::string> &values)
{
	if (n->type != BE_LIST)
	{
		return 0;
	}
	for(int i = 0; n->val.l[i] != NULL; i++)
	{
		be_node *val = n->val.l[i];
		if (val->type != BE_STR)	
		{
			return 0;
		}
        	int len = be_str_len(val);
		std::string str;
		str.append(val->val.s, len);
		values.push_back(str);
	}
        return 1;
}


int beMsgGetUInt32(be_node *n, uint32_t *port)
{
	if (n->type != BE_INT)	
	{
		return 0;
	}
	*port = n->val.i;
	return 1;
}



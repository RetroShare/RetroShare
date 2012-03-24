/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. 
 * 
 * You may obtain a copy of the License at 
 *     http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * 
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** \file
 */

#include <openpgpsdk/packet.h>
#include <openpgpsdk/packet-parse.h>
#include <openpgpsdk/util.h>
#include <openpgpsdk/accumulate.h>
#include "keyring_local.h"
#include "parse_local.h"
#include <openpgpsdk/signature.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <openpgpsdk/final.h>

typedef struct
    {
    ops_keyring_t *keyring;
    } accumulate_arg_t;

/**
 * \ingroup Core_Callbacks
 */
static ops_parse_cb_return_t
accumulate_cb(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)
    {
    accumulate_arg_t *arg=ops_parse_cb_get_arg(cbinfo);
    const ops_parser_content_union_t *content=&content_->content;
    ops_keyring_t *keyring=arg->keyring;
    ops_keydata_t *cur=NULL;
    const ops_public_key_t *pkey;

    if(keyring->nkeys >= 0)
	cur=&keyring->keys[keyring->nkeys];

    switch(content_->tag)
	{
    case OPS_PTAG_CT_PUBLIC_KEY:
    case OPS_PTAG_CT_SECRET_KEY:
    case OPS_PTAG_CT_ENCRYPTED_SECRET_KEY:
	//	printf("New key\n");
	++keyring->nkeys;
	EXPAND_ARRAY(keyring,keys);

	if(content_->tag == OPS_PTAG_CT_PUBLIC_KEY)
	    pkey=&content->public_key;
	else
	    pkey=&content->secret_key.public_key;

	memset(&keyring->keys[keyring->nkeys],'\0',
	       sizeof keyring->keys[keyring->nkeys]);

	ops_keyid(keyring->keys[keyring->nkeys].key_id,pkey);
	ops_fingerprint(&keyring->keys[keyring->nkeys].fingerprint,pkey);

	keyring->keys[keyring->nkeys].type=content_->tag;

	if(content_->tag == OPS_PTAG_CT_PUBLIC_KEY)
	    keyring->keys[keyring->nkeys].key.pkey=*pkey;
	else
	    keyring->keys[keyring->nkeys].key.skey=content->secret_key;
	return OPS_KEEP_MEMORY;

    case OPS_PTAG_CT_USER_ID:
	//	printf("User ID: %s\n",content->user_id.user_id);
        if (!cur)
            {
            OPS_ERROR(cbinfo->errors,OPS_E_P_NO_USERID, "No user id found");
            return OPS_KEEP_MEMORY;
            }
        //	assert(cur);
    ops_add_userid_to_keydata(cur, &content->user_id);
	return OPS_KEEP_MEMORY;

    case OPS_PARSER_PACKET_END:
	if(!cur)
	    return OPS_RELEASE_MEMORY;
    ops_add_packet_to_keydata(cur, &content->packet);
	return OPS_KEEP_MEMORY;

    case OPS_PARSER_ERROR:
	fprintf(stderr,"Error: %s\n",content->error.error);
	assert(0);
	break;

    case OPS_PARSER_ERRCODE:
	switch(content->errcode.errcode)
	    {
	default:
	    fprintf(stderr,"parse error: %s\n",
		    ops_errcode(content->errcode.errcode));
	    //assert(0);
	    }
	break;

    default:
	break;
	}

    // XXX: we now exclude so many things, we should either drop this or
    // do something to pass on copies of the stuff we keep
    return ops_parse_stacked_cb(content_,cbinfo);
    }

/**
 * \ingroup Core_Parse
 *
 * Parse packets from an input stream until EOF or error.
 *
 * Key data found in the parsed data is added to #keyring.
 *
 * \param keyring Pointer to an existing keyring
 * \param parse_info Options to use when parsing
*/

int ops_parse_and_accumulate(ops_keyring_t *keyring,
			      ops_parse_info_t *parse_info)
    {
    int rtn;

    accumulate_arg_t arg;

    assert(!parse_info->rinfo.accumulate);

    memset(&arg,'\0',sizeof arg);

    arg.keyring=keyring;
    /* Kinda weird, but to do with counting, and we put it back after */
    --keyring->nkeys;

    ops_parse_cb_push(parse_info,accumulate_cb,&arg);

    parse_info->rinfo.accumulate=ops_true;

    rtn=ops_parse(parse_info);
    ++keyring->nkeys;

    return rtn;
    }

static void dump_one_keydata(const ops_keydata_t *key)
    {
    unsigned n;

    printf("Key ID: ");
    hexdump(key->key_id,8);

    printf("\nFingerpint: ");
    hexdump(key->fingerprint.fingerprint,key->fingerprint.length);

    printf("\n\nUIDs\n====\n\n");
    for(n=0 ; n < key->nuids ; ++n)
	printf("%s\n",key->uids[n].user_id);

    printf("\nPackets\n=======\n");
    for(n=0 ; n < key->npackets ; ++n)
	{
	printf("\n%03d: ",n);
	hexdump(key->packets[n].raw,key->packets[n].length);
	}
    printf("\n\n");
    }

// XXX: not a maintained part of the API - use ops_keyring_list()
/** ops_dump_keyring
*/
void ops_dump_keyring(const ops_keyring_t *keyring)
    {
    int n;

    for(n=0 ; n < keyring->nkeys ; ++n)
	dump_one_keydata(&keyring->keys[n]);
    }

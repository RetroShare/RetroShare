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

#ifndef OPS_KEYRING_H
#define OPS_KEYRING_H

#include "packet.h"
#include "memory.h"

typedef struct ops_keydata ops_keydata_t;

/** \struct ops_keyring_t
 * A keyring
 */

typedef struct
    {
    int nkeys; // while we are constructing a key, this is the offset
    int nkeys_allocated;
    ops_keydata_t *keys;
    } ops_keyring_t;    

const ops_keydata_t *
ops_keyring_find_key_by_id(const ops_keyring_t *keyring,
			   const unsigned char keyid[OPS_KEY_ID_SIZE]);
const ops_keydata_t *
ops_keyring_find_key_by_userid(const ops_keyring_t *keyring,
			       const char* userid);
void ops_keydata_free(ops_keydata_t *key);
void ops_keyring_free(ops_keyring_t *keyring);
void ops_dump_keyring(const ops_keyring_t *keyring);
const ops_public_key_t *
ops_get_public_key_from_data(const ops_keydata_t *data);
ops_boolean_t ops_is_key_secret(const ops_keydata_t *data);
const ops_secret_key_t *
ops_get_secret_key_from_data(const ops_keydata_t *data);
ops_secret_key_t *
ops_get_writable_secret_key_from_data(ops_keydata_t *data);
ops_secret_key_t *ops_decrypt_secret_key_from_data(const ops_keydata_t *key,
						   const char *pphrase);

ops_boolean_t ops_keyring_read_from_file(ops_keyring_t *keyring, const ops_boolean_t armour, const char *filename);
ops_boolean_t ops_keyring_read_from_mem(ops_keyring_t *keyring, const ops_boolean_t armour, ops_memory_t *mem);

char *ops_malloc_passphrase(char *passphrase);
char *ops_get_passphrase(void);

void ops_keyring_list(const ops_keyring_t* keyring);

void ops_set_secret_key(ops_parser_content_union_t* content,const ops_keydata_t *key);

const unsigned char* ops_get_key_id(const ops_keydata_t *key);
unsigned ops_get_user_id_count(const ops_keydata_t *key);
const unsigned char* ops_get_user_id(const ops_keydata_t *key, unsigned index);
ops_boolean_t ops_is_key_supported(const ops_keydata_t *key);
const ops_keydata_t* ops_keyring_get_key_by_index(const ops_keyring_t *keyring, int index);

ops_user_id_t* ops_add_userid_to_keydata(ops_keydata_t* keydata, const ops_user_id_t* userid);
ops_packet_t* ops_add_packet_to_keydata(ops_keydata_t* keydata, const ops_packet_t* packet);
void ops_add_signed_userid_to_keydata(ops_keydata_t* keydata, const ops_user_id_t* userid, const ops_packet_t* packet);

ops_boolean_t ops_add_selfsigned_userid_to_keydata(ops_keydata_t* keydata, ops_user_id_t* userid);

ops_keydata_t *ops_keydata_new(void);
void ops_keydata_init(ops_keydata_t* keydata, const ops_content_tag_t type);

#endif

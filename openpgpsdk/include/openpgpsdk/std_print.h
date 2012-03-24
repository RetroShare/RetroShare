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

#ifndef OPS_STD_PRINT_H
#define OPS_STD_PRINT_H

#include "openpgpsdk/packet.h"
#include "openpgpsdk/packet-parse.h"
#include "openpgpsdk/keyring.h"

void print_bn( const char *name, 
		      const BIGNUM *bn);
void ops_print_pk_session_key(ops_content_tag_t tag,
                          const ops_pk_session_key_t *key);
void ops_print_public_keydata(const ops_keydata_t *key);

void ops_print_public_keydata_verbose(const ops_keydata_t *key);
void ops_print_public_key(const ops_public_key_t *pkey);

void ops_print_secret_keydata(const ops_keydata_t *key);
void ops_print_secret_keydata_verbose(const ops_keydata_t *key);
//void ops_print_secret_key(const ops_content_tag_t type, const ops_secret_key_t* skey);
int ops_print_packet(const ops_parser_content_t *content_);
void ops_list_packets(char *filename, ops_boolean_t armour, ops_keyring_t* pubring, ops_parse_cb_t* cb_get_passphrase);

#endif

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

#ifndef __OPS_ARMOUR_H__
#define __OPS_ARMOUR_H__

#include "packet-parse.h"
#include "signature.h"

unsigned ops_crc24(unsigned checksum,unsigned char c);

void ops_reader_push_dearmour(ops_parse_info_t *parse_info);

void ops_reader_pop_dearmour(ops_parse_info_t *parse_info);
ops_boolean_t ops_writer_push_clearsigned(ops_create_info_t *info,
				  ops_create_signature_t *sig);
void ops_writer_push_armoured_message(ops_create_info_t *info);
ops_boolean_t ops_writer_switch_to_armoured_signature(ops_create_info_t *info);

typedef enum 
    {
    OPS_PGP_MESSAGE=1,
    OPS_PGP_PUBLIC_KEY_BLOCK,
    OPS_PGP_PRIVATE_KEY_BLOCK,
    OPS_PGP_MULTIPART_MESSAGE_PART_X_OF_Y,
    OPS_PGP_MULTIPART_MESSAGE_PART_X,
    OPS_PGP_SIGNATURE
    } ops_armor_type_t;

void ops_writer_push_armoured(ops_create_info_t *info, ops_armor_type_t type);

#define CRC24_INIT 0xb704ceL

#endif /* __OPS_ARMOUR_H__ */

// EOF

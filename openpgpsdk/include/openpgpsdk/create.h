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

#ifndef OPS_CREATE_H
#define OPS_CREATE_H

#include <openpgpsdk/types.h>
#include <openpgpsdk/packet.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/memory.h>
#include <openpgpsdk/errors.h>
#include <openpgpsdk/keyring.h>
#include <openpgpsdk/writer.h>

/**
 * \ingroup Create
 * This struct contains the required information about how to write this stream
 */
struct ops_create_info
    {
    ops_writer_info_t winfo;
    ops_error_t *errors;	/*!< an error stack */
    };

ops_create_info_t *ops_create_info_new(void);
void ops_create_info_delete(ops_create_info_t *info);

ops_memory_t* ops_write_mem_from_file(const char *filename, int* errnum);
int ops_write_file_from_buf(const char *filename, const char* buf, const size_t len, const ops_boolean_t overwrite);

ops_boolean_t ops_calc_session_key_checksum(ops_pk_session_key_t *session_key, unsigned char *cs);
void ops_build_public_key(ops_memory_t *out,const ops_public_key_t *key,
			  ops_boolean_t make_packet);
ops_boolean_t ops_write_struct_user_id(ops_user_id_t *id,
				       ops_create_info_t *info);
ops_boolean_t ops_write_struct_public_key(const ops_public_key_t *key,
					  ops_create_info_t *info);

ops_boolean_t ops_write_ss_header(unsigned length,ops_content_tag_t type,
				  ops_create_info_t *info);
ops_boolean_t ops_write_struct_secret_key(const ops_secret_key_t *key,
                                          const unsigned char* passphrase,
                                          const size_t pplen,
					  ops_create_info_t *info);
ops_boolean_t ops_write_one_pass_sig(const ops_secret_key_t* skey,
                                     const ops_hash_algorithm_t hash_alg,
                                     const ops_sig_type_t sig_type,
                                     ops_create_info_t* info);
ops_boolean_t ops_write_literal_data_from_buf(const unsigned char *data, 
                                              const int maxlen, 
                                              const ops_literal_data_type_t type,
                                              ops_create_info_t *info);
ops_pk_session_key_t *ops_create_pk_session_key(const ops_keydata_t *key);
ops_boolean_t ops_write_pk_session_key(ops_create_info_t *info,
				       ops_pk_session_key_t *pksk);
ops_boolean_t ops_write_transferable_public_key(const ops_keydata_t *key, ops_boolean_t armoured, ops_create_info_t *info);
ops_boolean_t ops_write_transferable_secret_key(const ops_keydata_t *key, const unsigned char* passphrase, const size_t pplen, ops_boolean_t armoured, ops_create_info_t *info);

#endif /*OPS_CREATE_H*/

// eof

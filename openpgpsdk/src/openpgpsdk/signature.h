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

#ifndef OPS_SIGNATURE_H
#define OPS_SIGNATURE_H

#include "packet.h"
#include "util.h"
#include "create.h"

typedef struct ops_create_signature ops_create_signature_t;

ops_create_signature_t *ops_create_signature_new(void);
void ops_create_signature_delete(ops_create_signature_t *sig);

ops_boolean_t
ops_check_user_id_certification_signature(const ops_public_key_t *key,
					  const ops_user_id_t *id,
					  const ops_signature_t *sig,
					  const ops_public_key_t *signer,
					  const unsigned char *raw_packet);
ops_boolean_t
ops_check_user_attribute_certification_signature(const ops_public_key_t *key,
						 const ops_user_attribute_t *attribute,
						 const ops_signature_t *sig,
						 const ops_public_key_t *signer,
						 const unsigned char *raw_packet);
ops_boolean_t
ops_check_subkey_signature(const ops_public_key_t *key,
			   const ops_public_key_t *subkey,
			   const ops_signature_t *sig,
			   const ops_public_key_t *signer,
			   const unsigned char *raw_packet);
ops_boolean_t
ops_check_direct_signature(const ops_public_key_t *key,
			   const ops_signature_t *sig,
			   const ops_public_key_t *signer,
			   const unsigned char *raw_packet);
ops_boolean_t
ops_check_hash_signature(ops_hash_t *hash,
			 const ops_signature_t *sig,
			 const ops_public_key_t *signer);
void ops_signature_start_key_signature(ops_create_signature_t *sig,
				       const ops_public_key_t *key,
				       const ops_user_id_t *id,
				       ops_sig_type_t type);
void ops_signature_start_cleartext_signature(ops_create_signature_t *sig,
					     const ops_secret_key_t *key,
					     const ops_hash_algorithm_t hash,
					     const ops_sig_type_t type);
void ops_signature_start_message_signature(ops_create_signature_t *sig,
					     const ops_secret_key_t *key,
					     const ops_hash_algorithm_t hash,
					     const ops_sig_type_t type);

void ops_signature_add_data(ops_create_signature_t *sig,const void *buf,
			    size_t length);
ops_hash_t *ops_signature_get_hash(ops_create_signature_t *sig);
ops_boolean_t ops_signature_hashed_subpackets_end(ops_create_signature_t *sig);
ops_boolean_t ops_write_signature(ops_create_signature_t *sig,const ops_public_key_t *key,
			 const ops_secret_key_t *skey, ops_create_info_t *opt);
ops_boolean_t ops_signature_add_creation_time(ops_create_signature_t *sig,time_t when);
ops_boolean_t ops_signature_add_issuer_key_id(ops_create_signature_t *sig,
				     const unsigned char keyid[OPS_KEY_ID_SIZE]);
void ops_signature_add_primary_user_id(ops_create_signature_t *sig,
				       ops_boolean_t primary);

// Standard Interface
ops_boolean_t ops_sign_file_as_cleartext(const char* input_filename, const char* output_filename, const ops_secret_key_t *skey, const ops_boolean_t overwrite);
ops_boolean_t ops_sign_buf_as_cleartext(const char* input, const size_t len, ops_memory_t** output, const ops_secret_key_t *skey);
ops_boolean_t ops_sign_file(const char* input_filename, const char* output_filename, const ops_secret_key_t *skey, const ops_boolean_t use_armour, const ops_boolean_t overwrite);
ops_memory_t * ops_sign_buf(const void* input, const size_t input_len, const ops_sig_type_t sig_type, const ops_hash_algorithm_t hash_algorithm,  const ops_secret_key_t *skey, const ops_boolean_t use_armour, ops_boolean_t include_data, ops_boolean_t include_creation_time, ops_boolean_t include_key_id);
ops_boolean_t ops_writer_push_signed(ops_create_info_t *cinfo, const ops_sig_type_t sig_type, const ops_secret_key_t *skey);

#endif

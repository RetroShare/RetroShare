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


typedef struct
    {
    unsigned int valid_count;
    ops_signature_info_t * valid_sigs;
    unsigned int invalid_count;
    ops_signature_info_t * invalid_sigs;
    unsigned int unknown_signer_count;
    ops_signature_info_t * unknown_sigs;
    } ops_validate_result_t;

void ops_validate_result_free(ops_validate_result_t *result);

ops_boolean_t ops_validate_key_signatures(ops_validate_result_t *result,
                                 const ops_keydata_t* keydata,
                                 const ops_keyring_t *ring,
                                 ops_parse_cb_return_t cb (const ops_parser_content_t *, ops_parse_cb_info_t *));
ops_boolean_t ops_validate_all_signatures(ops_validate_result_t *result,
                                 const ops_keyring_t *ring,
                                 ops_parse_cb_return_t (const ops_parser_content_t *, ops_parse_cb_info_t *));

void ops_keydata_reader_set(ops_parse_info_t *pinfo,
			     const ops_keydata_t *key);

typedef struct
    {
    const ops_keydata_t *key;
    unsigned packet;
    unsigned offset;
    } validate_reader_arg_t;

/** Struct used with the validate_key_cb callback */
typedef struct
    {
    ops_public_key_t pkey;
    ops_public_key_t subkey;
    ops_secret_key_t skey;
    enum
	{
	ATTRIBUTE=1,
	ID,
	} last_seen;
    ops_user_id_t user_id;
    ops_user_attribute_t user_attribute;
    unsigned char hash[OPS_MAX_HASH_SIZE];
    const ops_keyring_t *keyring;
    validate_reader_arg_t *rarg;
    ops_validate_result_t *result;
    ops_parse_cb_return_t (*cb_get_passphrase) (const ops_parser_content_t *, ops_parse_cb_info_t *);
    } validate_key_cb_arg_t;

/** Struct use with the validate_data_cb callback */
typedef struct
    {
    enum
        {
        LITERAL_DATA,
        SIGNED_CLEARTEXT
        } use; /*<! this is set to indicate what kind of data we have */
    union
        {
        ops_literal_data_body_t literal_data_body; /*<! Used to hold Literal Data */
        ops_signed_cleartext_body_t signed_cleartext_body; /*<! Used to hold Signed Cleartext */
        } data; /*<! the data itself */
    unsigned char hash[OPS_MAX_HASH_SIZE]; /*<! the hash */
    const ops_keyring_t *keyring; /*<! keyring to use */
    validate_reader_arg_t *rarg; /*<! reader-specific arg */
    ops_validate_result_t *result; /*<! where to put the result */
    } validate_data_cb_arg_t; /*<! used with validate_data_cb callback */

ops_boolean_t ops_check_signature(const unsigned char *hash,
                                  unsigned length,
                                  const ops_signature_t *sig,
                                  const ops_public_key_t *signer);
ops_parse_cb_return_t
ops_validate_key_cb(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);

ops_boolean_t ops_validate_file(ops_validate_result_t* result, const char* filename, const int armoured, const ops_keyring_t* keyring);
ops_boolean_t ops_validate_mem(ops_validate_result_t *result, ops_memory_t* mem, const int armoured, const ops_keyring_t* keyring);

// EOF

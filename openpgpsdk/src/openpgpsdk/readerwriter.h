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

#ifndef __OPS_READERWRITER_H__
#define __OPS_READERWRITER_H__

#include <openpgpsdk/memory.h>
#include <openpgpsdk/create.h>


void ops_reader_set_fd(ops_parse_info_t *pinfo,int fd);
void ops_reader_set_memory(ops_parse_info_t *pinfo,const void *buffer,
			   size_t length);

// Do a sum mod 65536 of all bytes read (as needed for secret keys)
void ops_reader_push_sum16(ops_parse_info_t *pinfo);
unsigned short ops_reader_pop_sum16(ops_parse_info_t *pinfo);

void ops_reader_push_se_ip_data(ops_parse_info_t *pinfo, ops_crypt_t *decrypt,
                                ops_region_t *region);
void ops_reader_pop_se_ip_data(ops_parse_info_t* pinfo);

//
ops_boolean_t ops_write_mdc(const unsigned char *hashed,
                                   ops_create_info_t* info);
ops_boolean_t ops_write_se_ip_pktset(const unsigned char *data,
                                            const unsigned int len,
                                            ops_crypt_t *crypt,
                                            ops_create_info_t *info);
void ops_writer_push_encrypt_crypt(ops_create_info_t *cinfo,
                                   ops_crypt_t *crypt);
void ops_writer_push_encrypt_se_ip(ops_create_info_t *cinfo,
                                   const ops_keydata_t *pub_key);
// Secret Key checksum

void ops_push_skey_checksum_writer(ops_create_info_t *cinfo, ops_secret_key_t *skey);
ops_boolean_t ops_pop_skey_checksum_writer(ops_create_info_t *cinfo);


// memory writing
void ops_setup_memory_write(ops_create_info_t **cinfo, ops_memory_t **mem, size_t bufsz);
void ops_teardown_memory_write(ops_create_info_t *cinfo, ops_memory_t *mem);

// memory reading
void ops_setup_memory_read(ops_parse_info_t **pinfo, ops_memory_t *mem,
                           void* arg,
                           ops_parse_cb_return_t callback(const ops_parser_content_t *, ops_parse_cb_info_t *),ops_boolean_t accumulate);
void ops_teardown_memory_read(ops_parse_info_t *pinfo, ops_memory_t *mem);

// file writing
int ops_setup_file_write(ops_create_info_t **cinfo, const char* filename, ops_boolean_t allow_overwrite);
void ops_teardown_file_write(ops_create_info_t *cinfo, int fd);

// file appending
int ops_setup_file_append(ops_create_info_t **cinfo, const char* filename);
void ops_teardown_file_append(ops_create_info_t *cinfo, int fd);

// file reading
int ops_setup_file_read(ops_parse_info_t **pinfo, const char *filename, void* arg,
                        ops_parse_cb_return_t callback(const ops_parser_content_t *, ops_parse_cb_info_t *), ops_boolean_t accumulate);
void ops_teardown_file_read(ops_parse_info_t *pinfo, int fd);

ops_boolean_t ops_reader_set_accumulate(ops_parse_info_t* pinfo, ops_boolean_t state);

// useful callbacks
ops_parse_cb_return_t
callback_literal_data(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);
ops_parse_cb_return_t
callback_pk_session_key(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);
ops_parse_cb_return_t
callback_cmd_get_secret_key(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);
ops_parse_cb_return_t
callback_cmd_get_passphrase_from_cmdline(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);

#endif /*OPS_READERWRITER_H__*/

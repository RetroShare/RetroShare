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
 * Parser for OpenPGP packets - headers.
 */

#ifndef OPS_PACKET_PARSE_H
#define OPS_PACKET_PARSE_H

#include "types.h"
#include "packet.h"
#include "lists.h"

/** ops_region_t */
typedef struct ops_region
    {
    struct ops_region *parent;
    unsigned length;
    unsigned length_read;
    unsigned last_read; /*!< length of last read, only valid in deepest child */
    ops_boolean_t indeterminate:1;
    } ops_region_t;

void ops_init_subregion(ops_region_t *subregion,ops_region_t *region);

#if 0
/** Return values for reader functions e.g. ops_packet_reader_t() */
enum ops_reader_ret_t
    {
    OPS_R_OK		=0,	/*!< success */
    OPS_R_EOF		=1,	/*!< reached end of file, no data has been returned */
    OPS_R_EARLY_EOF	=2,	/*!< could not read the requested
                                  number of bytes and either
                                  OPS_RETURN_LENGTH was not set and at
                                  least 1 byte was read, or there was
				  an abnormal end to the file (or
				  armoured block) */
    OPS_R_PARTIAL_READ	=3,	/*!< if OPS_RETURN_LENGTH is set and
				  the buffer was not filled */
    OPS_R_ERROR		=4,	/*!< if there was an error reading */
    };
#endif

/** ops_parse_callback_return_t */
typedef enum
    {
    OPS_RELEASE_MEMORY,
    OPS_KEEP_MEMORY,
    OPS_FINISHED
    } ops_parse_cb_return_t;

typedef struct ops_parse_cb_info ops_parse_cb_info_t;

typedef ops_parse_cb_return_t
ops_parse_cb_t(const ops_parser_content_t *content,
	       ops_parse_cb_info_t *cbinfo);

typedef struct ops_parse_info ops_parse_info_t;
typedef struct ops_reader_info ops_reader_info_t;
typedef struct ops_crypt_info ops_crypt_info_t;

/*
   A reader MUST read at least one byte if it can, and should read up
   to the number asked for. Whether it reads more for efficiency is
   its own decision, but if it is a stacked reader it should never
   read more than the length of the region it operates in (which it
   would have to be given when it is stacked).

   If a read is short because of EOF, then it should return the short
   read (obviously this will be zero on the second attempt, if not the
   first). Because a reader is not obliged to do a full read, only a
   zero return can be taken as an indication of EOF.

   If there is an error, then the callback should be notified, the
   error stacked, and -1 should be returned.

   Note that although length is a size_t, a reader will never be asked
   to read more than INT_MAX in one go.

 */

typedef int ops_reader_t(void *dest,size_t length,ops_error_t **errors,
			 ops_reader_info_t *rinfo,ops_parse_cb_info_t *cbinfo);

typedef void ops_reader_destroyer_t(ops_reader_info_t *rinfo);

ops_parse_info_t *ops_parse_info_new(void);
void ops_parse_info_delete(ops_parse_info_t *pinfo);
ops_error_t *ops_parse_info_get_errors(ops_parse_info_t *pinfo);
ops_crypt_t *ops_parse_get_decrypt(ops_parse_info_t *pinfo);

void ops_parse_cb_set(ops_parse_info_t *pinfo,ops_parse_cb_t *cb,void *arg);
void ops_parse_cb_push(ops_parse_info_t *pinfo,ops_parse_cb_t *cb,void *arg);
void *ops_parse_cb_get_arg(ops_parse_cb_info_t *cbinfo);
void *ops_parse_cb_get_errors(ops_parse_cb_info_t *cbinfo);
void ops_reader_set(ops_parse_info_t *pinfo,ops_reader_t *reader,ops_reader_destroyer_t *destroyer,void *arg);
void ops_reader_push(ops_parse_info_t *pinfo,ops_reader_t *reader,ops_reader_destroyer_t *destroyer,void *arg);
void ops_reader_pop(ops_parse_info_t *pinfo);
void *ops_reader_get_arg_from_pinfo(ops_parse_info_t *pinfo);

void *ops_reader_get_arg(ops_reader_info_t *rinfo);

ops_parse_cb_return_t ops_parse_cb(const ops_parser_content_t *content,
				   ops_parse_cb_info_t *cbinfo);
ops_parse_cb_return_t ops_parse_stacked_cb(const ops_parser_content_t *content,
					   ops_parse_cb_info_t *cbinfo);
ops_reader_info_t *ops_parse_get_rinfo(ops_parse_info_t *pinfo);

int ops_parse(ops_parse_info_t *parse_info);
int ops_parse_and_print_errors(ops_parse_info_t *parse_info);
int ops_parse_and_save_errs(ops_parse_info_t *parse_info,ops_ulong_list_t *errs);
int ops_parse_errs(ops_parse_info_t *parse_info,ops_ulong_list_t *errs);

void ops_parse_and_validate(ops_parse_info_t *parse_info);

/** Used to specify whether subpackets should be returned raw, parsed or ignored.
 */
enum ops_parse_type_t
    {
    OPS_PARSE_RAW,	/*!< Callback Raw */
    OPS_PARSE_PARSED,	/*!< Callback Parsed */
    OPS_PARSE_IGNORE, 	/*!< Don't callback */
    };

void ops_parse_options(ops_parse_info_t *pinfo,ops_content_tag_t tag,
		       ops_parse_type_t type);

ops_boolean_t ops_limited_read(unsigned char *dest,size_t length,
			       ops_region_t *region,ops_error_t **errors,
			       ops_reader_info_t *rinfo,
			       ops_parse_cb_info_t *cbinfo);
ops_boolean_t ops_stacked_limited_read(unsigned char *dest,unsigned length,
				       ops_region_t *region,
				       ops_error_t **errors,
				       ops_reader_info_t *rinfo,
				       ops_parse_cb_info_t *cbinfo);
void ops_parse_hash_init(ops_parse_info_t *pinfo,ops_hash_algorithm_t type,
			 const unsigned char *keyid);
void ops_parse_hash_data(ops_parse_info_t *pinfo,const void *data,
			 size_t length);
void ops_parse_hash_finish(ops_parse_info_t *pinfo);
ops_hash_t *ops_parse_hash_find(ops_parse_info_t *pinfo,
				const unsigned char keyid[OPS_KEY_ID_SIZE]);

ops_reader_t ops_stacked_read;

/* vim:set textwidth=120: */
/* vim:set ts=8: */


#endif

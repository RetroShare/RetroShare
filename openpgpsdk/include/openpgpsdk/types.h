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

#ifndef OPS_TYPES_H
#define OPS_TYPES_H

/** Special type for intermediate function casting, avoids warnings on
    some platforms
*/
typedef void (*ops_void_fptr)(void);
#define ops_fcast(f) ((ops_void_fptr)f)

/** ops_map_t
 */
typedef struct 
    {
    int type;
    char *string;
    } ops_map_t;

/** Boolean type */
typedef unsigned ops_boolean_t;

/** ops_content_tag_t */
typedef enum ops_content_tag_t ops_content_tag_t;

typedef struct _ops_crypt_t ops_crypt_t;

/** ops_hash_t */
typedef struct _ops_hash_t ops_hash_t;

/** 
   keep both ops_content_tag_t and ops_packet_tag_t because we might
   want to introduce some bounds checking i.e. is this really a valid value
   for a packet tag? 
*/
typedef enum ops_content_tag_t ops_packet_tag_t;
/** SS types are a subset of all content types.
*/
typedef enum ops_content_tag_t ops_ss_type_t;
/* typedef enum ops_sig_type_t ops_sig_type_t; */

/** Revocation Reason type */
typedef unsigned char ops_ss_rr_code_t;

/** ops_parse_type_t */
typedef enum ops_parse_type_t ops_parse_type_t;

/** ops_parser_content_t */
typedef struct ops_parser_content_t ops_parser_content_t;

/** Reader Flags */
/*
typedef enum
    {
    OPS_RETURN_LENGTH=1,
    } ops_reader_flags_t;
typedef enum ops_reader_ret_t ops_reader_ret_t;
*/

/** Writer flags */
typedef enum
    {
    OPS_WF_DUMMY,
    } ops_writer_flags_t;
/** ops_writer_ret_t */
typedef enum ops_writer_ret_t ops_writer_ret_t;

/**
 * \ingroup Create
 * Contains the required information about how to write
 */
typedef struct ops_create_info ops_create_info_t;

#endif

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

#ifndef OPS_ERRORS
#define OPS_ERRORS

#include "openpgpsdk/types.h"
#include <errno.h> 

/** error codes */
// Remember to add names to map in errors.c
typedef enum 
    {
    OPS_E_OK=0x0000,	/* no error */
    OPS_E_FAIL=0x0001,	/* general error */
    OPS_E_SYSTEM_ERROR=0x0002, /* system error, look at errno for details */
    OPS_E_UNIMPLEMENTED=0x0003, /* feature not yet implemented */

    /* reader errors */
    OPS_E_R=0x1000,	/* general reader error */
    OPS_E_R_READ_FAILED		=OPS_E_R+1,
    OPS_E_R_EARLY_EOF		=OPS_E_R+2,
    OPS_E_R_BAD_FORMAT		=OPS_E_R+3, // For example, malformed armour
    OPS_E_R_UNSUPPORTED		=OPS_E_R+4,
    OPS_E_R_UNCONSUMED_DATA	=OPS_E_R+5,

    /* writer errors */
    OPS_E_W=0x2000,	/* general writer error */
    OPS_E_W_WRITE_FAILED = OPS_E_W+1,
    OPS_E_W_WRITE_TOO_SHORT = OPS_E_W+2,

    /* parser errors */
    OPS_E_P=0x3000,	/* general parser error */
    OPS_E_P_NOT_ENOUGH_DATA	=OPS_E_P+1,
    OPS_E_P_UNKNOWN_TAG		=OPS_E_P+2,
    OPS_E_P_PACKET_CONSUMED	=OPS_E_P+3,
    OPS_E_P_MPI_FORMAT_ERROR	=OPS_E_P+4,
    OPS_E_P_PACKET_NOT_CONSUMED	=OPS_E_P+5,
    OPS_E_P_DECOMPRESSION_ERROR	=OPS_E_P+6,
    OPS_E_P_NO_USERID			=OPS_E_P+7,

    /* creator errors */
    OPS_E_C=0x4000,	/* general creator error */

    /* validation errors */
    OPS_E_V=0x5000, /* general validation error */
    OPS_E_V_BAD_SIGNATURE	=OPS_E_V+1,
    OPS_E_V_NO_SIGNATURE	=OPS_E_V+2,
    OPS_E_V_UNKNOWN_SIGNER	=OPS_E_V+3,
    OPS_E_V_BAD_HASH		=OPS_E_V+4,

    /* Algorithm support errors */
    OPS_E_ALG=0x6000,			/* general algorithm error */
    OPS_E_ALG_UNSUPPORTED_SYMMETRIC_ALG		=OPS_E_ALG+1,
    OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG	=OPS_E_ALG+2,
    OPS_E_ALG_UNSUPPORTED_SIGNATURE_ALG	=OPS_E_ALG+3,
    OPS_E_ALG_UNSUPPORTED_HASH_ALG		=OPS_E_ALG+4,
    OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG		=OPS_E_ALG+5,

    /* Protocol errors */
    OPS_E_PROTO=0x7000,	/* general protocol error */
    OPS_E_PROTO_BAD_SYMMETRIC_DECRYPT 	=OPS_E_PROTO+2,
    OPS_E_PROTO_UNKNOWN_SS		=OPS_E_PROTO+3,
    OPS_E_PROTO_CRITICAL_SS_IGNORED	=OPS_E_PROTO+4,
    OPS_E_PROTO_BAD_PUBLIC_KEY_VRSN	=OPS_E_PROTO+5,
    OPS_E_PROTO_BAD_SIGNATURE_VRSN	=OPS_E_PROTO+6,
    OPS_E_PROTO_BAD_ONE_PASS_SIG_VRSN	=OPS_E_PROTO+7,
    OPS_E_PROTO_BAD_PKSK_VRSN		=OPS_E_PROTO+8,
    OPS_E_PROTO_DECRYPTED_MSG_WRONG_LEN =OPS_E_PROTO+9,
    OPS_E_PROTO_BAD_SK_CHECKSUM		=OPS_E_PROTO+10,
    } ops_errcode_t;

/** ops_errcode_name_map_t */
typedef ops_map_t ops_errcode_name_map_t;

/** one entry in a linked list of errors */
typedef struct ops_error
    {
    ops_errcode_t errcode;
    int sys_errno; /*!< irrelevent unless errcode == OPS_E_SYSTEM_ERROR */
    char *comment;
    const char *file;
    int line;
    struct ops_error *next;
    } ops_error_t;

char *ops_errcode(const ops_errcode_t errcode);

void ops_push_error(ops_error_t **errstack,ops_errcode_t errcode,int sys_errno,
		const char *file,int line,const char *comment,...);
void ops_print_error(ops_error_t *err);
void ops_print_errors(ops_error_t *errstack);
void ops_free_errors(ops_error_t *errstack);
int ops_has_error(ops_error_t *errstack, ops_errcode_t errcode);

#define OPS_SYSTEM_ERROR_1(err,code,syscall,fmt,arg)	do { ops_push_error(err,OPS_E_SYSTEM_ERROR,errno,__FILE__,__LINE__,syscall); ops_push_error(err,code,0,__FILE__,__LINE__,fmt,arg); } while(0)
#define OPS_MEMORY_ERROR(err) {fprintf(stderr, "Memory error\n");} // \todo placeholder for better error handling
#define OPS_ERROR(err,code,fmt)	do { ops_push_error(err,code,0,__FILE__,__LINE__,fmt); } while(0)
#define OPS_ERROR_1(err,code,fmt,arg)	do { ops_push_error(err,code,0,__FILE__,__LINE__,fmt,arg); } while(0)
#define OPS_ERROR_2(err,code,fmt,arg,arg2)	do { ops_push_error(err,code,0,__FILE__,__LINE__,fmt,arg,arg2); } while(0)
#define OPS_ERROR_3(err,code,fmt,arg,arg2,arg3)	do { ops_push_error(err,code,0,__FILE__,__LINE__,fmt,arg,arg2,arg3); } while(0)
#define OPS_ERROR_4(err,code,fmt,arg,arg2,arg3,arg4)	do { ops_push_error(err,code,0,__FILE__,__LINE__,fmt,arg,arg2,arg3,arg4); } while(0)

#endif /* OPS_ERRORS */

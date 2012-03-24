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
 * \brief Error Handling
 */

#include <openpgpsdk/errors.h>
#include <openpgpsdk/util.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

#include <openpgpsdk/packet-show.h>
#include <openpgpsdk/final.h>

#define ERRNAME(code)	{ code, #code }

static ops_errcode_name_map_t errcode_name_map[] = 
    {
    ERRNAME(OPS_E_OK),
    ERRNAME(OPS_E_FAIL),
    ERRNAME(OPS_E_SYSTEM_ERROR),
    ERRNAME(OPS_E_UNIMPLEMENTED),

    ERRNAME(OPS_E_R),
    ERRNAME(OPS_E_R_READ_FAILED),
    ERRNAME(OPS_E_R_EARLY_EOF),
    ERRNAME(OPS_E_R_BAD_FORMAT),
    ERRNAME(OPS_E_R_UNCONSUMED_DATA),

    ERRNAME(OPS_E_W),
    ERRNAME(OPS_E_W_WRITE_FAILED),
    ERRNAME(OPS_E_W_WRITE_TOO_SHORT),

    ERRNAME(OPS_E_P),
    ERRNAME(OPS_E_P_NOT_ENOUGH_DATA),
    ERRNAME(OPS_E_P_UNKNOWN_TAG),
    ERRNAME(OPS_E_P_PACKET_CONSUMED),
    ERRNAME(OPS_E_P_MPI_FORMAT_ERROR),

    ERRNAME(OPS_E_C),

    ERRNAME(OPS_E_V),
    ERRNAME(OPS_E_V_BAD_SIGNATURE),
    ERRNAME(OPS_E_V_NO_SIGNATURE),
    ERRNAME(OPS_E_V_UNKNOWN_SIGNER),

    ERRNAME(OPS_E_ALG),
    ERRNAME(OPS_E_ALG_UNSUPPORTED_SYMMETRIC_ALG),
    ERRNAME(OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG),
    ERRNAME(OPS_E_ALG_UNSUPPORTED_SIGNATURE_ALG),
    ERRNAME(OPS_E_ALG_UNSUPPORTED_HASH_ALG),

    ERRNAME(OPS_E_PROTO),
    ERRNAME(OPS_E_PROTO_BAD_SYMMETRIC_DECRYPT),
    ERRNAME(OPS_E_PROTO_UNKNOWN_SS),
    ERRNAME(OPS_E_PROTO_CRITICAL_SS_IGNORED),
    ERRNAME(OPS_E_PROTO_BAD_PUBLIC_KEY_VRSN),
    ERRNAME(OPS_E_PROTO_BAD_SIGNATURE_VRSN),
    ERRNAME(OPS_E_PROTO_BAD_ONE_PASS_SIG_VRSN),
    ERRNAME(OPS_E_PROTO_BAD_PKSK_VRSN),
    ERRNAME(OPS_E_PROTO_DECRYPTED_MSG_WRONG_LEN),
    ERRNAME(OPS_E_PROTO_BAD_SK_CHECKSUM),

    { 0x00,		NULL }, /* this is the end-of-array marker */
    };

/**
 * \ingroup Core_Errors
 * \brief returns error code name
 * \param errcode
 * \return error code name or "Unknown"
 */
char *ops_errcode(const ops_errcode_t errcode)
    {
    return(ops_str_from_map((int) errcode, (ops_map_t *) errcode_name_map));
    }

/** 
 * \ingroup Core_Errors
 * \brief Pushes the given error on the given errorstack
 * \param errstack Error stack to use
 * \param errcode Code of error to push
 * \param sys_errno System errno (used if errcode=OPS_E_SYSTEM_ERROR)
 * \param file Source filename where error occurred
 * \param line Line in source file where error occurred
 * \param fmt Comment
 *
 */

void ops_push_error(ops_error_t **errstack,ops_errcode_t errcode,int sys_errno,
		const char *file,int line,const char *fmt,...)
    {
    // first get the varargs and generate the comment
    char *comment;
    int maxbuf=128;
    va_list args;
    ops_error_t *err;
    
    comment=malloc(maxbuf+1);
    assert(comment);

    va_start(args, fmt);
    vsnprintf(comment,maxbuf+1,fmt,args);
    va_end(args);

    // alloc a new error and add it to the top of the stack

    err=malloc(sizeof(ops_error_t));
    assert(err);

    err->next=*errstack;
    *errstack=err;

    // fill in the details
    err->errcode=errcode;
    err->sys_errno=sys_errno;
    err->file=file;
    err->line=line;

    err->comment=comment;
    }

/**
\ingroup Core_Errors
\brief print this error
\param err Error to print
*/
void ops_print_error(ops_error_t *err)
    {
    printf("%s:%d: ",err->file,err->line);
    if(err->errcode==OPS_E_SYSTEM_ERROR)
	printf("system error %d returned from %s()\n",err->sys_errno,
	       err->comment);
    else
	printf("%s, %s\n",ops_errcode(err->errcode),err->comment);
    }

/**
\ingroup Core_Errors
\brief Print all errors on stack
\param errstack Error stack to print
*/
void ops_print_errors(ops_error_t *errstack)
    {
    ops_error_t *err;

    for(err=errstack ; err!=NULL ; err=err->next)
	ops_print_error(err);
    }

/**
\ingroup Core_Errors
\brief Return true if given error is present anywhere on stack
\param errstack Error stack to check
\param errcode Error code to look for
\return 1 if found; else 0
*/
int ops_has_error(ops_error_t *errstack, ops_errcode_t errcode)
    {
    ops_error_t *err;
    for (err=errstack; err!=NULL; err=err->next)
        {
        if (err->errcode==errcode)
            return 1;
        }
    return 0;
    }

/**
\ingroup Core_Errors
\brief Frees all errors on stack
\param errstack Error stack to free
*/
void ops_free_errors(ops_error_t *errstack)
{
    ops_error_t *next;
    while(errstack!=NULL) {
        next=errstack->next;
        free(errstack->comment);
        free(errstack);
        errstack=next;
    }
}

// EOF

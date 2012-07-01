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

#include <openpgpsdk/util.h>
#include <openpgpsdk/packet-parse.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/create.h>
#include <openpgpsdk/errors.h>
#include <stdio.h>
#include <assert.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <string.h>

#include <openpgpsdk/final.h>

typedef struct
    {
    const unsigned char *buffer;
    size_t length;
    size_t offset;
    } reader_mem_arg_t;

static int mem_reader(void *dest,size_t length,ops_error_t **errors,
		      ops_reader_info_t *rinfo,ops_parse_cb_info_t *cbinfo)
    {
    reader_mem_arg_t *arg=ops_reader_get_arg(rinfo);
    unsigned n;

    OPS_USED(cbinfo);
    OPS_USED(errors);

    if(arg->offset+length > arg->length)
	n=arg->length-arg->offset;
    else
	n=length;

    if(n == 0)
	return 0;

    memcpy(dest,arg->buffer+arg->offset,n);
    arg->offset+=n;

    return n;
    }

static void mem_destroyer(ops_reader_info_t *rinfo)
    { free(ops_reader_get_arg(rinfo)); }

/**
   \ingroup Core_Readers_First
   \brief Starts stack with memory reader
*/

void ops_reader_set_memory(ops_parse_info_t *pinfo,const void *buffer,
			   size_t length)
    {
    reader_mem_arg_t *arg=malloc(sizeof *arg);

    arg->buffer=buffer;
    arg->length=length;
    arg->offset=0;
    ops_reader_set(pinfo,mem_reader,mem_destroyer,arg);
    }

/* eof */

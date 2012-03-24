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

/** Arguments for reader_fd
 */
typedef struct
    {
    int fd; /*!< file descriptor */
    } reader_fd_arg_t;

/**
 * \ingroup Core_Readers
 *
 * ops_reader_fd() attempts to read up to "plength" bytes from the file 
 * descriptor in "parse_info" into the buffer starting at "dest" using the
 * rules contained in "flags"
 *
 * \param	dest	Pointer to previously allocated buffer
 * \param	plength Number of bytes to try to read
 * \param	flags	Rules about reading to use
 * \param	parse_info	Gets cast to ops_reader_fd_arg_t
 *
 * \return	OPS_R_EOF 	if no bytes were read
 * \return	OPS_R_PARTIAL_READ	if not enough bytes were read, and OPS_RETURN_LENGTH is set in "flags"
 * \return	OPS_R_EARLY_EOF	if not enough bytes were read, and OPS_RETURN_LENGTH was not set in "flags"
 * \return	OPS_R_OK	if expected length was read
 * \return 	OPS_R_ERROR	if cannot read
 *
 * OPS_R_EARLY_EOF and OPS_R_ERROR push errors on the stack
 *
 * \sa enum opt_reader_ret_t
 *
 * \todo change arg_ to typesafe? 
 */
static int fd_reader(void *dest,size_t length,ops_error_t **errors,
		     ops_reader_info_t *rinfo,ops_parse_cb_info_t *cbinfo)
    {
    reader_fd_arg_t *arg=ops_reader_get_arg(rinfo);
    int n=read(arg->fd,dest,length);

    OPS_USED(cbinfo);

    if(n == 0)
	return 0;

    if(n < 0)
	{
	OPS_SYSTEM_ERROR_1(errors,OPS_E_R_READ_FAILED,"read",
			   "file descriptor %d",arg->fd);
	return -1;
	}

    return n;
    }

static void fd_destroyer(ops_reader_info_t *rinfo)
    { free(ops_reader_get_arg(rinfo)); }

/**
   \ingroup Core_Readers_First
   \brief Starts stack with file reader
*/

void ops_reader_set_fd(ops_parse_info_t *pinfo,int fd)
    {
    reader_fd_arg_t *arg=malloc(sizeof *arg);

    arg->fd=fd;
    ops_reader_set(pinfo,fd_reader,fd_destroyer,arg);
    }

// eof

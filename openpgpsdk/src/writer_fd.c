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

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <openpgpsdk/create.h>

#include <openpgpsdk/final.h>

typedef struct
    {
    int fd;
    } writer_fd_arg_t;

static ops_boolean_t fd_writer(const unsigned char *src,unsigned length,
			       ops_error_t **errors,
			       ops_writer_info_t *winfo)
    {
    writer_fd_arg_t *arg=ops_writer_get_arg(winfo);
    int n=write(arg->fd,src,length);

    if(n == -1)
	{
	OPS_SYSTEM_ERROR_1(errors,OPS_E_W_WRITE_FAILED,"write",
			   "file descriptor %d",arg->fd);
	return ops_false;
	}

    if((unsigned)n != length)
	{
	OPS_ERROR_1(errors,OPS_E_W_WRITE_TOO_SHORT,
		    "file descriptor %d",arg->fd);
	return ops_false;
	}

    return ops_true;
    }

static void fd_destroyer(ops_writer_info_t *winfo)
    {
    free(ops_writer_get_arg(winfo));
    }

/**
 * \ingroup Core_WritersFirst
 * \brief Write to a File
 *
 * Set the writer in info to be a stock writer that writes to a file
 * descriptor. If another writer has already been set, then that is
 * first destroyed.
 * 
 * \param info The info structure
 * \param fd The file descriptor
 *
 */

void ops_writer_set_fd(ops_create_info_t *info,int fd)
    {
    writer_fd_arg_t *arg=malloc(sizeof *arg);

    arg->fd=fd;
    ops_writer_set(info,fd_writer,NULL,fd_destroyer,arg);
    }

// EOF

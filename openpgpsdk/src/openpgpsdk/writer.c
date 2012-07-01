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
 * This file contains the base functions used by the writers.
 */

#include <openssl/cast.h>

#include <openpgpsdk/armour.h>
#include <openpgpsdk/writer.h>
#include <openpgpsdk/keyring.h>
#include <openpgpsdk/memory.h>
#include <openpgpsdk/random.h>
#include <openpgpsdk/readerwriter.h>
#include "keyring_local.h"
#include <openpgpsdk/packet.h>
#include <openpgpsdk/util.h>
#include <openpgpsdk/std_print.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include <openpgpsdk/final.h>

//static int debug=0;

/*
 * return true if OK, otherwise false
 */
static ops_boolean_t base_write(const void *src,unsigned length,
				ops_create_info_t *info)
    {
    return info->winfo.writer(src,length,&info->errors,&info->winfo);
    }

/**
 * \ingroup Core_WritePackets
 *
 * \param src
 * \param length
 * \param info
 * \return 1 if OK, otherwise 0
 */

ops_boolean_t ops_write(const void *src,unsigned length,
			ops_create_info_t *info)
    {
    return base_write(src,length,info);
    }

/**
 * \ingroup Core_WritePackets
 * \param n
 * \param length
 * \param info
 * \return ops_true if OK, otherwise ops_false
 */

ops_boolean_t ops_write_scalar(unsigned n,unsigned length,
			       ops_create_info_t *info)
    {
    while(length-- > 0)
	{
	unsigned char c[1];

	c[0]=n >> (length*8);
	if(!base_write(c,1,info))
	    return ops_false;
	}
    return ops_true;
    }

/** 
 * \ingroup Core_WritePackets
 * \param bn
 * \param info
 * \return 1 if OK, otherwise 0
 */

ops_boolean_t ops_write_mpi(const BIGNUM *bn,ops_create_info_t *info)
    {
    unsigned char buf[8192];
    int bits=BN_num_bits(bn);

    assert(bits <= 65535);
    BN_bn2bin(bn,buf);
    return ops_write_scalar(bits,2,info)
	&& ops_write(buf,(bits+7)/8,info);
    }

/** 
 * \ingroup Core_WritePackets
 * \param tag
 * \param info
 * \return 1 if OK, otherwise 0
 */

ops_boolean_t ops_write_ptag(ops_content_tag_t tag,ops_create_info_t *info)
    {
    unsigned char c[1];

    c[0]=tag|OPS_PTAG_ALWAYS_SET|OPS_PTAG_NEW_FORMAT;

    return base_write(c,1,info);
    }

/** 
 * \ingroup Core_WritePackets
 * \param length
 * \param info
 * \return 1 if OK, otherwise 0
 */

ops_boolean_t ops_write_length(unsigned length,ops_create_info_t *info)
    {
    unsigned char c[2];

    if(length < 192)
	{
	c[0]=length;
	return base_write(c,1,info);
	}
    else if(length < 8384)
	{
	c[0]=((length-192) >> 8)+192;
	c[1]=(length-192)%256;
	return base_write(c,2,info);
	}
    return ops_write_scalar(0xff,1,info) && ops_write_scalar(length,4,info);
    }

/* Note that we finalise from the top down, so we don't use writers below
 * that have already been finalised
 */
ops_boolean_t writer_info_finalise(ops_error_t **errors,
                                   ops_writer_info_t *winfo)
    {
    ops_boolean_t ret=ops_true;

    if(winfo->finaliser)
	{
	ret=winfo->finaliser(errors,winfo);
	winfo->finaliser=NULL;
	}
    if(winfo->next && !writer_info_finalise(errors,winfo->next))
	{
	winfo->finaliser=NULL;
	return ops_false;
	}
    return ret;
    }

void writer_info_delete(ops_writer_info_t *winfo)
    {
    // we should have finalised before deleting
    assert(!winfo->finaliser);
    if(winfo->next)
	{
	writer_info_delete(winfo->next);
	free(winfo->next);
	winfo->next=NULL;
	}
    if(winfo->destroyer)
	{
	winfo->destroyer(winfo);
	winfo->destroyer=NULL;
	}
    winfo->writer=NULL;
    }

/**
 * \ingroup Core_Writers
 *
 * Set a writer in info. There should not be another writer set.
 *
 * \param info The info structure
 * \param writer 
 * \param finaliser
 * \param destroyer 
 * \param arg The argument for the writer and destroyer
 */
void ops_writer_set(ops_create_info_t *info,
		    ops_writer_t *writer,
		    ops_writer_finaliser_t *finaliser,
		    ops_writer_destroyer_t *destroyer,
		    void *arg)
    {
    assert(!info->winfo.writer);
    info->winfo.writer=writer;
    info->winfo.finaliser=finaliser;
    info->winfo.destroyer=destroyer;
    info->winfo.arg=arg;
    }

/**
 * \ingroup Core_Writers
 *
 * Push a writer in info. There must already be another writer set.
 *
 * \param info The info structure
 * \param writer 
 * \param finaliser 
 * \param destroyer 
 * \param arg The argument for the writer and destroyer
 */
void ops_writer_push(ops_create_info_t *info,
		     ops_writer_t *writer,
		     ops_writer_finaliser_t *finaliser,
		     ops_writer_destroyer_t *destroyer,
		     void *arg)
    {
    ops_writer_info_t *copy=ops_mallocz(sizeof *copy);

    assert(info->winfo.writer);
    *copy=info->winfo;
    info->winfo.next=copy;

    info->winfo.writer=writer;
    info->winfo.finaliser=finaliser;
    info->winfo.destroyer=destroyer;
    info->winfo.arg=arg;
    }

void ops_writer_pop(ops_create_info_t *info)
    { 
    ops_writer_info_t *next;

    // Make sure the finaliser has been called.
    assert(!info->winfo.finaliser);
    // Make sure this is a stacked writer
    assert(info->winfo.next);
    if(info->winfo.destroyer)
	info->winfo.destroyer(&info->winfo);

    next=info->winfo.next;
    info->winfo=*next;

    free(next);
    }

/**
 * \ingroup Core_Writers
 *
 * Close the writer currently set in info.
 *
 * \param info The info structure
 */
ops_boolean_t ops_writer_close(ops_create_info_t *info)
    {
    ops_boolean_t ret=writer_info_finalise(&info->errors,&info->winfo);

    writer_info_delete(&info->winfo);

    return ret;
    }

/**
 * \ingroup Core_Writers
 *
 * Get the arg supplied to ops_create_info_set_writer().
 *
 * \param winfo The writer_info structure
 * \return The arg
 */
void *ops_writer_get_arg(ops_writer_info_t *winfo)
    { return winfo->arg; }

/**
 * \ingroup Core_Writers
 *
 * Write to the next writer down in the stack.
 *
 * \param src The data to write.
 * \param length The length of src.
 * \param errors A place to store errors.
 * \param winfo The writer_info structure.
 * \return Success - if ops_false, then errors should contain the error.
 */
ops_boolean_t ops_stacked_write(const void *src,unsigned length,
				ops_error_t **errors,ops_writer_info_t *winfo)
    {
    return winfo->next->writer(src,length,errors,winfo->next);
    }

/**
 * \ingroup Core_Writers
 *
 * Free the arg. Many writers just have a malloc()ed lump of storage, this
 * function releases it.
 *
 * \param winfo the info structure.
 */
void ops_writer_generic_destroyer(ops_writer_info_t *winfo)
    { free(ops_writer_get_arg(winfo)); }

/**
 * \ingroup Core_Writers
 *
 * A writer that just writes to the next one down. Useful for when you
 * want to insert just a finaliser into the stack.
 */
ops_boolean_t ops_writer_passthrough(const unsigned char *src,
				     unsigned length,
				     ops_error_t **errors,
				     ops_writer_info_t *winfo)
    { return ops_stacked_write(src,length,errors,winfo); }


// EOF

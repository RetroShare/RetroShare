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

#include <openpgpsdk/create.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <openpgpsdk/final.h>

struct ops_memory
    {
    unsigned char *buf;
    size_t length;
    size_t allocated;
    };

/**
\ingroup HighLevel_Memory
\brief Memory to initialise
\param mem memory to initialise
\param initial_size Size to initialise to
*/
void ops_memory_init(ops_memory_t *mem,size_t initial_size)
    {
    mem->length=0;
    if(mem->buf)
	{
	if(mem->allocated < initial_size)
	    {
	    mem->buf=realloc(mem->buf,initial_size);
	    mem->allocated=initial_size;
	    }
	return;
	}
    mem->buf=malloc(initial_size);
    mem->allocated=initial_size;
    }

/**
\ingroup HighLevel_Memory
\brief Pad memory to required length
\param mem Memory to use
\param length New size
*/
void ops_memory_pad(ops_memory_t *mem,size_t length)
    {
    assert(mem->allocated >= mem->length);
    if(mem->allocated < mem->length+length)
	{
	mem->allocated=mem->allocated*2+length;
	mem->buf=realloc(mem->buf,mem->allocated);
	}
    assert(mem->allocated >= mem->length+length);
    }

/**
\ingroup HighLevel_Memory
\brief Add data to memory
\param mem Memory to which to add
\param src Data to add
\param length Length of data to add
*/
void ops_memory_add(ops_memory_t *mem,const unsigned char *src,size_t length)
    {
    ops_memory_pad(mem,length);
    memcpy(mem->buf+mem->length,src,length);
    mem->length+=length;
    }

// XXX: this could be refactored via the writer, but an awful lot of
// hoops to jump through for 2 lines of code!
void ops_memory_place_int(ops_memory_t *mem,unsigned offset,unsigned n,
			  size_t length)
    {
    assert(mem->allocated >= offset+length);
    
    while(length--)
	mem->buf[offset++]=n >> (length*8);
    }

/**
 * \ingroup HighLevel_Memory
 * \brief Retains allocated memory and set length of stored data to zero.
 * \param mem Memory to clear
 * \sa ops_memory_release()
 * \sa ops_memory_free()
 */
void ops_memory_clear(ops_memory_t *mem)
    { mem->length=0; }

/**
\ingroup HighLevel_Memory
\brief Free memory and associated data
\param mem Memory to free
\note This does not free mem itself
\sa ops_memory_clear()
\sa ops_memory_free()
*/
void ops_memory_release(ops_memory_t *mem)
    {
    free(mem->buf);
    mem->buf=NULL;
    mem->length=0;
    }

void ops_memory_make_packet(ops_memory_t *out,ops_content_tag_t tag)
    {
    size_t extra;

    if(out->length < 192)
	extra=1;
    else if(out->length < 8384)
	extra=2;
    else
	extra=5;

    ops_memory_pad(out,extra+1);
    memmove(out->buf+extra+1,out->buf,out->length);

    out->buf[0]=OPS_PTAG_ALWAYS_SET|OPS_PTAG_NEW_FORMAT|tag;

    if(out->length < 192)
	out->buf[1]=out->length;
    else if(out->length < 8384)
	{
	out->buf[1]=((out->length-192) >> 8)+192;
	out->buf[2]=out->length-192;
	}
    else
	{
	out->buf[1]=0xff;
	out->buf[2]=out->length >> 24;
	out->buf[3]=out->length >> 16;
	out->buf[4]=out->length >> 8;
	out->buf[5]=out->length;
	}

    out->length+=extra+1;
    }

/**
   \ingroup HighLevel_Memory
   \brief Create a new zeroed ops_memory_t
   \return Pointer to new ops_memory_t
   \note Free using ops_memory_free() after use.
   \sa ops_memory_free()
*/

ops_memory_t *ops_memory_new()
    { return ops_mallocz(sizeof(ops_memory_t)); }

/**
   \ingroup HighLevel_Memory
   \brief Free memory ptr and associated memory
   \param mem Memory to be freed
   \sa ops_memory_release()
   \sa ops_memory_clear()
*/

void ops_memory_free(ops_memory_t *mem)
    {
    ops_memory_release(mem);
    free(mem);
    }

/**
   \ingroup HighLevel_Memory
   \brief Get length of data stored in ops_memory_t struct
   \return Number of bytes in data
*/
size_t ops_memory_get_length(const ops_memory_t *mem)
    { return mem->length; }

/**
   \ingroup HighLevel_Memory
   \brief Get data stored in ops_memory_t struct
   \return Pointer to data
*/
void *ops_memory_get_data(ops_memory_t *mem)
    { return mem->buf; }

// EOF

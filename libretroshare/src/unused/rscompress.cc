/*******************************************************************************
 * libretroshare/src/util: rscompress.cc                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013 Cyril Soler <csoler@users.sourceforge.net>                   *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <string.h>
#include <assert.h>
#include "rscompress.h"
#include "zlib.h"
#include "util/rsmemory.h"

// 16K buffer size. 
//
static const unsigned int CHUNK = 16384u ;

bool RsCompress::compress_memory_chunk(const uint8_t *input_mem,const uint32_t input_size,uint8_t *& output_mem,uint32_t& output_size) 
{

	uint32_t remaining_input = input_size ;
	uint32_t output_offset = 0 ;
	uint32_t input_offset = 0 ;
	output_size = 1024 ;
	output_mem = (uint8_t*)rs_malloc(output_size) ;
    
    	if(!output_mem)
            return false ;

	int ret, flush;
	unsigned have;
	int level = 9 ;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];
	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	ret = deflateInit(&strm, level);
	if (ret != Z_OK)
		return false;

	/* compress until end of file */
	do 
	{
		uint32_t available_in = std::min(CHUNK,remaining_input) ;
		memcpy(in,input_mem+input_offset,available_in) ;
		strm.avail_in = available_in ;
		remaining_input -= available_in ;
		input_offset += available_in ;

		flush = /*feof(source)*/ remaining_input ? Z_NO_FLUSH: Z_FINISH ;
		strm.next_in = in;

		/* run deflate() on input until output buffer not full, finish
			compression if all of source has been read in */
		do 
		{
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = deflate(&strm, flush);    /* no bad return value */
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			have = CHUNK - strm.avail_out;

			if(output_size < have+output_offset)
			{
				//std::cerr << "Growing outputbuffer from " << output_size << " to " << have+output_offset << std::endl;
				output_mem = (uint8_t*)realloc(output_mem,have+output_offset) ;
				output_size = have+output_offset ;
			}
			
			memcpy(output_mem+output_offset,out,have) ;
			output_offset += have ;
			
			//std::cerr << "Copying " << have << " bytes to output. New offset=" << output_offset << std::endl;
		} while (strm.avail_out == 0);
		assert(strm.avail_in == 0);     /* all input will be used */

		/* done when last data in file processed */
	} while (flush != Z_FINISH);
	assert(ret == Z_STREAM_END);        /* stream will be complete */

	/* clean up and return */
	(void)deflateEnd(&strm);

	output_size = output_offset ;

	return true ;
}

bool RsCompress::uncompress_memory_chunk(const uint8_t *input_mem,const uint32_t input_size,uint8_t *& output_mem,uint32_t& output_size) 
{
	uint32_t remaining_input = input_size ;
	output_size = input_size ;
	uint32_t output_offset = 0 ;
	uint32_t input_offset = 0 ;
	output_mem = (uint8_t*)rs_malloc(output_size) ;

	if(!output_mem)
	    return false ;
    
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	/* allocate inflate state */
	memset(&strm,0,sizeof(strm)) ;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = Z_NULL;
	strm.next_out = Z_NULL;

	ret = inflateInit(&strm);
	if (ret != Z_OK)
		return ret;

	/* decompress until deflate stream ends or end of file */
	do 
	{
		uint32_t available_in = std::min(CHUNK,remaining_input) ;
		memcpy(in,input_mem+input_offset,available_in) ;
		strm.avail_in = available_in ;
		remaining_input -= available_in ;
		input_offset += available_in ;
		
		if (strm.avail_in == 0)
			break;

		strm.next_in = in;
		/* run inflate() on input until output buffer not full */
		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;

			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
					/* fallthrough */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return ret;
			}


			have = CHUNK - strm.avail_out;

			if(output_size < have+output_offset)
			{
				output_mem = (uint8_t*)realloc(output_mem,have+output_offset) ;
				output_size = have+output_offset ;
			}
			
			memcpy(output_mem+output_offset,out,have) ;
			output_offset += have ;

		} while (strm.avail_out == 0);
		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);

	output_size = output_offset ;
	return ret == Z_STREAM_END ;
}







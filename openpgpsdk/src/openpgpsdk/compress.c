/*
 * Copyright (c) 2005-2009 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer, Alasdair Mackintosh.
 * The Contributors have asserted their moral rights under the
 * UK Copyright Design and Patents Act 1988 to
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

#include <zlib.h>
#include <bzlib.h>
#include <assert.h>
#include <string.h>

#include <openpgpsdk/compress.h>
#include <openpgpsdk/packet-parse.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/errors.h>
#include "parse_local.h"
#include <openpgpsdk/final.h>
#include <openpgpsdk/partial.h>

static const int debug = 0;

#define DECOMPRESS_BUFFER	1024
#define COMPRESS_BUFFER	        32768

typedef struct
    {
    ops_compression_type_t type;
    ops_region_t *region;
    unsigned char in[DECOMPRESS_BUFFER];
    unsigned char out[DECOMPRESS_BUFFER];
    z_stream zstream; // ZIP and ZLIB
    size_t offset;
    int inflate_ret;
    } z_decompress_arg_t;

typedef struct
    {
    ops_compression_type_t type;
    ops_region_t *region;
    char in[DECOMPRESS_BUFFER];
    char out[DECOMPRESS_BUFFER];
    bz_stream bzstream; // BZIP2
    size_t offset;
    int inflate_ret;
    } bz_decompress_arg_t;

typedef struct
    {
    z_stream stream;
    unsigned char *src;
    unsigned char *dst;
    size_t bytes_in;
    size_t bytes_out;
    } compress_arg_t;

// \todo remove code duplication between this and bzip2_compressed_data_reader
static int zlib_compressed_data_reader(void *dest,size_t length,
				       ops_error_t **errors,
				       ops_reader_info_t *rinfo,
				       ops_parse_cb_info_t *cbinfo)
    {
    z_decompress_arg_t *arg=ops_reader_get_arg(rinfo);
    assert(arg->type==OPS_C_ZIP || arg->type==OPS_C_ZLIB);

    //ops_parser_content_t content;
    int saved=length;

    if(/*arg->region->indeterminate && */ arg->inflate_ret == Z_STREAM_END
       && arg->zstream.next_out == &arg->out[arg->offset])
        return 0;

    if(arg->region->length_read == arg->region->length)
        {
        if(arg->inflate_ret != Z_STREAM_END)
            OPS_ERROR(cbinfo->errors, OPS_E_P_DECOMPRESSION_ERROR,
		      "Compressed data didn't end when region ended.");
        /*
          else
          return 0;
          www.zlib.org
        */
        }

    while(length > 0)
	{
	unsigned len;

	if(&arg->out[arg->offset] == arg->zstream.next_out)
	    {
	    int ret;

	    arg->zstream.next_out=arg->out;
	    arg->zstream.avail_out=sizeof arg->out;
	    arg->offset=0;
	    if(arg->zstream.avail_in == 0)
		{
		unsigned n=arg->region->length;

		if(!arg->region->indeterminate)
		    {
		    n-=arg->region->length_read;
		    if(n > sizeof arg->in)
			n=sizeof arg->in;
		    }
		else
		    n=sizeof arg->in;

		if(!ops_stacked_limited_read(arg->in,n,arg->region,
					     errors,rinfo,cbinfo))
		    return -1;

		arg->zstream.next_in=arg->in;
		arg->zstream.avail_in=arg->region->indeterminate
		    ? arg->region->last_read : n;
		}

	    ret=inflate(&arg->zstream,Z_SYNC_FLUSH);
	    if(ret == Z_STREAM_END)
		{
		if(!arg->region->indeterminate
		   && arg->region->length_read != arg->region->length)
		    OPS_ERROR(cbinfo->errors,OPS_E_P_DECOMPRESSION_ERROR,
			      "Compressed stream ended before packet end.");
		}
	    else if(ret != Z_OK)
		{
		fprintf(stderr,"ret=%d\n",ret);
		OPS_ERROR(cbinfo->errors,OPS_E_P_DECOMPRESSION_ERROR, arg->zstream.msg);
		}
	    arg->inflate_ret=ret;
	    }
	assert(arg->zstream.next_out > &arg->out[arg->offset]);
	len=arg->zstream.next_out-&arg->out[arg->offset];
	if(len > length)
	    len=length;
	memcpy(dest,&arg->out[arg->offset],len);
	arg->offset+=len;
	length-=len;
	}

    return saved;
    }

// \todo remove code duplication between this and zlib_compressed_data_reader
static int bzip2_compressed_data_reader(void *dest,size_t length,
					ops_error_t **errors,
					ops_reader_info_t *rinfo,
					ops_parse_cb_info_t *cbinfo)
    {
    bz_decompress_arg_t *arg=ops_reader_get_arg(rinfo);
    assert(arg->type==OPS_C_BZIP2);

    //ops_parser_content_t content;
    int saved=length;

    if(arg->inflate_ret == BZ_STREAM_END
       && arg->bzstream.next_out == &arg->out[arg->offset])
        return 0;

    if(arg->region->length_read == arg->region->length)
        {
        if(arg->inflate_ret != BZ_STREAM_END)
            OPS_ERROR(cbinfo->errors, OPS_E_P_DECOMPRESSION_ERROR,
		      "Compressed data didn't end when region ended.");
        }

    while(length > 0)
	{
	unsigned len;

	if(&arg->out[arg->offset] == arg->bzstream.next_out)
	    {
	    int ret;

	    arg->bzstream.next_out=(char *) arg->out;
	    arg->bzstream.avail_out=sizeof arg->out;
	    arg->offset=0;
	    if(arg->bzstream.avail_in == 0)
		{
		unsigned n=arg->region->length;

		if(!arg->region->indeterminate)
		    {
		    n-=arg->region->length_read;
		    if(n > sizeof arg->in)
			n=sizeof arg->in;
		    }
		else
		    n=sizeof arg->in;

		if(!ops_stacked_limited_read(arg->in, n, arg->region, errors,
					     rinfo, cbinfo))
		    return -1;

		arg->bzstream.next_in=arg->in;
		arg->bzstream.avail_in=arg->region->indeterminate
		    ? arg->region->last_read : n;
		}

	    ret=BZ2_bzDecompress(&arg->bzstream);
	    if(ret == BZ_STREAM_END)
		{
		if(!arg->region->indeterminate
		   && arg->region->length_read != arg->region->length)
		    OPS_ERROR(cbinfo->errors, OPS_E_P_DECOMPRESSION_ERROR,
			      "Compressed stream ended before packet end.");
		}
	    else if(ret != BZ_OK)
		{
                OPS_ERROR_1(cbinfo->errors, OPS_E_P_DECOMPRESSION_ERROR,
			    "Invalid return %d from BZ2_bzDecompress", ret);
		}
	    arg->inflate_ret=ret;
	    }
	assert(arg->bzstream.next_out > &arg->out[arg->offset]);
	len=arg->bzstream.next_out-&arg->out[arg->offset];
	if(len > length)
	    len=length;
	memcpy(dest,&arg->out[arg->offset],len);
	arg->offset+=len;
	length-=len;
	}

    return saved;
    }

/**
 * \ingroup Core_Compress
 * 
 * \param *region 	Pointer to a region
 * \param *parse_info 	How to parse
 * \param type Which compression type to expect
*/

int ops_decompress(ops_region_t *region,ops_parse_info_t *parse_info,
		   ops_compression_type_t type)
    {
    z_decompress_arg_t z_arg;
    bz_decompress_arg_t bz_arg;
    int ret;

    switch (type)
        {
    case OPS_C_ZIP:
    case OPS_C_ZLIB:
        memset(&z_arg,'\0',sizeof z_arg);

        z_arg.region=region;
        z_arg.offset=0;
        z_arg.type=type;

        z_arg.zstream.next_in=Z_NULL;
        z_arg.zstream.avail_in=0;
        z_arg.zstream.next_out=z_arg.out;
        z_arg.zstream.zalloc=Z_NULL;
        z_arg.zstream.zfree=Z_NULL;
        z_arg.zstream.opaque=Z_NULL;
        break;

    case OPS_C_BZIP2:
        memset(&bz_arg,'\0',sizeof bz_arg);

        bz_arg.region=region;
        bz_arg.offset=0;
        bz_arg.type=type;

        bz_arg.bzstream.next_in=NULL;
        bz_arg.bzstream.avail_in=0;
        bz_arg.bzstream.next_out=bz_arg.out;
        bz_arg.bzstream.bzalloc=NULL;
        bz_arg.bzstream.bzfree=NULL;
        bz_arg.bzstream.opaque=NULL;
        break;

    default:
        OPS_ERROR_1(&parse_info->errors, OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG,
		    "Compression algorithm %d is not yet supported", type);
        return 0;
        }

    switch(type)
        {
    case OPS_C_ZIP:
        ret=inflateInit2(&z_arg.zstream,-15);
        break;

    case OPS_C_ZLIB:
        ret=inflateInit(&z_arg.zstream);
        break;

    case OPS_C_BZIP2:
        /*
        OPS_ERROR_1(&parse_info->errors, OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG, "Compression algorithm %s is not yet supported", "BZIP2");
        return 0;
        */
        ret=BZ2_bzDecompressInit(&bz_arg.bzstream, 1, 0);
        break;

    default:
        OPS_ERROR_1(&parse_info->errors, OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG,
		    "Compression algorithm %d is not yet supported", type);
        return 0;
        }

    switch (type)
        {
    case OPS_C_ZIP:
    case OPS_C_ZLIB:
        if(ret != Z_OK)
            {
            OPS_ERROR_1(&parse_info->errors, OPS_E_P_DECOMPRESSION_ERROR,
			"Cannot initialise ZIP or ZLIB stream "
			"for decompression: error=%d", ret);
            return 0;
            }
        ops_reader_push(parse_info,zlib_compressed_data_reader,NULL,&z_arg);
        break;

    case OPS_C_BZIP2:
        if (ret != BZ_OK)
            {
            OPS_ERROR_1(&parse_info->errors, OPS_E_P_DECOMPRESSION_ERROR,
			"Cannot initialise BZIP2 stream "
			"for decompression: error=%d", ret);
            return 0;
            }
        ops_reader_push(parse_info,bzip2_compressed_data_reader,NULL,&bz_arg);
        break;

    default:
        OPS_ERROR_1(&parse_info->errors, OPS_E_ALG_UNSUPPORTED_COMPRESS_ALG,
		    "Compression algorithm %d is not yet supported", type);
        return 0;
        }

    ret=ops_parse(parse_info);

    ops_reader_pop(parse_info);

    return ret;
    }

/**
\ingroup Core_WritePackets
\brief Writes Compressed packet
\param data Data to write out
\param len Length of data
\param cinfo Write settings
\return ops_true if OK; else ops_false
*/

ops_boolean_t ops_write_compressed(const unsigned char *data,
                                   const unsigned int len,
                                   ops_create_info_t *cinfo)
    {
    int r=0;
    int sz_in=0;
    int sz_out=0;
    compress_arg_t* compress=ops_mallocz(sizeof *compress);

    // compress the data
    const int level=Z_DEFAULT_COMPRESSION; // \todo allow varying levels
    compress->stream.zalloc=Z_NULL;
    compress->stream.zfree=Z_NULL;
    compress->stream.opaque=NULL;

    // all other fields set to zero by use of ops_mallocz

    if (deflateInit(&compress->stream,level) != Z_OK)
        {
        // can't initialise
        assert(0);
        }

    // do necessary transformation
    // copy input to maintain const'ness of src
    assert(compress->src==NULL);
    assert(compress->dst==NULL);

    sz_in=len * sizeof (unsigned char);
    sz_out= (sz_in * 1.01) + 12; // from zlib webpage
    compress->src=ops_mallocz(sz_in);
    compress->dst=ops_mallocz(sz_out);
    memcpy(compress->src,data,len);

    // setup stream
    compress->stream.next_in=compress->src;
    compress->stream.avail_in=sz_in;
    compress->stream.total_in=0;

    compress->stream.next_out=compress->dst;
    compress->stream.avail_out=sz_out;
    compress->stream.total_out=0;

    r=deflate(&compress->stream, Z_FINISH);
    assert(r==Z_STREAM_END); // need to loop if not

    // write it out
    return (ops_write_ptag(OPS_PTAG_CT_COMPRESSED, cinfo)
            && ops_write_length(1+compress->stream.total_out, cinfo)
            && ops_write_scalar(OPS_C_ZLIB,1,cinfo)
            && ops_write(compress->dst, compress->stream.total_out,cinfo));
    }


// Writes out the header for the compressed packet. Invoked by the
// partial stream writer. Note that writing the packet tag and the
// packet length is handled by the partial stream writer.
static ops_boolean_t write_compressed_header(ops_create_info_t *info,
                                             void *header_data)
    {
    OPS_USED(header_data);
    // Write the compression type. Currently we just use ZLIB
    ops_write_scalar(OPS_C_ZLIB, 1, info);
    return ops_true;
    }

static void zlib_error(ops_error_t **errors, z_stream *stream, int error)
    {
    OPS_ERROR_2(errors,OPS_E_FAIL,
		"Error from compression stream %d (%s)", error,
		stream->msg == NULL ? "Unknown" :  stream->msg);
    }

static ops_boolean_t stream_compress_writer(const unsigned char *src,
                                            unsigned length,
                                            ops_error_t **errors,
                                            ops_writer_info_t *winfo)
    {
    // ZLib doesn't like being asked to compress nothing, so return if
    // we are given no input.
    if (length == 0)
	return ops_true;
    if (debug)
	fprintf(stderr, "Compressing %u bytes\n", length);
    compress_arg_t* compress = ops_writer_get_arg(winfo);
    compress->bytes_in += length;
    compress->stream.next_in = (void*) src;
    compress->stream.avail_in = length;
    ops_boolean_t result = ops_true;
    do
	{
	compress->stream.next_out = compress->dst;
	compress->stream.avail_out = COMPRESS_BUFFER;
	int retcode = deflate(&compress->stream, Z_NO_FLUSH);
	if (retcode != Z_OK)
	    {
	    zlib_error(errors, &compress->stream, retcode);
	    deflateEnd(&compress->stream);
	    return ops_false;
	    }
	unsigned bytes_to_write = COMPRESS_BUFFER - compress->stream.avail_out;
	if (debug)
	    fprintf(stderr, "bytes_to_write = %u\n", bytes_to_write);
	compress->bytes_out += bytes_to_write;
	result = ops_stacked_write(compress->dst, bytes_to_write, errors,
				   winfo);
	}
    while (result && compress->stream.avail_out == 0);

    return result;
    }

static ops_boolean_t stream_compress_finaliser(ops_error_t **errors,
                                               ops_writer_info_t *winfo)
    {
    compress_arg_t* compress = ops_writer_get_arg(winfo);
    compress->stream.next_in = NULL;
    compress->stream.avail_in = 0;
    int retcode = Z_OK;
    int output_size = COMPRESS_BUFFER;
    ops_boolean_t result = ops_true;
    do
	{
	compress->stream.next_out = compress->dst;
	compress->stream.avail_out = output_size;
	retcode = deflate(&compress->stream, Z_FINISH);
	if (retcode != Z_STREAM_END && retcode != Z_OK)
	    {
	    zlib_error(errors, &compress->stream, retcode);
	    deflateEnd(&compress->stream);
	    return ops_false;
	    }
	int bytes_to_write = output_size - compress->stream.avail_out;
	if (debug)
	    fprintf(stderr, "At end, bytes_to_write = %u\n", bytes_to_write);
	compress->bytes_out += bytes_to_write;
	result = ops_stacked_write(compress->dst, bytes_to_write, errors,
				   winfo);

	// If deflate returns Z_OK after we have asked to flush, it means
	// that there was not enough space in the output buffer. Increase
	// the buffer size and try again.
	if (retcode != Z_STREAM_END)
	    {
	    if (debug)
		fprintf(stderr, "Reallocating %u\n", output_size * 2);
	    output_size *= 2;
	    compress->dst = realloc(compress->dst, output_size);
	    }
	}
    while (result && retcode != Z_STREAM_END);
  
    int error = deflateEnd(&compress->stream);
    if (error != Z_OK)
	{
	zlib_error(errors, &compress->stream, error);
	return ops_false;
	}
    return result;
    }

static void stream_compress_destroyer(ops_writer_info_t *winfo)
    {
    compress_arg_t* compress = ops_writer_get_arg(winfo);
    if (debug)
	fprintf(stderr, "Compressed %zu to %zu\n", compress->bytes_in,
		compress->bytes_out);
    free(compress->dst);
    free(compress);
    }

/**
\ingroup Core_WritePackets
\brief Pushes a compressed writer onto the stack. Data written
       will be encoded as a compressed packet.
\param cinfo Write settings
*/
void ops_writer_push_compressed(ops_create_info_t *cinfo)
    {
    // This is a streaming writer, so we don't know the length in
    // advance. Use a partial writer to handle the partial body
    // packet lengths.
    ops_writer_push_partial(COMPRESS_BUFFER,
			    cinfo, OPS_PTAG_CT_COMPRESSED,
			    write_compressed_header, NULL);

    // Create arg to be used with this writer
    // Remember to free this in the destroyer
    compress_arg_t *compress = ops_mallocz(sizeof *compress);

    compress->dst = malloc(COMPRESS_BUFFER);
    const int level=Z_DEFAULT_COMPRESSION; // \todo allow varying levels
    compress->stream.zalloc=Z_NULL;
    compress->stream.zfree=Z_NULL;
    compress->stream.opaque=NULL;
    compress->stream.avail_out = COMPRESS_BUFFER;
    // all other fields set to zero by use of ops_mallocz

    if (deflateInit(&compress->stream, level) != Z_OK)
	// can't initialise. Is there a better way to handle this?
	assert(0);

    // And push writer on stack
    ops_writer_push(cinfo, stream_compress_writer, stream_compress_finaliser,
		    stream_compress_destroyer, compress);
    }

// EOF

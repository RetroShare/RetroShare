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

/** Writes data using a series of partial body length headers.
 *  (See RFC 4880 4.2.2.4). This is normally used in conjunction
 *  with a streaming writer of some kind that needs to write out
 *  data packets of unknown length.
 */

#include <string.h>
#include <assert.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <openpgpsdk/create.h>
#include <openpgpsdk/memory.h>
#include <openpgpsdk/partial.h>
#include <openpgpsdk/readerwriter.h>

static const int debug = 0;

#define PACKET_SIZE             2048
#define MIN_PARTIAL_DATA_LENGTH 512
#define MAX_PARTIAL_DATA_LENGTH 1073741824

typedef struct
    {
    size_t packet_size;          // size of packets
    ops_memory_t *buffer;        // Data is buffered here until written
    ops_content_tag_t tag;       // Packet tag
    ops_memory_t *header;        // Header is written here
    ops_boolean_t written_first; // Has the first packet been written?
    ops_write_partial_trailer_t *trailer_fn; // Custom end-of-packet fn
    void *trailer_data;                      // data for end-of-packet fn
    } stream_partial_arg_t;



static unsigned int ops_calc_partial_data_length(unsigned int len)
    {
    int i;
    unsigned int mask = MAX_PARTIAL_DATA_LENGTH;
    assert( len > 0 );

    if (len > MAX_PARTIAL_DATA_LENGTH)
	return MAX_PARTIAL_DATA_LENGTH;

    for (i = 0 ; i <= 30 ; i++)
	{
	if (mask & len)
	    break; 
	mask >>= 1;
	}

    return mask;
    }

static ops_boolean_t ops_write_partial_data_length(unsigned int len, 
                                                   ops_create_info_t *info)
    {
    // len must be a power of 2 from 0 to 30
    unsigned i;
    unsigned char c[1];

    for (i = 0 ; i <= 30 ; i++)
	if ((len >> i) & 1)
	    break; 

    assert((1u << i) == len);

    c[0] = 224 + i;

    return ops_write(c, 1, info);
    }

static ops_boolean_t write_partial_data(const unsigned char *data, 
                                        size_t len, 
                                        ops_create_info_t *info)
    {
    if (debug)
	fprintf(stderr, "Writing %zu bytes\n", len);
    while (len > 0)
	{
	size_t pdlen = ops_calc_partial_data_length(len);
	ops_write_partial_data_length(pdlen, info);
	ops_write(data, pdlen, info);
	data += pdlen;
	len -= pdlen;
	}
    return ops_true;
    }

static ops_boolean_t write_partial_data_first(stream_partial_arg_t *arg,
                                              const unsigned char *data, 
                                              unsigned int len, 
                                              ops_create_info_t *info)
    {
    size_t header_len = ops_memory_get_length(arg->header);

    size_t sz_towrite = len + header_len;
    size_t sz_pd = ops_calc_partial_data_length(sz_towrite);
    size_t first_data_len = (sz_pd - header_len);
    assert(sz_pd >= MIN_PARTIAL_DATA_LENGTH);
  
    if (debug)
	fprintf(stderr, "Writing first packet of len %zu (%zu + %u)\n",
		sz_towrite, header_len, len);
  
    // Write the packet tag, the partial size and the header, followed
    // by the first chunk of data and then the remainder of the data.
    // (We have to do this in two chunks, as the partial length may not
    // match the number of bytes to write.)
    return ops_write_ptag(arg->tag, info) &&
	ops_write_partial_data_length(sz_pd, info) &&
	ops_write(ops_memory_get_data(arg->header), header_len, info) &&
	ops_write(data, first_data_len, info) &&
	write_partial_data(data + first_data_len, len - first_data_len, info);
    }

/*
 * Writes out the last packet. The length is encoded as a fixed-length
 * packet.  Note that even if there is no data accumulated in the
 * buffer, we stil lneed to write out a packet, as the final packet in
 * a partially-encoded stream must be a fixed-lngth packet.
 */
static ops_boolean_t write_partial_data_last(stream_partial_arg_t *arg,
                                             ops_create_info_t *info)
    {
    size_t buffer_length = ops_memory_get_length(arg->buffer);
    if (debug)
	fprintf(stderr, "writing final packet of %zu bytes\n", buffer_length);
    return ops_write_length(buffer_length, info) &&
	ops_write(ops_memory_get_data(arg->buffer), buffer_length, info);
    }

/*
 * Writes out the data accumulated in the in-memory buffer.
 */
static ops_boolean_t flush_buffer(stream_partial_arg_t *arg,
                                  ops_create_info_t *info)
    {
    ops_boolean_t result = ops_true;
    size_t buffer_length = ops_memory_get_length(arg->buffer);
    if (buffer_length > 0)
	{
	if (debug)
	    fprintf(stderr, "Flushing %zu bytes\n", buffer_length);

	result = write_partial_data(ops_memory_get_data(arg->buffer),
				    buffer_length,
				    info);
	ops_memory_clear(arg->buffer);
	}
    return result;
    }

static ops_boolean_t stream_partial_writer(const unsigned char *src,
                                           unsigned length,
                                           ops_error_t **errors,
                                           ops_writer_info_t *winfo)
    {
    stream_partial_arg_t *arg = ops_writer_get_arg(winfo);

    // For the first write operation, we need to write out the header
    // plus the data. The total size that we write out must be at least
    // MIN_PARTIAL_DATA_LENGTH bytes. (See RFC 4880, sec 4.2.2.4,
    // Partial Body Lengths.) If we are given less than this,
    // then we need to store the data in the buffer until we have the
    // minumum
    if (!arg->written_first)
	{
	ops_memory_add(arg->buffer, src, length); 
	size_t buffer_length = ops_memory_get_length(arg->buffer);
	size_t header_length = ops_memory_get_length(arg->header);
	if (header_length +  buffer_length < MIN_PARTIAL_DATA_LENGTH)
	    {
	    if (debug)
		fprintf(stderr, "Storing %zu (%zu + %zu) bytes\n",
			header_length + buffer_length, header_length,
			buffer_length);
	    return ops_true; // will wait for more data or end of stream
	    }
	arg->written_first = ops_true;

	// Create a writer that will write to the parent stream. Allows
	// useage of ops_write_ptag, etc.
	ops_create_info_t parent_info;
	ops_prepare_parent_info(&parent_info, winfo);
	ops_boolean_t result =
	    write_partial_data_first(arg, ops_memory_get_data(arg->buffer),
				     buffer_length, &parent_info);
	ops_memory_clear(arg->buffer);
	ops_move_errors(&parent_info, errors);
	return result;
	}
    else
	{
	size_t buffer_length = ops_memory_get_length(arg->buffer);
	if (buffer_length + length < arg->packet_size)
	    {
	    ops_memory_add(arg->buffer, src, length);
	    if (debug)
		fprintf(stderr, "Storing %u bytes (total %zu)\n",
			length, buffer_length);
	    return ops_true;
	    }
	else
	    {
	    ops_create_info_t parent_info;
	    parent_info.winfo = *winfo->next;
	    parent_info.errors = *errors;
	    return flush_buffer(arg, &parent_info) && 
		write_partial_data(src, length, &parent_info);
	    }
	}
    return ops_true;
    }

/*
 * Invoked when the total packet size is less than
 * MIN_PARTIAL_DATA_LENGTH. In that case, we write out the whole
 * packet in a single operation, without using partial body length
 * packets.
 */
static ops_boolean_t write_complete_packet(stream_partial_arg_t *arg,
                                           ops_create_info_t *info)
    {
    size_t data_len = ops_memory_get_length(arg->buffer);
    size_t header_len = ops_memory_get_length(arg->header);
  
    // Write the header tag, the length of the packet, and the
    // packet. Note that the packet includes the header
    // bytes.
    size_t total = data_len + header_len;
    if (debug)
	fprintf(stderr, "writing entire packet  with length %zu (%zu + %zu)\n",
		total, data_len, header_len);
    return ops_write_ptag(arg->tag, info) &&
	ops_write_length(total, info) &&
	ops_write(ops_memory_get_data(arg->header), header_len, info) &&
	ops_write(ops_memory_get_data(arg->buffer), data_len, info);
    }

static ops_boolean_t stream_partial_finaliser(ops_error_t **errors,
                                              ops_writer_info_t *winfo)
    {
    stream_partial_arg_t *arg = ops_writer_get_arg(winfo);
    // write last chunk of data

    // Create a writer that will write to the parent stream. Allows
    // useage of ops_write_ptag, etc.
    ops_create_info_t parent_info;
    ops_prepare_parent_info(&parent_info, winfo);
    ops_boolean_t result;
    if (!arg->written_first)
	result = write_complete_packet(arg, &parent_info);
    else
	// finish writing
	result = write_partial_data_last(arg, &parent_info);
    if (result && arg->trailer_fn != NULL)
	result = arg->trailer_fn(&parent_info, arg->trailer_data);
    ops_move_errors(&parent_info, errors);
    return result;
    }

static void stream_partial_destroyer(ops_writer_info_t *winfo)
    {
    stream_partial_arg_t *arg = ops_writer_get_arg(winfo);
    ops_memory_free(arg->buffer);
    ops_memory_free(arg->header);
    free(arg);
    }

/**
 * \ingroup InternalAPI
 * \brief Pushes a partial packet writer onto the stack.
 *
 * This writer is used in conjunction with another writer that
 * generates streaming data of unknown length. The partial writer
 * handles the various partial body length packets. When writing the
 * initial packet header, the partial writer will write out the given
 * tag, write out an initial length, and then invoke the 'header'
 * function to write the remainder of the header. Note that the header
 * function should not write a packet tag or a length.
 *
 *  \param packet_size the expected size of the incoming packets. Must
 *         be >= 512 bytes. Must be a power of 2. The partial writer
 *         will buffer incoming writes into packets of this size. Note
 *         that writes will be most efficient if done in chunks of
 *         packet_size. If the packet size is unknown, specify 0, and
 *         the default size will be used.
 *  \param cinfo the writer info
 *  \param tag the packet tag
 *  \param header_writer a function that writes the packet header.
 *  \param header_data passed into header_writer
 */
void ops_writer_push_partial(size_t packet_size,
                             ops_create_info_t *cinfo,
                             ops_content_tag_t tag,
                             ops_write_partial_header_t *header_writer,
                             void *header_data)
    {
    ops_writer_push_partial_with_trailer(packet_size, cinfo, tag, header_writer,
					 header_data, NULL, NULL);
    }

/**
 * \ingroup  InternalAPI
 * \brief Pushes a partial packet writer onto the stack. Adds a trailer
 *        function that will be invoked after writing out the partial
 *        packet.
 *
 * This writer is primarily used by the signature writer, which needs
 * to append a signature packet after the literal data packet.
 *
 *  \param trailer_writer a function that writes the trailer
 *  \param trailer_data passed into trailer_data
 *  \see ops_writer_push_partial
 *  \see ops_writer_push_signed
 */
void ops_writer_push_partial_with_trailer(
    size_t packet_size,
    ops_create_info_t *cinfo,
    ops_content_tag_t tag,
    ops_write_partial_header_t *header_writer,
    void *header_data,
    ops_write_partial_trailer_t *trailer_writer,
    void *trailer_data)
    {
    if (packet_size == 0)
	packet_size = PACKET_SIZE;
    assert(packet_size >= MIN_PARTIAL_DATA_LENGTH);
    // Verify that the packet size is a valid power of 2.
    assert(ops_calc_partial_data_length(packet_size) == packet_size);
  
    // Create arg to be used with this writer
    // Remember to free this in the destroyer
    stream_partial_arg_t *arg = ops_mallocz(sizeof *arg);
    arg->tag = tag;
    arg->written_first = ops_false;
    arg->packet_size = packet_size;
    arg->buffer = ops_memory_new();
    ops_memory_init(arg->buffer, arg->packet_size);
    arg->trailer_fn = trailer_writer;
    arg->trailer_data = trailer_data;

    // Write out the header into the memory buffer. Later we will write
    // this buffer to the underlying output stream.
    ops_create_info_t *header_info;
    ops_setup_memory_write(&header_info, &arg->header, 128);
    header_writer(header_info, header_data);
    ops_writer_close(header_info);
    ops_create_info_delete(header_info);

    // And push writer on stack
    ops_writer_push(cinfo, stream_partial_writer, stream_partial_finaliser,
		    stream_partial_destroyer, arg);
    }

// EOF

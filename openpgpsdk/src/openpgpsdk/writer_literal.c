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

/** Writes a literal data packet, using the partial data length encoding.
 */

#include <string.h>
#include <assert.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <openpgpsdk/create.h>
#include <openpgpsdk/literal.h>
#include <openpgpsdk/partial.h>

#define MIN_PARTIAL_DATA_LENGTH 512
#define MAX_PARTIAL_DATA_LENGTH 1073741824


ops_boolean_t write_literal_header(ops_create_info_t *info,
				   void *header_data)
    {
    OPS_USED(header_data);
    // \todo add the literal type as a header_data argument
    // \todo add filename 
    // \todo add date
    // \todo do we need to check text data for <cr><lf> line endings ?

    ops_write_scalar(OPS_LDT_BINARY, 1, info); // data type
    ops_write_scalar(0, 1, info);    // Filename (length = 0)
    ops_write_scalar(0, 4, info);    // Date (unspecified)
    return ops_true;
    }

/**
 * \ingroup  InternalAPI
 * \brief Pushes a literal writer onto the stack.
 * \param cinfo the writer info
 * \param buf_size the size of the internal buffer. For best
 * throughput, write data in multiples of buf_size
 */
void ops_writer_push_literal_with_opts(ops_create_info_t *cinfo,
                                       unsigned int buf_size)
    {
    // The literal writer doesn't need to transform the data, so we just
    // push a partial packet writer onto the stack. This will handle
    // the packet length encoding. All we need to provide is a function
    // to write the header.
    ops_writer_push_partial(buf_size, cinfo, OPS_PTAG_CT_LITERAL_DATA,
			    write_literal_header, NULL);

}

/**
 * \ingroup  InternalAPI
 * \brief Pushes a literal writer onto the stack.
 * \param cinfo the writer info
 */
void ops_writer_push_literal(ops_create_info_t *cinfo)
    {
    ops_writer_push_literal_with_opts(cinfo, 0);
    }

// EOF

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

#ifndef __OPS_PARTIAL_H__
#define __OPS_PARTIAL_H__

#include "types.h"
#include "writer.h"

/**
 * Function that writes out a packet header. See
 * ops_writer_push_partial
 */
typedef ops_boolean_t ops_write_partial_header_t(ops_create_info_t *info,
                                                 void *data);

typedef ops_boolean_t ops_write_partial_trailer_t(ops_create_info_t *info,
                                                  void *data);

void ops_writer_push_partial(size_t packet_size,
                             ops_create_info_t *info,
                             ops_content_tag_t tag,
                             ops_write_partial_header_t *header_writer,
                             void *header_data);

void ops_writer_push_partial_with_trailer(
    size_t packet_size,
    ops_create_info_t *cinfo,
    ops_content_tag_t tag,
    ops_write_partial_header_t *header_writer,
    void *header_data,
    ops_write_partial_trailer_t *trailer_writer,
    void *trailer_data);

#endif /* __OPS_PARTIAL_H__ */

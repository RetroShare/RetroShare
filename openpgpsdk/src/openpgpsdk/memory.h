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
#include <openssl/bn.h>
#include "packet.h"

#ifndef OPS_MEMORY_H
#define OPS_MEMORY_H

/** ops_memory_t
 */
typedef struct ops_memory ops_memory_t;

ops_memory_t *ops_memory_new(void);
void ops_memory_free(ops_memory_t *mem);
void ops_memory_init(ops_memory_t *mem,size_t initial_size);
void ops_memory_pad(ops_memory_t *mem,size_t length);
void ops_memory_add(ops_memory_t *mem,const unsigned char *src,size_t length);
void ops_memory_place_int(ops_memory_t *mem,unsigned offset,unsigned n,
			  size_t length);
void ops_memory_make_packet(ops_memory_t *out,ops_content_tag_t tag);
void ops_memory_clear(ops_memory_t *mem);
void ops_memory_release(ops_memory_t *mem);

void ops_writer_set_memory(ops_create_info_t *info,ops_memory_t *mem);

size_t ops_memory_get_length(const ops_memory_t *mem);
void *ops_memory_get_data(ops_memory_t *mem);

#endif

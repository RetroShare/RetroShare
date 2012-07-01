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

static ops_boolean_t memory_writer(const unsigned char *src,unsigned length,
				      ops_error_t **errors,
				      ops_writer_info_t *winfo)
    {
    ops_memory_t *mem=ops_writer_get_arg(winfo);

    OPS_USED(errors);
    ops_memory_add(mem,src,length);
    return ops_true;
    }

/**
 * \ingroup Core_WritersFirst
 * \brief Write to memory
 *
 * Set a memory writer. 
 *
 * \param info The info structure
 * \param mem The memory structure 
 * \note It is the caller's responsiblity to call ops_memory_free(mem)
 * \sa ops_memory_free()
 */

void ops_writer_set_memory(ops_create_info_t *info,ops_memory_t *mem)
    {
    ops_writer_set(info,memory_writer,NULL,NULL,mem);
    }


// EOF

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

#ifndef __OPS_HASH_H__
#define __OPS_HASH_H__
#include "openpgpsdk/packet.h"

void ops_calc_mdc_hash(const unsigned char* preamble, const size_t sz_preamble, const unsigned char* plaintext, const unsigned int sz_plaintext, unsigned char *hashed);
ops_boolean_t ops_is_hash_alg_supported(const ops_hash_algorithm_t *hash_alg);

#endif /*__OPS_HASH_H__*/

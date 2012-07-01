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

#include <openpgpsdk/armour.h>

ops_boolean_t ops_writer_push_clearsigned(ops_create_info_t *info,
				  ops_create_signature_t *sig);
void ops_writer_push_armoured_message(ops_create_info_t *info);
ops_boolean_t ops_writer_switch_to_armoured_signature(ops_create_info_t *info);

void ops_writer_push_armoured(ops_create_info_t *info, ops_armor_type_t type);

// EOF

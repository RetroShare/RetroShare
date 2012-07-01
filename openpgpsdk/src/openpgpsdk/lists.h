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

#ifndef OPS_LISTS_H
#define OPS_LISTS_H

/** ops_ulong_list_t */
typedef struct
    {
    unsigned int size;/* num of array slots allocated */
    unsigned int used; /* num of array slots currently used */
    unsigned long *ulongs;
    } ops_ulong_list_t;

void ops_ulong_list_init(ops_ulong_list_t *list);
void ops_ulong_list_free(ops_ulong_list_t *list);
unsigned int ops_ulong_list_add(ops_ulong_list_t *list, unsigned long *ulong);

#endif /* OPS_LISTS_H */

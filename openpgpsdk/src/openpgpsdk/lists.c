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

/**
 * \file
 *
 * Set of functions to manage a dynamic list
 */

#include <openpgpsdk/lists.h>

#include <stdlib.h>

#include <openpgpsdk/final.h>

/**
 * \ingroup Core_Lists
 * \brief Initialises ulong list
 * \param *list	Pointer to existing list structure
 */
void ops_ulong_list_init(ops_ulong_list_t *list)
    {
    list->size=0;
    list->used=0;
    list->ulongs=NULL;
    }
 
/**
 * \ingroup Core_Lists
 * \brief Frees allocated memory in ulong list. Does not free *list itself.
 * \param *list
 */
void ops_ulong_list_free(ops_ulong_list_t *list)
    {
    if (list->ulongs)
	free(list->ulongs);
    ops_ulong_list_init(list);
    }

/**
 * \ingroup Core_Lists
 * \brief Resizes ulong list.
 *
 * We only resize in one direction - upwards.
 * Algorithm used : double the current size then add 1
 *
 * \param *list	Pointer to list
 * \return 1 if success, else 0
 */

static unsigned int ops_ulong_list_resize(ops_ulong_list_t *list)
    {

    int newsize=0;

    newsize=list->size*2 + 1;
    list->ulongs=realloc(list->ulongs,newsize*sizeof *list->ulongs);
    if (list->ulongs)
	{
	list->size=newsize;
	return 1;
	}
    else
	{
	/* xxx - realloc failed. error message? - rachel */
	return 0;
	}
    }

/**
 * \ingroup Core_Lists
 * Adds entry to ulong list
 *
 * \param *list
 * \param *ulong
 *
 * \return 1 if success, else 0
 */
unsigned int ops_ulong_list_add(ops_ulong_list_t *list, unsigned long *ulong)
    {
    if (list->size==list->used) 
	if (!ops_ulong_list_resize(list))
	    return 0;

    list->ulongs[list->used]=*ulong;
    list->used++;
    return 1;
    }


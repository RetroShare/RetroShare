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

#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <direct.h>
#endif
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <openpgpsdk/readerwriter.h>
#include <openpgpsdk/callback.h>

#include "parse_local.h"


/**
 * \ingroup Internal_Readers_Generic
 * \brief Starts reader stack
 * \param pinfo Parse settings
 * \param reader Reader to use
 * \param destroyer Destroyer to use
 * \param arg Reader-specific arg
 */
void ops_reader_set(ops_parse_info_t *pinfo,ops_reader_t *reader,ops_reader_destroyer_t *destroyer,void *arg)
    {
    pinfo->rinfo.reader=reader;
    pinfo->rinfo.destroyer=destroyer;
    pinfo->rinfo.arg=arg;
    }

/**
 * \ingroup Internal_Readers_Generic
 * \brief Adds to reader stack
 * \param pinfo Parse settings
 * \param reader Reader to use
 * \param destroyer Reader's destroyer
 * \param arg Reader-specific arg
 */
void ops_reader_push(ops_parse_info_t *pinfo,ops_reader_t *reader,ops_reader_destroyer_t *destroyer,void *arg)
    {
    ops_reader_info_t *rinfo=malloc(sizeof *rinfo);

    *rinfo=pinfo->rinfo;
    memset(&pinfo->rinfo,'\0',sizeof pinfo->rinfo);
    pinfo->rinfo.next=rinfo;
    pinfo->rinfo.pinfo=pinfo;

    // should copy accumulate flags from other reader? RW
    pinfo->rinfo.accumulate=rinfo->accumulate;
    
    ops_reader_set(pinfo,reader,destroyer,arg);
    }

/**
 * \ingroup Internal_Readers_Generic
 * \brief Removes from reader stack
 * \param pinfo Parse settings
 */
void ops_reader_pop(ops_parse_info_t *pinfo)
    { 
    ops_reader_info_t *next=pinfo->rinfo.next;

    pinfo->rinfo=*next;
    free(next);
    }

/**
 * \ingroup Internal_Readers_Generic
 * \brief Gets arg from reader
 * \param rinfo Reader info
 * \return Pointer to reader info's arg
 */
void *ops_reader_get_arg(ops_reader_info_t *rinfo)
    { return rinfo->arg; }

/**
 * \ingroup Internal_Readers_Generic
 * \brief Gets reader's arg from parse_info
 * \param pinfo
 * \return Pointer to parse_info's reader_info's arg
 */
void *ops_reader_get_arg_from_pinfo(ops_parse_info_t *pinfo)
    { return pinfo->rinfo.arg; }

// EOF

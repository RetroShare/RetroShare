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

#include <openpgpsdk/final.h>

//static int debug=0;

typedef struct
    {
    ops_hash_algorithm_t hash_algorithm;
    ops_hash_t hash;
    unsigned char *hashed;
    } skey_checksum_arg_t;

static ops_boolean_t skey_checksum_writer(const unsigned char *src, const unsigned length, ops_error_t **errors, ops_writer_info_t *winfo)
    {
    skey_checksum_arg_t *arg=ops_writer_get_arg(winfo);
    ops_boolean_t rtn=ops_true;

    // add contents to hash
    arg->hash.add(&arg->hash, src, length);

    // write to next stacked writer
    rtn=ops_stacked_write(src,length,errors,winfo);

    // tidy up and return
    return rtn;
    }

static ops_boolean_t skey_checksum_finaliser(ops_error_t **errors __attribute__((unused)), ops_writer_info_t *winfo)
    {
    skey_checksum_arg_t *arg=ops_writer_get_arg(winfo);
    arg->hash.finish(&arg->hash, arg->hashed);
    return ops_true;
    }

static void skey_checksum_destroyer(ops_writer_info_t* winfo)
    {
    skey_checksum_arg_t *arg=ops_writer_get_arg(winfo);
    free(arg);
    }

/**
\ingroup Core_WritersNext
\param cinfo
\param skey
*/
void ops_push_skey_checksum_writer(ops_create_info_t *cinfo, ops_secret_key_t *skey)
    {
    //    OPS_USED(info);
    // XXX: push a SHA-1 checksum writer (and change s2k to 254).
    skey_checksum_arg_t *arg=ops_mallocz(sizeof *arg);

    // configure the arg
    arg->hash_algorithm=skey->hash_algorithm;
    arg->hashed=&skey->checkhash[0];

    // init the hash
    ops_hash_any(&arg->hash, arg->hash_algorithm);
    arg->hash.init(&arg->hash);

    ops_writer_push(cinfo, skey_checksum_writer, skey_checksum_finaliser, skey_checksum_destroyer, arg);
    }
 
// EOF

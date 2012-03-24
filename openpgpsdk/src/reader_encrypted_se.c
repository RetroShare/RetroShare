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

#include <openpgpsdk/crypto.h>
#include <string.h>
#include <assert.h>
#include <openssl/cast.h>
#ifndef OPENSSL_NO_IDEA
#include <openssl/idea.h>
#endif
#include <openssl/aes.h>
#include <openssl/des.h>
#include "parse_local.h"

#include <openpgpsdk/packet-show.h>
#include <openpgpsdk/final.h>

static int debug=0;

#ifndef ATTRIBUTE_UNUSED

#ifndef WIN32
#define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#else
#define ATTRIBUTE_UNUSED 
#endif // #ifndef WIN32

#endif /* ATTRIBUTE_UNUSED */


// \todo there's also a encrypted_arg_t in adv_create.c 
// which is used for *encrypting* whereas this is used
// for *decrypting*

typedef struct
    {
    unsigned char decrypted[1024];
    size_t decrypted_count;
    size_t decrypted_offset;
    ops_crypt_t *decrypt;
    ops_region_t *region;
    ops_boolean_t prev_read_was_plain:1;
    } encrypted_arg_t;

static int encrypted_data_reader(void *dest,size_t length,ops_error_t **errors,
				 ops_reader_info_t *rinfo,
				 ops_parse_cb_info_t *cbinfo)
    {
    encrypted_arg_t *arg=ops_reader_get_arg(rinfo);
    int saved=length;

    // V3 MPIs have the count plain and the cipher is reset after each count
    if(arg->prev_read_was_plain && !rinfo->pinfo->reading_mpi_length)
	{
	assert(rinfo->pinfo->reading_v3_secret);
	arg->decrypt->decrypt_resync(arg->decrypt);
	arg->prev_read_was_plain=ops_false;
	}
    else if(rinfo->pinfo->reading_v3_secret
	    && rinfo->pinfo->reading_mpi_length)
        {
	arg->prev_read_was_plain=ops_true;
        }

    while(length > 0)
	{
	if(arg->decrypted_count)
	    {

	    unsigned n;

	    // if we are reading v3 we should never read more than
	    // we're asked for
	    assert(length >= arg->decrypted_count
		   || (!rinfo->pinfo->reading_v3_secret
		       && !rinfo->pinfo->exact_read));

	    if(length > arg->decrypted_count)
		n=arg->decrypted_count;
	    else
		n=length;

	    memcpy(dest,arg->decrypted+arg->decrypted_offset,n);
	    arg->decrypted_count-=n;
	    arg->decrypted_offset+=n;
	    length-=n;
#ifdef WIN32
	    (char*)dest+=n;
#else
        dest+=n;
#endif
	    }
	else
	    {
	    unsigned n=arg->region->length;
	    unsigned char buffer[1024];

	    if(!n)
            {
		return -1;
            }

	    if(!arg->region->indeterminate)
		{
		n-=arg->region->length_read;
		if(n == 0)
		    return saved-length;
		if(n > sizeof buffer)
		    n=sizeof buffer;
		}
	    else
            {
		n=sizeof buffer;
            }

	    // we can only read as much as we're asked for in v3 keys
	    // because they're partially unencrypted!
	    if((rinfo->pinfo->reading_v3_secret || rinfo->pinfo->exact_read)
	       && n > length)
		n=length;

	    if(!ops_stacked_limited_read(buffer,n,arg->region,errors,rinfo,
					 cbinfo))
            {
		return -1;
            }

	    if(!rinfo->pinfo->reading_v3_secret
	       || !rinfo->pinfo->reading_mpi_length)
                {
                arg->decrypted_count=ops_decrypt_se_ip(arg->decrypt,
                                  arg->decrypted,
                                  buffer,n);

                if (debug)
                    {
                    fprintf(stderr,"READING:\nencrypted: ");
                    int i=0;
                    for (i=0; i<16; i++)
                        fprintf(stderr,"%2x ", buffer[i]);
                    fprintf(stderr,"\n");
                    fprintf(stderr,"decrypted:   ");
                    for (i=0; i<16; i++)
                        fprintf(stderr,"%2x ", arg->decrypted[i]);
                    fprintf(stderr,"\n");
                    }
                }
	    else
		{
		memcpy(arg->decrypted,buffer,n);
		arg->decrypted_count=n;
		}

	    assert(arg->decrypted_count > 0);

	    arg->decrypted_offset=0;
	    }
	}

    return saved;
    }

static void encrypted_data_destroyer(ops_reader_info_t *rinfo)
    { free(ops_reader_get_arg(rinfo)); }

/**
 * \ingroup Core_Readers_SE
 * \brief Pushes decryption reader onto stack
 * \sa ops_reader_pop_decrypt()
 */
void ops_reader_push_decrypt(ops_parse_info_t *pinfo,ops_crypt_t *decrypt,
			     ops_region_t *region)
    {
    encrypted_arg_t *arg=ops_mallocz(sizeof *arg);

    arg->decrypt=decrypt;
    arg->region=region;

    ops_decrypt_init(arg->decrypt);

    ops_reader_push(pinfo,encrypted_data_reader,encrypted_data_destroyer,arg);
    }

/**
 * \ingroup Core_Readers_Encrypted
 * \brief Pops decryption reader from stack
 * \sa ops_reader_push_decrypt()
 */
void ops_reader_pop_decrypt(ops_parse_info_t *pinfo)
    {
    encrypted_arg_t *arg=ops_reader_get_arg(ops_parse_get_rinfo(pinfo));

    arg->decrypt->decrypt_finish(arg->decrypt);
    free(arg);
    
    ops_reader_pop(pinfo);
    }

// eof

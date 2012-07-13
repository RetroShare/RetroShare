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
 * \brief Parser for OpenPGP packets
 */

#include <openssl/cast.h>

#include <openpgpsdk/callback.h>
#include <openpgpsdk/packet.h>
#include <openpgpsdk/packet-parse.h>
#include <openpgpsdk/keyring.h>
#include <openpgpsdk/util.h>
#include <openpgpsdk/compress.h>
#include <openpgpsdk/errors.h>
#include <openpgpsdk/readerwriter.h>
#include <openpgpsdk/packet-show.h>
#include <openpgpsdk/std_print.h>
#include <openpgpsdk/create.h>
#include <openpgpsdk/hash.h>

#include "parse_local.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <errno.h>
#include <limits.h>

#include <openpgpsdk/final.h>

static int debug=0;

typedef struct
    {
    // boolean: false once we've done the preamble/MDC checks
    // and are reading from the plaintext
    int passed_checks; 
    unsigned char *plaintext;
    size_t plaintext_available;
    size_t plaintext_offset;
    ops_region_t *region;
    ops_crypt_t *decrypt;
    } decrypt_se_ip_arg_t;

static int se_ip_data_reader(void *dest_, size_t len, ops_error_t **errors,
                             ops_reader_info_t *rinfo,
                             ops_parse_cb_info_t *cbinfo)
    {

    /*
      Gets entire SE_IP data packet.
      Verifies leading preamble
      Verifies trailing MDC packet
      Then passes up plaintext as requested
    */

    unsigned int n=0;

    ops_region_t decrypted_region;

    decrypt_se_ip_arg_t *arg=ops_reader_get_arg(rinfo);

    if (!arg->passed_checks)
        {
        unsigned char*buf=NULL;

        ops_hash_t hash;
        unsigned char hashed[SHA_DIGEST_LENGTH];

        size_t b;
        size_t sz_preamble;
        size_t sz_mdc_hash;
        size_t sz_mdc;
        size_t sz_plaintext;

        unsigned char* preamble;
        unsigned char* plaintext;
        unsigned char* mdc;
        unsigned char* mdc_hash;

        ops_hash_any(&hash,OPS_HASH_SHA1);
        hash.init(&hash);

        ops_init_subregion(&decrypted_region,NULL);
        decrypted_region.length = arg->region->length - arg->region->length_read;
        buf=ops_mallocz(decrypted_region.length);

        // read entire SE IP packet
        
        if (!ops_stacked_limited_read(buf,decrypted_region.length, &decrypted_region,errors,rinfo,cbinfo))
            {
            free (buf);
            return -1;
            }

        if (debug)
            {
            unsigned int i=0;
            fprintf(stderr,"\n\nentire SE IP packet (len=%d):\n",decrypted_region.length);
            for (i=0; i<decrypted_region.length; i++)
                {
                fprintf(stderr,"0x%02x ", buf[i]);
                if (!((i+1)%8))
                    fprintf(stderr,"\n");
                }
            fprintf(stderr,"\n");
            fprintf(stderr,"\n");
            }

        // verify leading preamble

        if (debug)
            {
            unsigned int i=0;
            fprintf(stderr,"\npreamble: ");
            for (i=0; i<arg->decrypt->blocksize+2;i++)
                fprintf(stderr," 0x%02x", buf[i]);
            fprintf(stderr,"\n");
            }

        b=arg->decrypt->blocksize;
        if(buf[b-2] != buf[b] || buf[b-1] != buf[b+1])
            {
            fprintf(stderr,"Bad symmetric decrypt (%02x%02x vs %02x%02x)\n",
                    buf[b-2],buf[b-1],buf[b],buf[b+1]);
            OPS_ERROR(errors, OPS_E_PROTO_BAD_SYMMETRIC_DECRYPT,"Bad symmetric decrypt when parsing SE IP packet");
            free(buf);
            return -1;
            }

        // Verify trailing MDC hash

        sz_preamble=arg->decrypt->blocksize+2;
        sz_mdc_hash=OPS_SHA1_HASH_SIZE;
        sz_mdc=1+1+sz_mdc_hash;
        sz_plaintext=decrypted_region.length-sz_preamble-sz_mdc;

        preamble=buf;
        plaintext=buf+sz_preamble;
        mdc=plaintext+sz_plaintext;
        mdc_hash=mdc+2;
    
#ifdef DEBUG
        if (debug)
            {
            unsigned int i=0;

            fprintf(stderr,"\nplaintext (len=%ld): ",sz_plaintext);
            for (i=0; i<sz_plaintext;i++)
                fprintf(stderr," 0x%02x", plaintext[i]);
            fprintf(stderr,"\n");

            fprintf(stderr,"\nmdc (len=%ld): ",sz_mdc);
            for (i=0; i<sz_mdc;i++)
                fprintf(stderr," 0x%02x", mdc[i]);
            fprintf(stderr,"\n");
            }
#endif /*DEBUG*/

        ops_calc_mdc_hash(preamble,sz_preamble,plaintext,sz_plaintext,&hashed[0]);

        if (memcmp(mdc_hash,hashed,OPS_SHA1_HASH_SIZE))
            {
            OPS_ERROR(errors, OPS_E_V_BAD_HASH, "Bad hash in MDC packet");
            free(buf);
            return 0;
            }

        // all done with the checks
        // now can start reading from the plaintext
        assert(!arg->plaintext);
        arg->plaintext=ops_mallocz(sz_plaintext);
        memcpy(arg->plaintext, plaintext, sz_plaintext);
        arg->plaintext_available=sz_plaintext;

        arg->passed_checks=1;

        free(buf);
        }

    n=len;
    if (n > arg->plaintext_available)
        n=arg->plaintext_available;

    memcpy(dest_, arg->plaintext+arg->plaintext_offset, n);
    arg->plaintext_available-=n;
    arg->plaintext_offset+=n;
    len-=n;

    return n;
    }

static void se_ip_data_destroyer(ops_reader_info_t *rinfo)
    {
    decrypt_se_ip_arg_t* arg=ops_reader_get_arg(rinfo);
    free (arg->plaintext);
    free (arg);
    //    free(ops_reader_get_arg(rinfo));
    }

/**
   \ingroup Internal_Readers_SEIP
*/
void ops_reader_push_se_ip_data(ops_parse_info_t *pinfo, ops_crypt_t *decrypt,
                                ops_region_t *region)
    {
    decrypt_se_ip_arg_t *arg=ops_mallocz(sizeof *arg);
    arg->region=region;
    arg->decrypt=decrypt;

    ops_reader_push(pinfo, se_ip_data_reader, se_ip_data_destroyer,arg);
    }

/**
   \ingroup Internal_Readers_SEIP
 */
void ops_reader_pop_se_ip_data(ops_parse_info_t* pinfo)
    {
    //    decrypt_se_ip_arg_t *arg=ops_reader_get_arg(ops_parse_get_rinfo(pinfo));
    //    free(arg);
    ops_reader_pop(pinfo);
    }

// eof

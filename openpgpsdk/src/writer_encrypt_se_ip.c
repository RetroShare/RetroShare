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

#include <string.h>
#include <assert.h>

#ifndef WIN32
 #include <unistd.h>
#endif

#include <openssl/cast.h>

#include "keyring_local.h"
#include <openpgpsdk/compress.h>
#include <openpgpsdk/create.h>
#include <openpgpsdk/hash.h>
#include <openpgpsdk/keyring.h>
#include <openpgpsdk/random.h>
#include <openpgpsdk/readerwriter.h>

static int debug=0;

typedef struct 
    {
    ops_crypt_t* crypt;
    } encrypt_se_ip_arg_t;

static ops_boolean_t encrypt_se_ip_writer(const unsigned char *src,
                                          unsigned length,
                                          ops_error_t **errors,
                                          ops_writer_info_t *winfo);
static void encrypt_se_ip_destroyer (ops_writer_info_t *winfo);

//

/**
\ingroup Core_WritersNext
\brief Push Encrypted SE IP Writer onto stack
*/
void ops_writer_push_encrypt_se_ip(ops_create_info_t *cinfo,
                             const ops_keydata_t *pub_key)
    {
    ops_crypt_t* encrypt;
    unsigned char *iv=NULL;

    // Create arg to be used with this writer
    // Remember to free this in the destroyer
    encrypt_se_ip_arg_t *arg=ops_mallocz(sizeof *arg);

    // Create and write encrypted PK session key
    ops_pk_session_key_t* encrypted_pk_session_key;
    encrypted_pk_session_key=ops_create_pk_session_key(pub_key);
    ops_write_pk_session_key(cinfo,encrypted_pk_session_key);

    // Setup the arg
    encrypt=ops_mallocz(sizeof *encrypt);
    ops_crypt_any(encrypt, encrypted_pk_session_key->symmetric_algorithm);
    iv=ops_mallocz(encrypt->blocksize);
    encrypt->set_iv(encrypt, iv);
    encrypt->set_key(encrypt, &encrypted_pk_session_key->key[0]);
    ops_encrypt_init(encrypt);

    arg->crypt=encrypt;

    // And push writer on stack
    ops_writer_push(cinfo,encrypt_se_ip_writer,NULL,encrypt_se_ip_destroyer,arg);
    // tidy up
    free(encrypted_pk_session_key);
    free(iv);
    }

static ops_boolean_t encrypt_se_ip_writer(const unsigned char *src,
                                          unsigned length,
                                          ops_error_t **errors,
                                          ops_writer_info_t *winfo)
    {
    encrypt_se_ip_arg_t *arg=ops_writer_get_arg(winfo);

    ops_boolean_t rtn=ops_true;

    ops_memory_t *mem_literal;
    ops_create_info_t *cinfo_literal;

    ops_memory_t *mem_compressed;
    ops_create_info_t *cinfo_compressed;

    ops_memory_t *my_mem;
    ops_create_info_t *my_cinfo;

    const unsigned int bufsz=128; // initial value; gets expanded as necessary
    ops_setup_memory_write(&cinfo_literal,&mem_literal,bufsz);
    ops_setup_memory_write(&cinfo_compressed,&mem_compressed,bufsz);
    ops_setup_memory_write(&my_cinfo,&my_mem,bufsz);

    // create literal data packet from source data
    ops_write_literal_data_from_buf(src, length, OPS_LDT_BINARY, cinfo_literal);
    assert(ops_memory_get_length(mem_literal)>length);

    // create compressed packet from literal data packet
    ops_write_compressed(ops_memory_get_data(mem_literal),
                         ops_memory_get_length(mem_literal),
                         cinfo_compressed);

    // create SE IP packet set from this compressed literal data
    ops_write_se_ip_pktset(ops_memory_get_data(mem_compressed), 
                           ops_memory_get_length(mem_compressed), 
                           arg->crypt, my_cinfo);
    assert(ops_memory_get_length(my_mem)>ops_memory_get_length(mem_compressed));

    // now write memory to next writer
    rtn=ops_stacked_write(ops_memory_get_data(my_mem),
                          ops_memory_get_length(my_mem),
                          errors, winfo);
    
    ops_memory_free(my_mem);
    ops_memory_free(mem_compressed);
    ops_memory_free(mem_literal);

    return rtn;
    }

static void encrypt_se_ip_destroyer (ops_writer_info_t *winfo)
     
    {
    encrypt_se_ip_arg_t *arg=ops_writer_get_arg(winfo);

    free(arg->crypt);
    free(arg);
    }

ops_boolean_t ops_write_se_ip_pktset(const unsigned char *data,
                                   const unsigned int len,
                                   ops_crypt_t *crypt,
                                   ops_create_info_t *cinfo)
    {
    unsigned char hashed[SHA_DIGEST_LENGTH];
    const size_t sz_mdc=1+1+SHA_DIGEST_LENGTH;

    size_t sz_preamble=crypt->blocksize+2;
    unsigned char* preamble=ops_mallocz(sz_preamble);

    size_t sz_buf=sz_preamble+len+sz_mdc;

    ops_memory_t *mem_mdc;
    ops_create_info_t *cinfo_mdc;

    if (!ops_write_ptag(OPS_PTAG_CT_SE_IP_DATA,cinfo)
        || !ops_write_length(1+sz_buf,cinfo)
        || !ops_write_scalar(SE_IP_DATA_VERSION,1,cinfo))
        {
        free (preamble);
        return 0;
        }

    ops_random(preamble, crypt->blocksize);
    preamble[crypt->blocksize]=preamble[crypt->blocksize-2];
    preamble[crypt->blocksize+1]=preamble[crypt->blocksize-1];

    if (debug)
        {
        unsigned int i=0;
        fprintf(stderr,"\npreamble: ");
        for (i=0; i<sz_preamble;i++)
            fprintf(stderr," 0x%02x", preamble[i]);
        fprintf(stderr,"\n");
        }

    // now construct MDC packet and add to the end of the buffer

    ops_setup_memory_write(&cinfo_mdc, &mem_mdc,sz_mdc);

    ops_calc_mdc_hash(preamble,sz_preamble,data,len,&hashed[0]);

    ops_write_mdc(hashed, cinfo_mdc);

    if (debug)
        {
        unsigned int i=0;
        size_t sz_plaintext=len;
        size_t sz_mdc=1+1+OPS_SHA1_HASH_SIZE;
        unsigned char* mdc=NULL;

        fprintf(stderr,"\nplaintext: ");
        for (i=0; i<sz_plaintext;i++)
            fprintf(stderr," 0x%02x", data[i]);
        fprintf(stderr,"\n");
        
        fprintf(stderr,"\nmdc: ");
        mdc=ops_memory_get_data(mem_mdc);
        for (i=0; i<sz_mdc;i++)
            fprintf(stderr," 0x%02x", mdc[i]);
        fprintf(stderr,"\n");
        }
    
    // and write it out

    ops_writer_push_encrypt_crypt(cinfo, crypt);

#ifdef DEBUG
    if (debug)
        {
        fprintf(stderr,"writing %ld + %d + %ld\n", sz_preamble, len, ops_memory_get_length(mem_mdc));
        }
#endif /*DEBUG*/

    if (!ops_write(preamble, sz_preamble,cinfo)
        || !ops_write(data, len, cinfo)
        || !ops_write(ops_memory_get_data(mem_mdc), ops_memory_get_length(mem_mdc), cinfo))
        // \todo fix cleanup here and in old code functions
        return 0;

    ops_writer_pop(cinfo);

    // cleanup 
    ops_teardown_memory_write(cinfo_mdc, mem_mdc);
    free (preamble);

    return 1;
    }

// EOF

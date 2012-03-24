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
#include <openpgpsdk/create.h>
#include <openpgpsdk/keyring.h>
#include <openpgpsdk/random.h>
#include <openpgpsdk/readerwriter.h>

#define MAX_PARTIAL_DATA_LENGTH 1073741824

typedef struct 
    {
    ops_crypt_t*crypt;
    ops_memory_t *mem_data;
    ops_memory_t *mem_literal;
    ops_create_info_t *cinfo_literal;
    ops_memory_t *mem_se_ip;
    ops_create_info_t *cinfo_se_ip;
    ops_hash_t hash;
    } stream_encrypt_se_ip_arg_t;


static ops_boolean_t stream_encrypt_se_ip_writer(const unsigned char *src,
                                                 unsigned length,
                                                 ops_error_t **errors,
                                                 ops_writer_info_t *winfo);

static ops_boolean_t stream_encrypt_se_ip_finaliser(ops_error_t**      errors,
                                                    ops_writer_info_t* winfo);

static void stream_encrypt_se_ip_destroyer (ops_writer_info_t *winfo);

//

/**
\ingroup Core_WritersNext
\param cinfo
\param pub_key
*/
void ops_writer_push_stream_encrypt_se_ip(ops_create_info_t *cinfo,
                                          const ops_keydata_t *pub_key)
    {
    ops_crypt_t* encrypt;
    unsigned char *iv=NULL;

    const unsigned int bufsz=1024; // initial value; gets expanded as necessary

    // Create arg to be used with this writer
    // Remember to free this in the destroyer
    stream_encrypt_se_ip_arg_t *arg=ops_mallocz(sizeof *arg);

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

    arg->mem_data=ops_memory_new();
    ops_memory_init(arg->mem_data,bufsz); 
    
    arg->mem_literal = NULL;
    arg->cinfo_literal = NULL;

    ops_setup_memory_write(&arg->cinfo_se_ip, &arg->mem_se_ip, bufsz);

    // And push writer on stack
    ops_writer_push(cinfo,
                    stream_encrypt_se_ip_writer,
                    stream_encrypt_se_ip_finaliser,
                    stream_encrypt_se_ip_destroyer,arg);
    // tidy up
    free(encrypted_pk_session_key);
    free(iv);
    }



unsigned int ops_calc_partial_data_length(unsigned int len)
    {
    int i;
    unsigned int mask = MAX_PARTIAL_DATA_LENGTH;
    assert( len > 0 );

    if ( len > MAX_PARTIAL_DATA_LENGTH ) {
        return MAX_PARTIAL_DATA_LENGTH;
    }

    for ( i = 0; i <= 30; i++ ) {
        if ( mask & len) break; 
        mask >>= 1;
    }

    return mask;
    }

ops_boolean_t ops_write_partial_data_length(unsigned int len, 
                                            ops_create_info_t *info)
    {
    // len must be a power of 2 from 0 to 30
    int i;
    unsigned char c[1];

    for ( i = 0; i <= 30; i++ ) {
        if ( (len >> i) & 1) break; 
    }

    c[0] = 224 + i;
 	return ops_write(c,1,info);
    }

ops_boolean_t ops_stream_write_literal_data(const unsigned char *data, 
                                            unsigned int len, 
                                            ops_create_info_t *info)
    {
    while (len > 0) {
        size_t pdlen = ops_calc_partial_data_length(len);
        ops_write_partial_data_length(pdlen, info);
        ops_write(data, pdlen, info);
        data += pdlen;
        len -= pdlen;
    }
    return ops_true;
    }

ops_boolean_t ops_stream_write_literal_data_first(const unsigned char *data, 
                                                  unsigned int len, 
                                                  const ops_literal_data_type_t type,
                                                  ops_create_info_t *info)
    {
    // \todo add filename 
    // \todo add date
    // \todo do we need to check text data for <cr><lf> line endings ?

    size_t sz_towrite = 1 + 1 + 4 + len;
    size_t sz_pd = ops_calc_partial_data_length(sz_towrite);
    assert(sz_pd >= 512);

    ops_write_ptag(OPS_PTAG_CT_LITERAL_DATA, info);
    ops_write_partial_data_length(sz_pd, info);
    ops_write_scalar(type, 1, info);
    ops_write_scalar(0, 1, info);
    ops_write_scalar(0, 4, info);
    ops_write(data, sz_pd - 6, info);

    data += (sz_pd - 6);  
    sz_towrite -= sz_pd;

    ops_stream_write_literal_data(data, sz_towrite, info);
    return ops_true;
    }

ops_boolean_t ops_stream_write_literal_data_last(const unsigned char *data, 
                                                 unsigned int len, 
                                                 ops_create_info_t *info)
    {
    ops_write_length(len, info);
    ops_write(data, len, info);
    return ops_true;
    }

ops_boolean_t ops_stream_write_se_ip(const unsigned char *data,
                                     unsigned int len,
                                     stream_encrypt_se_ip_arg_t *arg,
                                     ops_create_info_t *cinfo)
    {
    while (len > 0) {
        size_t pdlen = ops_calc_partial_data_length(len);
        ops_write_partial_data_length(pdlen, cinfo);

        ops_writer_push_encrypt_crypt(cinfo, arg->crypt);
        ops_write(data, pdlen, cinfo);
        ops_writer_pop(cinfo);

        arg->hash.add(&arg->hash, data, pdlen);

        data += pdlen;
        len -= pdlen;
    }
    return ops_true;
    }

ops_boolean_t ops_stream_write_se_ip_first(const unsigned char *data,
                                           unsigned int len,
                                           stream_encrypt_se_ip_arg_t *arg,
                                           ops_create_info_t *cinfo)
    {
    size_t sz_preamble = arg->crypt->blocksize + 2;
    size_t sz_towrite = sz_preamble + 1 + len;
    unsigned char* preamble = ops_mallocz(sz_preamble);

    size_t sz_pd = ops_calc_partial_data_length(sz_towrite);
    assert(sz_pd >= 512);

    ops_write_ptag(OPS_PTAG_CT_SE_IP_DATA, cinfo);
    ops_write_partial_data_length(sz_pd, cinfo);
    ops_write_scalar(SE_IP_DATA_VERSION, 1, cinfo);

    ops_writer_push_encrypt_crypt(cinfo, arg->crypt);

    ops_random(preamble, arg->crypt->blocksize);
    preamble[arg->crypt->blocksize]=preamble[arg->crypt->blocksize-2];
    preamble[arg->crypt->blocksize+1]=preamble[arg->crypt->blocksize-1];

    ops_hash_any(&arg->hash, OPS_HASH_SHA1);
    arg->hash.init(&arg->hash);

    ops_write(preamble, sz_preamble, cinfo);
    arg->hash.add(&arg->hash, preamble, sz_preamble);

    ops_write(data, sz_pd - sz_preamble - 1, cinfo);
    arg->hash.add(&arg->hash, data, sz_pd - sz_preamble - 1);

    data += (sz_pd - sz_preamble -1);  
    sz_towrite -= sz_pd;

    ops_writer_pop(cinfo);

    ops_stream_write_se_ip(data, sz_towrite, arg, cinfo);

    free(preamble);

    return ops_true;
    }

ops_boolean_t ops_stream_write_se_ip_last(const unsigned char *data,
                                          unsigned int len,
                                          stream_encrypt_se_ip_arg_t *arg,
                                          ops_create_info_t *cinfo)
    {
    unsigned char c[1];
    unsigned char hashed[SHA_DIGEST_LENGTH];
    const size_t sz_mdc = 1 + 1 + SHA_DIGEST_LENGTH;
    size_t sz_buf = len + sz_mdc;

    ops_memory_t *mem_mdc;
    ops_create_info_t *cinfo_mdc;

    arg->hash.add(&arg->hash, data, len);

    // MDC packet tag
    c[0]=0xD3;
    arg->hash.add(&arg->hash, &c[0], 1);   

    // MDC packet len
    c[0]=0x14;
    arg->hash.add(&arg->hash, &c[0], 1);   

    //finish
    arg->hash.finish(&arg->hash, hashed);

    ops_setup_memory_write(&cinfo_mdc, &mem_mdc, sz_mdc);
    ops_write_mdc(hashed, cinfo_mdc);

    // write length of last se_ip chunk
    ops_write_length(sz_buf, cinfo);

    // encode everting
    ops_writer_push_encrypt_crypt(cinfo, arg->crypt);

    ops_write(data, len, cinfo);
    ops_write(ops_memory_get_data(mem_mdc), ops_memory_get_length(mem_mdc), cinfo);

    ops_writer_pop(cinfo);

    ops_teardown_memory_write(cinfo_mdc, mem_mdc);

    return ops_true;
    }

static ops_boolean_t stream_encrypt_se_ip_writer(const unsigned char *src,
                                                 unsigned length,
                                                 ops_error_t **errors,
                                                 ops_writer_info_t *winfo)
    {
    stream_encrypt_se_ip_arg_t *arg=ops_writer_get_arg(winfo);

    ops_boolean_t rtn=ops_true;

    if ( arg->cinfo_literal == NULL ) { // first literal data chunk is not yet written
        size_t datalength;
        
        ops_memory_add(arg->mem_data,src,length); 
        datalength = ops_memory_get_length(arg->mem_data);
        
        // 4.2.2.4. Partial Body Lengths
        // The first partial length MUST be at least 512 octets long.
        if ( datalength < 512 ) {
            return ops_true; // will wait for more data or end of stream            
        }

        ops_setup_memory_write(&arg->cinfo_literal,&arg->mem_literal,datalength+32);
        ops_stream_write_literal_data_first(ops_memory_get_data(arg->mem_data),
                                            datalength,
                                            OPS_LDT_BINARY,
                                            arg->cinfo_literal);

        ops_stream_write_se_ip_first(ops_memory_get_data(arg->mem_literal), 
                                     ops_memory_get_length(arg->mem_literal), 
                                     arg, arg->cinfo_se_ip);
    } else {
        ops_stream_write_literal_data(src, length, arg->cinfo_literal);
        ops_stream_write_se_ip(ops_memory_get_data(arg->mem_literal), 
                               ops_memory_get_length(arg->mem_literal), 
                               arg, arg->cinfo_se_ip);
    }

    // now write memory to next writer
    rtn=ops_stacked_write(ops_memory_get_data(arg->mem_se_ip),
                          ops_memory_get_length(arg->mem_se_ip),
                          errors, winfo);
    
    ops_memory_clear(arg->mem_literal);
    ops_memory_clear(arg->mem_se_ip);

    return rtn;
    }

static ops_boolean_t stream_encrypt_se_ip_finaliser(ops_error_t**      errors,
                                                    ops_writer_info_t* winfo)
    {
    stream_encrypt_se_ip_arg_t *arg=ops_writer_get_arg(winfo);
    // write last chunk of data

    if ( arg->cinfo_literal == NULL ) { 
        // first literal data chunk was not written
        // so we know the total length of data, write a simple packet

        // create literal data packet from buffered data
        ops_setup_memory_write(&arg->cinfo_literal,
                               &arg->mem_literal,
                               ops_memory_get_length(arg->mem_data)+32);

        ops_write_literal_data_from_buf(ops_memory_get_data(arg->mem_data),
                               ops_memory_get_length(arg->mem_data),
                               OPS_LDT_BINARY, arg->cinfo_literal);

        // create SE IP packet set from this literal data
        ops_write_se_ip_pktset(ops_memory_get_data(arg->mem_literal), 
                               ops_memory_get_length(arg->mem_literal), 
                               arg->crypt, arg->cinfo_se_ip);

    } else {
        // finish writing
        ops_stream_write_literal_data_last(NULL, 0, arg->cinfo_literal);
        ops_stream_write_se_ip_last(ops_memory_get_data(arg->mem_literal), 
                                    ops_memory_get_length(arg->mem_literal), 
                                    arg, arg->cinfo_se_ip);
    }

    // now write memory to next writer
    return ops_stacked_write(ops_memory_get_data(arg->mem_se_ip),
                             ops_memory_get_length(arg->mem_se_ip),
                             errors, winfo);
    }

static void stream_encrypt_se_ip_destroyer (ops_writer_info_t *winfo)
     
    {
    stream_encrypt_se_ip_arg_t *arg=ops_writer_get_arg(winfo);

    ops_memory_free(arg->mem_data);
    ops_teardown_memory_write(arg->cinfo_literal, arg->mem_literal);
    ops_teardown_memory_write(arg->cinfo_se_ip, arg->mem_se_ip);

    arg->crypt->decrypt_finish(arg->crypt);

    free(arg->crypt);
    free(arg);
    }


// EOF

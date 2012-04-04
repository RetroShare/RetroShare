/*
 * Copyright (c) 2005-2009 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer, Alasdair Mackintosh.
 * The Contributors have asserted their moral rights under the
 * UK Copyright Design and Patents Act 1988 to
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
#include <openpgpsdk/partial.h>
#include <openpgpsdk/random.h>
#include <openpgpsdk/streamwriter.h>

typedef struct 
    {
    ops_crypt_t*crypt;
    ops_memory_t *mem_se_ip;
    ops_create_info_t *cinfo_se_ip;
    ops_hash_t hash;
    } stream_encrypt_se_ip_arg_t;

static ops_boolean_t write_encrypt_se_ip_header(ops_create_info_t *info,
                                                void *header_data);


static ops_boolean_t stream_encrypt_se_ip_writer(const unsigned char *src,
                                                 unsigned length,
                                                 ops_error_t **errors,
                                                 ops_writer_info_t *winfo);

static ops_boolean_t stream_encrypt_se_ip_finaliser(ops_error_t **errors,
                                                    ops_writer_info_t *winfo);

static void stream_encrypt_se_ip_destroyer (ops_writer_info_t *winfo);


/**
\ingroup Core_WritersNext
\brief Pushes a streaming encryption writer onto the stack.

Data written to the stream will be encoded in a Symmetrically
Encrypted Integrity Protected packet. Note that this writer must be
used in conjunction with a literal writer or a signed writer.

\param cinfo
\param pub_key
Example Code:
\code
    ops_writer_push_stream_encrypt_se_ip(cinfo, public_key);
    if (compress)
        ops_writer_push_compressed(cinfo);
    if (sign)
       ops_writer_push_signed(cinfo, OPS_SIG_BINARY, secret_key);
    else
        ops_writer_push_literal(cinfo);
\endcode
*/

void ops_writer_push_stream_encrypt_se_ip(ops_create_info_t *cinfo,
                                          const ops_keydata_t *pub_key)
    {
    ops_crypt_t *encrypt;
    unsigned char *iv=NULL;
    const unsigned int bufsz=1024; // initial value; gets expanded as necessary

    // Create arg to be used with this writer
    // Remember to free this in the destroyer
    stream_encrypt_se_ip_arg_t *arg=ops_mallocz(sizeof *arg);

    // Create and write encrypted PK session key
    ops_pk_session_key_t *encrypted_pk_session_key;
    encrypted_pk_session_key=ops_create_pk_session_key(pub_key);
    ops_write_pk_session_key(cinfo, encrypted_pk_session_key);

    // Setup the arg
    encrypt=ops_mallocz(sizeof *encrypt);
    ops_crypt_any(encrypt, encrypted_pk_session_key->symmetric_algorithm);
    iv=ops_mallocz(encrypt->blocksize);
    encrypt->set_iv(encrypt, iv);
    encrypt->set_key(encrypt, &encrypted_pk_session_key->key[0]);
    ops_encrypt_init(encrypt);

    arg->crypt=encrypt;

    ops_setup_memory_write(&arg->cinfo_se_ip, &arg->mem_se_ip, bufsz);

    ops_hash_any(&arg->hash, OPS_HASH_SHA1);
    arg->hash.init(&arg->hash);
  
    // This is a streaming writer, so we don't know the length in
    // advance. Use a partial writer to handle the partial body
    // packet lengths.
    ops_writer_push_partial(2048, cinfo, OPS_PTAG_CT_SE_IP_DATA,
			    write_encrypt_se_ip_header, arg);

    // And push encryption writer on stack
    ops_writer_push(cinfo,
		    stream_encrypt_se_ip_writer,
		    stream_encrypt_se_ip_finaliser,
		    stream_encrypt_se_ip_destroyer, arg);
    // tidy up
    ops_pk_session_key_free(encrypted_pk_session_key);
    free(encrypted_pk_session_key);
    free(iv);
    }

static ops_boolean_t ops_stream_write_se_ip(const unsigned char *data,
                                            unsigned int len,
                                            stream_encrypt_se_ip_arg_t *arg,
                                            ops_create_info_t *cinfo)
    {
    ops_writer_push_encrypt_crypt(cinfo, arg->crypt);
    ops_write(data, len, cinfo);
    ops_writer_pop(cinfo);
    arg->hash.add(&arg->hash, data, len);
    return ops_true;
    }

// Writes out the header for the encrypted packet. Invoked by the
// partial stream writer. Note that writing the packet tag and the
// packet length is handled by the partial stream writer.
static ops_boolean_t write_encrypt_se_ip_header(ops_create_info_t *cinfo,
						void *data)
    {
    stream_encrypt_se_ip_arg_t *arg = data;
    size_t sz_preamble = arg->crypt->blocksize + 2;
    unsigned char* preamble = ops_mallocz(sz_preamble);

    ops_write_scalar(SE_IP_DATA_VERSION, 1, cinfo);

    ops_writer_push_encrypt_crypt(cinfo, arg->crypt);

    ops_random(preamble, arg->crypt->blocksize);
    preamble[arg->crypt->blocksize]=preamble[arg->crypt->blocksize-2];
    preamble[arg->crypt->blocksize+1]=preamble[arg->crypt->blocksize-1];

    ops_write(preamble, sz_preamble, cinfo);
    arg->hash.add(&arg->hash, preamble, sz_preamble);
    ops_writer_pop(cinfo);
    free(preamble);

    return ops_true;
    }

static ops_boolean_t
ops_stream_write_se_ip_last(stream_encrypt_se_ip_arg_t *arg,
			    ops_create_info_t *cinfo)
    {
    unsigned char c[1];
    unsigned char hashed[SHA_DIGEST_LENGTH];
    const size_t sz_mdc = 1 + 1 + SHA_DIGEST_LENGTH;

    ops_memory_t *mem_mdc;
    ops_create_info_t *cinfo_mdc;

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

    // encode everthing
    ops_writer_push_encrypt_crypt(cinfo, arg->crypt);

    ops_write(ops_memory_get_data(mem_mdc), ops_memory_get_length(mem_mdc),
	      cinfo);

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

    ops_stream_write_se_ip(src, length,
                           arg, arg->cinfo_se_ip);
    // now write memory to next writer
    rtn=ops_stacked_write(ops_memory_get_data(arg->mem_se_ip),
                          ops_memory_get_length(arg->mem_se_ip),
                          errors, winfo);
    
    ops_memory_clear(arg->mem_se_ip);

    return rtn;
    }

static ops_boolean_t stream_encrypt_se_ip_finaliser(ops_error_t **errors,
                                                    ops_writer_info_t *winfo)
    {
    stream_encrypt_se_ip_arg_t *arg=ops_writer_get_arg(winfo);
    // write trailer
    ops_stream_write_se_ip_last(arg, arg->cinfo_se_ip);
    // now write memory to next writer
    return ops_stacked_write(ops_memory_get_data(arg->mem_se_ip),
                             ops_memory_get_length(arg->mem_se_ip),
                             errors, winfo);
    }

static void stream_encrypt_se_ip_destroyer(ops_writer_info_t *winfo)
     
    {
    stream_encrypt_se_ip_arg_t *arg=ops_writer_get_arg(winfo);

    ops_teardown_memory_write(arg->cinfo_se_ip, arg->mem_se_ip);
    arg->crypt->decrypt_finish(arg->crypt);

    free(arg->crypt);
    free(arg);
    }

// EOF

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

#include <assert.h>
#include <string.h>
#include <openpgpsdk/readerwriter.h>

static int debug=0;

typedef struct 
    {
    ops_crypt_t* crypt;
    int free_crypt;
    } crypt_arg_t;

/*
 * This writer simply takes plaintext as input, 
 * encrypts it with the given key
 * and outputs the resulting encrypted text
 */
static ops_boolean_t encrypt_writer(const unsigned char *src,
				      unsigned length,
				      ops_error_t **errors,
				      ops_writer_info_t *winfo)
    {

#define BUFSZ 1024 // arbitrary number
    unsigned char encbuf[BUFSZ];
    unsigned remaining=length;
    unsigned done=0; 

    crypt_arg_t *arg=(crypt_arg_t *)ops_writer_get_arg(winfo);

    if (!ops_is_sa_supported(arg->crypt->algorithm))
        assert(0); // \todo proper error handling

    while (remaining)
        {
        unsigned len = remaining < BUFSZ ? remaining : BUFSZ;
        //        memcpy(buf,src,len); // \todo copy needed here?
        
        arg->crypt->cfb_encrypt(arg->crypt, encbuf, src+done, len);

        if (debug)
            {
            int i=0;
            fprintf(stderr,"WRITING:\nunencrypted: ");
            for (i=0; i<16; i++)
                fprintf(stderr,"%2x ", src[done+i]);
            fprintf(stderr,"\n");
            fprintf(stderr,"encrypted:   ");
            for (i=0; i<16; i++)
                fprintf(stderr,"%2x ", encbuf[i]);
            fprintf(stderr,"\n");
            }

        if (!ops_stacked_write(encbuf,len,errors,winfo))
            {
            if (debug)
                { fprintf(stderr, "encrypted_writer got error from stacked write, returning\n"); }
            return ops_false;
            }
        remaining-=len;
        done+=len;
        }

    return ops_true;
    }

static void encrypt_destroyer (ops_writer_info_t *winfo)
     
    {
    crypt_arg_t *arg=(crypt_arg_t *)ops_writer_get_arg(winfo);
    if (arg->free_crypt)
        free(arg->crypt);
    free (arg);
    }

/**
\ingroup Core_WritersNext
\brief Push Encrypted Writer onto stack (create SE packets)
*/
void ops_writer_push_encrypt_crypt(ops_create_info_t *cinfo,
                                   ops_crypt_t *crypt)
    {
    // Create arg to be used with this writer
    // Remember to free this in the destroyer

    crypt_arg_t *arg=ops_mallocz(sizeof *arg);

    // Setup the arg

    arg->crypt=crypt;
    arg->free_crypt=0;

    // And push writer on stack
    ops_writer_push(cinfo,encrypt_writer,NULL,encrypt_destroyer,arg);

    }

// EOF

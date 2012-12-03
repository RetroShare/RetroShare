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

#include <openpgpsdk/compress.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/literal.h>
#include <openpgpsdk/random.h>
#include <openpgpsdk/readerwriter.h>
#include <openpgpsdk/streamwriter.h>
#include <openpgpsdk/writer_armoured.h>
#include "parse_local.h"

#include <assert.h>
#include <string.h>
#include <fcntl.h>

#include <openpgpsdk/final.h>
#include <util/opsdir.h>

/**
\ingroup Core_MPI
\brief Decrypt and unencode MPI
\param buf Buffer in which to write decrypted unencoded MPI
\param buflen Length of buffer
\param encmpi
\param skey
\return length of MPI
\note only RSA at present
*/
int ops_decrypt_and_unencode_mpi(unsigned char *buf, unsigned buflen,
				 const BIGNUM *encmpi,
				 const ops_secret_key_t *skey)
    {
    unsigned char encmpibuf[8192];
    unsigned char mpibuf[8192];
    unsigned mpisize;
    int n;
    int i;

    mpisize=BN_num_bytes(encmpi);
    /* MPI can't be more than 65,536 */
    assert(mpisize <= sizeof encmpibuf);
    BN_bn2bin(encmpi, encmpibuf);

    assert(skey->public_key.algorithm == OPS_PKA_RSA);

    /*
    fprintf(stderr,"\nDECRYPTING\n");
    fprintf(stderr,"encrypted data     : ");
    for (i=0; i<16; i++)
        fprintf(stderr,"%2x ", encmpibuf[i]);
    fprintf(stderr,"\n");
    */

    n=ops_rsa_private_decrypt(mpibuf, encmpibuf, (BN_num_bits(encmpi)+7)/8,
			      &skey->key.rsa, &skey->public_key.key.rsa);
    assert(n != -1);

    /*
    fprintf(stderr,"decrypted encoded m buf     : ");
    for (i=0; i<16; i++)
        fprintf(stderr,"%2x ", mpibuf[i]);
    fprintf(stderr,"\n");
    */

    if(n <= 0)
	return -1;

    /*
    printf(" decrypted=%d ",n);
    hexdump(mpibuf,n);
    printf("\n");
    */

    // Decode EME-PKCS1_V1_5 (RFC 2437).

    if(mpibuf[0] != 0 || mpibuf[1] != 2)
        return ops_false;

    // Skip the random bytes.
    for(i=2 ; i < n && mpibuf[i] ; ++i)
        ;

    if(i == n || i < 10)
        return ops_false;

    // Skip the zero
    ++i;

    // this is the unencoded m buf
    if((unsigned)(n-i) <= buflen)
        memcpy(buf, mpibuf+i, n-i);

    /*
    printf("decoded m buf:\n");
    int j;
    for (j=0; j<n-i; j++)
        printf("%2x ",buf[j]);
    printf("\n");
    */

    return n-i;
    }

/**
\ingroup Core_MPI
\brief RSA-encrypt an MPI
*/
ops_boolean_t ops_rsa_encrypt_mpi(const unsigned char *encoded_m_buf,
				  const size_t sz_encoded_m_buf,
				  const ops_public_key_t *pkey,
				  ops_pk_session_key_parameters_t *skp)
    {
    assert(sz_encoded_m_buf==(size_t) BN_num_bytes(pkey->key.rsa.n));

    unsigned char encmpibuf[8192];
    int n=0;

    n=ops_rsa_public_encrypt(encmpibuf, encoded_m_buf, sz_encoded_m_buf,
			     &pkey->key.rsa);
    assert(n!=-1);

    if(n <= 0)
	return ops_false;

    skp->rsa.encrypted_m=BN_bin2bn(encmpibuf, n, NULL);

    /*
    fprintf(stderr,"encrypted mpi buf     : ");
    int i;
    for (i=0; i<16; i++)
        fprintf(stderr,"%2x ", encmpibuf[i]);
    fprintf(stderr,"\n");
    */

    return ops_true;
    }

#define MAXBUF 1024

static ops_parse_cb_return_t
callback_write_parsed(const ops_parser_content_t *content_,
		      ops_parse_cb_info_t *cbinfo);

/**
\ingroup HighLevel_Crypto
Encrypt a file
\param input_filename Name of file to be encrypted
\param output_filename Name of file to write to. If NULL, name is constructed from input_filename
\param pub_key Public Key to encrypt file for
\param use_armour Write armoured text, if set
\param allow_overwrite Allow output file to be overwrwritten if it exists
\return ops_true if OK; else ops_false
*/
ops_boolean_t ops_encrypt_file(const char* input_filename,
			       const char* output_filename,
			       const ops_keydata_t *pub_key,
			       const ops_boolean_t use_armour,
			       const ops_boolean_t allow_overwrite)
    {
    int fd_in=0;
    int fd_out=0;

    ops_create_info_t *cinfo;
#ifdef WINDOWS_SYS
    fd_in=ops_open(input_filename, O_RDONLY | O_BINARY);
#else
    fd_in=ops_open(input_filename, O_RDONLY, 0);
#endif
    if(fd_in < 0)
        {
        perror(input_filename);
        return ops_false;
        }
    
    fd_out=ops_setup_file_write(&cinfo, output_filename, allow_overwrite);
    if (fd_out < 0)
        return ops_false;

    // set armoured/not armoured here
    if (use_armour)
        ops_writer_push_armoured_message(cinfo);

    // Push the encrypted writer
    ops_writer_push_stream_encrypt_se_ip(cinfo, pub_key);
    ops_writer_push_literal(cinfo);

    // Do the writing

    unsigned buffer[10240];
    for (;;)
        {
	int n=0;

	n=read(fd_in, buffer, sizeof buffer);
	if (!n)
	    break;
	assert(n >= 0);

	// FIXME: apparently writing can't fail.
	ops_write(buffer, n, cinfo);
        }


    // tidy up
    close(fd_in);
    ops_teardown_file_write(cinfo, fd_out);

    return ops_true;
    }

/**
   \ingroup HighLevel_Crypto
   Encrypt a compressed, signed stream.
   \param cinfo the structure describing where the output will be written.
   \param public_key the key used to encrypt the data
   \param secret_key the key used to sign the data. If NULL, the data
          will not be signed
   \param compress If true, compress the stream before encrypting
   \param use_armour Write armoured text, if set
   \see ops_setup_file_write

   Example Code:
   \code
    const char* filename = "armour_nocompress_sign.asc";
    ops_create_info_t *info;
    int fd = ops_setup_file_write(&info, filename, ops_true);
    if (fd < 0) {
      fprintf(stderr, "Cannot write to %s\n", filename);
      return -1;
    }
    ops_encrypt_stream(info, public_key, secret_key, ops_false, ops_true);
    ops_write(cleartext, strlen(cleartext), info);
    ops_writer_close(info);
    ops_create_info_delete(info);
   \endcode
*/
extern void ops_encrypt_stream(ops_create_info_t* cinfo,
                               const ops_keydata_t* public_key,
                               const ops_secret_key_t* secret_key,
                               const ops_boolean_t compress,
                               const ops_boolean_t use_armour)
    {
    if (use_armour)
	ops_writer_push_armoured_message(cinfo);
    ops_writer_push_stream_encrypt_se_ip(cinfo, public_key);
    if (compress)
	ops_writer_push_compressed(cinfo);
    if (secret_key != NULL)
	ops_writer_push_signed(cinfo, OPS_SIG_BINARY, secret_key);
    else
	ops_writer_push_literal(cinfo);
    }

/**
   \ingroup HighLevel_Crypto
   \brief Decrypt a chunk of memory, containing a encrypted stream.
   \param input_filename Name of file to be decrypted
   \param output_filename Name of file to write to. If NULL, the filename is constructed from the input filename, following GPG conventions.
   \param keyring Keyring to use
   \param use_armour Expect armoured text, if set
   \param allow_overwrite Allow output file to overwritten, if set.
   \param cb_get_passphrase Callback to use to get passphrase
*/

ops_boolean_t ops_decrypt_memory(const unsigned char *encrypted_memory,int em_length,
			       					 unsigned char **decrypted_memory,int *out_length,
										 ops_keyring_t* keyring,
										 const ops_boolean_t use_armour,
										 ops_parse_cb_t* cb_get_passphrase)
{
    int fd_in=0;
    int fd_out=0;
    char* myfilename=NULL;

    //
    ops_parse_info_t *pinfo=NULL;

    // setup for reading from given input file

	 ops_memory_t *input_mem = ops_memory_new() ;
	 ops_memory_add(input_mem,encrypted_memory,em_length) ;

	 ops_setup_memory_read(&pinfo, input_mem, NULL, callback_write_parsed, ops_false);

    if (pinfo == NULL)
	 {
		 perror("cannot create memory read");
		 return ops_false;
	 }

    // setup memory chunk 

     ops_memory_t *output_mem;

	 ops_setup_memory_write(&pinfo->cbinfo.cinfo, &output_mem,0) ;

	 if (output_mem == NULL)
	 { 
		 perror("Cannot create output memory"); 
		 ops_teardown_memory_read(pinfo, input_mem);
		 return ops_false;
	 }

    // \todo check for suffix matching armour param

    // setup keyring and passphrase callback
    pinfo->cbinfo.cryptinfo.keyring=keyring;
    pinfo->cbinfo.cryptinfo.cb_get_passphrase=cb_get_passphrase;

    // Set up armour/passphrase options

    if (use_armour)
        ops_reader_push_dearmour(pinfo);
    
    // Do it

    ops_parse_and_print_errors(pinfo);

    // Unsetup

    if (use_armour)
        ops_reader_pop_dearmour(pinfo);

	 ops_boolean_t res = ops_true ;

	 // copy output memory to supplied buffer.
	 //
	 *out_length = ops_memory_get_length(output_mem) ;
 	 *decrypted_memory = ops_mallocz(*out_length) ;
	 memcpy(*decrypted_memory,ops_memory_get_data(output_mem),*out_length) ;

ops_decrypt_memory_ABORT: 
    ops_teardown_memory_write(pinfo->cbinfo.cinfo, output_mem);
    ops_teardown_memory_read(pinfo, input_mem);

    return res ;
}

/**
   \ingroup HighLevel_Crypto
   \brief Decrypt a file.
   \param input_filename Name of file to be decrypted
   \param output_filename Name of file to write to. If NULL, the filename is constructed from the input filename, following GPG conventions.
   \param keyring Keyring to use
   \param use_armour Expect armoured text, if set
   \param allow_overwrite Allow output file to overwritten, if set.
   \param cb_get_passphrase Callback to use to get passphrase
*/

ops_boolean_t ops_decrypt_file(const char* input_filename,
			       const char* output_filename,
			       ops_keyring_t* keyring,
			       const ops_boolean_t use_armour,
			       const ops_boolean_t allow_overwrite,
			       ops_parse_cb_t* cb_get_passphrase)
{
    int fd_in=0;
    int fd_out=0;
    char* myfilename=NULL;

    //
    ops_parse_info_t *pinfo=NULL;

    // setup for reading from given input file
    fd_in=ops_setup_file_read(&pinfo, input_filename, 
			      NULL,
			      callback_write_parsed,
			      ops_false);
    if (fd_in < 0)
        {
        perror(input_filename);
        return ops_false;
        }

    // setup output filename

    if (output_filename)
        {
        fd_out=ops_setup_file_write(&pinfo->cbinfo.cinfo, output_filename,
				    allow_overwrite);

        if (fd_out < 0)
            { 
            perror(output_filename); 
            ops_teardown_file_read(pinfo, fd_in);
            return ops_false;
            }
        }
    else
        {
        int suffixlen=4;
        char *defaultsuffix=".decrypted";
        const char *suffix=input_filename+strlen(input_filename)-suffixlen;
        if (!strcmp(suffix, ".gpg") || !strcmp(suffix, ".asc"))
            {
            myfilename=ops_mallocz(strlen(input_filename)-suffixlen+1);
            strncpy(myfilename, input_filename,
		    strlen(input_filename)-suffixlen);
            }
        else
            {
            unsigned filenamelen=strlen(input_filename)+strlen(defaultsuffix)+1;
            myfilename=ops_mallocz(filenamelen);
            snprintf(myfilename, filenamelen, "%s%s", input_filename,
		     defaultsuffix);
            }

        fd_out=ops_setup_file_write(&pinfo->cbinfo.cinfo, myfilename,
				    allow_overwrite);
        
        if (fd_out < 0)
            { 
            perror(myfilename); 
            free(myfilename);
            ops_teardown_file_read(pinfo, fd_in);
            return ops_false;
            }

        free (myfilename);
        }

    // \todo check for suffix matching armour param

    // setup for writing decrypted contents to given output file

    // setup keyring and passphrase callback
    pinfo->cbinfo.cryptinfo.keyring=keyring;
    pinfo->cbinfo.cryptinfo.cb_get_passphrase=cb_get_passphrase;

    // Set up armour/passphrase options

    if (use_armour)
        ops_reader_push_dearmour(pinfo);
    
    // Do it

    ops_parse_and_print_errors(pinfo);

    // Unsetup

    if (use_armour)
        ops_reader_pop_dearmour(pinfo);

    ops_teardown_file_write(pinfo->cbinfo.cinfo, fd_out);
    ops_teardown_file_read(pinfo, fd_in);
    // \todo cleardown crypt

    return ops_true;
}
static ops_parse_cb_return_t
callback_write_parsed(const ops_parser_content_t *content_,
		      ops_parse_cb_info_t *cbinfo)
    {
    ops_parser_content_union_t* content
	=(ops_parser_content_union_t *)&content_->content;
    static ops_boolean_t skipping;
    //    ops_boolean_t write=ops_true;

    OPS_USED(cbinfo);

//    ops_print_packet(content_);

    if(content_->tag != OPS_PTAG_CT_UNARMOURED_TEXT && skipping)
	{
	puts("...end of skip");
	skipping=ops_false;
	}

    switch(content_->tag)
	{
    case OPS_PTAG_CT_UNARMOURED_TEXT:
	printf("OPS_PTAG_CT_UNARMOURED_TEXT\n");
	if(!skipping)
	    {
	    puts("Skipping...");
	    skipping=ops_true;
	    }
	fwrite(content->unarmoured_text.data, 1,
	       content->unarmoured_text.length, stdout);
	break;

    case OPS_PTAG_CT_PK_SESSION_KEY:
        return callback_pk_session_key(content_, cbinfo);
        break;

    case OPS_PARSER_CMD_GET_SECRET_KEY:
        return callback_cmd_get_secret_key(content_, cbinfo);
        break;

    case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
    case OPS_PARSER_CMD_GET_SK_PASSPHRASE_PREV_WAS_BAD:
        //        return callback_cmd_get_secret_key_passphrase(content_,cbinfo);
        return cbinfo->cryptinfo.cb_get_passphrase(content_, cbinfo);
        break;

    case OPS_PTAG_CT_LITERAL_DATA_BODY:
        return callback_literal_data(content_, cbinfo);
	break;

    case OPS_PTAG_CT_ARMOUR_HEADER:
    case OPS_PTAG_CT_ARMOUR_TRAILER:
    case OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY:
    case OPS_PTAG_CT_COMPRESSED:
    case OPS_PTAG_CT_LITERAL_DATA_HEADER:
    case OPS_PTAG_CT_SE_IP_DATA_BODY:
    case OPS_PTAG_CT_SE_IP_DATA_HEADER:
    case OPS_PTAG_CT_SE_DATA_BODY:
    case OPS_PTAG_CT_SE_DATA_HEADER:

	// Ignore these packets 
	// They're handled in ops_parse_one_packet()
	// and nothing else needs to be done
	break;

    default:
        //        return callback_general(content_,cbinfo);
        break;
        //	fprintf(stderr,"Unexpected packet tag=%d (0x%x)\n",content_->tag,
        //		content_->tag);
        //	assert(0);
	}

    return OPS_RELEASE_MEMORY;
    }

// EOF

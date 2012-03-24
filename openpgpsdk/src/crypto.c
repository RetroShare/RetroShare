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
#include <openpgpsdk/random.h>
#include <openpgpsdk/readerwriter.h>
#include <openpgpsdk/writer_armoured.h>
#include "parse_local.h"

#include <assert.h>
#include <string.h>
#include <fcntl.h>

#include <openpgpsdk/final.h>

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
int ops_decrypt_and_unencode_mpi(unsigned char *buf,unsigned buflen,const BIGNUM *encmpi,
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
    BN_bn2bin(encmpi,encmpibuf);

    assert(skey->public_key.algorithm == OPS_PKA_RSA);

    /*
    fprintf(stderr,"\nDECRYPTING\n");
    fprintf(stderr,"encrypted data     : ");
    for (i=0; i<16; i++)
        fprintf(stderr,"%2x ", encmpibuf[i]);
    fprintf(stderr,"\n");
    */

    n=ops_rsa_private_decrypt(mpibuf,encmpibuf,(BN_num_bits(encmpi)+7)/8,
			      &skey->key.rsa,&skey->public_key.key.rsa);
    assert(n!=-1);

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
        memcpy(buf,mpibuf+i,n-i);

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

    n=ops_rsa_public_encrypt(encmpibuf, encoded_m_buf, sz_encoded_m_buf, &pkey->key.rsa);
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
callback_write_parsed(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);

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
ops_boolean_t ops_encrypt_file(const char* input_filename, const char* output_filename, const ops_keydata_t *pub_key, const ops_boolean_t use_armour, const ops_boolean_t allow_overwrite)
    {
    int fd_in=0;
    int fd_out=0;

    ops_create_info_t *cinfo;

#ifdef WIN32
    fd_in=open(input_filename,O_RDONLY | O_BINARY);
#else
    fd_in=open(input_filename,O_RDONLY);
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
    ops_writer_push_encrypt_se_ip(cinfo,pub_key);

    // Do the writing

    unsigned char* buf=NULL;
    size_t bufsz=16;
    int done=0;
    for (;;)
        {
        buf=realloc(buf,done+bufsz);
        
	    int n=0;

	    n=read(fd_in,buf+done,bufsz);
	    if (!n)
		    break;
	    assert(n>=0);
        done+=n;
        }

    // This does the writing
    ops_write(buf,done,cinfo);

    // tidy up
    close(fd_in);
    free(buf);
    ops_teardown_file_write(cinfo,fd_out);

    return ops_true;
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

ops_boolean_t ops_decrypt_file(const char* input_filename, const char* output_filename, ops_keyring_t* keyring, const ops_boolean_t use_armour, const ops_boolean_t allow_overwrite, ops_parse_cb_t* cb_get_passphrase)
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
        fd_out=ops_setup_file_write(&pinfo->cbinfo.cinfo, output_filename, allow_overwrite);

        if (fd_out < 0)
            { 
            perror(output_filename); 
            ops_teardown_file_read(pinfo,fd_in);
            return ops_false;
            }
        }
    else
        {
        int suffixlen=4;
        char *defaultsuffix=".decrypted";
        const char *suffix=input_filename+strlen((char *)input_filename)-suffixlen;
        if (!strcmp(suffix,".gpg") || !strcmp(suffix,".asc"))
            {
            myfilename=ops_mallocz(strlen(input_filename)-suffixlen+1);
            strncpy(myfilename,input_filename,strlen(input_filename)-suffixlen);
            }
        else
            {
            unsigned filenamelen=strlen(input_filename)+strlen(defaultsuffix)+1;
            myfilename=ops_mallocz(filenamelen);
            snprintf(myfilename,filenamelen,"%s%s",input_filename,defaultsuffix);
            }

        fd_out=ops_setup_file_write(&pinfo->cbinfo.cinfo, myfilename, allow_overwrite);
        
        if (fd_out < 0)
            { 
            perror(myfilename); 
            free(myfilename);
            ops_teardown_file_read(pinfo,fd_in);
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
callback_write_parsed(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)
    {
    ops_parser_content_union_t* content=(ops_parser_content_union_t *)&content_->content;
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
	fwrite(content->unarmoured_text.data,1,
	       content->unarmoured_text.length,stdout);
	break;

    case OPS_PTAG_CT_PK_SESSION_KEY:
        return callback_pk_session_key(content_,cbinfo);
        break;

    case OPS_PARSER_CMD_GET_SECRET_KEY:
        return callback_cmd_get_secret_key(content_,cbinfo);
        break;

    case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
        //        return callback_cmd_get_secret_key_passphrase(content_,cbinfo);
        return cbinfo->cryptinfo.cb_get_passphrase(content_,cbinfo);
        break;

    case OPS_PTAG_CT_LITERAL_DATA_BODY:
        return callback_literal_data(content_,cbinfo);
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

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

#include <openpgpsdk/packet-parse.h>
#include <openpgpsdk/packet-show.h>
#include <openpgpsdk/keyring.h>
#include "keyring_local.h"
#include "parse_local.h"
#include <openpgpsdk/util.h>
#include <openpgpsdk/armour.h>
#include <openpgpsdk/signature.h>
#include <openpgpsdk/memory.h>
#include <openpgpsdk/validate.h>
#include <openpgpsdk/readerwriter.h>
#include <assert.h>
#include <string.h>

#include <openpgpsdk/final.h>

static int debug=0;

static ops_boolean_t check_binary_signature(const unsigned len,
                                            const unsigned char *data,
                                            const ops_signature_t *sig, 
                                            const ops_public_key_t *signer __attribute__((unused)))
    {
    // Does the signed hash match the given hash?

    int n=0;
    ops_hash_t hash;
    unsigned char hashout[OPS_MAX_HASH_SIZE];
    unsigned char trailer[6];
    unsigned int hashedlen;

    //common_init_signature(&hash,sig);
    ops_hash_any(&hash,sig->info.hash_algorithm);
    hash.init(&hash);
    hash.add(&hash,data,len);
    switch (sig->info.version)
        {
    case OPS_V3:
        trailer[0]=sig->info.type;
        trailer[1]=sig->info.creation_time >> 24;
        trailer[2]=sig->info.creation_time >> 16;
        trailer[3]=sig->info.creation_time >> 8;
        trailer[4]=sig->info.creation_time;
        hash.add(&hash,&trailer[0],5);
        break;

    case OPS_V4:
        hash.add(&hash,sig->info.v4_hashed_data,sig->info.v4_hashed_data_length);

        trailer[0]=0x04; // version
        trailer[1]=0xFF;
        hashedlen=sig->info.v4_hashed_data_length;
        trailer[2]=hashedlen >> 24;
        trailer[3]=hashedlen >> 16;
        trailer[4]=hashedlen >> 8;
        trailer[5]=hashedlen;
        hash.add(&hash,&trailer[0],6);
        
        break;

    default:
        fprintf(stderr,"Invalid signature version %d\n", sig->info.version);
        return ops_false;
        }

    n=hash.finish(&hash,hashout);

    //    return ops_false;
    return ops_check_signature(hashout,n,sig,signer);
    }

static int keydata_reader(void *dest,size_t length,ops_error_t **errors,
			   ops_reader_info_t *rinfo,
			   ops_parse_cb_info_t *cbinfo)
    {
    validate_reader_arg_t *arg=ops_reader_get_arg(rinfo);

    OPS_USED(errors);
    OPS_USED(cbinfo);
    if(arg->offset == arg->key->packets[arg->packet].length)
	{
	++arg->packet;
	arg->offset=0;
	}

    if(arg->packet == arg->key->npackets)
	return 0;

    // we should never be asked to cross a packet boundary in a single read
    assert(arg->key->packets[arg->packet].length >= arg->offset+length);

    memcpy(dest,&arg->key->packets[arg->packet].raw[arg->offset],length);
    arg->offset+=length;

    return length;
    }

static void free_signature_info(ops_signature_info_t *sig)
    {
    free (sig->v4_hashed_data);
    free (sig);
    }

static void copy_signature_info(ops_signature_info_t* dst, const ops_signature_info_t* src)
    {
    memcpy(dst,src,sizeof *src);
    dst->v4_hashed_data=ops_mallocz(src->v4_hashed_data_length);
    memcpy(dst->v4_hashed_data,src->v4_hashed_data,src->v4_hashed_data_length);
    }

static void add_sig_to_valid_list(ops_validate_result_t * result, const ops_signature_info_t* sig)
    {
    size_t newsize;
    size_t start;

    // increment count
    ++result->valid_count;

    // increase size of array
    newsize=(sizeof *sig) * result->valid_count;
    if (!result->valid_sigs)
        result->valid_sigs=malloc(newsize);
    else
        result->valid_sigs=realloc(result->valid_sigs, newsize);

    // copy key ptr to array
    start=(sizeof *sig) * (result->valid_count-1);
    copy_signature_info(result->valid_sigs+start,sig);
    }

static void add_sig_to_invalid_list(ops_validate_result_t * result, const ops_signature_info_t *sig)
    {
    size_t newsize;
    size_t start;

    // increment count
    ++result->invalid_count;

    // increase size of array
    newsize=(sizeof *sig) * result->invalid_count;
    if (!result->invalid_sigs)
        result->invalid_sigs=malloc(newsize);
    else
        result->invalid_sigs=realloc(result->invalid_sigs, newsize);

    // copy key ptr to array
    start=(sizeof *sig) * (result->invalid_count-1);
    copy_signature_info(result->invalid_sigs+start, sig);
    }

static void add_sig_to_unknown_list(ops_validate_result_t * result, const ops_signature_info_t *sig)
    {
    size_t newsize;
    size_t start;

    // increment count
    ++result->unknown_signer_count;

    // increase size of array
    newsize=(sizeof *sig) * result->unknown_signer_count;
    if (!result->unknown_sigs)
        result->unknown_sigs=malloc(newsize);
    else
        result->unknown_sigs=realloc(result->unknown_sigs, newsize);

    // copy key id to array
    start=OPS_KEY_ID_SIZE * (result->unknown_signer_count-1);
    copy_signature_info(result->unknown_sigs+start, sig);
    }

ops_parse_cb_return_t
ops_validate_key_cb(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)
    {
    const ops_parser_content_union_t *content=&content_->content;
    validate_key_cb_arg_t *arg=ops_parse_cb_get_arg(cbinfo);
    ops_error_t **errors=ops_parse_cb_get_errors(cbinfo);
    const ops_keydata_t *signer;
    ops_boolean_t valid=ops_false;

    if (debug)
        printf("%s\n",ops_show_packet_tag(content_->tag));

    switch(content_->tag)
        {
    case OPS_PTAG_CT_PUBLIC_KEY:
        assert(arg->pkey.version == 0);
        arg->pkey=content->public_key;
        return OPS_KEEP_MEMORY;
        
    case OPS_PTAG_CT_PUBLIC_SUBKEY:
        if(arg->subkey.version)
            ops_public_key_free(&arg->subkey);
        arg->subkey=content->public_key;
        return OPS_KEEP_MEMORY;
        
    case OPS_PTAG_CT_SECRET_KEY:
        arg->skey=content->secret_key;
        arg->pkey=arg->skey.public_key;
        return OPS_KEEP_MEMORY;

    case OPS_PTAG_CT_USER_ID:
	if(arg->user_id.user_id)
	    ops_user_id_free(&arg->user_id);
	arg->user_id=content->user_id;
	arg->last_seen=ID;
	return OPS_KEEP_MEMORY;

    case OPS_PTAG_CT_USER_ATTRIBUTE:
	assert(content->user_attribute.data.len);
	printf("user attribute, length=%d\n",(int)content->user_attribute.data.len);
	if(arg->user_attribute.data.len)
	    ops_user_attribute_free(&arg->user_attribute);
	arg->user_attribute=content->user_attribute;
	arg->last_seen=ATTRIBUTE;
	return OPS_KEEP_MEMORY;

    case OPS_PTAG_CT_SIGNATURE: // V3 sigs
    case OPS_PTAG_CT_SIGNATURE_FOOTER: // V4 sigs
        /*
        printf("  type=%02x signer_id=",content->signature.type);
        hexdump(content->signature.signer_id,
		sizeof content->signature.signer_id);
        */

	signer=ops_keyring_find_key_by_id(arg->keyring,
					   content->signature.info.signer_id);
	if(!signer)
	    {
        add_sig_to_unknown_list(arg->result, &content->signature.info);
	    break;
	    }

	switch(content->signature.info.type)
	    {
	case OPS_CERT_GENERIC:
	case OPS_CERT_PERSONA:
	case OPS_CERT_CASUAL:
	case OPS_CERT_POSITIVE:
	case OPS_SIG_REV_CERT:
	    if(arg->last_seen == ID)
		valid=ops_check_user_id_certification_signature(&arg->pkey,
								&arg->user_id,
								&content->signature,
								ops_get_public_key_from_data(signer),
								arg->rarg->key->packets[arg->rarg->packet].raw);
	    else
		valid=ops_check_user_attribute_certification_signature(&arg->pkey,
								       &arg->user_attribute,
								       &content->signature,
								       ops_get_public_key_from_data(signer),
								       arg->rarg->key->packets[arg->rarg->packet].raw);
		
	    break;

	case OPS_SIG_SUBKEY:
	    // XXX: we should also check that the signer is the key we are validating, I think.
	    valid=ops_check_subkey_signature(&arg->pkey,&arg->subkey,
	     	    &content->signature,
		    ops_get_public_key_from_data(signer),
		    arg->rarg->key->packets[arg->rarg->packet].raw);
	    break;

	case OPS_SIG_DIRECT:
	    valid=ops_check_direct_signature(&arg->pkey,&content->signature,
		    ops_get_public_key_from_data(signer),
		    arg->rarg->key->packets[arg->rarg->packet].raw);
	    break;

    case OPS_SIG_STANDALONE:
    case OPS_SIG_PRIMARY:
    case OPS_SIG_REV_KEY:
    case OPS_SIG_REV_SUBKEY:
    case OPS_SIG_TIMESTAMP:
    case OPS_SIG_3RD_PARTY:
        OPS_ERROR_1(errors, OPS_E_UNIMPLEMENTED,
                    "Verification of signature type 0x%02x not yet implemented\n", content->signature.info.type);
                    break;

	default:
            OPS_ERROR_1(errors, OPS_E_UNIMPLEMENTED,
                    "Unexpected signature type 0x%02x\n", content->signature.info.type);
	    }

	if(valid)
	    {
        //	    printf(" validated\n");
	    //++arg->result->valid_count;
        add_sig_to_valid_list(arg->result, &content->signature.info);
	    }
	else
	    {
        OPS_ERROR(errors,OPS_E_V_BAD_SIGNATURE,"Bad Signature");
        //	    printf(" BAD SIGNATURE\n");
        //	    ++arg->result->invalid_count;
        add_sig_to_invalid_list(arg->result, &content->signature.info);
	    }
	break;

	// ignore these
    case OPS_PARSER_PTAG:
    case OPS_PTAG_CT_SIGNATURE_HEADER:
    case OPS_PARSER_PACKET_END:
	break;

 case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
     if (arg->cb_get_passphrase)
         {
         return arg->cb_get_passphrase(content_,cbinfo);
         }
     break;

    default:
	fprintf(stderr,"unexpected tag=0x%x\n",content_->tag);
	assert(0);
	break;
	}
    return OPS_RELEASE_MEMORY;
    }

ops_parse_cb_return_t
validate_data_cb(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)
    {
    const ops_parser_content_union_t *content=&content_->content;
    validate_data_cb_arg_t *arg=ops_parse_cb_get_arg(cbinfo);
    ops_error_t **errors=ops_parse_cb_get_errors(cbinfo);
    const ops_keydata_t *signer;
    ops_boolean_t valid=ops_false;
    ops_memory_t* mem=NULL;

    if (debug)
        printf("%s\n",ops_show_packet_tag(content_->tag));

    switch(content_->tag)
	{
    case OPS_PTAG_CT_SIGNED_CLEARTEXT_HEADER:
        // ignore - this gives us the "Armor Header" line "Hash: SHA1" or similar
        break;

    case OPS_PTAG_CT_LITERAL_DATA_HEADER:
        // ignore
        break;

    case OPS_PTAG_CT_LITERAL_DATA_BODY:
        arg->data.literal_data_body=content->literal_data_body;
        arg->use=LITERAL_DATA;
        return OPS_KEEP_MEMORY;
        break;

    case OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY:
        arg->data.signed_cleartext_body=content->signed_cleartext_body;
        arg->use=SIGNED_CLEARTEXT;
        return OPS_KEEP_MEMORY;
        break;

    case OPS_PTAG_CT_SIGNED_CLEARTEXT_TRAILER:
        // this gives us an ops_hash_t struct
        break;

    case OPS_PTAG_CT_SIGNATURE: // V3 sigs
    case OPS_PTAG_CT_SIGNATURE_FOOTER: // V4 sigs
        
        if (debug)
            {
            printf("\n*** hashed data:\n");
            unsigned int zzz=0;
            for (zzz=0; zzz<content->signature.info.v4_hashed_data_length; zzz++)
                printf("0x%02x ", content->signature.info.v4_hashed_data[zzz]);
            printf("\n");
            printf("  type=%02x signer_id=",content->signature.info.type);
            hexdump(content->signature.info.signer_id,
                    sizeof content->signature.info.signer_id);
            }

        signer=ops_keyring_find_key_by_id(arg->keyring,
                                          content->signature.info.signer_id);
        if(!signer)
            {
            OPS_ERROR(errors,OPS_E_V_UNKNOWN_SIGNER,"Unknown Signer");
            add_sig_to_unknown_list(arg->result, &content->signature.info);
            break;
            }
        
        mem=ops_memory_new();
        ops_memory_init(mem,128);
        
        switch(content->signature.info.type)
            {
        case OPS_SIG_BINARY:
        case OPS_SIG_TEXT:
            switch(arg->use)
                {
            case LITERAL_DATA:
                ops_memory_add(mem,
                               arg->data.literal_data_body.data,
                               arg->data.literal_data_body.length);
                break;
                
            case SIGNED_CLEARTEXT:
                ops_memory_add(mem,
                               arg->data.signed_cleartext_body.data,
                               arg->data.signed_cleartext_body.length);
                break;
                
            default:
                OPS_ERROR_1(errors,OPS_E_UNIMPLEMENTED,"Unimplemented Sig Use %d", arg->use);
                printf(" Unimplemented Sig Use %d\n", arg->use);
                break;
                }
            
            valid=check_binary_signature(ops_memory_get_length(mem), 
                                         ops_memory_get_data(mem),
                                         &content->signature,
                                         ops_get_public_key_from_data(signer));
            break;

        default:
            OPS_ERROR_1(errors, OPS_E_UNIMPLEMENTED,
                        "Verification of signature type 0x%02x not yet implemented\n", content->signature.info.type);
            break;
            
	    }
    ops_memory_free(mem);

	if(valid)
	    {
        add_sig_to_valid_list(arg->result, &content->signature.info);
	    }
	else
	    {
        OPS_ERROR(errors,OPS_E_V_BAD_SIGNATURE,"Bad Signature");
        add_sig_to_invalid_list(arg->result, &content->signature.info);
	    }
	break;

	// ignore these
 case OPS_PARSER_PTAG:
 case OPS_PTAG_CT_SIGNATURE_HEADER:
 case OPS_PTAG_CT_ARMOUR_HEADER:
 case OPS_PTAG_CT_ARMOUR_TRAILER:
 case OPS_PTAG_CT_ONE_PASS_SIGNATURE:
 case OPS_PARSER_PACKET_END:
	break;

    default:
	fprintf(stderr,"unexpected tag=0x%x\n",content_->tag);
	assert(0);
	break;
	}
    return OPS_RELEASE_MEMORY;
    }

static void keydata_destroyer(ops_reader_info_t *rinfo)
    { free(ops_reader_get_arg(rinfo)); }

void ops_keydata_reader_set(ops_parse_info_t *pinfo,const ops_keydata_t *key)
    {
    validate_reader_arg_t *arg=malloc(sizeof *arg);

    memset(arg,'\0',sizeof *arg);

    arg->key=key;
    arg->packet=0;
    arg->offset=0;

    ops_reader_set(pinfo,keydata_reader,keydata_destroyer,arg);
    }

/**
 * \ingroup HighLevel_Verify
 * \brief Indicicates whether any errors were found
 * \param result Validation result to check
 * \return ops_false if any invalid signatures or unknown signers or no valid signatures; else ops_true
 */
ops_boolean_t validate_result_status(ops_validate_result_t* result)
    {
    if (result->invalid_count || result->unknown_signer_count || !result->valid_count)
        return ops_false;
    else
        return ops_true;
    }

/**
 * \ingroup HighLevel_Verify
 * \brief Validate all signatures on a single key against the given keyring
 * \param result Where to put the result
 * \param key Key to validate
 * \param keyring Keyring to use for validation
 * \param cb_get_passphrase Callback to use to get passphrase
 * \return ops_true if all signatures OK; else ops_false
 * \note It is the caller's responsiblity to free result after use.
 * \sa ops_validate_result_free()
 
 Example Code:
\code
void example(const ops_keydata_t* key, const ops_keyring_t *keyring)
{
  ops_validate_result_t *result=NULL;
  if (ops_validate_key_signatures(result, key, keyring, callback_cmd_get_passphrase_from_cmdline)==ops_true)
    printf("OK");
  else
    printf("ERR");
  printf("valid=%d, invalid=%d, unknown=%d\n",
         result->valid_count,
         result->invalid_count,
         result->unknown_signer_count);
  ops_validate_result_free(result);
}

\endcode
 */
ops_boolean_t ops_validate_key_signatures(ops_validate_result_t *result,const ops_keydata_t *key,
                                 const ops_keyring_t *keyring,
                                 ops_parse_cb_return_t cb_get_passphrase (const ops_parser_content_t *, ops_parse_cb_info_t *)
                                 )
    {
    ops_parse_info_t *pinfo;
    validate_key_cb_arg_t carg;

    memset(&carg,'\0',sizeof carg);
    carg.result=result;
    carg.cb_get_passphrase=cb_get_passphrase;

    pinfo=ops_parse_info_new();
    //    ops_parse_options(&opt,OPS_PTAG_CT_SIGNATURE,OPS_PARSE_PARSED);

    carg.keyring=keyring;

    ops_parse_cb_set(pinfo,ops_validate_key_cb,&carg);
    pinfo->rinfo.accumulate=ops_true;
    ops_keydata_reader_set(pinfo,key);

    // Note: Coverity incorrectly reports an error that carg.rarg
    // is never used.
    carg.rarg=ops_reader_get_arg_from_pinfo(pinfo);

    ops_parse(pinfo);

    ops_public_key_free(&carg.pkey);
    if(carg.subkey.version)
	ops_public_key_free(&carg.subkey);
    ops_user_id_free(&carg.user_id);
    ops_user_attribute_free(&carg.user_attribute);

    ops_parse_info_delete(pinfo);

    if (result->invalid_count || result->unknown_signer_count || !result->valid_count)
        return ops_false;
    else
        return ops_true;
    }

/**
   \ingroup HighLevel_Verify
   \param result Where to put the result
   \param ring Keyring to use
   \param cb_get_passphrase Callback to use to get passphrase
   \note It is the caller's responsibility to free result after use.
   \sa ops_validate_result_free()
*/
ops_boolean_t ops_validate_all_signatures(ops_validate_result_t *result,
                                 const ops_keyring_t *ring,
                                 ops_parse_cb_return_t cb_get_passphrase (const ops_parser_content_t *, ops_parse_cb_info_t *)
                                 )
    {
    int n;

    memset(result,'\0',sizeof *result);
    for(n=0 ; n < ring->nkeys ; ++n)
        ops_validate_key_signatures(result,&ring->keys[n],ring, cb_get_passphrase);
    return validate_result_status(result);
    }

/**
   \ingroup HighLevel_Verify
   \brief Frees validation result and associated memory
   \param result Struct to be freed
   \note Must be called after validation functions
*/
void ops_validate_result_free(ops_validate_result_t *result)
    {
    if (!result)
        return;

    if (result->valid_sigs)
        free_signature_info(result->valid_sigs);
    if (result->invalid_sigs)
        free_signature_info(result->invalid_sigs);
    if (result->unknown_sigs)
        free_signature_info(result->unknown_sigs);

    free(result);
    result=NULL;
    }

/**
   \ingroup HighLevel_Verify
   \brief Verifies the signatures in a signed file
   \param result Where to put the result
   \param filename Name of file to be validated
   \param armoured Treat file as armoured, if set
   \param keyring Keyring to use
   \return ops_true if signatures validate successfully; ops_false if signatures fail or there are no signatures
   \note After verification, result holds the details of all keys which 
   have passed, failed and not been recognised.
   \note It is the caller's responsiblity to call ops_validate_result_free(result) after use.

Example code:
\code
void example(const char* filename, const int armoured, const ops_keyring_t* keyring)
{
  ops_validate_result_t* result=ops_mallocz(sizeof *result);
  
  if (ops_validate_file(result, filename, armoured, keyring)==ops_true)
  {
    printf("OK");
    // look at result for details of keys with good signatures
  }
  else
  {
    printf("ERR");
    // look at result for details of failed signatures or unknown signers
  }

  ops_validate_result_free(result);
}
\endcode
*/
ops_boolean_t ops_validate_file(ops_validate_result_t *result, const char* filename, const int armoured, const ops_keyring_t* keyring)
    {
    ops_parse_info_t *pinfo=NULL;
    validate_data_cb_arg_t validate_arg;

    int fd=0;

    //
    fd=ops_setup_file_read(&pinfo, filename, &validate_arg, validate_data_cb, ops_true);
    if (fd < 0)
        return ops_false;

    // Set verification reader and handling options

    memset(&validate_arg,'\0',sizeof validate_arg);
    validate_arg.result=result;
    validate_arg.keyring=keyring;
    // Note: Coverity incorrectly reports an error that carg.rarg
    // is never used.
    validate_arg.rarg=ops_reader_get_arg_from_pinfo(pinfo);

    if (armoured)
        ops_reader_push_dearmour(pinfo);
    
    // Do the verification

    ops_parse(pinfo);

    if (debug)
        {
        printf("valid=%d, invalid=%d, unknown=%d\n",
               result->valid_count,
               result->invalid_count,
               result->unknown_signer_count);
        }

    // Tidy up
    if (armoured)
        ops_reader_pop_dearmour(pinfo);
    ops_teardown_file_read(pinfo, fd);

    return validate_result_status(result);
    }

/**
   \ingroup HighLevel_Verify
   \brief Verifies the signatures in a ops_memory_t struct
   \param result Where to put the result
   \param mem Memory to be validated
   \param armoured Treat data as armoured, if set
   \param keyring Keyring to use
   \return ops_true if signature validates successfully; ops_false if not
   \note After verification, result holds the details of all keys which 
   have passed, failed and not been recognised.
   \note It is the caller's responsiblity to call ops_validate_result_free(result) after use.
*/

ops_boolean_t ops_validate_mem(ops_validate_result_t *result, ops_memory_t* mem, const int armoured, const ops_keyring_t* keyring)
    {
    ops_parse_info_t *pinfo=NULL;
    validate_data_cb_arg_t validate_arg;

    //
    ops_setup_memory_read(&pinfo, mem, &validate_arg, validate_data_cb, ops_true);

    // Set verification reader and handling options

    memset(&validate_arg,'\0',sizeof validate_arg);
    validate_arg.result=result;
    validate_arg.keyring=keyring;
    // Note: Coverity incorrectly reports an error that carg.rarg
    // is never used.
    validate_arg.rarg=ops_reader_get_arg_from_pinfo(pinfo);

    if (armoured)
        ops_reader_push_dearmour(pinfo);
    
    // Do the verification

    ops_parse(pinfo);

    if (debug)
        {
        printf("valid=%d, invalid=%d, unknown=%d\n",
               result->valid_count,
               result->invalid_count,
               result->unknown_signer_count);
        }

    // Tidy up
    if (armoured)
        ops_reader_pop_dearmour(pinfo);
    ops_teardown_memory_read(pinfo, mem);

    return validate_result_status(result);
    }

// eof

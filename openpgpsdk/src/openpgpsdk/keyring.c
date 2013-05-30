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

#include <openpgpsdk/armour.h>
#include <openpgpsdk/std_print.h>
#include <openpgpsdk/keyring.h>
#include <openpgpsdk/packet-parse.h>
#include <openpgpsdk/util.h>
#include <openpgpsdk/accumulate.h>
#include <openpgpsdk/validate.h>
#include <openpgpsdk/signature.h>
#include <openpgpsdk/readerwriter.h>
#include <openpgpsdk/defs.h>

#include "keyring_local.h"
#include "parse_local.h"

#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#include <termios.h>
#endif
#include <fcntl.h>
#include <assert.h>

#include <openpgpsdk/final.h>
#include <openpgpsdk/opsdir.h>

/**
   \ingroup HighLevel_Keyring
   
   \brief Creates a new ops_keydata_t struct

   \return A new ops_keydata_t struct, initialised to zero.

   \note The returned ops_keydata_t struct must be freed after use with ops_keydata_free.
*/

ops_keydata_t *ops_keydata_new(void)
    { return ops_mallocz(sizeof(ops_keydata_t)); }


// Frees the content of a keydata structure, but not the keydata itself.
static void keydata_internal_free(ops_keydata_t *keydata)
    {
    unsigned n;

    for(n=0 ; n < keydata->nuids ; ++n)
	ops_user_id_free(&keydata->uids[n]);
    free(keydata->uids);
    keydata->uids=NULL;
    keydata->nuids=0;

    for(n=0 ; n < keydata->npackets ; ++n)
	ops_packet_free(&keydata->packets[n]);
    free(keydata->packets);
    keydata->packets=NULL;
    keydata->npackets=0;

/*	 for(n=0;n<keydata->nsigs;++n)
	 {
		 ops_user_id_free(keydata->sigs[n].userid) ;
		 ops_packet_free(keydata->sigs[n].packet) ;
	 }*/
	 free(keydata->sigs) ;

    if(keydata->type == OPS_PTAG_CT_PUBLIC_KEY)
	ops_public_key_free(&keydata->key.pkey);
    else
	ops_secret_key_free(&keydata->key.skey);

    }

/**
 \ingroup HighLevel_Keyring

 \brief Frees keydata and its memory

 \param keydata Key to be freed.

 \note This frees the keydata itself, as well as any other memory
       alloc-ed by it.
*/
void ops_keydata_free(ops_keydata_t *keydata)
    {
    keydata_internal_free(keydata);
    free(keydata);
    }

// \todo check where userid pointers are copied
/**
\ingroup Core_Keys
\brief Copy user id, including contents
\param dst Destination User ID
\param src Source User ID
\note If dst already has a user_id, it will be freed.
*/
void ops_copy_userid(ops_user_id_t* dst, const ops_user_id_t* src)
    {
    int len=strlen((char *)src->user_id);
    if (dst->user_id)
        free(dst->user_id);
    dst->user_id=ops_mallocz(len+1);

    memcpy(dst->user_id, src->user_id, len);
    }
// \todo check where pkt pointers are copied
/**
\ingroup Core_Keys
\brief Copy packet, including contents
\param dst Destination packet
\param src Source packet
\note If dst already has a packet, it will be freed.
*/
void ops_copy_packet(ops_packet_t* dst, const ops_packet_t* src)
    {
    if (dst->raw)
        free(dst->raw);
    dst->raw=ops_mallocz(src->length);

    dst->length=src->length;
    memcpy(dst->raw, src->raw, src->length);
    }



/**
\ingroup Core_Keys
\brief Copies entire key data
\param dst Destination key where to copy
\param src Source key to copy
*/
void ops_keydata_copy(ops_keydata_t *dst,const ops_keydata_t *src)
{
	unsigned n;

	keydata_internal_free(dst) ;
	memset(dst,0,sizeof(ops_keydata_t)) ;

	dst->uids = (ops_user_id_t*)ops_mallocz(src->nuids * sizeof(ops_user_id_t)) ;
	dst->nuids = src->nuids ;
	dst->nuids_allocated = src->nuids ;

	for(n=0 ; n < src->nuids ; ++n)
		ops_copy_userid(&dst->uids[n],&src->uids[n]) ;

	dst->packets = (ops_packet_t*)ops_mallocz(src->npackets * sizeof(ops_packet_t)) ;
	dst->npackets = src->npackets ;
	dst->npackets_allocated = src->npackets ;

	for(n=0 ; n < src->npackets ; ++n)
		ops_copy_packet(&(dst->packets[n]),&(src->packets[n]));

	dst->nsigs = src->nsigs ;
	dst->sigs = (sigpacket_t*)ops_mallocz(src->nsigs * sizeof(sigpacket_t)) ;
	dst->nsigs_allocated = src->nsigs ;

	for(n=0 ; n < src->nsigs ; ++n)
	{
		dst->sigs[n].userid = (ops_user_id_t*)ops_mallocz(sizeof(ops_user_id_t)) ;
		dst->sigs[n].packet = (ops_packet_t*)ops_mallocz(sizeof(ops_packet_t)) ;

		ops_copy_userid(dst->sigs[n].userid,src->sigs[n].userid) ;
		ops_copy_packet(dst->sigs[n].packet,src->sigs[n].packet) ;
	}

	dst->key_id[0] = src->key_id[0] ;
	dst->key_id[1] = src->key_id[1] ;
	dst->key_id[2] = src->key_id[2] ;
	dst->key_id[3] = src->key_id[3] ;
	dst->key_id[4] = src->key_id[4] ;
	dst->key_id[5] = src->key_id[5] ;
	dst->key_id[6] = src->key_id[6] ;
	dst->key_id[7] = src->key_id[7] ;
	dst->type = src->type ;
	dst->key = src->key ;
	dst->fingerprint = src->fingerprint ;

	if(src->type == OPS_PTAG_CT_PUBLIC_KEY)
		ops_public_key_copy(&dst->key.pkey,&src->key.pkey);
	else                  
		ops_secret_key_copy(&dst->key.skey,&src->key.skey);
}


/**
 \ingroup HighLevel_KeyGeneral

 \brief Returns the public key in the given keydata.
 \param keydata

  \return Pointer to public key

  \note This is not a copy, do not free it after use.
*/

const ops_public_key_t *
ops_get_public_key_from_data(const ops_keydata_t *keydata)
    {
    if(keydata->type == OPS_PTAG_CT_PUBLIC_KEY)
	return &keydata->key.pkey;
    return &keydata->key.skey.public_key;
    }

/**
\ingroup HighLevel_KeyGeneral

\brief Check whether this is a secret key or not.
*/

ops_boolean_t ops_is_key_secret(const ops_keydata_t *data)
    { return data->type != OPS_PTAG_CT_PUBLIC_KEY; }

/**
 \ingroup HighLevel_KeyGeneral

 \brief Returns the secret key in the given keydata.

 \note This is not a copy, do not free it after use.

 \note This returns a const. If you need to be able to write to this pointer, use ops_get_writable_secret_key_from_data
*/

const ops_secret_key_t *
ops_get_secret_key_from_data(const ops_keydata_t *data)
    {
    if(data->type != OPS_PTAG_CT_SECRET_KEY)
        return NULL;

    return &data->key.skey;
    }

/**
 \ingroup HighLevel_KeyGeneral

  \brief Returns the secret key in the given keydata.

  \note This is not a copy, do not free it after use.

  \note If you do not need to be able to modify this key, there is an equivalent read-only function ops_get_secret_key_from_data.
*/

ops_secret_key_t *
ops_get_writable_secret_key_from_data(ops_keydata_t *data)
    {
    if (data->type != OPS_PTAG_CT_SECRET_KEY)
        return NULL;

    return &data->key.skey;
    }

typedef struct
    {
    const ops_keydata_t *key;
    char *pphrase;
    ops_secret_key_t *skey;
    } decrypt_arg_t;

static ops_parse_cb_return_t decrypt_cb(const ops_parser_content_t *content_,
					ops_parse_cb_info_t *cbinfo)
    {
    const ops_parser_content_union_t *content=&content_->content;
    decrypt_arg_t *arg=ops_parse_cb_get_arg(cbinfo);

    OPS_USED(cbinfo);

    switch(content_->tag)
	{
    case OPS_PARSER_PTAG:
    case OPS_PTAG_CT_USER_ID:
    case OPS_PTAG_CT_SIGNATURE:
    case OPS_PTAG_CT_SIGNATURE_HEADER:
    case OPS_PTAG_CT_SIGNATURE_FOOTER:
    case OPS_PTAG_CT_TRUST:
	break;

    case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
	*content->secret_key_passphrase.passphrase=arg->pphrase;
	return OPS_KEEP_MEMORY;

    case OPS_PARSER_ERRCODE:
	switch(content->errcode.errcode)
	    {
	case OPS_E_P_MPI_FORMAT_ERROR:
	    /* Generally this means a bad passphrase */
	    fprintf(stderr,"Bad passphrase!\n");
	    goto done;

	case OPS_E_P_PACKET_CONSUMED:
	    /* And this is because of an error we've accepted */
	    goto done;

	default:
	    fprintf(stderr,"parse error: %s\n",
		    ops_errcode(content->errcode.errcode));
	    assert(0);
	    break;
	    }

	break;

    case OPS_PARSER_ERROR:
	printf("parse error: %s\n",content->error.error);
	assert(0);
	break;

    case OPS_PTAG_CT_SECRET_KEY:
	arg->skey=malloc(sizeof *arg->skey);
	*arg->skey=content->secret_key;
	return OPS_KEEP_MEMORY;

 case OPS_PARSER_PACKET_END:
     // nothing to do
     break;

    default:
	fprintf(stderr,"Unexpected tag %d (0x%x)\n",content_->tag,
		content_->tag);
	assert(0);
	}

 done:
    return OPS_RELEASE_MEMORY;
    }

/**
\ingroup Core_Keys
\brief Decrypts secret key from given keydata with given passphrase
\param key Key from which to get secret key
\param pphrase Passphrase to use to decrypt secret key
\return secret key
*/
ops_secret_key_t *ops_decrypt_secret_key_from_data(const ops_keydata_t *key,
						   const char *pphrase)
    {
    ops_parse_info_t *pinfo;
    decrypt_arg_t arg;

    memset(&arg,'\0',sizeof arg);
    arg.key=key;
    arg.pphrase=strdup(pphrase);

    pinfo=ops_parse_info_new();

    ops_keydata_reader_set(pinfo,key);
    ops_parse_cb_set(pinfo,decrypt_cb,&arg);
    pinfo->rinfo.accumulate=ops_true;

    ops_parse(pinfo);

    ops_parse_info_delete(pinfo);

    return arg.skey;
    }

/** 
\ingroup Core_Keys
\brief Set secret key in content
\param content Content to be set
\param key Keydata to get secret key from
*/
void ops_set_secret_key(ops_parser_content_union_t* content,const ops_keydata_t *key)
    {
    *content->get_secret_key.secret_key=&key->key.skey;
    }

/**
\ingroup Core_Keys
\brief Get Key ID from keydata
\param key Keydata to get Key ID from
\return Pointer to Key ID inside keydata
*/
const unsigned char* ops_get_key_id(const ops_keydata_t *key)
    {
    return key->key_id;
    }

/**
\ingroup Core_Keys
\brief How many User IDs in this key?
\param key Keydata to check
\return Num of user ids
*/
unsigned ops_get_user_id_count(const ops_keydata_t *key)
    {
    return key->nuids;
    }

/**
\ingroup Core_Keys
\brief Get indexed user id from key
\param key Key to get user id from
\param index Which key to get
\return Pointer to requested user id
*/
const unsigned char* ops_get_user_id(const ops_keydata_t *key, unsigned index)
    {
    return key->uids[index].user_id;
    }

/**
   \ingroup HighLevel_Supported
   \brief Checks whether key's algorithm and type are supported by OpenPGP::SDK
   \param keydata Key to be checked
   \return ops_true if key algorithm and type are supported by OpenPGP::SDK; ops_false if not
*/

ops_boolean_t ops_is_key_supported(const ops_keydata_t *keydata)
{
	if(keydata->type == OPS_PTAG_CT_PUBLIC_KEY)
	{
		if(keydata->key.pkey.algorithm == OPS_PKA_RSA)
			return ops_true;
	}
	return ops_false;
}


/** 
    \ingroup HighLevel_KeyringFind

    \brief Returns key inside a keyring, chosen by index

    \param keyring Pointer to existing keyring
    \param index Index of required key

    \note Index starts at 0

    \note This returns a pointer to the original key, not a copy. You do not need to free the key after use.

    \return Pointer to the required key; or NULL if index too large.

    Example code:
    \code
    void example(const ops_keyring_t* keyring)
    {
    ops_keydata_t* keydata=NULL;
    keydata=ops_keyring_get_key_by_index(keyring, 0);
    ...
    }
    \endcode
*/

const ops_keydata_t* ops_keyring_get_key_by_index(const ops_keyring_t *keyring, int index)
    {
    if (index >= keyring->nkeys)
        return NULL;
    return &keyring->keys[index]; 
    }

/**
\ingroup Core_Keys
\brief Add User ID to keydata
\param keydata Key to which to add User ID
\param userid User ID to add
\return Pointer to new User ID
*/
ops_user_id_t* ops_add_userid_to_keydata(ops_keydata_t* keydata, const ops_user_id_t* userid)
    {
    ops_user_id_t* new_uid=NULL;

    EXPAND_ARRAY(keydata, uids);

    // initialise new entry in array
    new_uid=&keydata->uids[keydata->nuids];

    new_uid->user_id=NULL;

    // now copy it
    ops_copy_userid(new_uid,userid);
    keydata->nuids++;

    return new_uid;
    }

/**
\ingroup Core_Keys
\brief Add packet to key
\param keydata Key to which to add packet
\param packet Packet to add
\return Pointer to new packet
*/
ops_packet_t* ops_add_packet_to_keydata(ops_keydata_t* keydata, const ops_packet_t* packet)
    {
    ops_packet_t* new_pkt=NULL;

    EXPAND_ARRAY(keydata, packets);

    // initialise new entry in array
    new_pkt=&keydata->packets[keydata->npackets];
    new_pkt->length=0;
    new_pkt->raw=NULL;

    // now copy it
    ops_copy_packet(new_pkt, packet);
    keydata->npackets++;

    return new_pkt;
    }

/**
\ingroup Core_Keys
\brief Add signed User ID to key
\param keydata Key to which to add signed User ID
\param user_id User ID to add
\param sigpacket Packet to add
*/
void ops_add_signed_userid_to_keydata(ops_keydata_t* keydata, const ops_user_id_t* user_id, const ops_packet_t* sigpacket)
    {
    //int i=0;
    ops_user_id_t * uid=NULL;
    ops_packet_t * pkt=NULL;

    uid=ops_add_userid_to_keydata(keydata, user_id);
    pkt=ops_add_packet_to_keydata(keydata, sigpacket);

    /*
     * add entry in sigs array to link the userid and sigpacket
     */

    // and add ptr to it from the sigs array
    EXPAND_ARRAY(keydata, sigs);

    // setup new entry in array

    keydata->sigs[keydata->nsigs].userid=uid;
    keydata->sigs[keydata->nsigs].packet=pkt;

    keydata->nsigs++;
    }

/**
\ingroup Core_Keys
\brief Add selfsigned User ID to key
\param keydata Key to which to add user ID
\param userid Self-signed User ID to add
\return ops_true if OK; else ops_false
*/
ops_boolean_t ops_add_selfsigned_userid_to_keydata(ops_keydata_t* keydata, ops_user_id_t* userid)
    {
    ops_packet_t sigpacket;

    ops_memory_t* mem_userid=NULL;
    ops_create_info_t* cinfo_userid=NULL;

    ops_memory_t* mem_sig=NULL;
    ops_create_info_t* cinfo_sig=NULL;

    ops_create_signature_t *sig=NULL;

    /*
     * create signature packet for this userid
     */

    // create userid pkt
    ops_setup_memory_write(&cinfo_userid, &mem_userid, 128);
    ops_write_struct_user_id(userid, cinfo_userid);

    // create sig for this pkt

    sig=ops_create_signature_new();
    ops_signature_start_key_signature(sig, &keydata->key.skey.public_key, userid, OPS_CERT_POSITIVE);
    ops_signature_add_creation_time(sig,time(NULL));
    ops_signature_add_issuer_key_id(sig,keydata->key_id);
    ops_signature_add_primary_user_id(sig, ops_true);
    ops_signature_hashed_subpackets_end(sig);

    ops_setup_memory_write(&cinfo_sig, &mem_sig, 128);
    ops_write_signature(sig,&keydata->key.skey.public_key,&keydata->key.skey, cinfo_sig);

    // add this packet to keydata

    sigpacket.length=ops_memory_get_length(mem_sig);
    sigpacket.raw=ops_memory_get_data(mem_sig);

    // add userid to keydata
    ops_add_signed_userid_to_keydata(keydata, userid, &sigpacket);

    // cleanup
    ops_create_signature_delete(sig);
    ops_create_info_delete(cinfo_userid);
    ops_create_info_delete(cinfo_sig);
    ops_memory_free(mem_userid);
    ops_memory_free(mem_sig);

    return ops_true;
    }

/**
\ingroup Core_Keys
\brief Add signature to given key
\return ops_true if OK; else ops_false
*/
ops_boolean_t ops_sign_key(ops_keydata_t* keydata, const unsigned char *signers_key_id,ops_secret_key_t *signers_key)
{
/*	ops_memory_t* mem_userid=NULL; */
	ops_create_info_t* cinfo_userid=NULL;

	ops_memory_t* mem_sig=NULL;
	ops_create_info_t* cinfo_sig=NULL;

	ops_create_signature_t *sig=NULL;

	/*
	 * create signature packet for this userid
	 */

	// create sig for this pkt

	sig=ops_create_signature_new();
	ops_signature_start_key_signature(sig, &keydata->key.skey.public_key, &keydata->uids[0], OPS_CERT_GENERIC);
	ops_signature_add_creation_time(sig,time(NULL)); 
	ops_signature_add_issuer_key_id(sig,signers_key_id);
	ops_signature_hashed_subpackets_end(sig);

	ops_setup_memory_write(&cinfo_sig, &mem_sig, 128);
	ops_write_signature(sig,&signers_key->public_key,signers_key, cinfo_sig);

	// add this packet to keydata

	ops_packet_t sigpacket;
	sigpacket.length=ops_memory_get_length(mem_sig);
	sigpacket.raw=ops_memory_get_data(mem_sig);

	// add userid to keydata
	ops_add_packet_to_keydata(keydata, &sigpacket);

	// cleanup
	ops_create_signature_delete(sig);
	ops_create_info_delete(cinfo_sig);
	ops_memory_free(mem_sig);

	return ops_true;
}
/**
\ingroup Core_Keys
\brief Initialise ops_keydata_t
\param keydata Keydata to initialise
\param type OPS_PTAG_CT_PUBLIC_KEY or OPS_PTAG_CT_SECRET_KEY
*/
void ops_keydata_init(ops_keydata_t* keydata, const ops_content_tag_t type)
    {
    assert(keydata->type==OPS_PTAG_CT_RESERVED);
    assert(type==OPS_PTAG_CT_PUBLIC_KEY || type==OPS_PTAG_CT_SECRET_KEY);

    keydata->type=type;
    }

/** 
    Example Usage:
    \code

    // definition of variables
    ops_keyring_t keyring;
    char* filename="~/.gnupg/pubring.gpg";

    // Read keyring from file
    ops_keyring_read_from_file(&keyring,filename);

    // do actions using keyring   
    ... 

    // Free memory alloc-ed in ops_keyring_read_from_file()
    ops_keyring_free(keyring);
    \endcode
*/

static ops_parse_cb_return_t
cb_keyring_read(const ops_parser_content_t *content_,
		ops_parse_cb_info_t *cbinfo);

/**
   \ingroup HighLevel_KeyringRead
   
   \brief Reads a keyring from a file
   
   \param keyring Pointer to an existing ops_keyring_t struct
   \param armour ops_true if file is armoured; else ops_false
   \param filename Filename of keyring to be read

   \return ops true if OK; ops_false on error

   \note Keyring struct must already exist.

   \note Can be used with either a public or secret keyring.

   \note You must call ops_keyring_free() after usage to free alloc-ed memory.

   \note If you call this twice on the same keyring struct, without calling
   ops_keyring_free() between these calls, you will introduce a memory leak.

   \sa ops_keyring_read_from_mem()
   \sa ops_keyring_free()

   Example code:
   \code
   ops_keyring_t* keyring=ops_mallocz(sizeof *keyring);
   ops_boolean_t armoured=ops_false;
   ops_keyring_read_from_file(keyring, armoured, "~/.gnupg/pubring.gpg");
   ...
   ops_keyring_free(keyring);
   free (keyring);
   
   \endcode
*/

ops_boolean_t ops_keyring_read_from_file(ops_keyring_t *keyring, const ops_boolean_t armour, const char *filename)
    {
    ops_parse_info_t *pinfo;
    int fd;
    ops_boolean_t res = ops_true;

    pinfo=ops_parse_info_new();

    // add this for the moment,
    // \todo need to fix the problems with reading signature subpackets later

    //    ops_parse_options(pinfo,OPS_PTAG_SS_ALL,OPS_PARSE_RAW);
    ops_parse_options(pinfo,OPS_PTAG_SS_ALL,OPS_PARSE_PARSED);

    fd=ops_open(filename,O_RDONLY | O_BINARY, 0);

    if(fd < 0)
        {
        ops_parse_info_delete(pinfo);
        perror(filename);
        return ops_false;
        }

    ops_reader_set_fd(pinfo,fd);

    ops_parse_cb_set(pinfo,cb_keyring_read,NULL);

    if (armour)
        { ops_reader_push_dearmour(pinfo); }

    if ( ops_parse_and_accumulate(keyring,pinfo) == 0 ) {
        res = ops_false; 
    }
    else
        {
        res = ops_true;
        }
    ops_print_errors(ops_parse_info_get_errors(pinfo));

    if (armour)
        ops_reader_pop_dearmour(pinfo);

    close(fd);

    ops_parse_info_delete(pinfo);

    return res;
    }

/**
   \ingroup HighLevel_KeyringRead
   
   \brief Reads a keyring from memory
   
   \param keyring Pointer to existing ops_keyring_t struct
   \param armour ops_true if file is armoured; else ops_false
   \param mem Pointer to a ops_memory_t struct containing keyring to be read
   
   \return ops true if OK; ops_false on error

   \note Keyring struct must already exist.

   \note Can be used with either a public or secret keyring.

   \note You must call ops_keyring_free() after usage to free alloc-ed memory.

   \note If you call this twice on the same keyring struct, without calling
   ops_keyring_free() between these calls, you will introduce a memory leak.

   \sa ops_keyring_read_from_file
   \sa ops_keyring_free

   Example code:
   \code
   ops_memory_t* mem; // Filled with keyring packets
   ops_keyring_t* keyring=ops_mallocz(sizeof *keyring);
   ops_boolean_t armoured=ops_false;
   ops_keyring_read_from_mem(keyring, armoured, mem);
   ...
   ops_keyring_free(keyring);
   free (keyring);
   \endcode
*/
ops_boolean_t ops_keyring_read_from_mem(ops_keyring_t *keyring, const ops_boolean_t armour, ops_memory_t* mem)
    {
    ops_parse_info_t *pinfo=NULL;
    ops_boolean_t res = ops_true;

    ops_setup_memory_read(&pinfo, mem, NULL, cb_keyring_read,
			  OPS_ACCUMULATE_NO);
    ops_parse_options(pinfo,OPS_PTAG_SS_ALL,OPS_PARSE_PARSED);

    if (armour)
        { ops_reader_push_dearmour(pinfo); }

    if ( ops_parse_and_accumulate(keyring,pinfo) == 0 ) 
        {
        res = ops_false; 
        } 
    else 
        {
        res = ops_true;
        }
    ops_print_errors(ops_parse_info_get_errors(pinfo));

    if (armour)
        ops_reader_pop_dearmour(pinfo);

    // don't call teardown_memory_read because memory was passed
    // in. But we need to free the parse_info object allocated by
    // ops_setup_memory_read().
    ops_parse_info_delete(pinfo);

    return res;
    }

/**
   \ingroup HighLevel_KeyringRead
 
   \brief Frees keyring's contents (but not keyring itself)
 
   \param keyring Keyring whose data is to be freed
   
   \note This does not free keyring itself, just the memory alloc-ed in it.
 */
void ops_keyring_free(ops_keyring_t *keyring)
    {
    int i;

    for (i = 0; i < keyring->nkeys; i++)
        keydata_internal_free(&keyring->keys[i]);

    free(keyring->keys);
    keyring->keys=NULL;
    keyring->nkeys=0;
    keyring->nkeys_allocated=0;
    }

void ops_keyring_remove_key(ops_keyring_t *keyring,int index)
{
	if(index > keyring->nkeys-1)
	{
		fprintf(stderr,"ops_keyring_remove_key: ERROR: cannot remove key with index %d > %d.",index,keyring->nkeys-1) ;
		return ;
	}

	if(index < keyring->nkeys-1)
		ops_keydata_copy(&keyring->keys[index],&keyring->keys[keyring->nkeys-1]) ;

	keydata_internal_free(&keyring->keys[keyring->nkeys-1]) ;
	keyring->nkeys-- ;

	// keyring->nkeys_allocated is left untouched intentionnaly. 
}

/**
   \ingroup HighLevel_KeyringFind

   \brief Finds key in keyring from its Key ID

   \param keyring Keyring to be searched
   \param keyid ID of required key

   \return Pointer to key, if found; NULL, if not found

   \note This returns a pointer to the key inside the given keyring, not a copy. Do not free it after use.
   
   Example code:
   \code
   void example(ops_keyring_t* keyring)
   {
   ops_keydata_t* keydata=NULL;
   unsigned char keyid[OPS_KEY_ID_SIZE]; // value set elsewhere
   keydata=ops_keyring_find_key_by_id(keyring,keyid);
   ...
   }
   \endcode
*/
const ops_keydata_t *
ops_keyring_find_key_by_id(const ops_keyring_t *keyring,
			   const unsigned char keyid[OPS_KEY_ID_SIZE])
    {
    int n;

    if (!keyring)
        return NULL;

    for(n=0 ; n < keyring->nkeys ; ++n)
        {
        if(!memcmp(keyring->keys[n].key_id,keyid,OPS_KEY_ID_SIZE))
            return &keyring->keys[n];
        }

    return NULL;
    }

/**
   \ingroup HighLevel_KeyringFind

   \brief Finds key from its User ID

   \param keyring Keyring to be searched
   \param userid User ID of required key

   \return Pointer to Key, if found; NULL, if not found

   \note This returns a pointer to the key inside the keyring, not a copy. Do not free it.

   Example code:
   \code
   void example(ops_keyring_t* keyring)
   {
   ops_keydata_t* keydata=NULL;
   keydata=ops_keyring_find_key_by_userid(keyring,"user@domain.com");
   ...
   }
   \endcode
*/
const ops_keydata_t *
ops_keyring_find_key_by_userid(const ops_keyring_t *keyring,
				 const char *userid)
    {
    int n=0;
    unsigned int i=0;

    if (!keyring)
        return NULL;

    for(n=0 ; n < keyring->nkeys ; ++n)
        {
        for(i=0; i<keyring->keys[n].nuids; i++)
            {
            //printf("[%d][%d] userid %s\n",n,i,keyring->keys[n].uids[i].user_id);
            if(!strncmp((char *)keyring->keys[n].uids[i].user_id,userid,strlen(userid)))
                return &keyring->keys[n];
            }
        }

    //printf("end: n=%d,i=%d\n",n,i);
    return NULL;
    }

/**
   \ingroup HighLevel_KeyringList

   \brief Prints all keys in keyring to stdout.

   \param keyring Keyring to use

   \return none

   Example code:
   \code
   void example()
   {
   ops_keyring_t* keyring=ops_mallocz(sizeof *keyring);
   ops_boolean_t armoured=ops_false;
   ops_keyring_read_from_file(keyring, armoured, "~/.gnupg/pubring.gpg");
   
   ops_keyring_list(keyring);
   
   ops_keyring_free(keyring);
   free (keyring);
   }
   \endcode
*/

void
ops_keyring_list(const ops_keyring_t* keyring)
    {
    int n;
    unsigned int i;
    ops_keydata_t* key;

    printf ("%d keys\n", keyring->nkeys);
    for(n=0,key=&keyring->keys[n] ; n < keyring->nkeys ; ++n,++key)
	{
	for(i=0; i<key->nuids; i++)
	    {
	    if (ops_is_key_secret(key))
		ops_print_secret_keydata(key);
	    else
		ops_print_public_keydata(key);
	    }

	}
    }

/* Static functions */

static ops_parse_cb_return_t
cb_keyring_read(const ops_parser_content_t *content_,
		ops_parse_cb_info_t *cbinfo)
{
	OPS_USED(cbinfo);

	switch(content_->tag)
	{
		case OPS_PARSER_PTAG:
		case OPS_PTAG_CT_ENCRYPTED_SECRET_KEY: // we get these because we didn't prompt
		case OPS_PTAG_CT_SIGNATURE_HEADER:
		case OPS_PTAG_CT_SIGNATURE_FOOTER:
		case OPS_PTAG_CT_SIGNATURE:
		case OPS_PTAG_CT_TRUST:
		case OPS_PARSER_ERRCODE:
			break;

		default:
			;
	}

	return OPS_RELEASE_MEMORY;
}

/**
   \ingroup HighLevel_KeyringList

   \brief Saves keyring to specified file

   \param keyring  Keyring to save
   \param armoured Save in ascii armoured format
   \param output filename

   \return ops_true is anything when ok
*/

ops_boolean_t ops_write_keyring_to_file(const ops_keyring_t *keyring,ops_boolean_t armoured,const char *filename,ops_boolean_t write_all_packets)
{
	ops_create_info_t *info;
	int fd = ops_setup_file_write(&info, filename, ops_true);

	if (fd < 0) 
	{
		fprintf(stderr,"ops_write_keyring(): ERROR: Cannot write to %s\n",filename ) ;
		return ops_false ;
	}

	int i;
	for(i=0;i<keyring->nkeys;++i)
//		if(keyring->keys[i].key.pkey.algorithm == OPS_PKA_RSA)
			if(write_all_packets)
				ops_write_transferable_public_key_from_packet_data(&keyring->keys[i],armoured,info) ;
			else
				ops_write_transferable_public_key(&keyring->keys[i],armoured,info) ;
//		else
//		{
//			fprintf(stdout, "ops_write_keyring: not writing key. Algorithm not handled: ") ;
//			ops_print_public_keydata(&keyring->keys[i]);
//			fprintf(stdout, "\n") ;
//		}

	ops_teardown_file_write(info, fd);

	return ops_true ;
}

/*\@}*/

// eof

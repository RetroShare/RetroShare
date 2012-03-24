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

#include <openpgpsdk/armour.h>
#include <openpgpsdk/create.h>
#include <openpgpsdk/signature.h>
#include <openpgpsdk/version.h>

#include <openpgpsdk/final.h>

static int debug=0;

/**
 * \struct dash_escaped_arg_t
 */
typedef struct
    {
    ops_boolean_t seen_nl:1;
    ops_boolean_t seen_cr:1;
    ops_create_signature_t *sig;
    ops_memory_t *trailing;
    } dash_escaped_arg_t;

static ops_boolean_t dash_escaped_writer(const unsigned char *src,
					 unsigned length,
					 ops_error_t **errors,
					 ops_writer_info_t *winfo)
    {
    dash_escaped_arg_t *arg=ops_writer_get_arg(winfo);
    unsigned n;

    if (debug)
        {
        unsigned int i=0;
        fprintf(stderr,"dash_escaped_writer writing %d:\n", length);
        for (i=0; i<length; i++)
            {
            fprintf(stderr,"0x%02x ", src[i]);
            if (!((i+1) % 16))
                fprintf(stderr,"\n");
            else if (!((i+1) % 8))
                fprintf(stderr,"  ");
            }
        fprintf(stderr,"\n");
        }

    // XXX: make this efficient
    for(n=0 ; n < length ; ++n)
	{
	unsigned l;

	if(arg->seen_nl)
	    {
	    if(src[n] == '-' && !ops_stacked_write("- ",2,errors,winfo))
		return ops_false;
	    arg->seen_nl=ops_false;
	    }

	arg->seen_nl=src[n] == '\n';

	if(arg->seen_nl && !arg->seen_cr)
	    {
	    if(!ops_stacked_write("\r",1,errors,winfo))
		return ops_false;
	    ops_signature_add_data(arg->sig,"\r",1);
	    }

	arg->seen_cr=src[n] == '\r';

	if(!ops_stacked_write(&src[n],1,errors,winfo))
	    return ops_false;

	/* trailing whitespace isn't included in the signature */
	if(src[n] == ' ' || src[n] == '\t')
	    ops_memory_add(arg->trailing,&src[n],1);
	else
	    {
	    if((l=ops_memory_get_length(arg->trailing)))
		{
		if(!arg->seen_nl && !arg->seen_cr)
		    ops_signature_add_data(arg->sig,
					   ops_memory_get_data(arg->trailing),
					   l);
		ops_memory_clear(arg->trailing);
		}
	    ops_signature_add_data(arg->sig,&src[n],1);
	    }
	}

    return ops_true;
    }

/**
 * \param winfo
 */
static void dash_escaped_destroyer(ops_writer_info_t *winfo)
    {
    dash_escaped_arg_t *arg=ops_writer_get_arg(winfo);

    ops_memory_free(arg->trailing);
    free(arg);
    }

/**
 * \ingroup Core_WritersNext
 * \brief Push Clearsigned Writer onto stack
 * \param info
 * \param sig
 */
ops_boolean_t ops_writer_push_clearsigned(ops_create_info_t *info,
				  ops_create_signature_t *sig)
    {
    static char header[]="-----BEGIN PGP SIGNED MESSAGE-----\r\nHash: ";
    const char *hash=ops_text_from_hash(ops_signature_get_hash(sig));
    dash_escaped_arg_t *arg=ops_mallocz(sizeof *arg);

    ops_boolean_t rtn;

    rtn= ( ops_write(header,sizeof header-1,info)
           && ops_write(hash,strlen(hash),info)
           && ops_write("\r\n\r\n",4,info));
      
    if (rtn==ops_false)
        {
        OPS_ERROR(&info->errors, OPS_E_W, "Error pushing clearsigned header");
        free(arg);
        return rtn;
        }

    arg->seen_nl=ops_true;
    arg->sig=sig;
    arg->trailing=ops_memory_new();
    ops_writer_push(info,dash_escaped_writer,NULL,dash_escaped_destroyer,arg);
    return rtn;
    }


/**
 * \struct base64_arg_t
 */
typedef struct
    {
    unsigned pos;
    unsigned char t;
    unsigned checksum;
    } base64_arg_t;

static char b64map[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static ops_boolean_t base64_writer(const unsigned char *src,
				   unsigned length,ops_error_t **errors,
				   ops_writer_info_t *winfo)
    {
    base64_arg_t *arg=ops_writer_get_arg(winfo);
    unsigned n;

    for(n=0 ; n < length ; )
	{
	arg->checksum=ops_crc24(arg->checksum,src[n]);
	if(arg->pos == 0)
	    {
	    /* XXXXXX00 00000000 00000000 */
	    if(!ops_stacked_write(&b64map[src[n] >> 2],1,errors,winfo))
		return ops_false;

	    /* 000000XX xxxx0000 00000000 */
	    arg->t=(src[n++]&3) << 4;
	    arg->pos=1;
	    }
	else if(arg->pos == 1)
	    {
	    /* 000000xx XXXX0000 00000000 */
	    arg->t+=src[n] >> 4;
	    if(!ops_stacked_write(&b64map[arg->t],1,errors,winfo))
		return ops_false;

	    /* 00000000 0000XXXX xx000000 */
	    arg->t=(src[n++]&0xf) << 2;
	    arg->pos=2;
	    }
	else if(arg->pos == 2)
	    {
	    /* 00000000 0000xxxx XX000000 */
	    arg->t+=src[n] >> 6;
	    if(!ops_stacked_write(&b64map[arg->t],1,errors,winfo))
		return ops_false;

	    /* 00000000 00000000 00XXXXXX */
	    if(!ops_stacked_write(&b64map[src[n++]&0x3f],1,errors,winfo))
		return ops_false;

	    arg->pos=0;
	    }
	}

    return ops_true;
    }

static ops_boolean_t signature_finaliser(ops_error_t **errors,
					 ops_writer_info_t *winfo)
    {
    base64_arg_t *arg=ops_writer_get_arg(winfo);
    static char trailer[]="\r\n-----END PGP SIGNATURE-----\r\n";
    unsigned char c[3];

    if(arg->pos)
	{
	if(!ops_stacked_write(&b64map[arg->t],1,errors,winfo))
	    return ops_false;
	if(arg->pos == 1 && !ops_stacked_write("==",2,errors,winfo))
	    return ops_false;
	if(arg->pos == 2 && !ops_stacked_write("=",1,errors,winfo))
	    return ops_false;
	}
    /* Ready for the checksum */
    if(!ops_stacked_write("\r\n=",3,errors,winfo))
	return ops_false;

    arg->pos=0; /* get ready to write the checksum */

    c[0]=arg->checksum >> 16;
    c[1]=arg->checksum >> 8;
    c[2]=arg->checksum;
    /* push the checksum through our own writer */
    if(!base64_writer(c,3,errors,winfo))
	return ops_false;

    return ops_stacked_write(trailer,sizeof trailer-1,errors,winfo);
    }

/**
 * \struct linebreak_arg_t
 */
typedef struct
    {
    unsigned pos;
    } linebreak_arg_t;

#define BREAKPOS	76

static ops_boolean_t linebreak_writer(const unsigned char *src,
					 unsigned length,
					 ops_error_t **errors,
					 ops_writer_info_t *winfo)
    {
    linebreak_arg_t *arg=ops_writer_get_arg(winfo);
    unsigned n;

    for(n=0 ; n < length ; ++n,++arg->pos)
	{
	if(src[n] == '\r' || src[n] == '\n')
	    arg->pos=0;

	if(arg->pos == BREAKPOS)
	    {
	    if(!ops_stacked_write("\r\n",2,errors,winfo))
		return ops_false;
	    arg->pos=0;
	    }
	if(!ops_stacked_write(&src[n],1,errors,winfo))
	    return ops_false;
	}

    return ops_true;
    }

/**
 * \ingroup Core_WritersNext
 * \brief Push armoured signature on stack
 * \param info
 */
ops_boolean_t ops_writer_switch_to_armoured_signature(ops_create_info_t *info)
    {
    static char header[]="\r\n-----BEGIN PGP SIGNATURE-----\r\nVersion: "
	OPS_VERSION_STRING "\r\n\r\n";
    base64_arg_t *base64;

    ops_writer_pop(info);
    if (ops_write(header,sizeof header-1,info)==ops_false)
        {
        OPS_ERROR(&info->errors, OPS_E_W, "Error switching to armoured signature");
        return ops_false;
        }

    ops_writer_push(info,linebreak_writer,NULL,ops_writer_generic_destroyer,
		    ops_mallocz(sizeof(linebreak_arg_t)));

    base64=ops_mallocz(sizeof *base64);
    if (!base64)
        {
        OPS_MEMORY_ERROR(&info->errors);
        return ops_false;
        }
    base64->checksum=CRC24_INIT;
    ops_writer_push(info,base64_writer,signature_finaliser,
		    ops_writer_generic_destroyer,base64);
    return ops_true;
    }

static ops_boolean_t armoured_message_finaliser(ops_error_t **errors,
					 ops_writer_info_t *winfo)
    {
    // TODO: This is same as signature_finaliser apart from trailer.
    base64_arg_t *arg=ops_writer_get_arg(winfo);
    static char trailer[]="\r\n-----END PGP MESSAGE-----\r\n";
    unsigned char c[3];

    if(arg->pos)
	{
	if(!ops_stacked_write(&b64map[arg->t],1,errors,winfo))
	    return ops_false;
	if(arg->pos == 1 && !ops_stacked_write("==",2,errors,winfo))
	    return ops_false;
	if(arg->pos == 2 && !ops_stacked_write("=",1,errors,winfo))
	    return ops_false;
	}
    /* Ready for the checksum */
    if(!ops_stacked_write("\r\n=",3,errors,winfo))
	return ops_false;

    arg->pos=0; /* get ready to write the checksum */

    c[0]=arg->checksum >> 16;
    c[1]=arg->checksum >> 8;
    c[2]=arg->checksum;
    /* push the checksum through our own writer */
    if(!base64_writer(c,3,errors,winfo))
	return ops_false;

    return ops_stacked_write(trailer,sizeof trailer-1,errors,winfo);
    }

/**
 \ingroup Core_WritersNext
 \brief Write a PGP MESSAGE 
 \todo replace with generic function
*/
void ops_writer_push_armoured_message(ops_create_info_t *info)
//				  ops_create_signature_t *sig)
    {
    static char header[]="-----BEGIN PGP MESSAGE-----\r\n";

    base64_arg_t *base64;

    ops_write(header,sizeof header-1,info);
    ops_write("\r\n",2,info);
    base64=ops_mallocz(sizeof *base64);
    base64->checksum=CRC24_INIT;
    ops_writer_push(info,base64_writer,armoured_message_finaliser,ops_writer_generic_destroyer,base64);
    }

static ops_boolean_t armoured_finaliser(ops_armor_type_t type, ops_error_t **errors,
					 ops_writer_info_t *winfo)
    {
    static char tail_public_key[]="\r\n-----END PGP PUBLIC KEY BLOCK-----\r\n";
    static char tail_private_key[]="\r\n-----END PGP PRIVATE KEY BLOCK-----\r\n";

    char* tail=NULL;
    unsigned int sz_tail=0;

    switch(type)
        {
    case OPS_PGP_PUBLIC_KEY_BLOCK:
        tail=tail_public_key;
        sz_tail=sizeof tail_public_key-1;
        break;

    case OPS_PGP_PRIVATE_KEY_BLOCK:
        tail=tail_private_key;
        sz_tail=sizeof tail_private_key-1;
        break;

    default:
        assert(0);
        }

    base64_arg_t *arg=ops_writer_get_arg(winfo);
    unsigned char c[3];

    if(arg->pos)
	{
	if(!ops_stacked_write(&b64map[arg->t],1,errors,winfo))
	    return ops_false;
	if(arg->pos == 1 && !ops_stacked_write("==",2,errors,winfo))
	    return ops_false;
	if(arg->pos == 2 && !ops_stacked_write("=",1,errors,winfo))
	    return ops_false;
	}

    /* Ready for the checksum */
    if(!ops_stacked_write("\r\n=",3,errors,winfo))
	return ops_false;

    arg->pos=0; /* get ready to write the checksum */

    c[0]=arg->checksum >> 16;
    c[1]=arg->checksum >> 8;
    c[2]=arg->checksum;
    /* push the checksum through our own writer */
    if(!base64_writer(c,3,errors,winfo))
	return ops_false;

    return ops_stacked_write(tail,sz_tail,errors,winfo);
    }

static ops_boolean_t armoured_public_key_finaliser(ops_error_t **errors,
					 ops_writer_info_t *winfo)
    {
    return armoured_finaliser(OPS_PGP_PUBLIC_KEY_BLOCK,errors,winfo);
    }

static ops_boolean_t armoured_private_key_finaliser(ops_error_t **errors,
					 ops_writer_info_t *winfo)
    {
    return armoured_finaliser(OPS_PGP_PRIVATE_KEY_BLOCK,errors,winfo);
    }

// \todo use this for other armoured types
/**
 \ingroup Core_WritersNext
 \brief Push Armoured Writer on stack (generic)
*/
void ops_writer_push_armoured(ops_create_info_t *info, ops_armor_type_t type)
    {
    static char hdr_public_key[]="-----BEGIN PGP PUBLIC KEY BLOCK-----\r\nVersion: "
        OPS_VERSION_STRING "\r\n\r\n";
    static char hdr_private_key[]="-----BEGIN PGP PRIVATE KEY BLOCK-----\r\nVersion: "
        OPS_VERSION_STRING "\r\n\r\n";

    char* header=NULL;
    unsigned int sz_hdr=0;
    ops_boolean_t (* finaliser)(ops_error_t **errors, ops_writer_info_t *winfo);

    switch(type)
        {
    case OPS_PGP_PUBLIC_KEY_BLOCK:
        header=hdr_public_key;
        sz_hdr=sizeof hdr_public_key-1;
        finaliser=armoured_public_key_finaliser;
        break;

    case OPS_PGP_PRIVATE_KEY_BLOCK:
        header=hdr_private_key;
        sz_hdr=sizeof hdr_private_key-1;
        finaliser=armoured_private_key_finaliser;
        break;

    default:
        assert(0);
        }

    ops_write(header,sz_hdr,info);

    ops_writer_push(info,linebreak_writer,NULL,ops_writer_generic_destroyer,
		    ops_mallocz(sizeof(linebreak_arg_t)));

    base64_arg_t *arg=ops_mallocz(sizeof *arg);
    arg->checksum=CRC24_INIT;
    ops_writer_push(info,base64_writer,finaliser,ops_writer_generic_destroyer,arg);
    }


// EOF

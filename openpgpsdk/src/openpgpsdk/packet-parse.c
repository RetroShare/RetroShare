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
static const size_t MAX_RECURSIVE_COMPRESSION_DEPTH = 32 ;

/**
 * limited_read_data reads the specified amount of the subregion's data 
 * into a data_t structure
 *
 * \param data	Empty structure which will be filled with data
 * \param len	Number of octets to read
 * \param subregion
 * \param pinfo	How to parse
 *
 * \return 1 on success, 0 on failure
 */
static int limited_read_data(ops_data_t *data,unsigned int len,
			     ops_region_t *subregion,ops_parse_info_t *pinfo)
{
   data->len = len;

   if(!(subregion->length-subregion->length_read >= len)) // ASSERT(subregion->length-subregion->length_read >= len);
   {
      fprintf(stderr,"Data length error: announced size %d larger than expected size %d. Giving up.",len,subregion->length-subregion->length_read) ;
      return 0 ;
   }

   data->contents=malloc(data->len);
   if (!data->contents)
      return 0;

   if (!ops_limited_read(data->contents, data->len,subregion,&pinfo->errors,
	    &pinfo->rinfo,&pinfo->cbinfo))
      return 0;

   return 1;
}

/**
 * read_data reads the remainder of the subregion's data 
 * into a data_t structure
 *
 * \param data
 * \param subregion
 * \param pinfo
 * 
 * \return 1 on success, 0 on failure
 */
static int read_data(ops_data_t *data,ops_region_t *subregion,
		     ops_parse_info_t *pinfo)
    {
    int len;

    len=subregion->length-subregion->length_read;

    if ( len >= 0 ) {
        return(limited_read_data(data,len,subregion,pinfo));
    }
    return 0;
    }

/**
 * Reads the remainder of the subregion as a string.
 * It is the user's responsibility to free the memory allocated here.
 */

static int read_unsigned_string(unsigned char **str,ops_region_t *subregion,
				ops_parse_info_t *pinfo)
    {
    int len=0;

    len=subregion->length-subregion->length_read;

    *str=malloc(len+1);
    if(!(*str))
	return 0;

    if(len && !ops_limited_read(*str,len,subregion,&pinfo->errors,
				&pinfo->rinfo,&pinfo->cbinfo))
	return 0;

    /*! ensure the string is NULL-terminated */

    (*str)[len]='\0';

    return 1;
    }

static int read_string(char **str, ops_region_t *subregion, ops_parse_info_t *pinfo)
    {
    return (read_unsigned_string((unsigned char **)str, subregion, pinfo));
    }

void ops_init_subregion(ops_region_t *subregion,ops_region_t *region)
    {
    memset(subregion,'\0',sizeof *subregion);
    subregion->parent=region;
    }

/*! macro to save typing */
#define C		content.content

/* XXX: replace ops_ptag_t with something more appropriate for limiting
   reads */

/**
 * low-level function to read data from reader function
 *
 * Use this function, rather than calling the reader directly.
 *
 * If the accumulate flag is set in *pinfo, the function
 * adds the read data to the accumulated data, and updates 
 * the accumulated length. This is useful if, for example, 
 * the application wants access to the raw data as well as the
 * parsed data.
 *
 * This function will also try to read the entire amount asked for, but not
 * if it is over INT_MAX. Obviously many callers will know that they
 * never ask for that much and so can avoid the extra complexity of
 * dealing with return codes and filled-in lengths.
 *
 * \param *dest
 * \param *plength
 * \param flags
 * \param *pinfo
 *
 * \return OPS_R_OK
 * \return OPS_R_PARTIAL_READ
 * \return OPS_R_EOF
 * \return OPS_R_EARLY_EOF
 * 
 * \sa #ops_reader_ret_t for details of return codes
 */

static int sub_base_read(void *dest,size_t length,ops_error_t **errors,
			 ops_reader_info_t *rinfo,ops_parse_cb_info_t *cbinfo)
{
   size_t n;

   /* reading more than this would look like an error */
   if(length > INT_MAX)
      length=INT_MAX;

   for(n=0 ; n < length ; )
   {
      int r=rinfo->reader((char*)dest+n,length-n,errors,rinfo,cbinfo);

      if(!(r <= (int)(length-n))) // ASSERT(r <= (int)(length-n))
      {
	 fprintf(stderr,"sub_base_read: error in length. Read %d, remaining length is %d",r,(int)(length-n)) ;
	 return -1 ;
      }

      // XXX: should we save the error and return what was read so far?
      //
      if(r < 0)
	 return r;

      if(r == 0)
	 break;

      n+=r;
   }

   if(n == 0)
      return 0;

   if(rinfo->accumulate)
   {
      if(!(rinfo->asize >= rinfo->alength)) // ASSERT(rinfo->asize >= rinfo->alength)
      {
	 fprintf(stderr,"sub_base_read: error in accumulated length.") ;
	 return -1 ;
      }

      if(rinfo->alength+n > rinfo->asize)
      {
	 rinfo->asize=rinfo->asize*2+n;
	 rinfo->accumulated=realloc(rinfo->accumulated,rinfo->asize);
      }
      if(!(rinfo->asize >= rinfo->alength+n)) // ASSERT(rinfo->asize >= rinfo->alength+n)
      {
	 fprintf(stderr,"sub_base_read: error in accumulated length.") ;
	 return -1 ;
      }

      memcpy(rinfo->accumulated+rinfo->alength,dest,n);
   }
   // we track length anyway, because it is used for packet offsets
   rinfo->alength+=n;
   // and also the position
   rinfo->position+=n;

   return n;
}

int ops_stacked_read(void *dest,size_t length,ops_error_t **errors,
		     ops_reader_info_t *rinfo,ops_parse_cb_info_t *cbinfo)
    { return sub_base_read(dest,length,errors,rinfo->next,cbinfo); }

/* This will do a full read so long as length < MAX_INT */
static int base_read(unsigned char *dest,size_t length,
		     ops_parse_info_t *pinfo)
    {
    return sub_base_read(dest,length,&pinfo->errors,&pinfo->rinfo,
			 &pinfo->cbinfo);
    }

/* Read a full size_t's worth. If the return is < than length, then
 * *last_read tells you why - < 0 for an error, == 0 for EOF */

static size_t full_read(unsigned char *dest,size_t length,int *last_read,
			ops_error_t **errors,ops_reader_info_t *rinfo,
			ops_parse_cb_info_t *cbinfo)
    {
    size_t t;
    int r=0; /* preset in case some loon calls with length == 0 */

    for(t=0 ; t < length ; )
	{
	r=sub_base_read(dest+t,length-t,errors,rinfo,cbinfo);

	if(r <= 0)
	    {
	    *last_read=r;
	    return t;
	    }

	t+=r;
	}

    *last_read=r;

    return t;
    }
	
	

/** Read a scalar value of selected length from reader.
 *
 * Read an unsigned scalar value from reader in Big Endian representation.
 *
 * This function does not know or care about packet boundaries. It
 * also assumes that an EOF is an error.
 *
 * \param *result	The scalar value is stored here
 * \param *reader	Our reader
 * \param length	How many bytes to read
 * \return		ops_true on success, ops_false on failure
 */
static ops_boolean_t _read_scalar(unsigned *result,unsigned length,
				    ops_parse_info_t *pinfo)
    {
    unsigned t=0;

    if(! (length <= sizeof(*result))) // ASSERT(length <= sizeof(*result))
    {
       fprintf(stderr,"_read_scalar: length to read is larger than buffer size.") ;
       return ops_false ;
    }

    while(length--)
	{
	unsigned char c[1];
	int r;

	r=base_read(c,1,pinfo);
	if(r != 1)
	    return ops_false;
	t=(t << 8)+c[0];
	}

    *result=t;
    return ops_true;
    }

/** 
 * \ingroup Core_ReadPackets
 * \brief Read bytes from a region within the packet.
 *
 * Read length bytes into the buffer pointed to by *dest.  
 * Make sure we do not read over the packet boundary.  
 * Updates the Packet Tag's ops_ptag_t::length_read.
 *
 * If length would make us read over the packet boundary, or if
 * reading fails, we call the callback with an error.
 *
 * Note that if the region is indeterminate, this can return a short
 * read - check region->last_read for the length. EOF is indicated by
 * a success return and region->last_read == 0 in this case (for a
 * region of known length, EOF is an error).
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param dest		The destination buffer
 * \param length	How many bytes to read
 * \param region	Pointer to packet region
 * \param errors    Error stack
 * \param rinfo		Reader info
 * \param cbinfo	Callback info
 * \return		ops_true on success, ops_false on error
 */
ops_boolean_t ops_limited_read(unsigned char *dest,size_t length,
			       ops_region_t *region,ops_error_t **errors,
			       ops_reader_info_t *rinfo,
			       ops_parse_cb_info_t *cbinfo)
{
   size_t r;
   int lr;

   if(!region->indeterminate && region->length_read+length > region->length)
   {
      OPS_ERROR(errors,OPS_E_P_NOT_ENOUGH_DATA,"Not enough data");
      return ops_false;
   }

   r=full_read(dest,length,&lr,errors,rinfo,cbinfo);

   if(lr < 0)
   {
      OPS_ERROR(errors,OPS_E_R_READ_FAILED,"Read failed");
      return ops_false;
   }

   if(!region->indeterminate && r != length)
   {
      OPS_ERROR(errors,OPS_E_R_READ_FAILED,"Read failed");
      return ops_false;
   }

   region->last_read=r;
   do
   {
      region->length_read+=r;

      if(!(!region->parent || region->length <= region->parent->length)) // ASSERT(!region->parent || region->length <= region->parent->length)
      {
	 OPS_ERROR(errors,OPS_E_R_READ_FAILED,"Read failed");
	 return ops_false;
      }
   }
   while((region=region->parent));

   return ops_true;
}

/**
   \ingroup Core_ReadPackets
   \brief Call ops_limited_read on next in stack
*/
ops_boolean_t ops_stacked_limited_read(void *dest, unsigned length,
				       ops_region_t *region,
				       ops_error_t **errors,
				       ops_reader_info_t *rinfo,
				       ops_parse_cb_info_t *cbinfo)
    {
    return ops_limited_read(dest, length, region, errors, rinfo->next, cbinfo);
    }

static ops_boolean_t limited_read(unsigned char *dest,unsigned length,
				  ops_region_t *region,ops_parse_info_t *info)
    {
    return ops_limited_read(dest,length,region,&info->errors,
			    &info->rinfo,&info->cbinfo);
    }

static ops_boolean_t exact_limited_read(unsigned char *dest,unsigned length,
					ops_region_t *region,
					ops_parse_info_t *pinfo)
    {
    ops_boolean_t ret;

    pinfo->exact_read=ops_true;
    ret=limited_read(dest,length,region,pinfo);
    pinfo->exact_read=ops_false;

    return ret;
    }

/** Skip over length bytes of this packet.
 *
 * Calls limited_read() to skip over some data.
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param length	How many bytes to skip
 * \param *region	Pointer to packet region
 * \param *pinfo	How to parse
 * \return		1 on success, 0 on error (calls the cb with OPS_PARSER_ERROR in limited_read()).
 */
static int limited_skip(unsigned length,ops_region_t *region,
			ops_parse_info_t *pinfo)
    {
    unsigned char buf[8192];

    while(length)
	{
	int n=length%8192;
	if(!limited_read(buf,n,region,pinfo))
	    return 0;
	length-=n;
	}
    return 1;
    }

/** Read a scalar.
 *
 * Read a big-endian scalar of length bytes, respecting packet
 * boundaries (by calling limited_read() to read the raw data).
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param *dest		The scalar value is stored here
 * \param length	How many bytes make up this scalar (at most 4)
 * \param *region	Pointer to current packet region
 * \param *pinfo	How to parse
 * \param *cb		The callback
 * \return		1 on success, 0 on error (calls the cb with OPS_PARSER_ERROR in limited_read()).
 *
 * \see RFC4880 3.1
 */
static int limited_read_scalar(unsigned *dest,unsigned length,
			       ops_region_t *region,
			       ops_parse_info_t *pinfo)
{
   unsigned char c[4]="";
   unsigned t;
   unsigned n;

   if(!(length <= 4)) // ASSERT(length <= 4)
   {
	fprintf(stderr,"limited_read_scalar: wrong size for scalar %d\n",length) ;
	return ops_false ;
   }
   if(!(sizeof(*dest) >= 4)) // ASSERT(sizeof(*dest) >= 4)
   {
	fprintf(stderr,"limited_read_scalar: wrong size for dest %lu\n",sizeof(*dest)) ;
	return ops_false ;
   }
   if(!limited_read(c,length,region,pinfo))
      return 0;

   for(t=0,n=0 ; n < length ; ++n)
      t=(t << 8)+c[n];
   *dest=t;

   return 1;
}

/** Read a scalar.
 *
 * Read a big-endian scalar of length bytes, respecting packet
 * boundaries (by calling limited_read() to read the raw data).
 *
 * The value read is stored in a size_t, which is a different size
 * from an unsigned on some platforms.
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param *dest		The scalar value is stored here
 * \param length	How many bytes make up this scalar (at most 4)
 * \param *region	Pointer to current packet region
 * \param *pinfo	How to parse
 * \param *cb		The callback
 * \return		1 on success, 0 on error (calls the cb with OPS_PARSER_ERROR in limited_read()).
 *
 * \see RFC4880 3.1
 */
static int limited_read_size_t_scalar(size_t *dest,unsigned length,
				      ops_region_t *region,
				      ops_parse_info_t *pinfo)
{
   unsigned tmp;

   if(!(sizeof(*dest) >= 4)) // ASSERT(sizeof(*dest) >= 4)
   {
	fprintf(stderr,"limited_read_scalar: wrong dest size for scalar %lu\n",sizeof(*dest)) ;
	return ops_false ;
   }

   /* Note that because the scalar is at most 4 bytes, we don't care
      if size_t is bigger than usigned */
   if(!limited_read_scalar(&tmp,length,region,pinfo))
      return 0;

   *dest=tmp;
   return 1;
}

/** Read a timestamp.
 *
 * Timestamps in OpenPGP are unix time, i.e. seconds since The Epoch (1.1.1970).  They are stored in an unsigned scalar
 * of 4 bytes.
 *
 * This function reads the timestamp using limited_read_scalar().
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param *dest		The timestamp is stored here
 * \param *ptag		Pointer to current packet's Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		see limited_read_scalar()
 *
 * \see RFC4880 3.5
 */
static int limited_read_time(time_t *dest,ops_region_t *region,
			     ops_parse_info_t *pinfo)
{
   /*
    * Cannot assume that time_t is 4 octets long - 
    * there is at least one architecture (SunOS 5.10) where it is 8.
    */
   if (sizeof(*dest)==4)
   {
      return limited_read_scalar((unsigned *)dest,4,region,pinfo);
   }
   else
   {
      time_t mytime=0;
      int i=0;
      unsigned char c[1];
      for (i=0; i<4; i++)
      {
	 if (!limited_read(c,1,region,pinfo))
	    return 0;
	 mytime=(mytime << 8) + c[0];
      }
      *dest=mytime;
      return 1;
   }
}

/** 
 * \ingroup Core_MPI
 * Read a multiprecision integer.
 *
 * Large numbers (multiprecision integers, MPI) are stored in OpenPGP in two parts.  First there is a 2 byte scalar
 * indicating the length of the following MPI in Bits.  Then follow the bits that make up the actual number, most
 * significant bits first (Big Endian).  The most significant bit in the MPI is supposed to be 1 (unless the MPI is
 * encrypted - then it may be different as the bit count refers to the plain text but the bits are encrypted).
 *
 * Unused bits (i.e. those filling up the most significant byte from the left to the first bits that counts) are
 * supposed to be cleared - I guess. XXX - does anything actually say so?
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param **pgn		return the integer there - the BIGNUM is created by BN_bin2bn() and probably needs to be freed
 * 				by the caller XXX right ben?
 * \param *ptag		Pointer to current packet's Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error (by limited_read_scalar() or limited_read() or if the MPI is not properly formed (XXX
 * 				 see comment below - the callback is called with a OPS_PARSER_ERROR in case of an error)
 *
 * \see RFC4880 3.2
 */
static int limited_read_mpi(BIGNUM **pbn,ops_region_t *region,
			    ops_parse_info_t *pinfo)
    {
    unsigned length;
    unsigned nonzero;
    unsigned char buf[8192]=""; /* an MPI has a 2 byte length part.  Length
                                is given in bits, so the largest we should
                                ever need for the buffer is 8192 bytes. */
    ops_boolean_t ret;

    pinfo->reading_mpi_length=ops_true;
    ret=limited_read_scalar(&length,2,region,pinfo);

    pinfo->reading_mpi_length=ops_false;
    if(!ret)
	return 0;

    nonzero=length&7; /* there should be this many zero bits in the MS byte */
    if(!nonzero)
	nonzero=8;
    length=(length+7)/8;

    if(!(length <= 8192)) // ASSERT(length <= 8192)
    {
       fprintf(stderr,"limited_read_mpi: wrong size to read %d > 8192",length) ;
       return 0 ;
    }

    if(!limited_read(buf,length,region,pinfo))
	return 0;

    if((buf[0] >> nonzero) != 0 || !(buf[0]&(1 << (nonzero-1))))
	{
	OPS_ERROR(&pinfo->errors,OPS_E_P_MPI_FORMAT_ERROR,"MPI Format error");  /* XXX: Ben, one part of this constraint does not apply to encrypted MPIs the draft says. -- peter */
	return 0;
	}

    *pbn=BN_bin2bn(buf,length,NULL);
    return 1;
    }

/** Read some data with a New-Format length from reader.
 *
 * \sa Internet-Draft RFC4880.txt Section 4.2.2
 *
 * \param *length	Where the decoded length will be put
 * \param *pinfo	How to parse
 * \return		ops_true if OK, else ops_false
 *
 */

static ops_boolean_t read_new_length(unsigned *length,ops_parse_info_t *pinfo)
    {
    unsigned char c[1];

    if(base_read(c,1,pinfo) != 1)
	return ops_false;
    if(c[0] < 192)
	{
    // 1. One-octet packet
	*length=c[0];
	return ops_true;
	}

    else if (c[0]>=192 && c[0]<=223)
        {
        // 2. Two-octet packet
        unsigned t=(c[0]-192) << 8;
        
        if(base_read(c,1,pinfo) != 1)
            return ops_false;
        *length=t+c[0]+192;
        return ops_true;
        }

    else if (c[0]==255)
        {
        // 3. Five-Octet packet
        return _read_scalar(length,4,pinfo);
        }

    else if (c[0]>=224 && c[0]<255)
        {
        // 4. Partial Body Length
        OPS_ERROR(&pinfo->errors,OPS_E_UNIMPLEMENTED,
                    "New format Partial Body Length fields not yet implemented");
        return ops_false;
        }
    return ops_false;
    }

/** Read the length information for a new format Packet Tag.
 *
 * New style Packet Tags encode the length in one to five octets.  This function reads the right amount of bytes and
 * decodes it to the proper length information.
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param *length	return the length here
 * \param *ptag		Pointer to current packet's Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error (by limited_read_scalar() or limited_read() or if the MPI is not properly formed (XXX
 * 				 see comment below)
 *
 * \see RFC4880 4.2.2
 * \see ops_ptag_t
 */
static int limited_read_new_length(unsigned *length,ops_region_t *region,
				   ops_parse_info_t *pinfo)
    {
    unsigned char c[1]="";

    if(!limited_read(c,1,region,pinfo))
	return 0;
    if(c[0] < 192)
	{
	*length=c[0];
	return 1;
	}
    if(c[0] < 255)
	{
	unsigned t=(c[0]-192) << 8;

	if(!limited_read(c,1,region,pinfo))
	    return 0;
	*length=t+c[0]+192;
	return 1;
	}
    return limited_read_scalar(length,4,region,pinfo);
    }

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void data_free(ops_data_t *data)
    {
    free(data->contents);
    data->contents=NULL;
    data->len=0;
    }

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void string_free(char **str)
    {
    free(*str);
    *str=NULL;
    }

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/*! Free packet memory, set pointer to NULL */
void ops_packet_free(ops_packet_t *packet)
    {
    free(packet->raw);
    packet->raw=NULL;
    }

/**
\ingroup Core_Create
\brief Free allocated memory
*/
void ops_headers_free(ops_headers_t *headers)
    {
    unsigned n;

    for(n=0 ; n < headers->nheaders ; ++n)
	{
	free(headers->headers[n].key);
	free(headers->headers[n].value);
	}
    free(headers->headers);
    headers->headers=NULL;
    }

/**
\ingroup Core_Create
\brief Free allocated memory
*/
void ops_signed_cleartext_trailer_free(ops_signed_cleartext_trailer_t *trailer)
    {
    free(trailer->hash);
    trailer->hash=NULL;
    }

/**
\ingroup Core_Create
\brief Free allocated memory
*/
void ops_cmd_get_passphrase_free(ops_secret_key_passphrase_t *skp)
    {
    if (skp->passphrase && *skp->passphrase)
        {
        free(*skp->passphrase);
        *skp->passphrase=NULL;
        }
    }

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/*! Free any memory allocated when parsing the packet content */
void ops_parser_content_free(ops_parser_content_t *c)
{
   switch(c->tag)
   {
      case OPS_PARSER_PTAG:
      case OPS_PTAG_CT_COMPRESSED:
      case OPS_PTAG_SS_CREATION_TIME:
      case OPS_PTAG_SS_EXPIRATION_TIME:
      case OPS_PTAG_SS_KEY_EXPIRATION_TIME:
      case OPS_PTAG_SS_TRUST:
      case OPS_PTAG_SS_ISSUER_KEY_ID:
      case OPS_PTAG_CT_ONE_PASS_SIGNATURE:
      case OPS_PTAG_SS_PRIMARY_USER_ID:
      case OPS_PTAG_SS_REVOCABLE:
      case OPS_PTAG_SS_REVOCATION_KEY:
      case OPS_PTAG_CT_LITERAL_DATA_HEADER:
      case OPS_PTAG_CT_LITERAL_DATA_BODY:
      case OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY:
      case OPS_PTAG_CT_UNARMOURED_TEXT:
      case OPS_PTAG_CT_ARMOUR_TRAILER:
      case OPS_PTAG_CT_SIGNATURE_HEADER:
      case OPS_PTAG_CT_SE_DATA_HEADER:
      case OPS_PTAG_CT_SE_IP_DATA_HEADER:
      case OPS_PTAG_CT_SE_IP_DATA_BODY:
      case OPS_PTAG_CT_MDC:
      case OPS_PARSER_CMD_GET_SECRET_KEY:
	 break;

      case OPS_PTAG_CT_SIGNED_CLEARTEXT_HEADER:
	 ops_headers_free(&c->content.signed_cleartext_header.headers);
	 break;

      case OPS_PTAG_CT_ARMOUR_HEADER:
	 ops_headers_free(&c->content.armour_header.headers);
	 break;

      case OPS_PTAG_CT_SIGNED_CLEARTEXT_TRAILER:
	 ops_signed_cleartext_trailer_free(&c->content.signed_cleartext_trailer);
	 break;

      case OPS_PTAG_CT_TRUST:
	 ops_trust_free(&c->content.trust);
	 break;

      case OPS_PTAG_CT_SIGNATURE:
      case OPS_PTAG_CT_SIGNATURE_FOOTER:
	 ops_signature_free(&c->content.signature);
	 break;

      case OPS_PTAG_CT_PUBLIC_KEY:
      case OPS_PTAG_CT_PUBLIC_SUBKEY:
	 ops_public_key_free(&c->content.public_key);
	 break;

      case OPS_PTAG_CT_USER_ID:
	 ops_user_id_free(&c->content.user_id);
	 break;

      case OPS_PTAG_SS_SIGNERS_USER_ID:
	 ops_user_id_free(&c->content.ss_signers_user_id);
	 break;

      case OPS_PTAG_CT_USER_ATTRIBUTE:
	 ops_user_attribute_free(&c->content.user_attribute);
	 break;

      case OPS_PTAG_SS_PREFERRED_SKA:
	 ops_ss_preferred_ska_free(&c->content.ss_preferred_ska);
	 break;

      case OPS_PTAG_SS_PREFERRED_HASH:
	 ops_ss_preferred_hash_free(&c->content.ss_preferred_hash);
	 break;

      case OPS_PTAG_SS_PREFERRED_COMPRESSION:
	 ops_ss_preferred_compression_free(&c->content.ss_preferred_compression);
	 break;

      case OPS_PTAG_SS_KEY_FLAGS:
	 ops_ss_key_flags_free(&c->content.ss_key_flags);
	 break;

      case OPS_PTAG_SS_KEY_SERVER_PREFS:
	 ops_ss_key_server_prefs_free(&c->content.ss_key_server_prefs);
	 break;

      case OPS_PTAG_SS_FEATURES:
	 ops_ss_features_free(&c->content.ss_features);
	 break;

      case OPS_PTAG_SS_NOTATION_DATA:
	 ops_ss_notation_data_free(&c->content.ss_notation_data);
	 break;

      case OPS_PTAG_SS_REGEXP:
	 ops_ss_regexp_free(&c->content.ss_regexp);
	 break;

      case OPS_PTAG_SS_POLICY_URI:
	 ops_ss_policy_url_free(&c->content.ss_policy_url);
	 break;

      case OPS_PTAG_SS_PREFERRED_KEY_SERVER:
	 ops_ss_preferred_key_server_free(&c->content.ss_preferred_key_server);
	 break;

      case OPS_PTAG_SS_USERDEFINED00:
      case OPS_PTAG_SS_USERDEFINED01:
      case OPS_PTAG_SS_USERDEFINED02:
      case OPS_PTAG_SS_USERDEFINED03:
      case OPS_PTAG_SS_USERDEFINED04:
      case OPS_PTAG_SS_USERDEFINED05:
      case OPS_PTAG_SS_USERDEFINED06:
      case OPS_PTAG_SS_USERDEFINED07:
      case OPS_PTAG_SS_USERDEFINED08:
      case OPS_PTAG_SS_USERDEFINED09:
      case OPS_PTAG_SS_USERDEFINED10:
	 ops_ss_userdefined_free(&c->content.ss_userdefined);
	 break;

      case OPS_PTAG_SS_RESERVED:
	 ops_ss_reserved_free(&c->content.ss_unknown);
	 break;

      case OPS_PTAG_SS_REVOCATION_REASON:
	 ops_ss_revocation_reason_free(&c->content.ss_revocation_reason);
	 break;

      case OPS_PTAG_SS_EMBEDDED_SIGNATURE:
	 ops_ss_embedded_signature_free(&c->content.ss_embedded_signature);
	 break;

      case OPS_PARSER_PACKET_END:
	 ops_packet_free(&c->content.packet);
	 break;

      case OPS_PARSER_ERROR:
      case OPS_PARSER_ERRCODE:
	 break;

      case OPS_PTAG_CT_SECRET_KEY:
      case OPS_PTAG_CT_ENCRYPTED_SECRET_KEY:
	 ops_secret_key_free(&c->content.secret_key);
	 break;

      case OPS_PTAG_CT_PK_SESSION_KEY:
      case OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY:
	 ops_pk_session_key_free(&c->content.pk_session_key);
	 break;

      case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
      case OPS_PARSER_CMD_GET_SK_PASSPHRASE_PREV_WAS_BAD:
	 ops_cmd_get_passphrase_free(&c->content.secret_key_passphrase);
	 break;

      default:
	 fprintf(stderr,"Can't free %d (0x%x)\n",c->tag,c->tag);
	 //ASSERT(0);
   }
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void free_BN(BIGNUM **pp)
    {
    BN_free(*pp);
    *pp=NULL;
    }

/**
\ingroup Core_Create
\brief Free allocated memory
*/
void ops_pk_session_key_free(ops_pk_session_key_t *sk)
{
   switch(sk->algorithm)
   {
      case OPS_PKA_RSA:
	 free_BN(&sk->parameters.rsa.encrypted_m);
	 break;

      case OPS_PKA_ELGAMAL:
	 free_BN(&sk->parameters.elgamal.g_to_k);
	 free_BN(&sk->parameters.elgamal.encrypted_m);
	 break;

      default:
	 fprintf(stderr,"ops_pk_session_key_free: Unknown algorithm: %d \n",sk->algorithm);
	 //ASSERT(0);
   }
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/*! Free the memory used when parsing a public key */
void ops_public_key_free(ops_public_key_t *p)
{
   switch(p->algorithm)
   {
      case OPS_PKA_RSA:
      case OPS_PKA_RSA_ENCRYPT_ONLY:
      case OPS_PKA_RSA_SIGN_ONLY:
	 free_BN(&p->key.rsa.n);
	 free_BN(&p->key.rsa.e);
	 break;

      case OPS_PKA_DSA:
	 free_BN(&p->key.dsa.p);
	 free_BN(&p->key.dsa.q);
	 free_BN(&p->key.dsa.g);
	 free_BN(&p->key.dsa.y);
	 break;

      case OPS_PKA_ELGAMAL:
      case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
	 free_BN(&p->key.elgamal.p);
	 free_BN(&p->key.elgamal.g);
	 free_BN(&p->key.elgamal.y);
	 break;

      default:
	 fprintf(stderr,"ops_public_key_free: Unknown algorithm: %d \n",p->algorithm);
	 //ASSERT(0);
   }
}

void ops_public_key_copy(ops_public_key_t *dst,const ops_public_key_t *src)
{
   *dst = *src ;

   switch(src->algorithm)
   {
      case OPS_PKA_RSA:
      case OPS_PKA_RSA_ENCRYPT_ONLY:
      case OPS_PKA_RSA_SIGN_ONLY:
	 dst->key.rsa.n = BN_dup(src->key.rsa.n);
	 dst->key.rsa.e = BN_dup(src->key.rsa.e);
	 break;

      case OPS_PKA_DSA:
	 dst->key.dsa.p = BN_dup(src->key.dsa.p);
	 dst->key.dsa.q = BN_dup(src->key.dsa.q);
	 dst->key.dsa.g = BN_dup(src->key.dsa.g);
	 dst->key.dsa.y = BN_dup(src->key.dsa.y);
	 break;

      case OPS_PKA_ELGAMAL:
      case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
	 dst->key.elgamal.p = BN_dup(src->key.elgamal.p);
	 dst->key.elgamal.g = BN_dup(src->key.elgamal.g);
	 dst->key.elgamal.y = BN_dup(src->key.elgamal.y);
	 break;

	 //case 0:
	 // nothing to free
	 //   break;

      default:
	 fprintf(stderr,"ops_public_key_copy: Unknown algorithm: %d \n",src->algorithm);
	 //ASSERT(0);
   }
}
/**
   \ingroup Core_ReadPackets
*/
static int parse_public_key_data(ops_public_key_t *key,ops_region_t *region,
				 ops_parse_info_t *pinfo)
{
   unsigned char c[1]="";

   if(!(region->length_read == 0)) // ASSERT(region->length_read == 0)  /* We should not have read anything so far */
   {
      fprintf(stderr,"parse_public_key_data: read length error\n") ;
      return 0 ;
   }

   if(!limited_read(c,1,region,pinfo))
      return 0;
   key->version=c[0];
   if(key->version < 2 || key->version > 4)
   {
      OPS_ERROR_1(&pinfo->errors,OPS_E_PROTO_BAD_PUBLIC_KEY_VRSN,
	    "Bad public key version (0x%02x)",key->version);
      return 0;
   }

   if(!limited_read_time(&key->creation_time,region,pinfo))
      return 0;

   key->days_valid=0;
   if((key->version == 2 || key->version == 3)
	 && !limited_read_scalar(&key->days_valid,2,region,pinfo))
      return 0;

   if(!limited_read(c,1,region,pinfo))
      return 0;

   key->algorithm=c[0];

   switch(key->algorithm)
   {
      case OPS_PKA_DSA:
	 if(!limited_read_mpi(&key->key.dsa.p,region,pinfo)
	       || !limited_read_mpi(&key->key.dsa.q,region,pinfo)
	       || !limited_read_mpi(&key->key.dsa.g,region,pinfo)
	       || !limited_read_mpi(&key->key.dsa.y,region,pinfo))
	    return 0;
	 break;

      case OPS_PKA_RSA:
      case OPS_PKA_RSA_ENCRYPT_ONLY:
      case OPS_PKA_RSA_SIGN_ONLY:
	 if(!limited_read_mpi(&key->key.rsa.n,region,pinfo)
	       || !limited_read_mpi(&key->key.rsa.e,region,pinfo))
	    return 0;
	 break;

      case OPS_PKA_ELGAMAL:
      case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
	 if(!limited_read_mpi(&key->key.elgamal.p,region,pinfo)
	       || !limited_read_mpi(&key->key.elgamal.g,region,pinfo)
	       || !limited_read_mpi(&key->key.elgamal.y,region,pinfo))
	    return 0;
	 break;

      default:
	 OPS_ERROR_1(&pinfo->errors,OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG,"Unsupported Public Key algorithm (%s)",ops_show_pka(key->algorithm));
	 return 0;
   }

   return 1;
}


/**
 * \ingroup Core_ReadPackets
 * \brief Parse a public key packet.
 *
 * This function parses an entire v3 (== v2) or v4 public key packet for RSA, ElGamal, and DSA keys.
 *
 * Once the key has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the current Packet Tag.  This function should consume the entire packet.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.5.2
 */
static int parse_public_key(ops_content_tag_t tag,ops_region_t *region,
			    ops_parse_info_t *pinfo)
    {
    ops_parser_content_t content;

    if(!parse_public_key_data(&C.public_key,region,pinfo))
	return 0;

    // XXX: this test should be done for all packets, surely?
    if(region->length_read != region->length)
        {
        OPS_ERROR_1(&pinfo->errors,OPS_E_R_UNCONSUMED_DATA,
                    "Unconsumed data (%d)", region->length-region->length_read);
        return 0;
        }

    CBP(pinfo,tag,&content);

    return 1;
    }


/**
\ingroup Core_Create
\brief Free allocated memory
*/
/*! Free the memory used when parsing this signature sub-packet type */
void ops_ss_regexp_free(ops_ss_regexp_t *regexp)
    {
    string_free(&regexp->text);
    }

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/*! Free the memory used when parsing this signature sub-packet type */
void ops_ss_policy_url_free(ops_ss_policy_url_t *policy_url)
    {
    string_free(&policy_url->text);
    }

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/*! Free the memory used when parsing this signature sub-packet type */
void ops_ss_preferred_key_server_free(ops_ss_preferred_key_server_t *preferred_key_server)
    {
    string_free(&preferred_key_server->text);
    }

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/*! Free the memory used when parsing this packet type */
void ops_user_attribute_free(ops_user_attribute_t *user_att)
    {
    data_free(&user_att->data);
    }

/** 
 * \ingroup Core_ReadPackets
 * \brief Parse one user attribute packet.
 *
 * User attribute packets contain one or more attribute subpackets.
 * For now, handle the whole packet as raw data.
 */

static int parse_user_attribute(ops_region_t *region, ops_parse_info_t *pinfo)
{

   ops_parser_content_t content;

   /* xxx- treat as raw data for now. Could break down further
      into attribute sub-packets later - rachel */

   if(!(region->length_read == 0)) // ASSERT(region->length_read == 0)  /* We should not have read anything so far */
   {
      fprintf(stderr,"parse_user_attribute: read length error\n") ;
      return 0 ;
   }

   if(!read_data(&C.user_attribute.data,region,pinfo))
      return 0;

   CBP(pinfo,OPS_PTAG_CT_USER_ATTRIBUTE,&content);

   return 1;
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/*! Free the memory used when parsing this packet type */
void ops_user_id_free(ops_user_id_t *id)
    {
    free(id->user_id);
    id->user_id=NULL;
    }

/** 
 * \ingroup Core_ReadPackets
 * \brief Parse a user id.
 *
 * This function parses an user id packet, which is basically just a char array the size of the packet.
 *
 * The char array is to be treated as an UTF-8 string.
 *
 * The userid gets null terminated by this function.  Freeing it is the responsibility of the caller.
 *
 * Once the userid has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.  This function should consume the entire packet.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.11
 */
static int parse_user_id(ops_region_t *region,ops_parse_info_t *pinfo)
{
   ops_parser_content_t content;

   if(!(region->length_read == 0)) // ASSERT(region->length_read == 0)  /* We should not have read anything so far */
   {
      fprintf(stderr,"parse_user_id: region read size should be 0. Corrupted data ?\n") ;
      return 0 ;
   }

   /* From gnupg parse-packet.c:
      Cap the size of a user ID at 2k: a value absurdly large enough
      that there is no sane user ID string (which is printable text
      as of RFC2440bis) that won't fit in it, but yet small enough to
      avoid allocation problems. */
   
   if(region->length > 2048)
   {
     fprintf(stderr,"parse_user_id(): invalid region length (%u)\n",region->length);
     return 0;
   }
   C.user_id.user_id=malloc(region->length +1);  /* XXX should we not like check malloc's return value? */

   if(C.user_id.user_id==NULL)
   {
      fprintf(stderr,"malloc failed in parse_user_id\n") ;
      return 0 ;
   }

   if(region->length && !limited_read(C.user_id.user_id,region->length,region,
	    pinfo))
      return 0;

   C.user_id.user_id[region->length]='\0'; /* terminate the string */

   CBP(pinfo,OPS_PTAG_CT_USER_ID,&content);

   return 1;
}

/**
 * \ingroup Core_Create
 * \brief Free the memory used when parsing a private/experimental PKA signature 
 * \param unknown_sig
 */
void free_unknown_sig_pka(ops_unknown_signature_t *unknown_sig)
    {
    data_free(&unknown_sig->data);
    }

/**
 * \ingroup Core_Create
 * \brief Free the memory used when parsing a signature 
 * \param sig
 */
void ops_signature_free(ops_signature_t *sig)
    {
    switch(sig->info.key_algorithm)
	{
    case OPS_PKA_RSA:
    case OPS_PKA_RSA_SIGN_ONLY:
	free_BN(&sig->info.signature.rsa.sig);
	break;

    case OPS_PKA_DSA:
	free_BN(&sig->info.signature.dsa.r);
	free_BN(&sig->info.signature.dsa.s);
	break;

    case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
	free_BN(&sig->info.signature.elgamal.r);
	free_BN(&sig->info.signature.elgamal.s);
	break;

    case OPS_PKA_PRIVATE00:
    case OPS_PKA_PRIVATE01:
    case OPS_PKA_PRIVATE02:
    case OPS_PKA_PRIVATE03:
    case OPS_PKA_PRIVATE04:
    case OPS_PKA_PRIVATE05:
    case OPS_PKA_PRIVATE06:
    case OPS_PKA_PRIVATE07:
    case OPS_PKA_PRIVATE08:
    case OPS_PKA_PRIVATE09:
    case OPS_PKA_PRIVATE10:
	free_unknown_sig_pka(&sig->info.signature.unknown);
	break;

    default:
	 fprintf(stderr,"ops_signature_free: Unknown algorithm: %d \n",sig->info.key_algorithm);
	//ASSERT(0);
	}
    free(sig->info.v4_hashed_data);
    }

/**
 * \ingroup Core_Parse
 * \brief Parse a version 3 signature.
 *
 * This function parses an version 3 signature packet, handling RSA and DSA signatures.
 *
 * Once the signature has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.  This function should consume the entire packet.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.2.2
 */
static int parse_v3_signature(ops_region_t *region,
			      ops_parse_info_t *pinfo)
    {
    unsigned char c[1]="";
    ops_parser_content_t content;

    // clear signature
    memset(&C.signature,'\0',sizeof C.signature);

    C.signature.info.version=OPS_V3;

    /* hash info length */
    if(!limited_read(c,1,region,pinfo))
	return 0;
    if(c[0] != 5)
	ERRP(pinfo,"bad hash info length");

    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.signature.info.type=c[0];
    /* XXX: check signature type */

    if(!limited_read_time(&C.signature.info.creation_time,region,pinfo))
	return 0;
    C.signature.info.creation_time_set=ops_true;

    if(!limited_read(C.signature.info.signer_id,OPS_KEY_ID_SIZE,region,pinfo))
	return 0;
    C.signature.info.signer_id_set=ops_true;

    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.signature.info.key_algorithm=c[0];
    /* XXX: check algorithm */

    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.signature.info.hash_algorithm=c[0];
    /* XXX: check algorithm */
    
    if(!limited_read(C.signature.hash2,2,region,pinfo))
	return 0;

    switch(C.signature.info.key_algorithm)
	{
    case OPS_PKA_RSA:
    case OPS_PKA_RSA_SIGN_ONLY:
	if(!limited_read_mpi(&C.signature.info.signature.rsa.sig,region,pinfo))
	    return 0;
	break;

    case OPS_PKA_DSA:
	if(!limited_read_mpi(&C.signature.info.signature.dsa.r,region,pinfo)
	   || !limited_read_mpi(&C.signature.info.signature.dsa.s,region,pinfo))
	    return 0;
	break;

    case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
	if(!limited_read_mpi(&C.signature.info.signature.elgamal.r,region,pinfo)
	   || !limited_read_mpi(&C.signature.info.signature.elgamal.s,region,pinfo))
	    return 0;
	break;

    default:
        OPS_ERROR_1(&pinfo->errors,OPS_E_ALG_UNSUPPORTED_SIGNATURE_ALG,
                    "Unsupported signature key algorithm (%s)",
                    ops_show_pka(C.signature.info.key_algorithm));
        return 0;
	}

    if(region->length_read != region->length)
        {
	OPS_ERROR_1(&pinfo->errors,OPS_E_R_UNCONSUMED_DATA,"Unconsumed data (%d)",region->length-region->length_read);
        return 0;
        }

    if(C.signature.info.signer_id_set)
	C.signature.hash=ops_parse_hash_find(pinfo,C.signature.info.signer_id);

    CBP(pinfo,OPS_PTAG_CT_SIGNATURE,&content);

    return 1;
    }

/**
 * \ingroup Core_ReadPackets
 * \brief Parse one signature sub-packet.
 *
 * Version 4 signatures can have an arbitrary amount of (hashed and unhashed) subpackets.  Subpackets are used to hold
 * optional attributes of subpackets.
 *
 * This function parses one such signature subpacket.
 *
 * Once the subpacket has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.  This function should consume the entire subpacket.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.2.3
 */
static int parse_one_signature_subpacket(ops_signature_t *sig,
					 ops_region_t *region,
					 ops_parse_info_t *pinfo)
    {
    ops_region_t subregion;
    unsigned char c[1]="";
    ops_parser_content_t content;
    unsigned t8,t7;
    ops_boolean_t read=ops_true;
    unsigned char bool[1]="";

    ops_init_subregion(&subregion,region);
    if(!limited_read_new_length(&subregion.length,region,pinfo))
	return 0;

    if(subregion.length > region->length)
	ERRP(pinfo,"Subpacket too long");

    if(!limited_read(c,1,&subregion,pinfo))
	return 0;

    t8=(c[0]&0x7f)/8;
    t7=1 << (c[0]&7);

    content.critical=c[0] >> 7;
    content.tag=OPS_PTAG_SIGNATURE_SUBPACKET_BASE+(c[0]&0x7f);

    /* Application wants it delivered raw */
    if(pinfo->ss_raw[t8]&t7)
	{
	C.ss_raw.tag=content.tag;
	C.ss_raw.length=subregion.length-1;
	C.ss_raw.raw=malloc(C.ss_raw.length);

	if(C.ss_raw.raw==NULL)
	{
	   fprintf(stderr,"malloc failed in parse_signature_subpackets") ;
	   return 0 ;
	}
	if(!limited_read(C.ss_raw.raw,C.ss_raw.length,&subregion,pinfo))
	    return 0;
	CBP(pinfo,OPS_PTAG_RAW_SS,&content);
    return 1;
	}

    switch(content.tag)
	{
    case OPS_PTAG_SS_CREATION_TIME:
    case OPS_PTAG_SS_EXPIRATION_TIME:
    case OPS_PTAG_SS_KEY_EXPIRATION_TIME:
	if(!limited_read_time(&C.ss_time.time,&subregion,pinfo))
	    return 0;
	if(content.tag == OPS_PTAG_SS_CREATION_TIME)
	    {
	    sig->info.creation_time=C.ss_time.time;
	    sig->info.creation_time_set=ops_true;
	    }
	break;

    case OPS_PTAG_SS_TRUST:
	if(!limited_read(&C.ss_trust.level,1,&subregion,pinfo)
	   || !limited_read(&C.ss_trust.amount,1,&subregion,pinfo))
	    return 0;
	break;

    case OPS_PTAG_SS_REVOCABLE:
	if(!limited_read(bool,1,&subregion,pinfo))
	    return 0;
	C.ss_revocable.revocable=!!bool[0];
	break;

    case OPS_PTAG_SS_ISSUER_KEY_ID:
	if(!limited_read(C.ss_issuer_key_id.key_id,OPS_KEY_ID_SIZE,
			     &subregion,pinfo))
	    return 0;
	memcpy(sig->info.signer_id,C.ss_issuer_key_id.key_id,OPS_KEY_ID_SIZE);
	sig->info.signer_id_set=ops_true;
	break;

    case OPS_PTAG_SS_PREFERRED_SKA:
	if(!read_data(&C.ss_preferred_ska.data,&subregion,pinfo))
	    return 0;
	break;
			    	
    case OPS_PTAG_SS_PREFERRED_HASH:
	if(!read_data(&C.ss_preferred_hash.data,&subregion,pinfo))
	    return 0;
	break;
			    	
    case OPS_PTAG_SS_PREFERRED_COMPRESSION:
	if(!read_data(&C.ss_preferred_compression.data,&subregion,pinfo))
	    return 0;
	break;
			    	
    case OPS_PTAG_SS_PRIMARY_USER_ID:
	if(!limited_read (bool,1,&subregion,pinfo))
	    return 0;
	C.ss_primary_user_id.primary_user_id = !!bool[0];
	break;
 
    case OPS_PTAG_SS_KEY_FLAGS:
	if(!read_data(&C.ss_key_flags.data,&subregion,pinfo))
	    return 0;
	break;

    case OPS_PTAG_SS_KEY_SERVER_PREFS:
	if(!read_data(&C.ss_key_server_prefs.data,&subregion,pinfo))
	    return 0;
	break;

    case OPS_PTAG_SS_FEATURES:
	if(!read_data(&C.ss_features.data,&subregion,pinfo))
	    return 0;
	break;

    case OPS_PTAG_SS_SIGNERS_USER_ID:
	if(!read_unsigned_string(&C.ss_signers_user_id.user_id,&subregion,pinfo))
	    return 0;
	break;

 case OPS_PTAG_SS_EMBEDDED_SIGNATURE:
     // \todo should do something with this sig?
     if (!read_data(&C.ss_embedded_signature.sig,&subregion,pinfo))
         return 0;
     break;

    case OPS_PTAG_SS_NOTATION_DATA:
	if(!limited_read_data(&C.ss_notation_data.flags,4,&subregion,pinfo))
	    return 0;
	if(!limited_read_size_t_scalar(&C.ss_notation_data.name.len,2,
				       &subregion,pinfo))
	    return 0;
	if(!limited_read_size_t_scalar(&C.ss_notation_data.value.len,2,
				       &subregion,pinfo))
	    return 0;
	if(!limited_read_data(&C.ss_notation_data.name,
			      C.ss_notation_data.name.len,&subregion,pinfo))
	    return 0;
	if(!limited_read_data(&C.ss_notation_data.value,
			      C.ss_notation_data.value.len,&subregion,pinfo))
	    return 0;
	break;

    case OPS_PTAG_SS_POLICY_URI:
	if(!read_string(&C.ss_policy_url.text,&subregion,pinfo))
	    return 0;
	break;

    case OPS_PTAG_SS_REGEXP:
	if(!read_string(&C.ss_regexp.text,&subregion, pinfo))
	    return 0;
	break;

    case OPS_PTAG_SS_PREFERRED_KEY_SERVER:
	if(!read_string(&C.ss_preferred_key_server.text,&subregion,pinfo))
	    return 0;
	break;

    case OPS_PTAG_SS_USERDEFINED00:
    case OPS_PTAG_SS_USERDEFINED01:
    case OPS_PTAG_SS_USERDEFINED02:
    case OPS_PTAG_SS_USERDEFINED03:
    case OPS_PTAG_SS_USERDEFINED04:
    case OPS_PTAG_SS_USERDEFINED05:
    case OPS_PTAG_SS_USERDEFINED06:
    case OPS_PTAG_SS_USERDEFINED07:
    case OPS_PTAG_SS_USERDEFINED08:
    case OPS_PTAG_SS_USERDEFINED09:
    case OPS_PTAG_SS_USERDEFINED10:
	if(!read_data(&C.ss_userdefined.data,&subregion,pinfo))
	    return 0;
	break;

    case OPS_PTAG_SS_RESERVED:
	if(!read_data(&C.ss_unknown.data,&subregion,pinfo))
	    return 0;
	break;

    case OPS_PTAG_SS_REVOCATION_REASON:
	/* first byte is the machine-readable code */
	if(!limited_read(&C.ss_revocation_reason.code,1,&subregion,pinfo))
	    return 0;

	/* the rest is a human-readable UTF-8 string */
	if(!read_string(&C.ss_revocation_reason.text,&subregion,pinfo))
	    return 0;
	break;

    case OPS_PTAG_SS_REVOCATION_KEY:
	/* octet 0 = clss. Bit 0x80 must be set */
	if(!limited_read (&C.ss_revocation_key.clss,1,&subregion,pinfo))
	    return 0;
	if(!(C.ss_revocation_key.clss&0x80))
	    {
	    printf("Warning: OPS_PTAG_SS_REVOCATION_KEY class: "
		   "Bit 0x80 should be set\n");
	    return 0;
	    }
 
	/* octet 1 = algid */
	if(!limited_read(&C.ss_revocation_key.algid,1,&subregion,pinfo))
	    return 0;
 
	/* octets 2-21 = fingerprint */
	if(!limited_read(&C.ss_revocation_key.fingerprint[0],20,&subregion,
			 pinfo))
	    return 0;
	break;
 
    default:
	if(pinfo->ss_parsed[t8]&t7)
	    OPS_ERROR_1(&pinfo->errors, OPS_E_PROTO_UNKNOWN_SS,
                        "Unknown signature subpacket type (%d)", c[0]&0x7f);
	read=ops_false;
	break;
	}

    /* Application doesn't want it delivered parsed */
    if(!(pinfo->ss_parsed[t8]&t7))
	{
	if(content.critical)
	    OPS_ERROR_1(&pinfo->errors,OPS_E_PROTO_CRITICAL_SS_IGNORED,
                        "Critical signature subpacket ignored (%d)",
                        c[0]&0x7f);
	if(!read && !limited_skip(subregion.length-1,&subregion,pinfo))
	    return 0;
	//	printf("skipped %d length %d\n",c[0]&0x7f,subregion.length);
	if(read)
	    ops_parser_content_free(&content);
	return 1;
	}

    if(read && subregion.length_read != subregion.length)
        {
	OPS_ERROR_1(&pinfo->errors,OPS_E_R_UNCONSUMED_DATA,
                    "Unconsumed data (%d)", 
                    subregion.length-subregion.length_read);
        return 0;
        }
 
    CBP(pinfo,content.tag,&content);

    return 1;
    }

/**
 \ingroup Core_Create
 \brief Free the memory used when parsing this signature sub-packet type
 \param ss_preferred_ska
*/
void ops_ss_preferred_ska_free(ops_ss_preferred_ska_t *ss_preferred_ska)
    {
    data_free(&ss_preferred_ska->data);
    }

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type 
   \param ss_preferred_hash
*/
void ops_ss_preferred_hash_free(ops_ss_preferred_hash_t *ss_preferred_hash)
    {
    data_free(&ss_preferred_hash->data);
    }

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type 
*/
void ops_ss_preferred_compression_free(ops_ss_preferred_compression_t *ss_preferred_compression)
    {
    data_free(&ss_preferred_compression->data);
    }

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type 
*/
void ops_ss_key_flags_free(ops_ss_key_flags_t *ss_key_flags)
    {
    data_free(&ss_key_flags->data);
    }

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type 
*/
void ops_ss_features_free(ops_ss_features_t *ss_features)
    {
    data_free(&ss_features->data);
    }

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type 
*/
void ops_ss_key_server_prefs_free(ops_ss_key_server_prefs_t *ss_key_server_prefs)
    {
    data_free(&ss_key_server_prefs->data);
    }

/** 
 * \ingroup Core_ReadPackets
 * \brief Parse several signature subpackets.
 *
 * Hashed and unhashed subpacket sets are preceded by an octet count that specifies the length of the complete set.
 * This function parses this length and then calls parse_one_signature_subpacket() for each subpacket until the
 * entire set is consumed.
 *
 * This function does not call the callback directly, parse_one_signature_subpacket() does for each subpacket.
 *
 * \param *ptag		Pointer to the Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.2.3
 */
static int parse_signature_subpackets(ops_signature_t *sig,
				      ops_region_t *region,
				      ops_parse_info_t *pinfo)
    {
    ops_region_t subregion;
    ops_parser_content_t content;

    ops_init_subregion(&subregion,region);
    if(!limited_read_scalar(&subregion.length,2,region,pinfo))
	return 0;

    if(subregion.length > region->length)
	ERRP(pinfo,"Subpacket set too long");

    while(subregion.length_read < subregion.length)
	if(!parse_one_signature_subpacket(sig,&subregion,pinfo))
	    return 0;

    if(subregion.length_read != subregion.length)
	{
	if(!limited_skip(subregion.length-subregion.length_read,&subregion,
			 pinfo))
	    ERRP(pinfo,"Read failed while recovering from subpacket length mismatch");
	ERRP(pinfo,"Subpacket length mismatch");
	}

    return 1;
    }

/** 
 * \ingroup Core_ReadPackets
 * \brief Parse a version 4 signature.
 *
 * This function parses a version 4 signature including all its hashed and unhashed subpackets.
 *
 * Once the signature packet has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.2.3
 */
static int parse_v4_signature(ops_region_t *region,ops_parse_info_t *pinfo)
    {
    unsigned char c[1]="";
    ops_parser_content_t content;
    
    //debug=1;
    if (debug)
        { fprintf(stderr, "\nparse_v4_signature\n"); }
    
    // clear signature
    memset(&C.signature,'\0',sizeof C.signature);

    /* We need to hash the packet data from version through the hashed subpacket data */

    C.signature.v4_hashed_data_start=pinfo->rinfo.alength-1;

    /* Set version,type,algorithms */

    C.signature.info.version=OPS_V4;

    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.signature.info.type=c[0];
    if (debug)
        { fprintf(stderr, "signature type=%d\n", C.signature.info.type); }

    /* XXX: check signature type */

    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.signature.info.key_algorithm=c[0];
    /* XXX: check algorithm */
    if (debug)
        { fprintf(stderr, "key_algorithm=%d\n", C.signature.info.key_algorithm); }

    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.signature.info.hash_algorithm=c[0];
    /* XXX: check algorithm */
    if (debug)
        { fprintf(stderr, "hash_algorithm=%d %s\n", C.signature.info.hash_algorithm, ops_show_hash_algorithm(C.signature.info.hash_algorithm)); }

    CBP(pinfo,OPS_PTAG_CT_SIGNATURE_HEADER,&content);

    if(!parse_signature_subpackets(&C.signature,region,pinfo))
	return 0;

    C.signature.info.v4_hashed_data_length=pinfo->rinfo.alength
        -C.signature.v4_hashed_data_start;

    // copy hashed subpackets
    if (C.signature.info.v4_hashed_data)
        free(C.signature.info.v4_hashed_data);
    C.signature.info.v4_hashed_data=ops_mallocz(C.signature.info.v4_hashed_data_length);

    if (!pinfo->rinfo.accumulate)
    {
       /* We must accumulate, else we can't check the signature */
       fprintf(stderr,"*** ERROR: must set accumulate to true\n");
       return 0 ; //ASSERT(0);
    }

    memcpy(C.signature.info.v4_hashed_data,
           pinfo->rinfo.accumulated+C.signature.v4_hashed_data_start,
           C.signature.info.v4_hashed_data_length);

    if(!parse_signature_subpackets(&C.signature,region,pinfo))
	return 0;
    
    if(!limited_read(C.signature.hash2,2,region,pinfo))
	return 0;

    switch(C.signature.info.key_algorithm)
	{
    case OPS_PKA_RSA:
	if(!limited_read_mpi(&C.signature.info.signature.rsa.sig,region,pinfo))
	    return 0;
	break;

    case OPS_PKA_DSA:
	if(!limited_read_mpi(&C.signature.info.signature.dsa.r,region,pinfo)) 
	    ERRP(pinfo,"Error reading DSA r field in signature");
	if (!limited_read_mpi(&C.signature.info.signature.dsa.s,region,pinfo))
	    ERRP(pinfo,"Error reading DSA s field in signature");
	break;

    case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
	if(!limited_read_mpi(&C.signature.info.signature.elgamal.r,region,pinfo)
	   || !limited_read_mpi(&C.signature.info.signature.elgamal.s,region,pinfo))
	    return 0;
	break;

    case OPS_PKA_PRIVATE00:
    case OPS_PKA_PRIVATE01:
    case OPS_PKA_PRIVATE02:
    case OPS_PKA_PRIVATE03:
    case OPS_PKA_PRIVATE04:
    case OPS_PKA_PRIVATE05:
    case OPS_PKA_PRIVATE06:
    case OPS_PKA_PRIVATE07:
    case OPS_PKA_PRIVATE08:
    case OPS_PKA_PRIVATE09:
    case OPS_PKA_PRIVATE10:
	if (!read_data(&C.signature.info.signature.unknown.data,region,pinfo))
	    return 0;
	break;

    default:
	OPS_ERROR_1(&pinfo->errors,OPS_E_ALG_UNSUPPORTED_SIGNATURE_ALG,
                    "Bad v4 signature key algorithm (%s)",
                    ops_show_pka(C.signature.info.key_algorithm));
        return 0;
	}

    if(region->length_read != region->length)
        {
	OPS_ERROR_1(&pinfo->errors,OPS_E_R_UNCONSUMED_DATA,
                    "Unconsumed data (%d)",
                    region->length-region->length_read);
        return 0;
        }

    CBP(pinfo,OPS_PTAG_CT_SIGNATURE_FOOTER,&content);

    return 1;
    }

/** 
 * \ingroup Core_ReadPackets
 * \brief Parse a signature subpacket.
 *
 * This function calls the appropriate function to handle v3 or v4 signatures.
 *
 * Once the signature packet has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 */
static int parse_signature(ops_region_t *region,ops_parse_info_t *pinfo)
{
   unsigned char c[1]="";
   ops_parser_content_t content;

   if(!(region->length_read == 0)) // ASSERT(region->length_read == 0)  /* We should not have read anything so far */
   {
      fprintf(stderr,"parse_signature: region read is not empty! Data is corrupted?") ;
      return 0 ;
   }

   memset(&content,'\0',sizeof content);

   if(!limited_read(c,1,region,pinfo))
      return 0;

   if(c[0] == 2 || c[0] == 3)
      return parse_v3_signature(region,pinfo);
   else if(c[0] == 4)
      return parse_v4_signature(region,pinfo);

   OPS_ERROR_1(&pinfo->errors,OPS_E_PROTO_BAD_SIGNATURE_VRSN,
	 "Bad signature version (%d)",c[0]);
   return 0;
}

/**
 \ingroup Core_ReadPackets
 \brief Parse Compressed packet
*/
static int parse_compressed(ops_region_t *region,ops_parse_info_t *pinfo)
    {
    unsigned char c[1]="";
    ops_parser_content_t content;

    if(pinfo->recursive_compression_depth > MAX_RECURSIVE_COMPRESSION_DEPTH)
    {
       fprintf(stderr,"Recursive compression down to depth 32. This is weird. Probably a packet crafted to crash PGP. Will be dropped!\n") ;
       return 0 ;
    }

    if(!limited_read(c,1,region,pinfo))
	return 0;

pinfo->recursive_compression_depth++ ;

    C.compressed.type=c[0];

    CBP(pinfo,OPS_PTAG_CT_COMPRESSED,&content);

    /* The content of a compressed data packet is more OpenPGP packets
       once decompressed, so recursively handle them */

    int res = ops_decompress(region,pinfo,C.compressed.type);

pinfo->recursive_compression_depth-- ;

    return res ;
    }

/**
   \ingroup Core_ReadPackets
   \brief Parse a One Pass Signature packet
*/
static int parse_one_pass(ops_region_t *region,ops_parse_info_t *pinfo)
    {
    unsigned char c[1]="";
    ops_parser_content_t content;

    if(!limited_read(&C.one_pass_signature.version,1,region,pinfo))
	return 0;
    if(C.one_pass_signature.version != 3)
        {
	OPS_ERROR_1(&pinfo->errors,OPS_E_PROTO_BAD_ONE_PASS_SIG_VRSN,
                    "Bad one-pass signature version (%d)",
                    C.one_pass_signature.version);
        return 0;
        }

    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.one_pass_signature.sig_type=c[0];

    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.one_pass_signature.hash_algorithm=c[0];

    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.one_pass_signature.key_algorithm=c[0];

    if(!limited_read(C.one_pass_signature.keyid,
			 sizeof C.one_pass_signature.keyid,region,pinfo))
	return 0;

    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.one_pass_signature.nested=!!c[0];

    CBP(pinfo,OPS_PTAG_CT_ONE_PASS_SIGNATURE,&content);

    // XXX: we should, perhaps, let the app choose whether to hash or not
    ops_parse_hash_init(pinfo,C.one_pass_signature.hash_algorithm,
			C.one_pass_signature.keyid);

    return 1;
    }

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type 
*/
void ops_ss_userdefined_free(ops_ss_userdefined_t *ss_userdefined)
    {
    data_free(&ss_userdefined->data);
    }

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type 
*/
void ops_ss_reserved_free(ops_ss_unknown_t *ss_unknown)
    {
    data_free(&ss_unknown->data);
    }

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type 
*/
void ops_ss_notation_data_free(ops_ss_notation_data_t *ss_notation_data)
     {
     data_free(&ss_notation_data->name);
     data_free(&ss_notation_data->value);
     }

void ops_ss_embedded_signature_free(ops_ss_embedded_signature_t *ss_embedded_signature)
    {
    data_free(&ss_embedded_signature->sig);
    }

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type 
*/
void ops_ss_revocation_reason_free(ops_ss_revocation_reason_t *ss_revocation_reason)
    {
    string_free(&ss_revocation_reason->text);
    }

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this packet type 
*/
void ops_trust_free(ops_trust_t *trust)
    {
    data_free(&trust->data);
    }

/**
 \ingroup Core_ReadPackets
 \brief Parse a Trust packet
*/
static int
parse_trust (ops_region_t *region, ops_parse_info_t *pinfo)
    {
    ops_parser_content_t content;

    if(!read_data(&C.trust.data,region,pinfo))
	    return 0;

    CBP(pinfo,OPS_PTAG_CT_TRUST, &content);

    return 1;
    }

/**
   \ingroup Core_ReadPackets
   \brief Parse a Literal Data packet
*/
static int parse_literal_data(ops_region_t *region,ops_parse_info_t *pinfo)
    {
    ops_parser_content_t content;
    unsigned char c[1]="";

    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.literal_data_header.format=c[0];

    if(!limited_read(c,1,region,pinfo))
	return 0;
    if(!limited_read((unsigned char *)C.literal_data_header.filename,c[0],
		     region,pinfo))
	return 0;
    C.literal_data_header.filename[c[0]]='\0';

    if(!limited_read_time(&C.literal_data_header.modification_time,region,pinfo))
	return 0;

    CBP(pinfo,OPS_PTAG_CT_LITERAL_DATA_HEADER,&content);

    while(region->length_read < region->length)
	{
		unsigned l=region->length-region->length_read;

		C.literal_data_body.data = (unsigned char *)malloc(l) ;

		if(C.literal_data_body.data == NULL)
		{
		   fprintf(stderr,"parse_literal_data(): cannot malloc for requested size %d.\n",l) ;
		   return 0 ;
		}

		if(!limited_read(C.literal_data_body.data,l,region,pinfo))
		{
			free(C.literal_data_body.data);
			return 0;
		}

		C.literal_data_body.length=l;

		ops_parse_hash_data(pinfo,C.literal_data_body.data,l);

		CBP(pinfo,OPS_PTAG_CT_LITERAL_DATA_BODY,&content);
		free(C.literal_data_body.data);
	}

    return 1;
    }

/**
 * \ingroup Core_Create
 *
 * ops_secret_key_free() frees the memory associated with "key". Note that
 * the key itself is not freed.
 * 
 * \param key
 */

void ops_secret_key_free(ops_secret_key_t *key)
    {
    switch(key->public_key.algorithm)
	{
    case OPS_PKA_RSA:
    case OPS_PKA_RSA_ENCRYPT_ONLY:
    case OPS_PKA_RSA_SIGN_ONLY:
	free_BN(&key->key.rsa.d);
	free_BN(&key->key.rsa.p);
	free_BN(&key->key.rsa.q);
	free_BN(&key->key.rsa.u);
	break;

    case OPS_PKA_DSA:
	free_BN(&key->key.dsa.x);
	break;

    default:
        fprintf(stderr,"ops_secret_key_free: Unknown algorithm: %d (%s)\n",key->public_key.algorithm, ops_show_pka(key->public_key.algorithm));
        //ASSERT(0);
	}

    ops_public_key_free(&key->public_key);
    }

void ops_secret_key_copy(ops_secret_key_t *dst,const ops_secret_key_t *src)
{
   *dst = *src ;
   ops_public_key_copy(&dst->public_key,&src->public_key);

   switch(src->public_key.algorithm)
   {
      case OPS_PKA_RSA:
      case OPS_PKA_RSA_ENCRYPT_ONLY:
      case OPS_PKA_RSA_SIGN_ONLY:
	 dst->key.rsa.d = BN_dup(src->key.rsa.d) ;
	 dst->key.rsa.p = BN_dup(src->key.rsa.p) ;
	 dst->key.rsa.q = BN_dup(src->key.rsa.q) ;
	 dst->key.rsa.u = BN_dup(src->key.rsa.u) ;
	 break;

      case OPS_PKA_DSA:
	 dst->key.dsa.x = BN_dup(src->key.dsa.x) ;
	 break;

      default:
	 fprintf(stderr,"ops_secret_key_copy: Unknown algorithm: %d (%s)\n",src->public_key.algorithm, ops_show_pka(src->public_key.algorithm));
	 //ASSERT(0);
   }
}
static int consume_packet(ops_region_t *region,ops_parse_info_t *pinfo,
			  ops_boolean_t warn)
    {
    ops_data_t remainder;
    ops_parser_content_t content;

    if(region->indeterminate)
	ERRP(pinfo,"Can't consume indeterminate packets");

    if(read_data(&remainder,region,pinfo))
	{
	/* now throw it away */
	data_free(&remainder);
	if(warn)
	    OPS_ERROR(&pinfo->errors,OPS_E_P_PACKET_CONSUMED,"Warning: packet consumer");
	}
    else if(warn)
        OPS_ERROR(&pinfo->errors,OPS_E_P_PACKET_NOT_CONSUMED,"Warning: Packet was not consumed");
    else
	{
	OPS_ERROR(&pinfo->errors,OPS_E_P_PACKET_NOT_CONSUMED,"Packet was not consumed");
	return 0;
	}

    return 1;
    }

/**
 * \ingroup Core_ReadPackets
 * \brief Parse a secret key
 */
static int parse_secret_key(ops_region_t *region,ops_parse_info_t *pinfo)
{
   ops_parser_content_t content;
   unsigned char c[1]="";
   ops_crypt_t decrypt;
   int ret=1;
   ops_region_t encregion;
   ops_region_t *saved_region=NULL;
   ops_hash_t checkhash;
   int blocksize;
   ops_boolean_t crypted;

   if (debug)
   { fprintf(stderr,"\n---------\nparse_secret_key:\n"); 
      fprintf(stderr,"region length=%d, length_read=%d, remainder=%d\n", region->length, region->length_read, region->length-region->length_read);
   }

   memset(&content,'\0',sizeof content);
   if(!parse_public_key_data(&C.secret_key.public_key,region,pinfo))
      return 0;

   if (debug)
   {
      fprintf(stderr,"parse_secret_key: public key parsed\n");
      ops_print_public_key(&C.secret_key.public_key);
   }

   pinfo->reading_v3_secret=C.secret_key.public_key.version != OPS_V4;

   if(!limited_read(c,1,region,pinfo))
      return 0;
   C.secret_key.s2k_usage=c[0];

   if(C.secret_key.s2k_usage == OPS_S2KU_ENCRYPTED
	 || C.secret_key.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED)
   {
      if(!limited_read(c,1,region,pinfo))
	 return 0;
      C.secret_key.algorithm=c[0];

      if(!limited_read(c,1,region,pinfo))
	 return 0;
      C.secret_key.s2k_specifier=c[0];

      if(!(C.secret_key.s2k_specifier == OPS_S2KS_SIMPLE || C.secret_key.s2k_specifier == OPS_S2KS_SALTED || C.secret_key.s2k_specifier == OPS_S2KS_ITERATED_AND_SALTED)) // ASSERT(C.secret_key.s2k_specifier == OPS_S2KS_SIMPLE || C.secret_key.s2k_specifier == OPS_S2KS_SALTED || C.secret_key.s2k_specifier == OPS_S2KS_ITERATED_AND_SALTED)
      {
	 fprintf(stderr,"parse_secret_key: error in secret key format. Bad flags combination.\n") ;
	 return 0;
      }

      if(!limited_read(c,1,region,pinfo))
	 return 0;
      C.secret_key.hash_algorithm=c[0];

      if(C.secret_key.s2k_specifier != OPS_S2KS_SIMPLE
	    && !limited_read(C.secret_key.salt,8,region,pinfo))
      {
	 return 0;
      }

      if(C.secret_key.s2k_specifier == OPS_S2KS_ITERATED_AND_SALTED)
      {
	 if(!limited_read(c,1,region,pinfo))
	    return 0;
	 C.secret_key.octet_count=(16+(c[0]&15)) << ((c[0] >> 4)+6);
      }
   }
   else if(C.secret_key.s2k_usage != OPS_S2KU_NONE)
   {
      // this is V3 style, looks just like a V4 simple hash
      C.secret_key.algorithm=C.secret_key.s2k_usage;
      C.secret_key.s2k_usage=OPS_S2KU_ENCRYPTED;
      C.secret_key.s2k_specifier=OPS_S2KS_SIMPLE;
      C.secret_key.hash_algorithm=OPS_HASH_MD5;
   }

   crypted=C.secret_key.s2k_usage == OPS_S2KU_ENCRYPTED
      || C.secret_key.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED;

   if(crypted)
   {
      int n;
      ops_parser_content_t pc;
      char *passphrase;
      unsigned char key[OPS_MAX_KEY_SIZE+OPS_MAX_HASH_SIZE];
      ops_hash_t hashes[(OPS_MAX_KEY_SIZE+OPS_MIN_HASH_SIZE-1)/OPS_MIN_HASH_SIZE];
      int keysize;
      int hashsize;
      size_t l;

      blocksize=ops_block_size(C.secret_key.algorithm);
      if(!(blocksize > 0 && blocksize <= OPS_MAX_BLOCK_SIZE)) // ASSERT(blocksize > 0 && blocksize <= OPS_MAX_BLOCK_SIZE);
      {
	 fprintf(stderr,"parse_secret_key: block size error. Data seems corrupted.") ;
	 return 0;
      }

      if(!limited_read(C.secret_key.iv,blocksize,region,pinfo))
	 return 0;

      memset(&pc,'\0',sizeof pc);
      passphrase=NULL;
      pc.content.secret_key_passphrase.passphrase=&passphrase;
      pc.content.secret_key_passphrase.secret_key=&C.secret_key;
      CBP(pinfo,OPS_PARSER_CMD_GET_SK_PASSPHRASE,&pc);
      if(!passphrase)
      {
	 if (debug)
	 {
	    // \todo make into proper error
	    fprintf(stderr,"parse_secret_key: can't get passphrase\n");
	 }

	 if(!consume_packet(region,pinfo,ops_false))
	    return 0;

	 CBP(pinfo,OPS_PTAG_CT_ENCRYPTED_SECRET_KEY,&content);

	 return 1;
      }

      keysize=ops_key_size(C.secret_key.algorithm);
      if(!(keysize > 0 && keysize <= OPS_MAX_KEY_SIZE)) // ASSERT(keysize > 0 && keysize <= OPS_MAX_KEY_SIZE);
      {
	 fprintf(stderr,"parse_secret_key: keysize %d exceeds maximum allowed size %d\n",keysize,OPS_MAX_KEY_SIZE);
	 return 0 ;
      }

      hashsize=ops_hash_size(C.secret_key.hash_algorithm);
      if(!(hashsize > 0 && hashsize <= OPS_MAX_HASH_SIZE)) // ASSERT(hashsize > 0 && hashsize <= OPS_MAX_HASH_SIZE);
      {
	 fprintf(stderr,"parse_secret_key: hashsize %d exceeds maximum allowed size %d\n",keysize,OPS_MAX_HASH_SIZE);
	 return 0 ;
      }

      for(n=0 ; n*hashsize < keysize ; ++n)
      {
	 int i;

	 ops_hash_any(&hashes[n],C.secret_key.hash_algorithm);
	 hashes[n].init(&hashes[n]);
	 // preload hashes with zeroes...
	 for(i=0 ; i < n ; ++i)
	    hashes[n].add(&hashes[n],(unsigned char *)"",1);
      }

      l=strlen(passphrase);

      for(n=0 ; n*hashsize < keysize ; ++n)
      {
	 unsigned i;

	 switch(C.secret_key.s2k_specifier)
	 {
	    case OPS_S2KS_SALTED:
	       hashes[n].add(&hashes[n],C.secret_key.salt,OPS_SALT_SIZE);
	       // flow through...
	    case OPS_S2KS_SIMPLE:
	       hashes[n].add(&hashes[n],(unsigned char*)passphrase,l);
	       break;

	    case OPS_S2KS_ITERATED_AND_SALTED:
	       for(i=0 ; i < C.secret_key.octet_count ; i+=l+OPS_SALT_SIZE)
	       {
		  int j=l+OPS_SALT_SIZE;

		  if(i+j > C.secret_key.octet_count && i != 0)
		     j=C.secret_key.octet_count-i;

		  hashes[n].add(&hashes[n],C.secret_key.salt,
			j > OPS_SALT_SIZE ? OPS_SALT_SIZE : j);
		  if(j > OPS_SALT_SIZE)
		     hashes[n].add(&hashes[n],(unsigned char *)passphrase,j-OPS_SALT_SIZE);
	       }

	 }
      }

      for(n=0 ; n*hashsize < keysize ; ++n)
      {
	 int r=hashes[n].finish(&hashes[n],key+n*hashsize);
	 if(!(r == hashsize)) // ASSERT(r == hashsize);
	 {
	    fprintf(stderr,"parse_secret_key: read error. Data probably corrupted.") ;
	    return 0 ;
	 }
      }

      free(passphrase);

      ops_crypt_any(&decrypt,C.secret_key.algorithm);
      if (debug)
      {
	 unsigned int i=0;
	 fprintf(stderr,"\nREADING:\niv=");
	 for (i=0; i<ops_block_size(C.secret_key.algorithm); i++)
	 {
	    fprintf(stderr, "%02x ", C.secret_key.iv[i]);
	 }
	 fprintf(stderr,"\n");
	 fprintf(stderr,"key=");
	 for (i=0; i<CAST_KEY_LENGTH; i++)
	 {
	    fprintf(stderr, "%02x ", key[i]);
	 }
	 fprintf(stderr,"\n");
      }
      decrypt.set_iv(&decrypt,C.secret_key.iv);
      decrypt.set_key(&decrypt,key);

      // now read encrypted data

      ops_reader_push_decrypt(pinfo,&decrypt,region);

      /* Since all known encryption for PGP doesn't compress, we can
	 limit to the same length as the current region (for now).
       */
      ops_init_subregion(&encregion,NULL);
      encregion.length=region->length-region->length_read;
      if(C.secret_key.public_key.version != OPS_V4)
      {
	 encregion.length-=2;
      }
      saved_region=region;
      region=&encregion;
   }

   if(C.secret_key.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED)
   {
      ops_hash_sha1(&checkhash);
      ops_reader_push_hash(pinfo,&checkhash);
   }
   else
   {
      ops_reader_push_sum16(pinfo);
   }

   switch(C.secret_key.public_key.algorithm)
   {
      case OPS_PKA_RSA:
      case OPS_PKA_RSA_ENCRYPT_ONLY:
      case OPS_PKA_RSA_SIGN_ONLY:
	 if(!limited_read_mpi(&C.secret_key.key.rsa.d,region,pinfo)
	       || !limited_read_mpi(&C.secret_key.key.rsa.p,region,pinfo)
	       || !limited_read_mpi(&C.secret_key.key.rsa.q,region,pinfo)
	       || !limited_read_mpi(&C.secret_key.key.rsa.u,region,pinfo))
	    ret=0;

	 break;

      case OPS_PKA_DSA:

	 if(!limited_read_mpi(&C.secret_key.key.dsa.x,region,pinfo))
	    ret=0;
	 break;

      default:
	 OPS_ERROR_2(&pinfo->errors,OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG,"Unsupported Public Key algorithm %d (%s)",C.secret_key.public_key.algorithm,ops_show_pka(C.secret_key.public_key.algorithm));
	 ret=0;
	 //	ASSERT(0);
   }

   if (debug)
   {
      fprintf(stderr,"4 MPIs read\n");
      //        ops_print_secret_key_verbose(OPS_PTAG_CT_SECRET_KEY, &C.secret_key);
   }

   pinfo->reading_v3_secret=ops_false;

   if(C.secret_key.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED)
   {
      unsigned char hash[20];

      ops_reader_pop_hash(pinfo);
      checkhash.finish(&checkhash,hash);

      if(crypted && C.secret_key.public_key.version != OPS_V4)
      {
	 ops_reader_pop_decrypt(pinfo);
	 region=saved_region;
      }

      if(ret)
      {
	 if(!limited_read(C.secret_key.checkhash,20,region,pinfo))
	    return 0;

	 if(memcmp(hash,C.secret_key.checkhash,20))
	    ERRP(pinfo,"Hash mismatch in secret key");
      }
   }
   else
   {
      unsigned short sum;

      sum=ops_reader_pop_sum16(pinfo);

      if(crypted && C.secret_key.public_key.version != OPS_V4)
      {
	 ops_reader_pop_decrypt(pinfo);
	 region=saved_region;
      }

      if(ret)
      {
	 if(!limited_read_scalar(&C.secret_key.checksum,2,region,
		  pinfo))
	    return 0;

	 if(sum != C.secret_key.checksum)
	    ERRP(pinfo,"Checksum mismatch in secret key");
      }
   }

   if(crypted && C.secret_key.public_key.version == OPS_V4)
   {
      ops_reader_pop_decrypt(pinfo);
   }

   if(!(!ret || region->length_read == region->length)) // ASSERT(!ret || region->length_read == region->length)
   {
      fprintf(stderr,"parse_secret_key: read error. data probably corrupted.") ;
      return 0 ;
   }

   if(!ret)
      return 0;

   CBP(pinfo,OPS_PTAG_CT_SECRET_KEY,&content);

   if (debug)
   { fprintf(stderr, "--- end of parse_secret_key\n\n"); }

   return 1;
}

/**
   \ingroup Core_ReadPackets
   \brief Parse a Public Key Session Key packet
*/
static int parse_pk_session_key(ops_region_t *region,
                                ops_parse_info_t *pinfo)
    {
    unsigned char c[1]="";
    ops_parser_content_t content;
    ops_parser_content_t pc;

    int n;
    BIGNUM *enc_m;
    unsigned k;
    const ops_secret_key_t *secret;
    unsigned char cs[2];
    unsigned char* iv;

    // Can't rely on it being CAST5
    // \todo FIXME RW
    //    const size_t sz_unencoded_m_buf=CAST_KEY_LENGTH+1+2;
    const size_t sz_unencoded_m_buf=1024;
    unsigned char unencoded_m_buf[sz_unencoded_m_buf];
    
    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.pk_session_key.version=c[0];
    if(C.pk_session_key.version != OPS_PKSK_V3)
        {
	OPS_ERROR_1(&pinfo->errors, OPS_E_PROTO_BAD_PKSK_VRSN,
	      "Bad public-key encrypted session key version (%d)",
	      C.pk_session_key.version);
        return 0;
        }

    if(!limited_read(C.pk_session_key.key_id,
		     sizeof C.pk_session_key.key_id,region,pinfo))
	return 0;

    if (debug)
        {
        int i;
        int x=sizeof C.pk_session_key.key_id;
        printf("session key: public key id: x=%d\n",x);
        for (i=0; i<x; i++)
            printf("%2x ", C.pk_session_key.key_id[i]);
        printf("\n");
        }

    if(!limited_read(c,1,region,pinfo))
	return 0;
    C.pk_session_key.algorithm=c[0];
    switch(C.pk_session_key.algorithm)
	{
    case OPS_PKA_RSA:
	if(!limited_read_mpi(&C.pk_session_key.parameters.rsa.encrypted_m,
			     region,pinfo))
	    return 0;
	enc_m=C.pk_session_key.parameters.rsa.encrypted_m;
	break;

    case OPS_PKA_ELGAMAL:
	if(!limited_read_mpi(&C.pk_session_key.parameters.elgamal.g_to_k,
			     region,pinfo)
	   || !limited_read_mpi(&C.pk_session_key.parameters.elgamal.encrypted_m,
			     region,pinfo))
	    return 0;
	enc_m=C.pk_session_key.parameters.elgamal.encrypted_m;
	break;

    default:
	OPS_ERROR_1(&pinfo->errors, OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG,
                    "Unknown public key algorithm in session key (%s)",
                    ops_show_pka(C.pk_session_key.algorithm));
	return 0;
	}

    memset(&pc,'\0',sizeof pc);
    secret=NULL;
    pc.content.get_secret_key.secret_key=&secret;
    pc.content.get_secret_key.pk_session_key=&C.pk_session_key;

    CBP(pinfo,OPS_PARSER_CMD_GET_SECRET_KEY,&pc);

    if(!secret)
	{
	CBP(pinfo,OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY,&content);

	return 1;
	}

    //    n=ops_decrypt_mpi(buf,sizeof buf,enc_m,secret);
    n=ops_decrypt_and_unencode_mpi(unencoded_m_buf,sizeof unencoded_m_buf,enc_m,secret);

    if(n < 1)
        {
        ERRP(pinfo,"decrypted message too short");
        return 0;
        }

    // PKA
    C.pk_session_key.symmetric_algorithm=unencoded_m_buf[0];

    if (!ops_is_sa_supported(C.pk_session_key.symmetric_algorithm))
        {
        // ERR1P
        OPS_ERROR_1(&pinfo->errors,OPS_E_ALG_UNSUPPORTED_SYMMETRIC_ALG,
                    "Symmetric algorithm %s not supported", 
                    ops_show_symmetric_algorithm(C.pk_session_key.symmetric_algorithm));
        return 0;
        }

    k=ops_key_size(C.pk_session_key.symmetric_algorithm);

    if((unsigned)n != k+3)
        {
        OPS_ERROR_2(&pinfo->errors,OPS_E_PROTO_DECRYPTED_MSG_WRONG_LEN,
                    "decrypted message wrong length (got %d expected %d)",
                    n,k+3);
        return 0;
        }
    
    if(!(k <= sizeof C.pk_session_key.key)) // ASSERT(k <= sizeof C.pk_session_key.key);
    {
       fprintf(stderr,"ops_decrypt_se_data: error in session key size. Corrupted data?") ;
       return 0 ;
    }

    memcpy(C.pk_session_key.key,unencoded_m_buf+1,k);

    if (debug)
        {
        printf("session key recovered (len=%d):\n",k);
        unsigned int j;
        for(j=0; j<k; j++)
            printf("%2x ", C.pk_session_key.key[j]);
        printf("\n");
        }

    C.pk_session_key.checksum=unencoded_m_buf[k+1]+(unencoded_m_buf[k+2] << 8);
    if (debug)
        {
        printf("session key checksum: %2x %2x\n", unencoded_m_buf[k+1], unencoded_m_buf[k+2]);
        }

    // Check checksum

    ops_calc_session_key_checksum(&C.pk_session_key, &cs[0]);
    if (unencoded_m_buf[k+1]!=cs[0] || unencoded_m_buf[k+2]!=cs[1])
        {
        OPS_ERROR_4(&pinfo->errors, OPS_E_PROTO_BAD_SK_CHECKSUM,
                    "Session key checksum wrong: expected %2x %2x, got %2x %2x",
              cs[0], cs[1], unencoded_m_buf[k+1], unencoded_m_buf[k+2]);
        return 0;
        }

    // all is well
    CBP(pinfo,OPS_PTAG_CT_PK_SESSION_KEY,&content);

    ops_crypt_any(&pinfo->decrypt,C.pk_session_key.symmetric_algorithm);
    iv=ops_mallocz(pinfo->decrypt.blocksize);
    pinfo->decrypt.set_iv(&pinfo->decrypt, iv);
    pinfo->decrypt.set_key(&pinfo->decrypt,C.pk_session_key.key);
    ops_encrypt_init(&pinfo->decrypt);
    return 1;
    }

// XXX: make this static?
int ops_decrypt_se_data(ops_content_tag_t tag,ops_region_t *region, ops_parse_info_t *pinfo)
{
   int r=1;
   ops_crypt_t *decrypt=ops_parse_get_decrypt(pinfo);

   if(decrypt)
   {
      unsigned char buf[OPS_MAX_BLOCK_SIZE+2]="";
      size_t b=decrypt->blocksize;
      //	ops_parser_content_t content;
      ops_region_t encregion;


      ops_reader_push_decrypt(pinfo,decrypt,region);

      ops_init_subregion(&encregion,NULL);
      encregion.length=b+2;

      if(!exact_limited_read(buf,b+2,&encregion,pinfo))
	 return 0;

      if(buf[b-2] != buf[b] || buf[b-1] != buf[b+1])
      {
	 ops_reader_pop_decrypt(pinfo);
	 OPS_ERROR_4(&pinfo->errors, OPS_E_PROTO_BAD_SYMMETRIC_DECRYPT,
	       "Bad symmetric decrypt (%02x%02x vs %02x%02x)",
	       buf[b-2],buf[b-1],buf[b],buf[b+1]);
	 return 0;
      }

      if(tag == OPS_PTAG_CT_SE_DATA_BODY)
      {
	 decrypt->decrypt_resync(decrypt);
	 decrypt->block_encrypt(decrypt,decrypt->civ,decrypt->civ);
      }


      r=ops_parse(pinfo,ops_false);

      ops_reader_pop_decrypt(pinfo);
   }
   else
   {
      ops_parser_content_t content;

      while(region->length_read < region->length)
      {
	 unsigned l=region->length-region->length_read;

	 if(l > sizeof C.se_data_body.data)
	    l=sizeof C.se_data_body.data;

	 if(!limited_read(C.se_data_body.data,l,region,pinfo))
	    return 0;

	 C.se_data_body.length=l;

	 CBP(pinfo,tag,&content);
      }
   }

   return r;
}

int ops_decrypt_se_ip_data(ops_content_tag_t tag,ops_region_t *region,
		     ops_parse_info_t *pinfo)
{
   int r=1;
   ops_crypt_t *decrypt=ops_parse_get_decrypt(pinfo);

   if(decrypt)
   {
      ops_reader_push_decrypt(pinfo,decrypt,region);
      ops_reader_push_se_ip_data(pinfo,decrypt,region);

      r=ops_parse(pinfo,ops_false);

      ops_reader_pop_se_ip_data(pinfo);
      ops_reader_pop_decrypt(pinfo);
   }
   else
   {
      ops_parser_content_t content;

      while(region->length_read < region->length)
      {
	 unsigned l=region->length-region->length_read;

	 if(l > sizeof C.se_data_body.data)
	    l=sizeof C.se_data_body.data;

	 if(!limited_read(C.se_data_body.data,l,region,pinfo))
	    return 0;

	 C.se_data_body.length=l;

	 CBP(pinfo,tag,&content);
      }
   }

   return r;
}

/**
   \ingroup Core_ReadPackets
   \brief Read a Symmetrically Encrypted packet
*/
static int parse_se_data(ops_region_t *region,ops_parse_info_t *pinfo)
    {
    ops_parser_content_t content;

    /* there's no info to go with this, so just announce it */
    CBP(pinfo,OPS_PTAG_CT_SE_DATA_HEADER,&content);

    /* The content of an encrypted data packet is more OpenPGP packets
       once decrypted, so recursively handle them */
    return ops_decrypt_se_data(OPS_PTAG_CT_SE_DATA_BODY,region,pinfo);
    }

/**
   \ingroup Core_ReadPackets
   \brief Read a Symmetrically Encrypted Integrity Protected packet
*/
static int parse_se_ip_data(ops_region_t *region,ops_parse_info_t *pinfo)
{
   unsigned char c[1]="";
   ops_parser_content_t content;

   if(!limited_read(c,1,region,pinfo))
      return 0;
   C.se_ip_data_header.version=c[0];

   if(!(C.se_ip_data_header.version == OPS_SE_IP_V1)) // ASSERT(C.se_ip_data_header.version == OPS_SE_IP_V1);
   {
      fprintf(stderr,"parse_se_ip_data: packet header version should be %d, it is %d. Packet dropped.",OPS_SE_IP_V1,C.se_ip_data_header.version) ;
      return 0 ;
   }

   /* The content of an encrypted data packet is more OpenPGP packets
      once decrypted, so recursively handle them */
   return ops_decrypt_se_ip_data(OPS_PTAG_CT_SE_IP_DATA_BODY,region,pinfo);
}

/**
   \ingroup Core_ReadPackets
   \brief Read a MDC packet
*/
static int parse_mdc(ops_region_t *region, ops_parse_info_t *pinfo)
	{
	ops_parser_content_t content;

	if (!limited_read((unsigned char *)&C.mdc,OPS_SHA1_HASH_SIZE,region,pinfo))
		return 0;

	CBP(pinfo,OPS_PTAG_CT_MDC,&content);

	return 1;
	}

/** 
 * \ingroup Core_ReadPackets
 * \brief Parse one packet.
 *
 * This function parses the packet tag.  It computes the value of the
 * content tag and then calls the appropriate function to handle the
 * content.
 *
 * \param *pinfo	How to parse
 * \param *pktlen	On return, will contain number of bytes in packet
 * \return 1 on success, 0 on error, -1 on EOF */
static int ops_parse_one_packet(ops_parse_info_t *pinfo,
				unsigned long *pktlen)
    {
    unsigned char ptag[1];
    ops_parser_content_t content;
    int r;
    ops_region_t region;
    ops_boolean_t indeterminate=ops_false;

    C.ptag.position=pinfo->rinfo.position;

    r=base_read(ptag,1,pinfo);

    // errors in the base read are effectively EOF.
    if(r <= 0)
	return -1;

    *pktlen=0;

    if(!(*ptag&OPS_PTAG_ALWAYS_SET))
	{
	C.error.error="Format error (ptag bit not set)";
	CBP(pinfo,OPS_PARSER_ERROR,&content);
        OPS_ERROR(&pinfo->errors, OPS_E_P_UNKNOWN_TAG, C.error.error);
	return 0;
	}
    C.ptag.new_format=!!(*ptag&OPS_PTAG_NEW_FORMAT);
    if(C.ptag.new_format)
	{
	C.ptag.content_tag=*ptag&OPS_PTAG_NF_CONTENT_TAG_MASK;
	C.ptag.length_type=0;
	if(!read_new_length(&C.ptag.length,pinfo))
	    return 0;

	}
    else
	{
	ops_boolean_t rb = ops_false;

	C.ptag.content_tag=(*ptag&OPS_PTAG_OF_CONTENT_TAG_MASK)
	    >> OPS_PTAG_OF_CONTENT_TAG_SHIFT;
	C.ptag.length_type=*ptag&OPS_PTAG_OF_LENGTH_TYPE_MASK;
	switch(C.ptag.length_type)
	    {
	case OPS_PTAG_OF_LT_ONE_BYTE:
	    rb=_read_scalar(&C.ptag.length,1,pinfo);
	    break;

	case OPS_PTAG_OF_LT_TWO_BYTE:
	    rb=_read_scalar(&C.ptag.length,2,pinfo);
	    break;

	case OPS_PTAG_OF_LT_FOUR_BYTE:
	    rb=_read_scalar(&C.ptag.length,4,pinfo);
	    break;

	case OPS_PTAG_OF_LT_INDETERMINATE:
	    C.ptag.length=0;
	    indeterminate=ops_true;
	    rb=ops_true;
	    break;
	    }
	if(!rb)
            {
            OPS_ERROR(&pinfo->errors, OPS_E_P, "Cannot read tag length");
            return 0;
            }
        }

    CBP(pinfo,OPS_PARSER_PTAG,&content);

    ops_init_subregion(&region,NULL);
    region.length=C.ptag.length;
    region.indeterminate=indeterminate;
    switch(C.ptag.content_tag)
	{
    case OPS_PTAG_CT_SIGNATURE:
	r=parse_signature(&region,pinfo);
	break;

    case OPS_PTAG_CT_PUBLIC_KEY:
    case OPS_PTAG_CT_PUBLIC_SUBKEY:
	r=parse_public_key(C.ptag.content_tag,&region,pinfo);
	break;

    case OPS_PTAG_CT_TRUST:
	r=parse_trust(&region, pinfo);
	break;
      
    case OPS_PTAG_CT_USER_ID:
	r=parse_user_id(&region,pinfo);
	break;

    case OPS_PTAG_CT_COMPRESSED:
	r=parse_compressed(&region,pinfo);
	break;

    case OPS_PTAG_CT_ONE_PASS_SIGNATURE:
	r=parse_one_pass(&region,pinfo);
	break;

    case OPS_PTAG_CT_LITERAL_DATA:
	r=parse_literal_data(&region,pinfo);
	break;

    case OPS_PTAG_CT_USER_ATTRIBUTE:
	r=parse_user_attribute(&region,pinfo);
	break;

    case OPS_PTAG_CT_SECRET_KEY:
	r=parse_secret_key(&region,pinfo);
	break;

    case OPS_PTAG_CT_SECRET_SUBKEY:
	r=parse_secret_key(&region,pinfo);
	break;

    case OPS_PTAG_CT_PK_SESSION_KEY:
	r=parse_pk_session_key(&region,pinfo);
	break;

    case OPS_PTAG_CT_SE_DATA:
	r=parse_se_data(&region,pinfo);
	break;

    case OPS_PTAG_CT_SE_IP_DATA:
	r=parse_se_ip_data(&region,pinfo);
	break;

    case OPS_PTAG_CT_MDC:
	 r=parse_mdc(&region, pinfo);
	 break;

    default:
	OPS_ERROR_1(&pinfo->errors,OPS_E_P_UNKNOWN_TAG,
                    "Unknown content tag 0x%x", C.ptag.content_tag);
	r=0;
	}

    /* Ensure that the entire packet has been consumed */

    if(region.length != region.length_read && !region.indeterminate)
	if(!consume_packet(&region,pinfo,ops_false))
	    r=-1;

    // also consume it if there's been an error?
    // \todo decide what to do about an error on an
    //       indeterminate packet
    if (r==0)
        {
        if (!consume_packet(&region,pinfo,ops_false))
            r=-1;
        }

    /* set pktlen */

    *pktlen=pinfo->rinfo.alength;

    /* do callback on entire packet, if desired and there was no error */

    if(r > 0 && pinfo->rinfo.accumulate)
	{
	C.packet.length=pinfo->rinfo.alength;
	C.packet.raw=pinfo->rinfo.accumulated;
        
	CBP(pinfo,OPS_PARSER_PACKET_END,&content);
	//free(pinfo->rinfo.accumulated);
	pinfo->rinfo.accumulated=NULL;
	pinfo->rinfo.asize=0;
	}
    else
       C.packet.raw = NULL ;

    pinfo->rinfo.alength=0;
	
    if(r < 0)
	return -1;

    return r ? 1 : 0;
    }

/**
 * \ingroup Core_ReadPackets
 * 
 * \brief Parse packets from an input stream until EOF or error.
 *
 * \details Setup the necessary parsing configuration in "pinfo" before calling ops_parse().
 *
 * That information includes :
 *
 * - a "reader" function to be used to get the data to be parsed
 *
 * - a "callback" function to be called when this library has identified 
 * a parseable object within the data
 *
 * - whether the calling function wants the signature subpackets returned raw, parsed or not at all.
 *
 * After returning, pinfo->errors holds any errors encountered while parsing.
 *
 * \param pinfo	Parsing configuration
 * \return		1 on success in all packets, 0 on error in any packet
 *
 * \sa CoreAPI Overview
 *
 * \sa ops_print_errors(), ops_parse_and_print_errors()
 *
 * Example code
 * \code
ops_parse_cb_t* example_callback();
void example()
 {
 int fd=0;
 ops_parse_info_t *pinfo=NULL;
 char *filename="pubring.gpg";

 // setup pinfo to read from file with example callback
 fd=ops_setup_file_read(&pinfo, filename, NULL, example_callback, ops_false);

 // specify how we handle signature subpackets
 ops_parse_options(pinfo, OPS_PTAG_SS_ALL, OPS_PARSE_PARSED);
 
 if (!ops_parse(pinfo))
   ops_print_errors(pinfo->errors);

 ops_teardown_file_read(pinfo,fd);
 }
 * \endcode
 */

int ops_parse(ops_parse_info_t *pinfo,ops_boolean_t limit_packets)
{
   int r;
   unsigned long pktlen;
   int n_packets = 0 ;

   do
      // Parse until we get a return code of 0 (error) or -1 (EOF)
   {
      r=ops_parse_one_packet(pinfo,&pktlen);

      if(++n_packets > 500 && limit_packets)
      {
	 fprintf(stderr,"More than 500 packets parsed in a row. This is likely to be a buggy certificate.") ;
	 return 0 ;
      }
   } while (r > 0);

   return pinfo->errors ? 0 : 1;
   return r == -1 ? 0 : 1;
}

/**
\ingroup Core_ReadPackets
\brief Parse packets and print any errors
 * \param pinfo	Parsing configuration
 * \return		1 on success in all packets, 0 on error in any packet
 * \sa CoreAPI Overview
 * \sa ops_parse()
*/

int ops_parse_and_print_errors(ops_parse_info_t *pinfo)
{
   ops_parse(pinfo,ops_false);
   ops_print_errors(pinfo->errors);
   return pinfo->errors ? 0 : 1;
}

/**
 * \ingroup Core_ReadPackets
 *
 * \brief Specifies whether one or more signature
 * subpacket types should be returned parsed; or raw; or ignored.
 *
 * \param	pinfo	Pointer to previously allocated structure
 * \param	tag	Packet tag. OPS_PTAG_SS_ALL for all SS tags; or one individual signature subpacket tag
 * \param	type	Parse type
 * \todo Make all packet types optional, not just subpackets */
void ops_parse_options(ops_parse_info_t *pinfo,
		       ops_content_tag_t tag,
		       ops_parse_type_t type)
{
   int t8,t7;

   if(tag == OPS_PTAG_SS_ALL)
   {
      int n;

      for(n=0 ; n < 256 ; ++n)
	 ops_parse_options(pinfo,OPS_PTAG_SIGNATURE_SUBPACKET_BASE+n,
	       type);
      return;
   }

   if(!(tag >= OPS_PTAG_SIGNATURE_SUBPACKET_BASE && tag <= OPS_PTAG_SIGNATURE_SUBPACKET_BASE+NTAGS-1)) // ASSERT(tag >= OPS_PTAG_SIGNATURE_SUBPACKET_BASE && tag <= OPS_PTAG_SIGNATURE_SUBPACKET_BASE+NTAGS-1)
   {
      fprintf(stderr,"ops_parse_options: format error in options. Will not be parsed. Correct your code!\n") ;
      return ;
   }

   t8=(tag-OPS_PTAG_SIGNATURE_SUBPACKET_BASE)/8;
   t7=1 << ((tag-OPS_PTAG_SIGNATURE_SUBPACKET_BASE)&7);
   switch(type)
   {
      case OPS_PARSE_RAW:
	 pinfo->ss_raw[t8] |= t7;
	 pinfo->ss_parsed[t8] &= ~t7;
	 break;

      case OPS_PARSE_PARSED:
	 pinfo->ss_raw[t8] &= ~t7;
	 pinfo->ss_parsed[t8] |= t7;
	 break;

      case OPS_PARSE_IGNORE:
	 pinfo->ss_raw[t8] &= ~t7;
	 pinfo->ss_parsed[t8] &= ~t7;
	 break;
   }
}

/**
\ingroup Core_ReadPackets
\brief Creates a new zero-ed ops_parse_info_t struct
\sa ops_parse_info_delete()
*/
ops_parse_info_t *ops_parse_info_new(void)
    { return ops_mallocz(sizeof(ops_parse_info_t)); }

/**
\ingroup Core_ReadPackets
\brief Free ops_parse_info_t struct and its contents
\sa ops_parse_info_new()
*/
void ops_parse_info_delete(ops_parse_info_t *pinfo)
    {
    ops_parse_cb_info_t *cbinfo,*next;

    for(cbinfo=pinfo->cbinfo.next ; cbinfo ; cbinfo=next)
	{
	next=cbinfo->next;
	free(cbinfo);
	}
    if(pinfo->rinfo.destroyer)
	pinfo->rinfo.destroyer(&pinfo->rinfo);
    ops_free_errors(pinfo->errors);
    if(pinfo->rinfo.accumulated)
        free(pinfo->rinfo.accumulated);
    free(pinfo);
    }

/**
\ingroup Core_ReadPackets
\brief Returns the parse_info's reader_info
\return Pointer to the reader_info inside the parse_info
*/
ops_reader_info_t *ops_parse_get_rinfo(ops_parse_info_t *pinfo)
    { return &pinfo->rinfo; }

/**
\ingroup Core_ReadPackets
\brief Sets the parse_info's callback
This is used when adding the first callback in a stack of callbacks.
\sa ops_parse_cb_push()
*/

void ops_parse_cb_set(ops_parse_info_t *pinfo,ops_parse_cb_t *cb,void *arg)
    {
    pinfo->cbinfo.cb=cb;
    pinfo->cbinfo.arg=arg;
    pinfo->cbinfo.errors=&pinfo->errors;
    }

/**
\ingroup Core_ReadPackets
\brief Adds a further callback to a stack of callbacks
\sa ops_parse_cb_set()
*/
void ops_parse_cb_push(ops_parse_info_t *pinfo,ops_parse_cb_t *cb,void *arg)
    {
    ops_parse_cb_info_t *cbinfo=malloc(sizeof *cbinfo);

    *cbinfo=pinfo->cbinfo;
    pinfo->cbinfo.next=cbinfo;
    ops_parse_cb_set(pinfo,cb,arg);
    }

/**
\ingroup Core_ReadPackets
\brief Returns callback's arg
*/
void *ops_parse_cb_get_arg(ops_parse_cb_info_t *cbinfo)
    { return cbinfo->arg; }

/**
\ingroup Core_ReadPackets
\brief Returns callback's errors
*/
void *ops_parse_cb_get_errors(ops_parse_cb_info_t *cbinfo)
    { return cbinfo->errors; }

/**
\ingroup Core_ReadPackets
\brief Calls the parse_cb_info's callback if present
\return Return value from callback, if present; else OPS_FINISHED
*/
ops_parse_cb_return_t ops_parse_cb(const ops_parser_content_t *content,
				   ops_parse_cb_info_t *cbinfo)
    { 
    if(cbinfo->cb)
	return cbinfo->cb(content,cbinfo); 
    else
	return OPS_FINISHED;
    }

/**
\ingroup Core_ReadPackets
\brief Calls the next callback  in the stack
\return Return value from callback
*/
ops_parse_cb_return_t ops_parse_stacked_cb(const ops_parser_content_t *content,
					   ops_parse_cb_info_t *cbinfo)
    { return ops_parse_cb(content,cbinfo->next); }

/**
\ingroup Core_ReadPackets
\brief Returns the parse_info's errors
\return parse_info's errors
*/
ops_error_t *ops_parse_info_get_errors(ops_parse_info_t *pinfo)
    { return pinfo->errors; }

ops_crypt_t *ops_parse_get_decrypt(ops_parse_info_t *pinfo)
    {
    if(pinfo->decrypt.algorithm)
	return &pinfo->decrypt;
    return NULL;
    }

// XXX: this could be improved by sharing all hashes that are the
// same, then duping them just before checking the signature.
void ops_parse_hash_init(ops_parse_info_t *pinfo,ops_hash_algorithm_t type,
			 const unsigned char *keyid)
    {
    ops_parse_hash_info_t *hash;

    pinfo->hashes=realloc(pinfo->hashes,
			  (pinfo->nhashes+1)*sizeof *pinfo->hashes);
    hash=&pinfo->hashes[pinfo->nhashes++];

    ops_hash_any(&hash->hash,type);
    hash->hash.init(&hash->hash);
    memcpy(hash->keyid,keyid,sizeof hash->keyid);
    }

void ops_parse_hash_data(ops_parse_info_t *pinfo,const void *data,
			 size_t length)
    {
    size_t n;

    for(n=0 ; n < pinfo->nhashes ; ++n)
	pinfo->hashes[n].hash.add(&pinfo->hashes[n].hash,data,length);
    }

ops_hash_t *ops_parse_hash_find(ops_parse_info_t *pinfo,
				const unsigned char keyid[OPS_KEY_ID_SIZE])
    {
    size_t n;

    for(n=0 ; n < pinfo->nhashes ; ++n)
	if(!memcmp(pinfo->hashes[n].keyid,keyid,OPS_KEY_ID_SIZE))
	    return &pinfo->hashes[n].hash;
    return NULL;
    }

/* vim:set textwidth=120: */
/* vim:set ts=8: */

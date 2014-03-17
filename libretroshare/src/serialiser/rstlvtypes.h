#ifndef RS_TLV_COMPOUND_TYPES_H
#define RS_TLV_COMPOUND_TYPES_H

/*
 * libretroshare/src/serialiser: rstlvtypes.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie, Chris Parker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

/*******************************************************************
 * These are the Compound TLV structures that must be (un)packed.
 *
 ******************************************************************/

#include <string>
#include <list>
#include <map>

#include <stdlib.h>
#include <stdint.h>
#include <retroshare/rstypes.h>
#include <serialiser/rstlvtypes.h>
#include <serialiser/rstlvbase.h>
#include <serialiser/rsbaseserial.h>


//! A base class for all tlv items 
/*! This class is provided to allow the serialisation and deserialization of compund 
tlv items 
*/
class RsTlvItem
{
	public:
	 RsTlvItem() { return; }
virtual ~RsTlvItem() { return; }
virtual uint32_t TlvSize() = 0;
virtual void	 TlvClear() = 0;
virtual	void	 TlvShallowClear(); /*! Don't delete allocated data */
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset) = 0; /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset) = 0; /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent) = 0;
std::ostream &printBase(std::ostream &out, std::string clsName, uint16_t indent);
std::ostream &printEnd(std::ostream &out, std::string clsName, uint16_t indent);
};

std::ostream &printIndent(std::ostream &out, uint16_t indent);

//! GENERIC Binary Data TLV
/*! Use to serialise and deserialise binary data, usually included in other compound tlvs
*/
class RsTlvBinaryData: public RsTlvItem
{
	public:
	 RsTlvBinaryData(uint16_t t);
         RsTlvBinaryData(const RsTlvBinaryData& b); // as per rule of three
         void operator=(const RsTlvBinaryData& b); // as per rule of three
virtual ~RsTlvBinaryData(); // as per rule of three
virtual	uint32_t TlvSize();
virtual	void	 TlvClear(); /*! Initialize fields to empty legal values ( "0", "", etc) */
virtual	void	 TlvShallowClear(); /*! Don't delete the binary data */

 /// Serialise.
/*! Serialise Tlv to buffer(*data) of 'size' bytes starting at *offset */
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset);

/// Deserialise.
/*! Deserialise Tlv buffer(*data) of 'size' bytes starting at *offset */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
virtual std::ostream &print(std::ostream &out, uint16_t indent); /*! Error/Debug util function */

bool    setBinData(const void *data, uint32_t size);

	uint16_t tlvtype;	/// set/checked against TLV input 
	uint32_t bin_len;	/// size of malloc'ed data (not serialised) 
	void    *bin_data;	/// mandatory
};


/**** MORE TLV *****
 *
 *
 */

class RsTlvStringSet: public RsTlvItem
{
	public:
	 RsTlvStringSet(uint16_t type);
virtual ~RsTlvStringSet() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);
virtual std::ostream &printHex(std::ostream &out, uint16_t indent); /* SPECIAL One */

	uint16_t mType;
	std::list<std::string> ids; /* Mandatory */
};

template<class ID_CLASS,uint32_t TLV_TYPE> class t_RsTlvIdSet: public RsTlvItem
{
	public:
		t_RsTlvIdSet() {}
		virtual ~t_RsTlvIdSet() {}

		virtual uint32_t TlvSize() { return ID_CLASS::SIZE_IN_BYTES * ids.size() + TLV_HEADER_SIZE; }
		virtual void TlvClear(){ ids.clear() ; }
		virtual bool SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
		{	/* must check sizes */
			uint32_t tlvsize = TlvSize();
			uint32_t tlvend  = *offset + tlvsize;

			if (size < tlvend)
				return false; /* not enough space */

			bool ok = true;

			/* start at data[offset] */
			ok = ok && SetTlvBase(data, tlvend, offset, TLV_TYPE, tlvsize);

			for(typename std::list<ID_CLASS>::const_iterator it(ids.begin());it!=ids.end();++it)
				ok = ok && (*it).serialise(data,size,*offset) ;

			return ok ;
		}
		virtual bool GetTlv(void *data, uint32_t size, uint32_t *offset) /* deserialise */
		{
			if (size < *offset + TLV_HEADER_SIZE)
				return false;

			uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
			uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
			uint32_t tlvend = *offset + tlvsize;

			if (size < tlvend)    /* check size */
				return false; /* not enough space */

			if (tlvtype != TLV_TYPE) /* check type */
				return false;

			bool ok = true;

			/* ready to load */
			TlvClear();

			/* skip the header */
			(*offset) += TLV_HEADER_SIZE;

			while(*offset + ID_CLASS::SIZE_IN_BYTES <= tlvend)
			{
				ID_CLASS id ;
				ok = ok && id.deserialise(data,size,*offset) ;
				ids.push_back(id) ;
			}
			if(*offset != tlvend)
				std::cerr << "(EE) deserialisaiton error in " << __PRETTY_FUNCTION__ << std::endl;
			return *offset == tlvend ;
		}
		virtual std::ostream &print(std::ostream &out, uint16_t indent)
		{
			std::cerr << __PRETTY_FUNCTION__ << ": not implemented" << std::endl;
			return out ;
		}
		virtual std::ostream &printHex(std::ostream &out, uint16_t indent) /* SPECIAL One */
		{
			std::cerr << __PRETTY_FUNCTION__ << ": not implemented" << std::endl;
			return out ;
		}

		std::list<ID_CLASS> ids ;
};

typedef t_RsTlvIdSet<RsPeerId,TLV_TYPE_PEERSET>		RsTlvPeerIdSet ;
typedef t_RsTlvIdSet<RsPgpId,TLV_TYPE_PGPIDSET>	RsTlvPgpIdSet ;
typedef t_RsTlvIdSet<Sha1CheckSum,TLV_TYPE_HASHSET> 	RsTlvHashSet ;

class RsTlvServiceIdSet: public RsTlvItem
{
	public:
	 RsTlvServiceIdSet() { return; }
virtual ~RsTlvServiceIdSet() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::list<uint32_t> ids; /* Mandatory */
};


class RsTlvStringSetRef: public RsTlvItem
{
	public:
	 RsTlvStringSetRef(uint16_t type, std::list<std::string> &refids);
virtual ~RsTlvStringSetRef() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	uint16_t mType;
	std::list<std::string> &ids; /* Mandatory */
};


/**** MORE TLV *****
 *
 * File Items/Data.
 *
 */


class RsTlvFileItem: public RsTlvItem
{
	public:
	 RsTlvFileItem();
virtual ~RsTlvFileItem() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	uint64_t filesize; /// Mandatory: size of file to be downloaded
    RsFileHash hash;  /// Mandatory: to find file
	std::string name;  /// Optional: name of file
	std::string path;  /// Optional: path on host computer
	uint32_t    pop;   /// Optional: Popularity of file
	uint32_t    age;   /// Optional: age of file
	// For chunk hashing.
	uint32_t    piecesize;  /// Optional: bytes/piece for hashset.
	RsTlvHashSet hashset; /// Optional: chunk hashes.

};

class RsTlvFileSet: public RsTlvItem
{
	public:
	 RsTlvFileSet() { return; }
virtual ~RsTlvFileSet() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::list<RsTlvFileItem> items; /// Mandatory 
	std::string title;   		/// Optional: title of file set
	std::string comment; 		/// Optional: comments for file
};


class RsTlvFileData: public RsTlvItem
{
	public:
	 RsTlvFileData(); 
	 virtual ~RsTlvFileData() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	RsTlvFileItem   file;         /// Mandatory: file information	
	uint64_t        file_offset;  /// Mandatory: where to start in bin data
	RsTlvBinaryData binData;      /// Mandatory: serialised file info
};




/**** MORE TLV *****
 * Key/Value + Set used for Generic Configuration Parameters.
 *
 */


class RsTlvKeyValue: public RsTlvItem
{
	public:
	 RsTlvKeyValue() { return; }
	 RsTlvKeyValue(const std::string& k,const std::string& v): key(k),value(v) {}
virtual ~RsTlvKeyValue() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::string key;	/// Mandatory : For use in hash tables
	std::string value;	/// Mandatory : For use in hash tables
};

class RsTlvKeyValueSet: public RsTlvItem
{
	public:
	 RsTlvKeyValueSet() { return; }
virtual ~RsTlvKeyValueSet() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::list<RsTlvKeyValue> pairs; /// For use in hash tables 
};


class RsTlvImage: public RsTlvItem
{
	public:
	 RsTlvImage(); 
	 RsTlvImage(const RsTlvImage& );
	 virtual ~RsTlvImage() { return; }
virtual uint32_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */

virtual std::ostream &print(std::ostream &out, uint16_t indent);

	uint32_t        image_type;   // Mandatory: 
	RsTlvBinaryData binData;      // Mandatory: serialised file info
};

#endif


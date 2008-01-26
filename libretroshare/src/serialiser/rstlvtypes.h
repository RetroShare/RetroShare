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

//#define RS_TLV_TYPE_FILE_ITEM   0x0000

//! A base class for all tlv items 
/*! This class is provided to allow the serialisation and deserialization of compund 
tlv items 
*/
class RsTlvItem
{
	public:
	 RsTlvItem() { return; }
virtual ~RsTlvItem() { return; }
virtual uint16_t TlvSize() = 0;
virtual void	 TlvClear() = 0;
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
	 virtual	~RsTlvBinaryData() { return;}
virtual	uint16_t TlvSize();
virtual	void	 TlvClear(); /*! Initialize fields to empty legal values ( "0", "", etc) */
 /// Serialise.
/*! Serialise Tlv to buffer(*data) of 'size' bytes starting at *offset */
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset);
/// Deserialise.
/*! Deserialise Tlv buffer(*data) of 'size' bytes starting at *offset */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); 
virtual std::ostream &print(std::ostream &out, uint16_t indent); /*! Error/Debug util function */

bool    setBinData(void *data, uint16_t size);

	uint16_t tlvtype;	/// set/checked against TLV input 
	uint16_t bin_len;	/// size of malloc'ed data (not serialised) 
	void    *bin_data;	/// mandatory
};

class RsTlvFileItem: public RsTlvItem
{
	public:
	 RsTlvFileItem() { return; }
virtual ~RsTlvFileItem() { return; }
virtual uint16_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

uint64_t filesize; /// Mandatory: size of file to be downloaded
	std::string hash;  /// Mandatory: to find file
	std::string name;  /// Optional: name of file
	std::string path;  /// Optional: path on host computer
	uint32_t    pop;   /// Optional: Popularity of file
	uint32_t    age;   /// Optional: age of file
};

class RsTlvFileSet: public RsTlvItem
{
	public:
	 RsTlvFileSet() { return; }
virtual ~RsTlvFileSet() { return; }
virtual uint16_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::list<RsTlvFileItem> items; /// Mandatory 
	std::wstring title;   		/// Optional: title of file set
	std::wstring comment; 		/// Optional: comments for file
};


class RsTlvFileData: public RsTlvItem
{
	public:
	 RsTlvFileData(); 
	 virtual ~RsTlvFileData() { return; }
virtual uint16_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	RsTlvFileItem   file;         /// Mandatory: file information	
	uint64_t        file_offset;  /// Mandatory: where to start in bin data
	RsTlvBinaryData binData;      /// Mandatory: serialised file info
};


/**** MORE TLV *****
 *
 *
 */


class RsTlvPeerIdSet: public RsTlvItem
{
	public:
	 RsTlvPeerIdSet() { return; }
virtual ~RsTlvPeerIdSet() { return; }
virtual uint16_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::list<std::string> ids; /* Mandatory */
};


class RsTlvServiceIdSet: public RsTlvItem
{
	public:
	 RsTlvServiceIdSet() { return; }
virtual ~RsTlvServiceIdSet() { return; }
virtual uint16_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::list<uint32_t> ids; /* Mandatory */
};




/**** MORE TLV *****
 * Key/Value + Set used for Generic Configuration Parameters.
 *
 */


class RsTlvKeyValue: public RsTlvItem
{
	public:
	 RsTlvKeyValue() { return; }
virtual ~RsTlvKeyValue() { return; }
virtual uint16_t TlvSize();
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
virtual uint16_t TlvSize();
virtual void	 TlvClear();
virtual bool     SetTlv(void *data, uint32_t size, uint32_t *offset); /* serialise   */
virtual bool     GetTlv(void *data, uint32_t size, uint32_t *offset); /* deserialise */
virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::list<RsTlvKeyValue> pairs; /// For use in hash tables 
};



#endif


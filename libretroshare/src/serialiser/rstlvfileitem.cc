/*******************************************************************************
 * libretroshare/src/serialiser: rstlvfileitem.cc                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie,Chris Parker <retroshare@lunamutt.com> *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include "serialiser/rstlvfileitem.h"
#include "serialiser/rsbaseserial.h"

#if 0
#include <iostream>

#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#endif

/***
 * #define TLV_FI_DEBUG 1
 **/


RsTlvFileItem::RsTlvFileItem()
{
	TlvClear();
}

void RsTlvFileItem::TlvClear()
{
	filesize = 0;
	hash.clear() ;
	name.clear();
	path.clear();
	pop = 0;
	age = 0;
	piecesize = 0;
	hashset.TlvClear();
}

uint32_t    RsTlvFileItem::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header */
	s += 8; /* filesize */
    s += hash.serial_size() ;
#ifdef TLV_FI_DEBUG
	std::cerr << "RsTlvFileItem::TlvSize() 8 + Hash: " << s << std::endl;
#endif
	

	/* now optional ones */
	if (name.length() > 0)
	{
		s += GetTlvStringSize(name); 
#ifdef TLV_FI_DEBUG
	std::cerr << "RsTlvFileItem::TlvSize() + Name: " << s << std::endl;
#endif
	}

	if (path.length() > 0)
	{
		s += GetTlvStringSize(path); 
#ifdef TLV_FI_DEBUG
	std::cerr << "RsTlvFileItem::TlvSize() + Path: " << s << std::endl;
#endif
	}

	if (pop != 0)
	{
		s += GetTlvUInt32Size();
#ifdef TLV_FI_DEBUG
	std::cerr << "RsTlvFileItem::TlvSize() + Pop: " << s << std::endl;
#endif
	}

	if (age != 0)
	{
		s += GetTlvUInt32Size(); 
#ifdef TLV_FI_DEBUG
	std::cerr << "RsTlvFileItem::TlvSize() 4 + Age: " << s << std::endl;
#endif
	}

	if (piecesize != 0)
	{
		s += GetTlvUInt32Size(); 
#ifdef TLV_FI_DEBUG
	std::cerr << "RsTlvFileItem::TlvSize() 4 + PieceSize: " << s << std::endl;
#endif
	}

	if (hashset.ids.size() != 0)
	{
		s += hashset.TlvSize(); 
#ifdef TLV_FI_DEBUG
	std::cerr << "RsTlvFileItem::TlvSize() 4 + HashSet: " << s << std::endl;
#endif
	}

#ifdef TLV_FI_DEBUG
	std::cerr << "RsTlvFileItem::TlvSize() Total: " << s << std::endl;
#endif

	return s;
}

/* serialise the data to the buffer */
bool     RsTlvFileItem::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
	{
#ifdef TLV_FI_DEBUG
		std::cerr << "RsTlvFileItem::SetTlv() Failed size (" << size;
		std::cerr << ") < tlvend (" << tlvend << ")" << std::endl;
#endif
		return false; /* not enough space */
	}

	bool ok = true;

	/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_FILEITEM, tlvsize);

#ifdef TLV_FI_DEBUG
	if (!ok)
	{
		std::cerr << "RsTlvFileItem::SetTlv() SetTlvBase Failed (or earlier)" << std::endl;
	}
	std::cerr << "RsTlvFileItem::SetTlv() PostBase:" << std::endl;
	std::cerr << "RsTlvFileItem::SetTlv() Data: " << data << " size: " << size << " offset: " << *offset;
	std::cerr << std::endl;
#endif


	/* add mandatory parts first */
	ok &= setRawUInt64(data, tlvend, offset, filesize);

#ifdef TLV_FI_DEBUG
	if (!ok)
	{
		std::cerr << "RsTlvFileItem::SetTlv() SetRawUInt32(FILESIZE) Failed (or earlier)" << std::endl;
	}
	std::cerr << "RsTlvFileItem::SetTlv() PostSize:" << std::endl;
	std::cerr << "RsTlvFileItem::SetTlv() Data: " << data << " size: " << size << " offset: " << *offset;
	std::cerr << std::endl;
#endif

    ok &= hash.serialise(data, tlvend, *offset) ;


#ifdef TLV_FI_DEBUG
	if (!ok)
	{
		std::cerr << "RsTlvFileItem::SetTlv() SetTlvString(HASH) Failed (or earlier)" << std::endl;
	}
	std::cerr << "RsTlvFileItem::SetTlv() PostHash:" << std::endl;
	std::cerr << "RsTlvFileItem::SetTlv() Data: " << data << " size: " << size << " offset: " << *offset;
	std::cerr << std::endl;
#endif

	/* now optional ones */
	if (name.length() > 0)
		ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_NAME, name);
#ifdef TLV_FI_DEBUG
	if (!ok)
	{
		std::cerr << "RsTlvFileItem::SetTlv() Setting Option:Name Failed (or earlier)" << std::endl;
	}
	std::cerr << "RsTlvFileItem::SetTlv() PostName:" << std::endl;
	std::cerr << "RsTlvFileItem::SetTlv() Data: " << data << " size: " << size << " offset: " << *offset;
	std::cerr << std::endl;
#endif

	if (path.length() > 0)
		ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_PATH, path);
#ifdef TLV_FI_DEBUG
	if (!ok)
	{
		std::cerr << "RsTlvFileItem::SetTlv() Setting Option:Path Failed (or earlier)" << std::endl;
	}
	std::cerr << "RsTlvFileItem::SetTlv() Pre Pop:" << std::endl;
	std::cerr << "RsTlvFileItem::SetTlv() Data: " << data << " size: " << size << " offset: " << *offset;
	std::cerr << " type: " << TLV_TYPE_UINT32_POP << " value: " << pop;
	std::cerr << std::endl;
#endif

	if (pop != 0)
		ok &= SetTlvUInt32(data, tlvend, offset, TLV_TYPE_UINT32_POP,  pop);
#ifdef TLV_FI_DEBUG
	if (!ok)
	{
		std::cerr << "RsTlvFileItem::SetTlv() Setting Option:Pop Failed (or earlier)" << std::endl;
	}
	std::cerr << "RsTlvFileItem::SetTlv() Post Pop/Pre Age:" << std::endl;
	std::cerr << "RsTlvFileItem::SetTlv() Data: " << data << " size: " << size << " offset: " << *offset;
	std::cerr << " type: " << TLV_TYPE_UINT32_AGE << " value: " << age;
	std::cerr << std::endl;
#endif

	if (age != 0)
		ok &= SetTlvUInt32(data, tlvend, offset, TLV_TYPE_UINT32_AGE,  age);
#ifdef TLV_FI_DEBUG
	if (!ok)
	{
		std::cerr << "RsTlvFileItem::SetTlv() Setting Option:Age Failed (or earlier)" << std::endl;
	}
	std::cerr << "RsTlvFileItem::SetTlv() Post Age:" << std::endl;
	std::cerr << "RsTlvFileItem::SetTlv() Data: " << data << " size: " << size << " offset: " << *offset;
	std::cerr << std::endl;
#endif

	if (piecesize != 0)
		ok &= SetTlvUInt32(data, tlvend, offset, TLV_TYPE_UINT32_SIZE,  piecesize);
#ifdef TLV_FI_DEBUG
	if (!ok)
	{
		std::cerr << "RsTlvFileItem::SetTlv() Setting Option:piecesize Failed (or earlier)" << std::endl;
	}
	std::cerr << "RsTlvFileItem::SetTlv() Post PieceSize:" << std::endl;
	std::cerr << "RsTlvFileItem::SetTlv() Data: " << data << " size: " << size << " offset: " << *offset;
	std::cerr << std::endl;
#endif

	if (hashset.ids.size() != 0)
		ok &= hashset.SetTlv(data, tlvend, offset);
#ifdef TLV_FI_DEBUG
	if (!ok)
	{
		std::cerr << "RsTlvFileItem::SetTlv() Setting Option:hashset Failed (or earlier)" << std::endl;
	}
	std::cerr << "RsTlvFileItem::SetTlv() Post HashSet:" << std::endl;
	std::cerr << "RsTlvFileItem::SetTlv() Data: " << data << " size: " << size << " offset: " << *offset;
	std::cerr << std::endl;
#endif



#ifdef TLV_FI_DEBUG
	if (!ok)
	{
		std::cerr << "RsTlvFileItem::SetTlv() Setting Options Failed (or earlier)" << std::endl;
	}
#endif

	return ok;
}

bool     RsTlvFileItem::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_FILEITEM) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	/* get mandatory parts first */
	ok &= getRawUInt64(data, tlvend, offset, &filesize);
    ok &= hash.deserialise(data, tlvend, *offset) ;

	/* while there is more TLV (optional part) */
	while((*offset) + 2 < tlvend)
	{
		/* get the next type */
		uint16_t tlvsubtype = GetTlvType( &(((uint8_t *) data)[*offset]) );
		switch(tlvsubtype)
		{
			case TLV_TYPE_STR_NAME:
				ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_NAME, name);
				break;
			case TLV_TYPE_STR_PATH:
				ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_PATH, path);
				break;
			case TLV_TYPE_UINT32_POP:
				ok &= GetTlvUInt32(data, tlvend, offset, TLV_TYPE_UINT32_POP, &pop);
				break;
			case TLV_TYPE_UINT32_AGE:
				ok &= GetTlvUInt32(data, tlvend, offset, TLV_TYPE_UINT32_AGE, &age);
				break;
			case TLV_TYPE_UINT32_SIZE:
				ok &= GetTlvUInt32(data, tlvend, offset, TLV_TYPE_UINT32_SIZE, &piecesize);
				break;
			case TLV_TYPE_HASHSET:
				ok &= hashset.GetTlv(data, tlvend, offset);
				break;
			default:
				ok &= SkipUnknownTlv(data, tlvend, offset);
				break;
		}
		if (!ok)
		{
			break;
		}
	}

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvFileItem::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
}


/* print it out */
std::ostream &RsTlvFileItem::print(std::ostream &out, uint16_t indent) const
{
	printBase(out, "RsTlvFileItem", indent);
	uint16_t int_Indent = indent + 2;


	printIndent(out, int_Indent);
	out << "Mandatory:  FileSize: " << filesize << " Hash: " << hash; 
	out << std::endl;

	printIndent(out, int_Indent);
	out << "Optional:" << std::endl;

	/* now optional ones */
	if (name.length() > 0)
	{
		printIndent(out, int_Indent);
        	out << "Name :  " << name << std::endl;
	}
	if (path.length() > 0)
	{
		printIndent(out, int_Indent);
        	out << "Path :  " << path  << std::endl;
	}
	if (pop != 0)
	{
		printIndent(out, int_Indent);
		out << "Pop: " << pop << std::endl;
	}
	if (age != 0)
	{
		printIndent(out, int_Indent);
		out << "Age: " << age << std::endl;
	}
	if (piecesize != 0)
	{
		printIndent(out, int_Indent);
		out << "PieceSize: " << piecesize << std::endl;
	}
	if (hashset.ids.size() != 0)
	{
                hashset.print(out, int_Indent);
	}

	printEnd(out, "RsTlvFileItem", indent);

	return out;
}


/************************************* RsTlvFileSet ************************************/

void RsTlvFileSet::TlvClear()
{
	title.clear();
	comment.clear();
	items.clear();  
}

uint32_t RsTlvFileSet::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header */

	/* first determine the total size of RstlvFileItems in list */

	std::list<RsTlvFileItem>::const_iterator it;
	
	for(it = items.begin(); it != items.end() ; ++it)
	{
		s += (*it).TlvSize();
	}

	/* now add comment and title length of this tlv object */

	if (title.length() > 0)
		s += GetTlvStringSize(title); 
	if (comment.length() > 0)
		s += GetTlvStringSize(comment);

	return s;
}


/* serialize data to the buffer */

bool     RsTlvFileSet::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_FILESET, tlvsize);
        
	    /* add mandatory parts first */
	std::list<RsTlvFileItem>::const_iterator it;
	
	for(it = items.begin(); it != items.end() ; ++it)  
	{
		ok &= (*it).SetTlv(data, size, offset);
		/* drop out if fails */
		if (!ok)
			return false;
	}

	/* now optional ones */
	if (title.length() > 0)
		ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_TITLE, title);
	if (comment.length() > 0)
		ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_COMMENT, comment); 
	
	return ok;

}


bool     RsTlvFileSet::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_FILESET) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	/* while there is more TLV  */
	while((*offset) + 2 < tlvend)
	{
		/* get the next type */
		uint16_t tlvsubtype = GetTlvType( &(((uint8_t *) data)[*offset]) );
		if (tlvsubtype == TLV_TYPE_FILEITEM)
		{
			RsTlvFileItem newitem;
			ok &= newitem.GetTlv(data, size, offset);
			if (ok)
			{
				items.push_back(newitem);
			}
		}
		else if (tlvsubtype == TLV_TYPE_STR_TITLE)
		{
			ok &= GetTlvString(data, tlvend, offset, 
					TLV_TYPE_STR_TITLE, title);
		}
		else if (tlvsubtype == TLV_TYPE_STR_COMMENT)
		{
			ok &= GetTlvString(data, tlvend, offset, 
					TLV_TYPE_STR_COMMENT, comment);
		}
		else
		{
			/* unknown subtype -> error */
			ok &= SkipUnknownTlv(data, tlvend, offset);
		}

		if (!ok)
		{
			break;
		}
	}

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvFileSet::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
}

/* print it out */

std::ostream &RsTlvFileSet::print(std::ostream &out, uint16_t indent) const
{
	printBase(out, "RsTlvFileSet", indent);
	uint16_t int_Indent = indent + 2;


	printIndent(out, int_Indent);
	out << "Mandatory:"  << std::endl;
	std::list<RsTlvFileItem>::const_iterator it;
	
	for(it = items.begin(); it != items.end() ; ++it)
	{
		it->print(out, int_Indent);
	}
	printIndent(out, int_Indent);
	out << "Optional:" << std::endl;

	/* now optional ones */
	if (title.length() > 0)
	{
		printIndent(out, int_Indent);
		std::string cnv_title(title.begin(), title.end());
		out << "Title: " << cnv_title << std::endl;
	}
	if (comment.length() > 0)
	{
		printIndent(out, int_Indent);
		std::string cnv_comment(comment.begin(), comment.end());
		out << "Comment: " << cnv_comment << std::endl;
	}

	printEnd(out, "RsTlvFileSet", indent);

	return out;
}


/************************************* RsTlvFileData ************************************/
	
RsTlvFileData::RsTlvFileData()
	:RsTlvItem(), file_offset(0), binData(TLV_TYPE_BIN_FILEDATA)
{
	return;
}

void RsTlvFileData::TlvClear()
{
	file.TlvClear();
	binData.TlvClear();
	file_offset = 0;
}


uint32_t RsTlvFileData::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header */

	/* collect sizes for both uInts and data length */
	s+= file.TlvSize();
	s+= GetTlvUInt64Size();
	s+= binData.TlvSize();

	return s;
}


bool RsTlvFileData::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_FILEDATA , tlvsize);

	/* add mandatory part */
	ok &= file.SetTlv(data, size, offset);
	ok &= SetTlvUInt64(data,size,offset,
			TLV_TYPE_UINT64_OFFSET,file_offset);
	ok &= binData.SetTlv(data, size, offset);

	return ok;


}

bool RsTlvFileData::GetTlv(void *data, uint32_t size, uint32_t *offset) 
{
	if (size < *offset + TLV_HEADER_SIZE)
	{
		return false;
	}

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_FILEDATA) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	ok &= file.GetTlv(data, size, offset);
	ok &= GetTlvUInt64(data,size,offset, 
			TLV_TYPE_UINT64_OFFSET,&file_offset);
	ok &= binData.GetTlv(data, size, offset);


	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvFileData::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;

}

/* print it out */
std::ostream &RsTlvFileData::print(std::ostream &out, uint16_t indent) const
{
	printBase(out, "RsTlvFileData", indent);
	uint16_t int_Indent = indent + 2;

	file.print(out, int_Indent);

	printIndent(out, int_Indent);
	out << "FileOffset: " << file_offset;
	out << std::endl;

	binData.print(out, int_Indent);

	printEnd(out, "RsTlvFileData", indent);
	return out;

}


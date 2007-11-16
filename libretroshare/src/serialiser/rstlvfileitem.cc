
/*
 * libretroshare/src/serialiser: rstlvfileitem.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include <iostream>

#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rsbaseserial.h"

/***
 * #define TLV_FI_DEBUG 1
 **/

void RsTlvFileItem::TlvClear()
{
	filesize = 0;
	hash = "";
	name = "";
	path = "";
	pop = 0;
	age = 0;
}

uint16_t    RsTlvFileItem::TlvSize()
{
	uint32_t s = 8; /* header + 4 for size */
	s += GetTlvStringSize(hash); 
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

#ifdef TLV_FI_DEBUG
	std::cerr << "RsTlvFileItem::TlvSize() Total: " << s << std::endl;
#endif

	return s;
}

/* serialise the data to the buffer */
bool     RsTlvFileItem::SetTlv(void *data, uint32_t size, uint32_t *offset)
{
	/* must check sizes */
	uint16_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

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
	ok &= setRawUInt32(data, tlvend, offset, filesize);

#ifdef TLV_FI_DEBUG
	if (!ok)
	{
		std::cerr << "RsTlvFileItem::SetTlv() SetRawUInt32(FILESIZE) Failed (or earlier)" << std::endl;
	}
	std::cerr << "RsTlvFileItem::SetTlv() PostSize:" << std::endl;
	std::cerr << "RsTlvFileItem::SetTlv() Data: " << data << " size: " << size << " offset: " << *offset;
	std::cerr << std::endl;
#endif

	ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_HASH, hash);


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
	uint16_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_FILEITEM) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += 4;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, tlvend, offset, &filesize);
	ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_HASH, hash);

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
			default:
				ok = false;
		}
		if (!ok)
		{
			return false;
		}
	}
	return ok;
}


/* print it out */
std::ostream &RsTlvFileItem::print(std::ostream &out, uint16_t indent)
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
		out << "Name: " << name << std::endl;
	}
	if (path.length() > 0)
	{
		printIndent(out, int_Indent);
		out << "Path: " << path << std::endl;
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

	printEnd(out, "RsTlvFileItem", indent);

	return out;
}


/************************************* RsTlvFileSet ************************************/

void RsTlvFileSet::TlvClear()
{
	title = "";
	comment = "";
	items.clear();  
}

uint16_t RsTlvFileSet::TlvSize()
{
	uint32_t s = 4; /* header */

	/* first determine the total size of RstlvFileItems in list */

	std::list<RsTlvFileItem>::iterator it;
	
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

bool     RsTlvFileSet::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	/* must check sizes */
	uint16_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_FILESET, tlvsize);
        
	    /* add mandatory parts first */
	std::list<RsTlvFileItem>::iterator it;
	
	for(it = items.begin(); it != items.end() ; ++it)  
	{
		ok &= (*it).SetTlv(data, size, offset);
		/* drop out if fails */
		if (!ok)
			return false;
	}

	/* now optional ones */
	if (title.length() > 0)
		ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_TITLE, title);  // no base tlv type for title?
	if (comment.length() > 0)
		ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_COMMENT, comment); // no base tlv type for comment?
	
	return ok;

}


bool     RsTlvFileSet::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + 4)
		return false;

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint16_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_FILESET) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += 4;

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
			ok = false;
		}

		if (!ok)
		{
			return false;
		}
	}

	return ok;
}

/* print it out */

std::ostream &RsTlvFileSet::print(std::ostream &out, uint16_t indent)
{
	printBase(out, "RsTlvFileSet", indent);
	uint16_t int_Indent = indent + 2;


	printIndent(out, int_Indent);
	out << "Mandatory:"  << std::endl;
	std::list<RsTlvFileItem>::iterator it;
	
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
		out << "Title: " << title << std::endl;
	}
	if (comment.length() > 0)
	{
		printIndent(out, int_Indent);
		out << "Comment: " << comment << std::endl;
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


uint16_t RsTlvFileData::TlvSize()
{
	uint32_t s = 4; /* header */

	/* collect sizes for both uInts and data length */
	s+= file.TlvSize();
	s+= GetTlvUInt32Size();
	s+= binData.TlvSize();

	return s;
}


bool RsTlvFileData::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	/* must check sizes */
	uint16_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_FILEDATA , tlvsize);

	/* add mandatory part */
	ok &= file.SetTlv(data, size, offset);
	ok &= SetTlvUInt32(data,size,offset,
			TLV_TYPE_UINT32_OFFSET,file_offset);
	ok &= binData.SetTlv(data, size, offset);

	return ok;


}

bool RsTlvFileData::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	if (size < *offset + 4)
	{
		return false;
	}

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint16_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_FILEDATA) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += 4;

	ok &= file.GetTlv(data, size, offset);
	ok &= GetTlvUInt32(data,size,offset, 
			TLV_TYPE_UINT32_OFFSET,&file_offset);
	ok &= binData.GetTlv(data, size, offset);

	return ok;

}

/* print it out */
std::ostream &RsTlvFileData::print(std::ostream &out, uint16_t indent)
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


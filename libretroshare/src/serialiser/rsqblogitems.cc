
/*
 * libretroshare/src/serialiser: rsqblogitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Chris Parker.
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

#include "serialiser/rsbaseserial.h"
#include "serialiser/rsqblogitems.h"
#include "serialiser/rstlvbase.h"

#include <iostream>

/************************************************************/

RsQblogItem::~RsQblogItem(void)
{
}

void RsQblogItem::clear()
{
	blogMsgs.empty();
	status = "";
}


std::ostream &RsQblogItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsQblogItem", indent);
		uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent);
        out << "blogMsgs: ";
        
   		for(std::multimap<uin32_t, std::string>::iterator it = blogMsgs.begin()
   		; it != blogMsgs.end(); it++)
   		{
   			std::pair<uint32_t, std::string> chkMsgs = (std::pair<uint32_t, std::String>) it;
   			out << chkMsgs.first << std::endl;
   			out << chkMsgs.second << std::endl;
   		}
   		
		std::string cnv_message(message.begin(), message.end());
        out << "status  " << status  << std::endl;

        printRsItemEnd(out, "RsQblogItem", indent);
        return out;
}

RsQblogSerialiser::~RsQblogSerialiser()
{
}


RsQblogSerialiser::sizeItem(RsQblogItem *item)
{
	uint32_t s = 8; // for header size
	for(std::multimap<uin32_t, std::string>::iterator it = blogMsgs.begin()
   		; it != blogMsgs.end(); it++)
   		{
   			std::pair<uint32_t, std::string> chkMsgs = (std::pair<uint32_t, std::String>) it;
   			s += 4; // client time
   			s += GetTlvStringSize(chkMsgs.second.size());
   		}
   	
   	return s;
}

bool RsQblogSerialiser::serialiseItem(RsQblogItem* item, void* data, u_int32_t *size)
{
	//TODO
	return true;
}

bool RsQblogSerialiser::deserialiseItem(void * data, uint32_t *size)
{
	//TODO
	return bool;
}

bool RsQblogSerialiser::serialise(RsItem *item, void* data, uint32_t* size)
{
	//TODO
	return bool;
}

bool RsQblogSerialiser::deserialise(void* datam, uint32_t* size)
{
	//TODO
	return bool;
}
	

uint32_t RsQblogSerialiser::size(RsItem *)
{
	return sizeItem((RsQblogItem *) item);
}





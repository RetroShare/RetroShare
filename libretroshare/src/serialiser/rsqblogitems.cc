
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

#include <iostream>

#include "../serialiser/rsbaseserial.h"
#include "../serialiser/rsqblogitems.h"
#include "../serialiser/rstlvbase.h"


/************************************************************/

RsQblogMsg::~RsQblogMsg(void)
{
	return;
}


std::ostream &RsQblogMsg::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsQblogMsg", indent);
		uint16_t int_Indent = indent + 2;

		/* print out the content of the item */
        printIndent(out, int_Indent);
        out << "blogMsg(send time): " << sendTime << std::endl;
        printIndent(out, int_Indent);
        out << "blogMsg(recvd time): " << recvTime << std::endl;
        printIndent(out, int_Indent);
        std::string cnv_blog(message.begin(), message.end());
        out << "blogMsg(message): " << cnv_blog << std::endl;
        printIndent(out, int_Indent);
      	attachment.print(out, int_Indent);
      	printIndent(out, int_Indent);
        return out;
}

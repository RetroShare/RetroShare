
/*
 * bitdht/bdobj.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */

#include "bitdht/bdobj.h"


void bdPrintTransId(std::ostream &out, bdToken *transId)
{
	out << transId->data;
	return;
}


void bdPrintToken(std::ostream &out, bdToken *token)
{
	for(unsigned int i = 0; i < token->len; i++)
	{
		out << std::hex << (uint32_t) token->data[i];
	}
	out << std::dec;
}

void bdPrintCompactPeerId(std::ostream &out, std::string /*cpi*/ )
{
	out << "DummyCompactPeerId";
}



/*******************************************************************************
 * bitdht/bdobj.cc                                                             *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2010 by Robert Fernie <bitdht@lunamutt.com>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "bitdht/bdobj.h"


void bdPrintTransId(std::ostream &out, bdToken *transId)
{
	//out << transId->data;
	bdPrintToken(out, transId);
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



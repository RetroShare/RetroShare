/*******************************************************************************
 * bitdht/bdobj.h                                                              *
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
#ifndef BITDHT_OBJECTS_H
#define BITDHT_OBJECTS_H

#define BITDHT_TOKEN_MAX_LEN 20

#include <iostream>
#include <inttypes.h>

class bdToken
{
	public:
	bdToken() :len(0) { return; }
	uint32_t len;
	unsigned char data[BITDHT_TOKEN_MAX_LEN];
};

class bdCompactIds
{
	public:
	bdCompactIds() :len(0) { return; }
	uint32_t len;
	unsigned char data[BITDHT_TOKEN_MAX_LEN];
};

class bdVersion
{
	public:
	bdVersion() :len(0) { return; }
	uint32_t len;
	unsigned char data[BITDHT_TOKEN_MAX_LEN];
};

void bdPrintTransId(std::ostream &out, bdToken *transId);
void bdPrintToken(std::ostream &out, bdToken *transId);
void bdPrintCompactPeerId(std::ostream &out, std::string cpi);

#endif


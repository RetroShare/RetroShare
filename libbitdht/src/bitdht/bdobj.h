#ifndef BITDHT_OBJECTS_H
#define BITDHT_OBJECTS_H

/*
 * bitdht/bdobj.h
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


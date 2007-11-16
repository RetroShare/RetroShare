
/*
 * libretroshare/src/serialiser: rsbaseserial.cc
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

#include <arpa/inet.h>

#include "serialiser/rsbaseserial.h"

/* UInt32 get/set */

bool getRawUInt32(void *data, uint32_t size, uint32_t *offset, uint32_t *out)
{
	/* first check there is space */
	if (size < *offset + 4)
	{
		return false;
	}
	void *buf = (void *) &(((uint8_t *) data)[*offset]);

	/* extract the data */
	uint32_t netorder_num;
	memcpy(&netorder_num, buf, sizeof(uint32_t));

	(*out) = ntohl(netorder_num);
	(*offset) += 4;
	return true;
}
	
bool setRawUInt32(void *data, uint32_t size, uint32_t *offset, uint32_t in)
{
	/* first check there is space */
	if (size < *offset + 4)
	{
		return false;
	}

	void *buf = (void *) &(((uint8_t *) data)[*offset]);

	/* convert the data to the right format */
	uint32_t netorder_num = htonl(in);

	/* pack it in */
	memcpy(buf, &netorder_num, sizeof(uint32_t));

	(*offset) += 4;
	return true;
}








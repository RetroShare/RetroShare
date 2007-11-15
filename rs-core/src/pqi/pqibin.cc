/*
 * "$Id: pqibin.cc,v 1.4 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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




#include "pqi/pqibin.h"

BinFileInterface::BinFileInterface(char *fname, int flags)
	:bin_flags(flags), buf(NULL) 
{
	/* if read + write - open r+ */
	if ((bin_flags & BIN_FLAGS_READABLE) &&
		(bin_flags & BIN_FLAGS_WRITEABLE))
	{
		buf = fopen(fname, "r+");
		/* if the file don't exist */
		if (!buf)
		{
			buf = fopen(fname, "w+");
		}

	}
	else if (bin_flags & BIN_FLAGS_READABLE)
	{
		buf = fopen(fname, "r");
	}
	else if (bin_flags & BIN_FLAGS_WRITEABLE)
	{
		// This is enough to remove old file in Linux...
		// but not in windows.... (what to do)
		buf = fopen(fname, "w");
		fflush(buf); /* this might help windows! */
	}
	else
	{	
		/* not read or write! */
	}

	if (buf)
	{
		/* no problem */

		/* go to the end */
		fseek(buf, 0L, SEEK_END);
		/* get size */
		size = ftell(buf);
		/* back to the start */
		fseek(buf, 0L, SEEK_SET);
	}
	else
	{
		size = 0;
	}

}


BinFileInterface::~BinFileInterface()
{
	if (buf)
	{
		fclose(buf);
	}
}
		
int	BinFileInterface::senddata(void *data, int len)
{
	if (!buf)
		return -1;

	if (1 != fwrite(data, len, 1, buf))
	{
		return -1;
	}
	return len;
}

int	BinFileInterface::readdata(void *data, int len)
{
	if (!buf)
		return -1;

	if (1 != fread(data, len, 1, buf))
	{
		return -1;
	}
	return len;
}


BinMemInterface::BinMemInterface(int defsize, int flags)
	:bin_flags(flags), buf(NULL), size(defsize), 
	recvsize(0), readloc(0)
	{
		buf = malloc(defsize);
	}

BinMemInterface::~BinMemInterface() 
	{ 
		if (buf)
			free(buf);
		return; 
	}

	/* some fns to mess with the memory */
int	BinMemInterface::fseek(int loc)
	{
		if (loc <= recvsize)
		{
			readloc = loc;
			return 1;
		}
		return 0;
	}


int	BinMemInterface::senddata(void *data, int len)
	{
		if(recvsize + len > size)
		{
			/* resize? */
			return -1;
		}
		memcpy((char *) buf + recvsize, data, len);
		recvsize += len;
		return len;
	}

		
int	BinMemInterface::readdata(void *data, int len)
	{
		if(readloc + len > recvsize)
		{
			/* no more stuff? */
			return -1;
		}
		memcpy(data, (char *) buf + readloc, len);
		readloc += len;
		return len;
	}



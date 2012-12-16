/*
 * RetroShare External Interface.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "menu/stdiocomms.h"

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>


StdioComms::StdioComms(int infd, int outfd)
	:mIn(infd), mOut(outfd)
{
// fcntl not available on Windows, search alternative when needed
#ifndef WINDOWS_SYS
#if 1
// THIS Code is strange...
// It seems to mess up stderr.
// But if you redirect it -> is comes out fine. Very Weird.
// HELP!!!

	std::cerr << "StdioComms() STDERR msg 0";
	std::cerr << std::endl;
	const int fcflags = fcntl(mIn,F_GETFL);
	if (fcflags < 0) 
	{ 
		std::cerr << "StdioComms() ERROR getting fcntl FLAGS";
		std::cerr << std::endl;
		exit(1);
	}
	if (fcntl(mIn,F_SETFL,fcflags | O_NONBLOCK) <0) 
	{ 
		std::cerr << "StdioComms() ERROR setting fcntl FLAGS";
		std::cerr << std::endl;
		exit(1);
	}
	std::cerr << "StdioComms() STDERR msg 1";
	std::cerr << std::endl;
#endif
#endif // WINDOWS_SYS
}


int StdioComms::isOkay()
{
	return 1;	
}


int StdioComms::active_channels(std::list<uint32_t> &chan_ids)
{
	if (isOkay())
	{
		chan_ids.push_back(1); // only one possible here (stdin/stdout)
	}
	return 1;
}

int StdioComms::error(uint32_t chan_id, std::string msg)
{
	std::cerr << "StdioComms::error(" << msg << ")";
	std::cerr << std::endl;
	return 1;
}



int StdioComms::recv_ready(uint32_t chan_id)
{
	/* should be proper poll / select! - but we don't use this at the moment */
	return 1;
}


int StdioComms::recv(uint32_t chan_id, uint8_t *buffer, int bytes)
{
	int size = read(mIn, buffer, bytes);
	std::cerr << "StdioComms::recv() returned: " << size;
	std::cerr << std::endl;

	if (size == -1)
	{
		std::cerr << "StdioComms::recv() ERROR: " << errno;
		std::cerr << std::endl;
 		if (errno == EAGAIN)
		{
			return 0; // OKAY;
		}
	}

	return size;
}


int StdioComms::recv(uint32_t chan_id, std::string &buffer, int bytes)
{
	uint8_t tmpbuffer[bytes];
	int size = read(mIn, tmpbuffer, bytes);
	for(int i = 0; i < size; i++)
	{
		buffer += tmpbuffer[i];
	}
	return size;
}

	// these make it easier...
int StdioComms::recv_blocking(uint32_t chan_id, uint8_t *buffer, int bytes)
{
	int totalread = 0;
	while(totalread < bytes)
	{
		int size = read(mIn, &(buffer[totalread]), bytes - totalread);
		if (size < 0)
		{
			if (totalread)
				break; // partial read.
			else
				return size; // error.
		}
		totalread += size;
		usleep(1000); // minisleep - so we don't 100% CPU.
		std::cerr << "StdioComms::recv_blocking() read so far: " << size;
		std::cerr << " / " << totalread;
		std::cerr << std::endl;
	}
	return totalread;
}


int StdioComms::recv_blocking(uint32_t chan_id, std::string &buffer, int bytes)
{
	uint8_t tmpbuffer[bytes];
	int size = recv_blocking(chan_id, tmpbuffer, bytes);

	if (size < 0)
		return size; // error.

	for(int i = 0; i < size; i++)
		buffer += tmpbuffer[i];

	return size;
}


int StdioComms::send(uint32_t chan_id, uint8_t *buffer, int bytes)
{
	write(mOut, buffer, bytes);
	return bytes;
}


int StdioComms::send(uint32_t chan_id, const std::string &output)
{
	if (output.size() > 0)
	{
		write(mOut, output.c_str(), output.size());
	}
	return output.size();
}



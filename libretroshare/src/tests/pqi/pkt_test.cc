/*
 * libretroshare/src/tests/pqi pkt_tst.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2009-2010 by Robert Fernie.
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

/******
 * pkt_tst.
 * 
 * This is not testing serialisation.... so we only need 1 type of packet.
 * using FileData packets - as these can have arbitary size.
 */

#include "pkt_test.h"
#include <iostream>

uint8_t testdata[TEST_DATA_LEN];

void generate_test_data()
{
	srand(1);

	int i;
	for(i = 0; i < TEST_DATA_LEN; i++)
	{
		testdata[i] = rand() % 256;
	}
}

bool check_data(void *data_in, int len)
{
	int i;
	uint8_t *data = (uint8_t *) data_in;

	for(i = 0; i < len; i++)
	{
		if (data[i] != testdata[i])
		{
			std::cerr << "check_data() Different Byte: " << i;
			std::cerr << std::endl;

			return false;
		}
	}
	return true;
}

RsFileData *gen_packet(int datasize, std::string pid)
{
	/* generate some packets */
	RsFileData *data = new RsFileData();
	data->fd.file.filesize = TEST_PKT_SIZE;
	data->fd.file.hash = TEST_PKT_HASH;
	data->fd.file_offset = TEST_PKT_OFFSET;
	data->fd.binData.setBinData(testdata, datasize);	
	data->PeerId(pid);

	return data;
}

bool check_packet(RsFileData *pkt)
{
	if (pkt->fd.file.filesize != TEST_PKT_SIZE)
	{
		std::cerr << "check_packet() FileSize Different";
		std::cerr << std::endl;
		
		return false;
	}

	if (pkt->fd.file.hash != TEST_PKT_HASH)
	{
		std::cerr << "check_packet() FileHash Different";
		std::cerr << std::endl;
		
		return false;
	}

	if (pkt->fd.file_offset != TEST_PKT_OFFSET)
	{
		std::cerr << "check_packet() FileOffset Different";
		std::cerr << std::endl;
		
		return false;
	}

	return check_data(pkt->fd.binData.bin_data, pkt->fd.binData.bin_len);
}



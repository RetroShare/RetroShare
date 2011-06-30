/*
 * libretroshare/src/tests/pqi pqiarchive_test.cc
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
 * pqiarchive test.
 * 
 * This is not testing serialisation.... so we only need 1 type of packet.
 * using FileData packets - as these can have arbitary size.
 */

#include "pqi/pqiarchive.h"
#include "pqi/pqibin.h"

#include "serialiser/rsbaseitems.h"

#include "util/rsnet.h"
#include <iostream>
#include <sstream>
#include "util/utest.h"

#include "pkt_test.h"

INITTEST();

int test_pqiarchive_generate();

int main(int argc, char **argv)
{
	std::cerr << "pqiarchive_test" << std::endl;

	generate_test_data();

	test_pqiarchive_generate();

	FINALREPORT("pqiarchive_test");

	return TESTRESULT();
}

const char *archive_fname = "/tmp/rs_tst_archive.tmp";

int test_pqiarchive_generate()
{

	std::string pid = "123456789012345678901234567890ZZ";
	/* create a pqiarchive */
	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsFileItemSerialiser());

	int flags = BIN_FLAGS_WRITEABLE;
	BinInterface *bio_in = new BinFileInterface(archive_fname, flags);

	int bio_flagsin = flags;
	pqiarchive *pqia = new pqiarchive(rss, bio_in, bio_flagsin);
	CHECK(pqia != NULL);

	int i;
	for(i = 10; i < TEST_DATA_LEN; i = (int) (i * 1.1))
	{
		RsFileData *pkt = gen_packet(i, pid);
		pqia->SendItem(pkt);
		pqia->tick();
	}

	/* close it up */
	delete pqia;

	/* setup read */
	flags = BIN_FLAGS_READABLE;

	rss = new RsSerialiser();
	rss->addSerialType(new RsFileItemSerialiser());

	bio_in = new BinFileInterface(archive_fname, flags);
	bio_flagsin = flags;
	pqia = new pqiarchive(rss, bio_in, bio_flagsin);

	for(i = 10; i < TEST_DATA_LEN; i = (int) (i * 1.1))
	//for(i = 10; i < TEST_DATA_LEN; i += 111)
	{
		RsItem *pkt = pqia->GetItem();
		pqia->tick();
		
		/* check the packet */
		RsFileData *data = dynamic_cast<RsFileData *>(pkt);
		CHECK(data != NULL);
		if (data)
		{
			CHECK(data->fd.binData.bin_len == i);
			CHECK(check_packet(data) == true);
			delete data;
		}
	}

	/* if there are anymore packets -> error! */
	for(i = 0; i < 1000; i++)
	{
		RsItem *pkt = pqia->GetItem();
		CHECK(pkt == NULL);	
	}

	REPORT("pqiarchive_generate()");

	return 1;
}




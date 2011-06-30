
/*
 * libretroshare/src/serialiser: rsserial_test.cc
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

/******************************************************************
 * tlvfileitem test.
 *
 *
 */

#include <iostream>
#include <sstream>
#include "serialiser/rsserial.h"
#include "serialiser/rsbaseitems.h"
#include "serialiser/rstlvutil.h"
#include "util/utest.h"

INITTEST();

static int test_RsFileItem();
static int test_RsFileData();

int main()
{
	std::cerr << "RsFile[Item/Data/...] Tests" << std::endl;


	test_RsFileItem(); 
	//test_RsFileData(); 
	
	FINALREPORT("RsTlvFile[Item/Data/...] Tests");

	return TESTRESULT();
}

int test_RsFileItem()
{
	/* make a serialisable FileItem */

	RsSerialiser srl;
	RsFileRequest rsfi;

	/* initialise */
	rsfi.file.filesize = 101010;
	rsfi.file.hash = "ABCDEFEGHE";
	rsfi.file.name = "TestFile.txt";
	rsfi.file.pop  = 12;
	rsfi.file.age  = 456;

	/* attempt to serialise it before we add it to the serialiser */

	CHECK(0 == srl.size(&rsfi));

#define MAX_BUFSIZE 16000

	char buffer[MAX_BUFSIZE];
	uint32_t sersize = MAX_BUFSIZE;

	CHECK(false == srl.serialise(&rsfi, (void *) buffer, &sersize));


	/* now add to serialiser */

	RsFileItemSerialiser *rsfis = new RsFileItemSerialiser();
	srl.addSerialType(rsfis);

	uint32_t size = srl.size(&rsfi);
	bool done = srl.serialise(&rsfi, (void *) buffer, &sersize);

	std::cerr << "test_RsFileItem() size: " << size << std::endl;
	std::cerr << "test_RsFileItem() done: " << done << std::endl;
	std::cerr << "test_RsFileItem() sersize: " << sersize << std::endl;

	std::cerr << "test_RsFileItem() serialised:" << std::endl;
	displayRawPacket(std::cerr, (void *) buffer, sersize);

	CHECK(done == true);

	uint32_t sersize2 = sersize;
	RsItem *output = srl.deserialise((void *) buffer, &sersize2);

	CHECK(output != NULL);
	CHECK(sersize2 == sersize);

	RsFileRequest *outfi = dynamic_cast<RsFileRequest *>(output);

	CHECK(outfi != NULL);

	if (outfi)
	{
		/* check the data is the same */
		CHECK(rsfi.file.filesize == outfi->file.filesize);
		CHECK(rsfi.file.hash == outfi->file.hash);
		CHECK(rsfi.file.name == outfi->file.name);
		CHECK(rsfi.file.path == outfi->file.path);
		CHECK(rsfi.file.pop  == outfi->file.pop);
		CHECK(rsfi.file.age  == outfi->file.age);
	}

	sersize2 = MAX_BUFSIZE;
	bool done2 = srl.serialise(outfi, (void *) &(buffer[16*8]), &sersize2);

	CHECK(sersize2 == sersize);

	displayRawPacket(std::cerr, (void *) buffer, 16 * 8 + sersize2);


	REPORT("Serialise/Deserialise RsFileRequest");

	return 1;
}




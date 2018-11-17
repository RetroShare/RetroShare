/*******************************************************************************
 * unittests/libretroshare/serialiser/support.h                                *
 *                                                                             *
 * Copyright 2007-2008 by Christopher Evi-Parker <retroshare.project@gmail.com>*
 * Copyright 2007-2008 by Cyril Soler            <retroshare.project@gmail.com>*
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#ifndef SUPPORT_H_
#define SUPPORT_H_

#include <gtest/gtest.h>

#include <string>
#include <stdint.h>
#include <iostream>

#include "serialiser/rsserial.h"
#include "serialiser/rstlvkeys.h"
#include "serialiser/rstlvbinary.h"
#include "serialiser/rstlvfileitem.h"
#include "serialiser/rstlvidset.h"
#include "serialiser/rstlvimage.h"

#include "rstlvutil.h"


/**
 * This contains functions that may be useful for testing throughout the
 * retroshare's serialiser
 * package, if you find a function you keep using everywhere, might be a good idea to add it here
 */


#define SHORT_STR 100
#define LARGE_STR 1000

void randString(const uint32_t, std::string&);
void randString(const uint32_t, std::wstring&);


/* for testing compound tlv items */

void init_item(RsTlvSecurityKeySet&);
void init_item(RsTlvKeySignature&);
void init_item(RsTlvKeySignatureSet&);
void init_item(RsTlvBinaryData&);
void init_item(RsTlvFileItem&);
void init_item(RsTlvFileSet&);
void init_item(RsTlvHashSet&);
void init_item(RsTlvPeerIdSet&);
void init_item(RsTlvImage&);
void init_item(RsTlvPeerIdSet&);

bool operator==(const RsTlvPublicRSAKey&, const RsTlvPublicRSAKey& );
bool operator==(const RsTlvSecurityKeySet&, const RsTlvSecurityKeySet& );
bool operator==(const RsTlvKeySignature&, const RsTlvKeySignature& );
bool operator==(const RsTlvBinaryData&, const RsTlvBinaryData&);
bool operator==(const RsTlvFileItem&, const RsTlvFileItem&);
bool operator==(const RsTlvFileSet&, const RsTlvFileSet& );
bool operator==(const RsTlvHashSet&, const RsTlvHashSet&);
bool operator==(const RsTlvImage&, const RsTlvImage& );
bool operator==(const RsTlvPeerIdSet& , const RsTlvPeerIdSet& );
bool operator==(const RsTlvKeySignatureSet& , const RsTlvKeySignatureSet& );



/*!
 * This templated test function which allows you to test
 * retroshare serialiser items (except compound tlv items)
 * you the function must implement a function
 *
 * 'RsSerialType* init_item(YourRsItem& rs_item)'
 * which returns valid serialiser that
 * can serialiser rs_item. You also need to implement an operator
 *
 * Also need to implement a function
 * 'bool operator =(YourRsItem& rs_itemL, YourRsItem& rs_temR)'
 * which allows this  function to test for equality between both parameters.
 * rs_temR is the result of deserialising the left operand (1st parameter).
 * not YourRsItem in specifier in about functions should be a derived type from RsItem
 *
 * @param T the item you want to test
 */

template<class ItemClass,class ItemSerialiser> int test_RsItem()
{
	/* make a serialisable RsTurtleItem */

	RsSerialiser srl;

	/* initialise */
	ItemClass rsfi ;
	RsSerialType *rsfis = new ItemSerialiser;

	init_item(rsfi);

	/* attempt to serialise it before we add it to the serialiser */
	std::cerr << "### These errors are expected." << std::endl;
	EXPECT_TRUE(0 == srl.size(&rsfi));

	static const uint32_t MAX_BUFSIZE = 22000 ;

	char *buffer = new char[MAX_BUFSIZE];
	uint32_t sersize = MAX_BUFSIZE;

	std::cerr << "### These errors are expected." << std::endl;
	EXPECT_TRUE(false == srl.serialise(&rsfi, (void *) buffer, &sersize));

	/* now add to serialiser */

	srl.addSerialType(rsfis);

	uint32_t size = srl.size(&rsfi);
	bool done = srl.serialise(&rsfi, (void *) buffer, &sersize);

	std::cerr << "test_Item() size: " << size << std::endl;
	std::cerr << "test_Item() done: " << done << std::endl;
	std::cerr << "test_Item() sersize: " << sersize << std::endl;

	//std::cerr << "test_Item() serialised:" << std::endl;
	//displayRawPacket(std::cerr, (void *) buffer, sersize);

	EXPECT_TRUE(done == true);

	uint32_t sersize2 = sersize;
	RsItem *output = srl.deserialise((void *) buffer, &sersize2);

	EXPECT_TRUE(output != NULL);
	if (!output)
	{
		return 1;
	}

	EXPECT_TRUE(sersize2 == sersize);

	ItemClass *outfi = dynamic_cast<ItemClass *>(output);

	EXPECT_TRUE(outfi != NULL);
	if (!outfi)
	{
		return 1;
	}

	if(!(*outfi == rsfi))
	{
		std::cerr << "Items differ: "<< std::endl;
		outfi->print(std::cerr,0) ;
		rsfi.print(std::cerr,0) ;
	}
	if (outfi)
	{
		EXPECT_TRUE(*outfi == rsfi) ;
	}


	sersize2 = MAX_BUFSIZE;
	bool done2 = srl.serialise(outfi, (void *) &(buffer[16*8]), &sersize2);

	EXPECT_TRUE(done2) ;
	EXPECT_TRUE(sersize2 == sersize);

	std::cerr << "Deleting output" <<std::endl;
	delete output ;
	delete[] buffer ;

	return 1;
}

template<class ItemClass,class ItemSerialiser> int test_RsItem(uint16_t servtype)
{
        /* make a serialisable RsTurtleItem */

        RsSerialiser srl;

        /* initialise */
        ItemClass rsfi(servtype) ;
        RsSerialType *rsfis = new ItemSerialiser(servtype) ;

		init_item(rsfi) ; // deleted on destruction of srl

        /* attempt to serialise it before we add it to the serialiser */
        std::cerr << "### These errors are expected." << std::endl;
        EXPECT_TRUE(0 == srl.size(&rsfi));

        static const uint32_t MAX_BUFSIZE = 22000 ;

        char *buffer = new char[MAX_BUFSIZE];
        uint32_t sersize = MAX_BUFSIZE;

        EXPECT_TRUE(false == srl.serialise(&rsfi, (void *) buffer, &sersize));

        /* now add to serialiser */

        srl.addSerialType(rsfis);

        uint32_t size = srl.size(&rsfi);
        bool done = srl.serialise(&rsfi, (void *) buffer, &sersize);

        std::cerr << "test_Item() size: " << size << std::endl;
        std::cerr << "test_Item() done: " << done << std::endl;
        std::cerr << "test_Item() sersize: " << sersize << std::endl;

        std::cerr << "test_Item() serialised:" << std::endl;
        //displayRawPacket(std::cerr, (void *) buffer, sersize);

        EXPECT_TRUE(done == true);

        uint32_t sersize2 = sersize;
        RsItem *output = srl.deserialise((void *) buffer, &sersize2);

        EXPECT_TRUE(output != NULL);
        EXPECT_TRUE(sersize2 == sersize);

        ItemClass *outfi = dynamic_cast<ItemClass *>(output);

        EXPECT_TRUE(outfi != NULL);

        if (outfi)
        {
                EXPECT_TRUE(*outfi == rsfi) ;
        }

        sersize2 = MAX_BUFSIZE;
        bool done2 = srl.serialise(outfi, (void *) &(buffer[16*8]), &sersize2);

        EXPECT_TRUE(done2) ;
        EXPECT_TRUE(sersize2 == sersize);

//	displayRawPacket(std::cerr, (void *) buffer, 16 * 8 + sersize2);

        delete[] buffer ;
		delete output ;

        return 1;
}
#endif /* SUPPORT_H_ */

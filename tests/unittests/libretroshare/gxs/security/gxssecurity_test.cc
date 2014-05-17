
/*
 * libretroshare/src/serialiser: rsturtleitems_test.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Cyril Soler
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
 */
#include <gtest/gtest.h>

#include <iostream>
#include <sstream>
#include "gxs/gxssecurity.h"
#include "util/rsdir.h"

TEST(libretroshare_gxs, GxsSecurity)
{
	RsTlvSecurityKey pub_key ;
	RsTlvSecurityKey priv_key ;

	EXPECT_TRUE(GxsSecurity::generateKeyPair(pub_key,priv_key)) ;

	srand48(getpid()) ;

	// create some random data and sign it / verify the signature.
	
	uint32_t data_len = 1000 + RSRandom::random_u32()%100 ;
	char *data = new char[data_len] ;

	RSRandom::random_bytes((unsigned char *)data,data_len) ;

	std::cerr << "  Generated random data. size=" << data_len << ", Hash=" << RsDirUtil::sha1sum((const uint8_t*)data,data_len) << std::endl;

	RsTlvKeySignature signature ;

	EXPECT_TRUE(GxsSecurity::getSignature(data,data_len,priv_key,signature) );
	EXPECT_TRUE(GxsSecurity::validateSignature(data,data_len,pub_key,signature) );

	std::cerr << "  Signature: size=" << signature.signData.bin_len << ", Hash=" << RsDirUtil::sha1sum((const uint8_t*)signature.signData.bin_data,signature.signData.bin_len) << std::endl;

	// test encryption/decryption

	uint8_t *out = NULL ;
	int outlen = 0 ;
	uint8_t *out2 = NULL ;
	int outlen2 = 0 ;

	EXPECT_TRUE(GxsSecurity::encrypt(out,outlen,(const uint8_t*)data,data_len,pub_key) );

	std::cerr << "  Encrypted text: size=" << outlen << ", Hash=" << RsDirUtil::sha1sum((const uint8_t*)out,outlen) << std::endl;

	EXPECT_TRUE(GxsSecurity::decrypt(out2,outlen2,out,outlen,priv_key) );

	std::cerr << "  Decrypted text: size=" << outlen2 << ", Hash=" << RsDirUtil::sha1sum((const uint8_t*)out2,outlen2) << std::endl;

	// Check that decrypted data is equal to original data.
	//
	EXPECT_TRUE((int)data_len == outlen2) ;
	EXPECT_TRUE(!memcmp(data,out2,outlen2)) ;
}



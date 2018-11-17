/*******************************************************************************
 * unittests/libretroshare/gxs/security/gxssecurity_tests.cc                   *
 *                                                                             *
 * Copyright 2007-2008 by Cyril Soler <retroshare.project@gmail.com>           *
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

#include <gtest/gtest.h>

#include <iostream>
#include <sstream>
#include "gxs/gxssecurity.h"
#include "util/rsdir.h"

TEST(libretroshare_gxs, GxsSecurity)
{
	RsTlvPublicRSAKey pub_key ;
	RsTlvPrivateRSAKey priv_key ;

	EXPECT_TRUE(GxsSecurity::generateKeyPair(pub_key,priv_key)) ;

#ifdef WIN32
	srand(getpid()) ;
#else
	srand48(getpid()) ;
#endif

	EXPECT_TRUE( pub_key.keyId   == priv_key.keyId   );
	EXPECT_TRUE( pub_key.startTS == priv_key.startTS );

	RsTlvPublicRSAKey pub_key2 ;
	EXPECT_TRUE(GxsSecurity::extractPublicKey(priv_key,pub_key2)) ;

	EXPECT_TRUE( pub_key.keyId    == pub_key2.keyId    );
	EXPECT_TRUE( pub_key.keyFlags == pub_key2.keyFlags );
	EXPECT_TRUE( pub_key.startTS  == pub_key2.startTS  );
	EXPECT_TRUE( pub_key.endTS    == pub_key2.endTS    );

	EXPECT_TRUE(pub_key.keyData.bin_len == pub_key2.keyData.bin_len) ;
	EXPECT_TRUE(!memcmp(pub_key.keyData.bin_data,pub_key2.keyData.bin_data,pub_key.keyData.bin_len));

	// create some random data and sign it / verify the signature.
	
	uint32_t data_len = 1000 + RSRandom::random_u32()%100 ;
	RsTemporaryMemory data(data_len) ;

	RSRandom::random_bytes((unsigned char *)data,data_len) ;

	std::cerr << "  Generated random data. size=" << data_len << ", Hash=" << RsDirUtil::sha1sum((const uint8_t*)data,data_len) << std::endl;

	RsTlvKeySignature signature ;

	EXPECT_TRUE(GxsSecurity::getSignature((char*)(unsigned char*)data,data_len,priv_key,signature) );
	EXPECT_TRUE(GxsSecurity::validateSignature((char*)(unsigned char*)data,data_len,pub_key,signature) );

	std::cerr << "  Signature: size=" << signature.signData.bin_len << ", Hash=" << RsDirUtil::sha1sum((const uint8_t*)signature.signData.bin_data,signature.signData.bin_len) << std::endl;

	// test encryption/decryption

	uint8_t *out = NULL ;
    uint32_t outlen = 0 ;
	uint8_t *out2 = NULL ;
    uint32_t outlen2 = 0 ;

	EXPECT_TRUE(GxsSecurity::encrypt(out,outlen,(const uint8_t*)data,data_len,pub_key) );

	std::cerr << "  Encrypted text: size=" << outlen << ", Hash=" << RsDirUtil::sha1sum((const uint8_t*)out,outlen) << std::endl;

	EXPECT_TRUE(GxsSecurity::decrypt(out2,outlen2,out,outlen,priv_key) );

	std::cerr << "  Decrypted text: size=" << outlen2 << ", Hash=" << RsDirUtil::sha1sum((const uint8_t*)out2,outlen2) << std::endl;

	// Check that decrypted data is equal to original data.
	//
	EXPECT_TRUE(data_len == outlen2) ;
	EXPECT_TRUE(!memcmp(data,out2,outlen2)) ;

	free(out2) ;
	free(out) ;
}




/*
 * "$Id: dirtest.cc,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Cyril Soler
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



#include "util/rsaes.h"
#include "util/utest.h"
#include <common/argstream.h>

#include <iostream>
#include <list>
#include <string>
#include <stdio.h>

void printHelp(int argc,char *argv[])
{
	std::cerr << argv[0] << ": tests AES encryption/decryption functions." << std::endl;
	std::cerr << "Usage: " << argv[0] << std::endl ;
}

void printHex(unsigned char *data,uint32_t length)
{
	static const char outh[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' } ;
	static const char outl[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' } ;

	for(uint32_t j = 0; j < length; j++)
	{
		std::cerr << outh[ (data[j]>>4) ] ;
		std::cerr << outh[ data[j] & 0xf ] ;
	}
}


INITTEST() ;

int main(int argc,char *argv[])
{
	std::string inputfile ;
	argstream as(argc,argv) ;

	as >> help() ;

	as.defaultErrorHandling() ;

	std::cerr << "Testing AES crypt" << std::endl;

	std::string source_string = "This is a very secret string ;-)" ;
	std::cerr << "Input string: length=" << source_string.length() << ", s=\"" << source_string << "\"" << std::endl;

	unsigned char key_data[16] ;
	unsigned char salt[8] ;

	for(int i=0;i<16;++i)
		key_data[i] = lrand48() & 0xff ;

	for(int i=0;i<50;++i)
	{
		for(int j=0;j<8;++j)
			salt[j] = lrand48() & 0xff ;

		unsigned char output_data[source_string.size() + 16] ;
		uint32_t output_data_length = source_string.size() + 16 ;

		CHECK(RsAes::aes_crypt_8_16( (const uint8_t*)source_string.c_str(),source_string.length(),key_data,salt,output_data,output_data_length)) ;

		std::cerr << "Round " << i << " salt=" ;
		printHex(salt,8) ;
		std::cerr << ": " << "output_length = " << output_data_length << ", encrypted string = " ;
		printHex(output_data,output_data_length) ;
		std::cerr << std::endl;

		unsigned char output_data2[output_data_length + 16] ;
		uint32_t output_data_length2 = output_data_length + 16 ;

		CHECK(RsAes::aes_decrypt_8_16(output_data,output_data_length,key_data,salt,output_data2,output_data_length2)) ;

		std::cerr << "                                output_length = " << output_data_length2 << ", decrypted string = " ;
		printHex(output_data2,output_data_length2) ;
		std::cerr << std::endl;
	
		CHECK(std::string( (const char *)output_data2,output_data_length2) == source_string) ;
	}

	FINALREPORT("AESTest") ;
	return TESTRESULT() ;
}


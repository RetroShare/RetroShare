/*******************************************************************************
 * libretroshare/src/util: rsprint.cc                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008-2008 Robert Fernie <retroshare@lunamutt.com>                 *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
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
 *******************************************************************************/

#include "util/rsprint.h"
#include "util/rsstring.h"
#include <stdio.h>
#include <iomanip>
#include <sstream>
#include <openssl/sha.h>
#include <sys/time.h>

#ifdef WINDOWS_SYS
#include "util/rstime.h"
#include <sys/timeb.h>
#endif

std::string RsUtil::NumberToString(uint64_t n,bool hex)
{
    std::ostringstream os ;

    if(hex)
        os << std::hex ;

    os << n ;
    os.flush() ;
    
    return os.str();
}

bool RsUtil::StringToInt(const std::string& s,int& n)
{
    if(sscanf(s.c_str(),"%d",&n) == 1)
        return true;
    else
        return false;
}
std::string RsUtil::BinToHex(const std::string &bin)
{
	return BinToHex(bin.c_str(), bin.length());
}

std::string RsUtil::BinToHex(const unsigned char *arr, const uint32_t len,uint32_t max_len)
{
	if(max_len > 0)
		return BinToHex((char*)arr,std::min(max_len,len)) + ((len > max_len)?"...":"") ;
	else
		return BinToHex((char*)arr,len) ;
}
std::string RsUtil::BinToHex(const char *arr, const uint32_t len)
{
	std::string out;

	for(uint32_t j = 0; j < len; j++)
	{
		rs_sprintf_append(out, "%02x", (int32_t) (((const uint8_t *) arr)[j]));
	}

	return out;
}

std::string RsUtil::HashId(const std::string &id, bool reverse)
{
	std::string hash;
	std::string tohash;
	if (reverse)
	{	
		std::string::const_reverse_iterator rit;
		for(rit = id.rbegin(); rit != id.rend(); ++rit)
		{
			tohash += (*rit);
		}
	}
	else
	{
		tohash = id;
	}

	SHA_CTX *sha_ctx = new SHA_CTX;
	uint8_t  sha_hash[SHA_DIGEST_LENGTH];

	SHA1_Init(sha_ctx);
	SHA1_Update(sha_ctx, tohash.c_str(), tohash.length());
	SHA1_Final(sha_hash, sha_ctx);

	for(uint16_t i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		hash += sha_hash[i];
	}

	/* cleanup */
	delete sha_ctx;

	return hash;
}

static int toHalfByte(char u,bool& ok)
{
    if(u >= 'a' && u <= 'f') return u-'a' + 0xa;
    if(u >= 'A' && u <= 'F') return u-'A' + 0xa;
    if(u >= '0' && u <= '9') return u-'0' + 0x0;

    ok = false ;

    return 0;
}

bool RsUtil::HexToBin(const std::string& input,unsigned char *data, const uint32_t len)
{
	if(input.size() & 1)
		return false ;

	if(len != input.size()/2)
		return false ;

	bool ok = true ;

	for(uint32_t i=0;(i<len) && ok;++i)
		data[i] = (toHalfByte(input[2*i],ok) << 4) + (toHalfByte(input[2*i+1],ok));

	return ok;
}

//static double getCurrentTS()
//{
//#ifndef WINDOWS_SYS
//	struct timeval cts_tmp;
//	gettimeofday(&cts_tmp, NULL);
//	double cts =  (cts_tmp.tv_sec) + ((double) cts_tmp.tv_usec) / 1000000.0;
//#else
//	struct _timeb timebuf;
//	_ftime( &timebuf);
//	double cts =  (timebuf.time) + ((double) timebuf.millitm) / 1000.0;
//#endif
//	return cts;
//}

// Little fn to get current timestamp in an independent manner.
//std::string RsUtil::AccurateTimeString()
//{
//	std::ostringstream out; // please do not use std::stringstream
//	out << std::setprecision(15) << getCurrentTS();
//	return out.str();
//}

std::vector<uint8_t> RsUtil::BinToSha256(const std::vector<uint8_t> &in)
{
	std::vector<uint8_t> out;

	SHA256_CTX *sha_ctx = new SHA256_CTX;
	uint8_t     sha_hash[SHA256_DIGEST_LENGTH] = {0};

	SHA256_Init(sha_ctx);
	SHA256_Update(sha_ctx, in.data(), in.size());
	SHA256_Final(sha_hash, sha_ctx);

	for(uint16_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		out.push_back(sha_hash[i]);
	}

	/* cleanup */
	delete sha_ctx;
	return out;
}

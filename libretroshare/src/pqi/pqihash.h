/*
 * libretroshare/src/pqi: pqihash.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2008 by Robert Fernie.
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

#ifndef PQI_HASH_
#define PQI_HASH_

#include <openssl/sha.h>
#include <string>
#include <iomanip>
#include "util/rsstring.h"
#include <inttypes.h>

class pqihash
{
	public:
	pqihash()
{
	
	sha_hash = new uint8_t[SHA_DIGEST_LENGTH];
	memset(sha_hash,0,SHA_DIGEST_LENGTH*sizeof(uint8_t)) ;
	sha_ctx = new SHA_CTX;
	SHA1_Init(sha_ctx);
	doHash = true;
}

	~pqihash()
{
	delete[] sha_hash;
	delete sha_ctx;
}


void    addData(void *data, uint32_t len)
{
	if (doHash)
	{
		SHA1_Update(sha_ctx, data, len);
	}
}

void 	Complete(std::string &hash)
{
	if (!doHash)
	{
		hash = endHash;
		return;
	}

	SHA1_Final(sha_hash, sha_ctx);

	endHash.clear();
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		rs_sprintf_append(endHash, "%02x", (unsigned int) (sha_hash[i]));
	}
	hash = endHash;
	doHash = false;

	return;
}

	private:

	bool	doHash;
	std::string endHash;
	uint8_t *sha_hash;
	SHA_CTX *sha_ctx;
};

#endif

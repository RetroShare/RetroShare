/*******************************************************************************
 * libretroshare/src/pqi: pqihash.h                                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef PQI_HASH_
#define PQI_HASH_

#include <retroshare/rsids.h>
#include <openssl/sha.h>
#include <string>
#include <iomanip>
#include <string.h>
#include "util/rsstring.h"
#include "retroshare/rstypes.h"
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


void    addData(const void *data, uint32_t len)
{
	if (doHash)
	{
		SHA1_Update(sha_ctx, data, len);
	}
}

void 	Complete(RsFileHash &hash)
{
	if (!doHash)
	{
		hash = endHash;
		return;
	}

	SHA1_Final(sha_hash, sha_ctx);

	endHash.clear();
	endHash = hash = Sha1CheckSum(sha_hash);

//	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
//	{
//		rs_sprintf_append(endHash, "%02x", (unsigned int) (sha_hash[i]));
//	}
//	hash = endHash;
	doHash = false;

	return;
}

	private:

	bool	doHash;
	RsFileHash endHash;
	uint8_t *sha_hash;
	SHA_CTX *sha_ctx;
};

#endif

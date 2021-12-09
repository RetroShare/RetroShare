/* Ricochet - https://ricochet.im/
 * Copyright (C) 2014, John Brooks <john.brooks@dereferenced.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 *    * Neither the names of the copyright owners nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>

#include "CryptoKey.h"
#include "Useful.h"

#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

#include "stdio.h"
#include "util/rsdebug.h"
#include "util/rsrandom.h"
#include "util/rsdir.h"
#include "retroshare/rsids.h"
#include "bytearray.h"

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
void RSA_get0_factors(const RSA *r, const BIGNUM **p, const BIGNUM **q)
{
  *p = r->p;
  *q = r->q;
}
#define RSA_bits(o) (BN_num_bits((o)->n))
#endif

CryptoKey::CryptoKey()
{
}

CryptoKey::~CryptoKey()
{
    clear();
}

void CryptoKey::clear()
{
    key_data.clear();
}

bool CryptoKey::loadFromFile(const std::string& path)
{
    FILE *file = fopen(path.c_str(),"r");

    if (!file)
    {
        RsWarn() << "Failed to open Tor key file " << path << ": errno = " << errno ;
        return false;
    }

    ByteArray data ;
    int c;
    while(EOF != (c=fgetc(file)))
        data.push_back((unsigned char)c);

    fclose(file);

    if(data.startsWith("-----BEGIN RSA PRIVATE KEY-----"))
    {
        std::cerr << "Note: Reading/converting Tor v2 key format." << std::endl;

        // This to be compliant with old format. New format is oblivious to the type of key so we dont need a header
        data = data.replace(ByteArray("-----BEGIN RSA PRIVATE KEY-----"),ByteArray());
        data = data.replace(ByteArray("-----END RSA PRIVATE KEY-----"),ByteArray());
        data = data.replace(ByteArray("\n"),ByteArray());
        data = data.replace(ByteArray("\t"),ByteArray());

        data = ByteArray("RSA1024:")+data;
    }

    std::cerr << "Have read the following key: " << std::endl;
    std::cerr << data.toString() << std::endl;

    key_data = data;

    return true;
}

bool CryptoKey::loadFromTorMessage(const ByteArray& b)
{
    // note: We should probably check the structure a bit more, for security.

    std::cerr << "Loading new key:" << std::endl;

    if(b.startsWith("RSA1024"))
        std::cerr << "  type: RSA-1024 (Tor v2)" << std::endl;
    else if(b.startsWith("ED25519-V3"))
        std::cerr << "  type: ED25519-V3 (Tor v3)" << std::endl;
    else if(b.indexOf(':'))
    {
        std::cerr << "  unknown type, or bad syntax in key: \"" << b.left(b.indexOf(':')).toString() << "\". Not accepted." << std::endl;
        return false;
    }

    key_data = b;
    return true;
}

/* Cryptographic hash of a password as expected by Tor's HashedControlPassword */
ByteArray torControlHashedPassword(const ByteArray &password)
{
    ByteArray salt(8);
    RsRandom::random_bytes(&salt[0],8);

    uint32_t count = ((uint32_t)16 + (96 & 15)) << ((96 >> 4) + 6);

    Sha1CheckSum md = RsDirUtil::sha1sum((salt+password).data(),count);

    /* 60 is the hex-encoded value of 96, which is a constant used by Tor's algorithm. */
    return ByteArray("16:") + salt.toHex().toUpper() + ByteArray("60") + ByteArray(md.toByteArray(), md.SIZE_IN_BYTES).toHex().toUpper();
}

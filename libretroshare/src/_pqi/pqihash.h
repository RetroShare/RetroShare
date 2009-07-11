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

#ifndef PQIHASH_H
#define PQIHASH_H

#include <openssl/sha.h>
#include <string>
#include <sstream>
#include <iomanip>

class pqihash
{
public:
    pqihash();
    ~pqihash();

    inline void addData(void *data, uint32_t len);
    inline void Complete(std::string &hash);

private:
    bool doHash;
    std::string endHash;
    uint8_t *sha_hash;
    SHA_CTX *sha_ctx;
};


#endif // PQIHASH_H

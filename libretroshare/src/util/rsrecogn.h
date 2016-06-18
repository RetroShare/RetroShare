
/*
 * libretroshare/src/util: rsrecogn.h
 *
 * RetroShare Utilities
 *
 * Copyright 2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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


#ifndef RSUTIL_RECOGN_H
#define RSUTIL_RECOGN_H

#include <inttypes.h>
#include <string>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include "serialiser/rsgxsrecognitems.h"
#include "retroshare/rsgxsifacetypes.h"

namespace RsRecogn {

EVP_PKEY *	loadMasterKey();
bool		loadSigningKeys(std::map<RsGxsId, RsGxsRecognSignerItem *> &signMap);
bool		validateTagSignature(RsGxsRecognSignerItem *signer, RsGxsRecognTagItem *item);

bool		signTag(EVP_PKEY *signKey, RsGxsRecognTagItem *item);
bool    	signSigner(EVP_PKEY *signKey, RsGxsRecognSignerItem *item);
bool 		signTagRequest(EVP_PKEY *signKey, RsGxsRecognReqItem *item);

bool    	itemToRadix64(RsItem *item, std::string &radstr);

std::string 	getRsaKeyId(RSA *pubkey);

RsGxsRecognTagItem *extractTag(const std::string &encoded);

bool 		createTagRequest(const RsTlvPrivateRSAKey &key, 
			const RsGxsId &id, const std::string &nickname,
			uint16_t tag_class, uint16_t tag_type, 
			const std::string &comment, std::string &tag);

}
	
#endif

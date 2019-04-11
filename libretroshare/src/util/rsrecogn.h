/*******************************************************************************
 * libretroshare/src/util: rsrandom.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 *  Copyright (C) 2013 Robert Fernie <retroshare@lunamutt.com>                 *
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

#ifndef RSUTIL_RECOGN_H
#define RSUTIL_RECOGN_H

#include <inttypes.h>
#include <string>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include "rsitems/rsgxsrecognitems.h"
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

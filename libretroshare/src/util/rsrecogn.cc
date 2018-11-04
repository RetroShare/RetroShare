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
#include "pqi/pqi_base.h"

#include "util/rsrecogn.h"
#include "util/radix64.h"
#include "util/rsstring.h"
#include "util/rsdir.h"

#include "gxs/gxssecurity.h"

#include <openssl/ssl.h>
#include <openssl/evp.h>

/***
 * #define DEBUG_RECOGN	1
 ***/

#define DEBUG_RECOGN	1

static std::string RecognKey = "MIICCgKCAgEA4/i2fy+zR27/H8fzphM8mR/Nz+yXjMJTtqKlCvEMQlyk7lKzDbKifNjGSiAXjSv3b9TRgMtje7hfEhs3//Oeu4KsCf6sz17aj2StBF579IdJTSUPDwq6jCsZ6NDEYpG8xz3FVV+Ac8q5Vpr/+jdg23ta09zq4aV8VIdIsroVOmZQqjwPcmQK57iWHd538i/XBtc2rnzbYq5bprnmtAKdx55gXVXDfALa0s6yR0HYvCaWguMEJhMIKWfi/9PEgLgwF9OmRwywc2TU/EdvYJo8fYHLfGk0PnYBuL1oSnn3cwAAef02W2JyCzQ84g30tLSUk+hC1LLi+iYj3x7IRR4q7Rlf/FYv/Q5fvjRtPT9eqM6fKyJ9ZO4NjlrSPFGydNbgABzP6WMhBzFjUkEKS27bGmr8Qxdj3Zp0TvR2IkyM6oM+6YknuM4RndUEgC1ZxtoIhugMjm6HdMQmoaHNK3kXewgQB90HHqzKA/J1gok3NcqL8Yls5g0LHepVHsU4cuaIqQr5yr665ZTLU2oqn1HIdkgydBYYUt6G3eWJKXYRbDhWPthGo/HK+W+iw6cTGWxzlCZS40EU9efxz4mDuhow67jOe704lBP3kiYXu05Y5uspaYnuvrvIwaRWBYapyR9UmKktnY8xJYrZvrcZgCovAbiodTzWeYg37xjFfGgYNI8CAwEAAQ==";

#define NUM_RECOGN_SIGN_KEYS	3
static std::string RecognSigningKeys[NUM_RECOGN_SIGN_KEYS] = 
{
	"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",


};



EVP_PKEY *RsRecogn::loadMasterKey()
{
	/* load master signing key */
    std::vector<uint8_t> decoded = Radix64::decode(RecognKey);
		
    const unsigned char *keyptr2 = decoded.data();
    long keylen2 = decoded.size();
	
	RSA *rsakey = d2i_RSAPublicKey(NULL, &(keyptr2), keylen2);

	if (!rsakey)
	{
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::loadMasterKeys() failed rsakey load";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
		return NULL;
	}	

	
	EVP_PKEY *signKey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(signKey, rsakey);

	return signKey;
}


bool	RsRecogn::loadSigningKeys(std::map<RsGxsId, RsGxsRecognSignerItem *> &signMap)
{

	EVP_PKEY *signKey = loadMasterKey();
	RsGxsRecognSerialiser recognSerialiser;

	if (!signKey)
	{
		std::cerr << "RsRecogn::loadSigningKeys() missing Master Key";
		return false;
	}


	rstime_t now = time(NULL);

	for(int i = 0; i < NUM_RECOGN_SIGN_KEYS; i++)
	{
        std::vector<uint8_t> signerbuf = Radix64::decode(RecognSigningKeys[i]);

        uint32_t pktsize = signerbuf.size();
        RsItem *pktitem = recognSerialiser.deserialise(signerbuf.data(), &pktsize);
		RsGxsRecognSignerItem *item = dynamic_cast<RsGxsRecognSignerItem *>(pktitem);

		if (!item)
		{
#ifdef DEBUG_RECOGN
			std::cerr << "RsRecogn::loadSigningKeys() failed to deserialise SignerItem from string \"" << RecognSigningKeys[i] << "\"";
			std::cerr << std::endl;
#endif // DEBUG_RECOGN
			continue;
		}

#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::loadSigningKeys() SignerItem: ";
		std::cerr << std::endl;
		item->print(std::cerr);
#endif // DEBUG_RECOGN
		
		/* check dates */
		if ((item->key.startTS > (unsigned) now) || (item->key.endTS < (unsigned) now))
		{
#ifdef DEBUG_RECOGN
			std::cerr << "RsRecogn::loadSigningKeys() failed timestamp";
			std::cerr << std::endl;
			std::cerr << "RsRecogn::loadSigningKeys() key.startTS: " << item->key.startTS;
			std::cerr << std::endl;
			std::cerr << "RsRecogn::loadSigningKeys() now: " << now;
			std::cerr << std::endl;
			std::cerr << "RsRecogn::loadSigningKeys() key.endTS: " << item->key.endTS;
			std::cerr << std::endl;
#endif // DEBUG_RECOGN
			delete item;
			continue;
		}

		/* check signature */
		RsTlvKeySignature signature = item->sign;
		item->sign.TlvShallowClear();

		unsigned int siglen = signature.signData.bin_len;
		unsigned char *sigbuf = (unsigned char *) signature.signData.bin_data;

		/* store in */
		uint32_t datalen = recognSerialiser.size(item);
        
        	RsTemporaryMemory data(datalen) ;
            
            	if(!data)
                    return false ;
                
		uint32_t pktlen = datalen;
		int signOk = 0;
		
		if (recognSerialiser.serialise(item, data, &pktlen))
		{
			EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

			EVP_VerifyInit(mdctx, EVP_sha1());
			EVP_VerifyUpdate(mdctx, data, pktlen);
			signOk = EVP_VerifyFinal(mdctx, sigbuf, siglen, signKey);
	
			EVP_MD_CTX_destroy(mdctx);

			item->sign = signature;
			signature.TlvShallowClear();

			if (signOk)
			{
#ifdef DEBUG_RECOGN
				std::cerr << "RsRecogn::loadSigningKeys() signature ok";
				std::cerr << std::endl;
#endif // DEBUG_RECOGN
				RsGxsId signerId = item->key.keyId;
				signMap[signerId] = item;
			}
		}
		
		if (!signOk)
		{
#ifdef DEBUG_RECOGN
			std::cerr << "RsRecogn::loadSigningKeys() signature failed";
			std::cerr << std::endl;
#endif // DEBUG_RECOGN
			delete item;
		}
	}

	/* clean up */
	EVP_PKEY_free(signKey);
	return true;
}



bool	RsRecogn::validateTagSignature(RsGxsRecognSignerItem *signer, RsGxsRecognTagItem *item)
{
	if (item->sign.keyId != signer->key.keyId)
	{
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::validateTagSignature() keyId mismatch";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
		return false;
	}
		
	const unsigned char *keyptr = (const unsigned char *) signer->key.keyData.bin_data;
	long keylen = signer->key.keyData.bin_len;
		
	/* extract admin key */
	RSA *rsakey = d2i_RSAPublicKey(NULL, &(keyptr), keylen);
	if (!rsakey)
	{
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::validateTagSignature() failed extract signkey";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
		return false;
	}
		
	/* check signature */
	RsTlvKeySignature signature = item->sign;
	item->sign.TlvShallowClear();
		
	unsigned int siglen = signature.signData.bin_len;
	unsigned char *sigbuf = (unsigned char *) signature.signData.bin_data;

	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	EVP_PKEY *signKey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(signKey, rsakey);

		
	/* store in */
	RsGxsRecognSerialiser serialiser;

	uint32_t datalen = serialiser.size(item);
    
    	RsTemporaryMemory data(datalen) ;
        
        if(!data)
            return false ;
        
	int signOk = 0;
		
	uint32_t pktlen = datalen;
	if (serialiser.serialise(item, data, &pktlen))
	{
		
		EVP_VerifyInit(mdctx, EVP_sha1());
		EVP_VerifyUpdate(mdctx, data, pktlen);
		signOk = EVP_VerifyFinal(mdctx, sigbuf, siglen, signKey);
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::validateTagSignature() sign_result: " << signOk;
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
	}
	else
	{
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::validateTagSignature() failed to serialise";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
	}
		
	// Clean up.
	item->sign = signature;
	signature.TlvShallowClear();
		
	EVP_MD_CTX_destroy(mdctx);
	EVP_PKEY_free(signKey);
		
	return (signOk == 1);		
}


bool rsa_sanity_check(RSA *rsa)
{
	std::cerr << "rsa_sanity_check()";
	std::cerr << std::endl;

	if (!rsa)
	{
		std::cerr << "rsa_sanity_check() RSA == NULL";
		std::cerr << std::endl;
		return false;
	}

	RSA *pubkey = RSAPublicKey_dup(rsa);

	std::string signId = RsRecogn::getRsaKeyId(rsa);
	std::string signId2 = RsRecogn::getRsaKeyId(pubkey);

	bool ok = true;
	if (signId != signId2)
	{
		std::cerr << "rsa_sanity_check() ERROR SignId Failure";
		std::cerr << std::endl;
		ok = false;
	}

	if (1 != RSA_check_key(rsa))
	{
		std::cerr << "rsa_sanity_check() ERROR RSA key is not private";
		std::cerr << std::endl;
		ok = false;
	}

#if 0
	if (1 == RSA_check_key(pubkey))
	{
		std::cerr << "rsa_sanity_check() ERROR RSA dup key is private";
		std::cerr << std::endl;
		ok = false;
	}
#endif

	RSA_free(pubkey);

	if (!ok)
	{
		exit(1);
	}

	return true;
}

		
#warning csoler: this code should be using GxsSecurity signature code. Not some own made signature call.

bool	RsRecogn::signTag(EVP_PKEY *signKey, RsGxsRecognTagItem *item)
{
	RsGxsRecognSerialiser serialiser;

	RSA *rsa = EVP_PKEY_get1_RSA(signKey);
	std::string signId = getRsaKeyId(rsa);
	rsa_sanity_check(rsa);
	RSA_free(rsa);

	item->sign.TlvClear();

	/* write out the item for signing */
	uint32_t len = serialiser.size(item);
	char *buf = new char[len];
	if (!serialiser.serialise(item, buf, &len))
	{
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::signTag() Failed serialise TagItem:";
		std::cerr << std::endl;
		item->print(std::cerr);
#endif // DEBUG_RECOGN
		delete []buf;
		return false;
	}

	/* calc and check signature */
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	EVP_SignInit(mdctx, EVP_sha1());
	EVP_SignUpdate(mdctx, buf, len);

	unsigned int siglen = EVP_PKEY_size(signKey);
	unsigned char sigbuf[siglen];
	EVP_SignFinal(mdctx, sigbuf, &siglen, signKey);

	/* save signature */
	item->sign.signData.setBinData(sigbuf, siglen);
	item->sign.keyId = RsGxsId(signId);

	/* clean up */
	EVP_MD_CTX_destroy(mdctx);
	delete []buf;

	return true;
}

#warning csoler: this code should be using GxsSecurity signature code. Not some own made signature call.

bool	RsRecogn::signSigner(EVP_PKEY *signKey, RsGxsRecognSignerItem *item)
{
	std::cerr << "RsRecogn::signSigner()";
	std::cerr << std::endl;

	RsGxsRecognSerialiser serialiser;

	std::cerr << "RsRecogn::signSigner() Checking Key";
	std::cerr << std::endl;

	RSA *rsa = EVP_PKEY_get1_RSA(signKey);
	std::string signId = getRsaKeyId(rsa);
	rsa_sanity_check(rsa);
	RSA_free(rsa);

	std::cerr << "RsRecogn::signSigner() Key Okay";
	std::cerr << std::endl;

	item->sign.TlvClear();

	/* write out the item for signing */
	uint32_t len = serialiser.size(item);
	char *buf = new char[len];
	if (!serialiser.serialise(item, buf, &len))
	{
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::signSigner() Failed serialise SignerItem:";
		std::cerr << std::endl;
		item->print(std::cerr);
#endif // DEBUG_RECOGN
		delete []buf;
		return false;
	}

	/* calc and check signature */
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	EVP_SignInit(mdctx, EVP_sha1());
	EVP_SignUpdate(mdctx, buf, len);

	unsigned int siglen = EVP_PKEY_size(signKey);
	unsigned char sigbuf[siglen];
	EVP_SignFinal(mdctx, sigbuf, &siglen, signKey);

	/* save signature */
	item->sign.signData.setBinData(sigbuf, siglen);
	item->sign.keyId = RsGxsId(signId);

	/* clean up */
	EVP_MD_CTX_destroy(mdctx);
	delete []buf;

	return true;
}

#warning csoler: this code should be using GxsSecurity signature code. Not some own made signature call.

bool	RsRecogn::signTagRequest(EVP_PKEY *signKey, RsGxsRecognReqItem *item)
{
	std::cerr << "RsRecogn::signTagRequest()";
	std::cerr << std::endl;

	RsGxsRecognSerialiser serialiser;

	RSA *rsa = EVP_PKEY_get1_RSA(signKey);
	std::string signId = getRsaKeyId(rsa);
	rsa_sanity_check(rsa);
	RSA_free(rsa);

	item->sign.TlvClear();

	/* write out the item for signing */
	uint32_t len = serialiser.size(item);
	char *buf = new char[len];
	if (!serialiser.serialise(item, buf, &len))
	{
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::signTagRequest() Failed serialise Tag Request:";
		std::cerr << std::endl;
		item->print(std::cerr);
#endif // DEBUG_RECOGN
		delete []buf;
		return false;
	}

	/* calc and check signature */
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	EVP_SignInit(mdctx, EVP_sha1());
	EVP_SignUpdate(mdctx, buf, len);

	unsigned int siglen = EVP_PKEY_size(signKey);
	unsigned char sigbuf[siglen];
	EVP_SignFinal(mdctx, sigbuf, &siglen, signKey);

	/* save signature */
	item->sign.signData.setBinData(sigbuf, siglen);
	item->sign.keyId = RsGxsId(signId);

	/* clean up */
	EVP_MD_CTX_destroy(mdctx);
	delete []buf;

	return true;
}


bool	RsRecogn::itemToRadix64(RsItem *item, std::string &radstr)
{
	RsGxsRecognSerialiser serialiser;

	/* write out the item for signing */
	uint32_t len = serialiser.size(item);
    
    	RsTemporaryMemory buf(len) ;
        
	if (!serialiser.serialise(item, buf, &len))
	{
		return false;
	}

	radstr.clear();
	Radix64::encode(buf, len, radstr);

	return true;
}


std::string RsRecogn::getRsaKeyId(RSA *pubkey)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	int len = BN_num_bytes(pubkey -> n);
	unsigned char tmp[len];
	BN_bn2bin(pubkey -> n, tmp);
#else
    const BIGNUM *nn=NULL ;
    RSA_get0_key(pubkey,&nn,NULL,NULL) ;

	int len = BN_num_bytes(nn);
	unsigned char tmp[len];
	BN_bn2bin(nn, tmp);
#endif

    return RsDirUtil::sha1sum(tmp,len).toStdString();

#ifdef OLD_VERSION_REMOVED
    // (cyril) I removed this because this is cryptographically insane, as it allows to easily forge a RSA key with the same ID.

	// copy first CERTSIGNLEN bytes...
	if (len > CERTSIGNLEN)
	{
		len = CERTSIGNLEN;
	}

	std::string id;
	for(uint32_t i = 0; i < CERTSIGNLEN; i++)
	{
		rs_sprintf_append(id, "%02x", (uint16_t) (((uint8_t *) (tmp))[i]));
	}

	return id;
#endif
}



RsGxsRecognTagItem *RsRecogn::extractTag(const std::string &encoded)
{
	// Decode from Radix64 encoded Packet.
	uint32_t pktsize;

    std::vector<uint8_t> buffer = Radix64::decode(encoded);
    pktsize = buffer.size();

	if( buffer.empty() )
		return NULL;

	RsGxsRecognSerialiser serialiser;
    RsItem *item = serialiser.deserialise(buffer.data(), &pktsize);

	if (!item)
	{
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::extractTag() ERROR Deserialise failed";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
		return NULL;
	}

	RsGxsRecognTagItem *tagitem = dynamic_cast<RsGxsRecognTagItem *>(item);

	if (!tagitem)
	{
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::extractTag() ERROR Not TagItem, is: ";
		std::cerr << std::endl;
		item->print(std::cerr);
#endif // DEBUG_RECOGN
		delete item;
	}

	return tagitem;
}


bool RsRecogn::createTagRequest(const RsTlvPrivateRSAKey &key, const RsGxsId &id, const std::string &nickname, uint16_t tag_class, uint16_t tag_type, const std::string &comment, std::string &tag)
{
	EVP_PKEY *signKey = EVP_PKEY_new();
	RSA *rsakey = d2i_RSAPrivateKey(NULL, (const unsigned char **)&key.keyData.bin_data, key.keyData.bin_len);

        if (!rsakey)
        {
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::createTagRequest() Failed to extract key";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
                return false;
        }

        if (!EVP_PKEY_assign_RSA(signKey, rsakey))
        {
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::createTagRequest() Failed to assign key";
		std::cerr << std::endl;
#endif // DEBUG_RECOGN
                return false;
        }

	RsGxsRecognReqItem *item = new RsGxsRecognReqItem();

	item->issued_at = time(NULL);
	item->period = 365 * 24 * 3600;
	item->tag_class = tag_class;
	item->tag_type  = tag_type;

	item->nickname = nickname;
	item->identity = id;
	item->comment = comment;

	bool signOk = RsRecogn::signTagRequest(signKey,item);
	EVP_PKEY_free(signKey);

	if (!signOk)
	{
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::createTagRequest() Failed to sign Tag Request:";
		std::cerr << std::endl;
		item->print(std::cerr);
#endif // DEBUG_RECOGN
		delete item;
		return false;
	}

	/* write out the item for signing */
	RsGxsRecognSerialiser serialiser;
	uint32_t len = serialiser.size(item);
    RsTemporaryMemory buf(len) ;
	bool serOk = serialiser.serialise(item, buf, &len);

	if (serOk)
	{
		Radix64::encode(buf, len, tag);
	}

	if (!serOk)
	{
#ifdef DEBUG_RECOGN
		std::cerr << "RsRecogn::createTagRequest() Failed serialise Tag Request:";
		std::cerr << std::endl;
		item->print(std::cerr);
#endif // DEBUG_RECOGN
        delete item;
        return false;
    }

    delete item;
	return serOk;
}



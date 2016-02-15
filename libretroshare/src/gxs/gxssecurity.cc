
/*
 * libretroshare/src/gxs: gxssecurity.cc
 *
 *
 * Copyright 2008-2010 by Robert Fernie
 *           2011-2012 Christopher Evi-Parker
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

#include "gxssecurity.h"
#include "pqi/authgpg.h"
#include "util/rsdir.h"
#include "util/rsmemory.h"
//#include "retroshare/rspeers.h"

/****
 * #define GXS_SECURITY_DEBUG 	1
 ***/

static RsGxsId getRsaKeyFingerprint(RSA *pubkey)
{
        int lenn = BN_num_bytes(pubkey -> n);
        int lene = BN_num_bytes(pubkey -> e);

        unsigned char *tmp = new unsigned char[lenn+lene];

        BN_bn2bin(pubkey -> n, tmp);
        BN_bn2bin(pubkey -> e, &tmp[lenn]);

		  Sha1CheckSum s = RsDirUtil::sha1sum(tmp,lenn+lene) ;
		  delete[] tmp ;

        // Copy first CERTSIGNLEN bytes from the hash of the public modulus and exponent
		  // We should not be using strings here, but a real ID. To be done later.

		  assert(Sha1CheckSum::SIZE_IN_BYTES >= CERTSIGNLEN) ;

        return RsGxsId(s.toStdString().substr(0,2*CERTSIGNLEN));
}

static RSA *extractPrivateKey(const RsTlvSecurityKey & key)
{
    assert(key.keyFlags & RSTLV_KEY_TYPE_FULL) ;

        const unsigned char *keyptr = (const unsigned char *) key.keyData.bin_data;
        long keylen = key.keyData.bin_len;

        /* extract admin key */
        RSA *rsakey = d2i_RSAPrivateKey(NULL, &(keyptr), keylen);

        return rsakey;
}

static RSA *extractPublicKey(const RsTlvSecurityKey& key)
{
    assert(!(key.keyFlags & RSTLV_KEY_TYPE_FULL)) ;

        const unsigned char *keyptr = (const unsigned char *) key.keyData.bin_data;
        long keylen = key.keyData.bin_len;

        /* extract admin key */
        RSA *rsakey = d2i_RSAPublicKey(NULL, &(keyptr), keylen);

        return rsakey;
}
static void setRSAPublicKeyData(RsTlvSecurityKey & key, RSA *rsa_pub)
{
        unsigned char *data = NULL ;	// this works for OpenSSL > 0.9.7
        int reqspace = i2d_RSAPublicKey(rsa_pub, &data);

        key.keyData.setBinData(data, reqspace);
        key.keyId = getRsaKeyFingerprint(rsa_pub);

		  free(data) ;
}

bool GxsSecurity::checkPrivateKey(const RsTlvSecurityKey& key)
{
    std::cerr << "Checking private key " << key.keyId << " ..." << std::endl;

    if( (key.keyFlags & RSTLV_KEY_TYPE_MASK) != RSTLV_KEY_TYPE_FULL)
    {
        std::cerr << "(WW) GxsSecurity::checkPrivateKey(): private key has wrong flags " << std::hex << (key.keyFlags & RSTLV_KEY_TYPE_MASK) << std::dec << ". This is unexpected." << std::endl;
        return false ;
    }
    RSA *rsa_prv = ::extractPrivateKey(key) ;

    if(rsa_prv == NULL)
    {
        std::cerr << "(WW) GxsSecurity::checkPrivateKey(): no private key can be extracted from key ID " << key.keyId << ". Key is corrupted?" << std::endl;
        return false ;
    }
    RSA *rsa_pub = RSAPublicKey_dup(rsa_prv);
    RSA_free(rsa_prv) ;

    if(rsa_pub == NULL)
    {
        std::cerr << "(WW) GxsSecurity::checkPrivateKey(): no public key can be extracted from key ID " << key.keyId << ". Key is corrupted?" << std::endl;
        return false ;
    }
    RsGxsId recomputed_key_id = getRsaKeyFingerprint(rsa_pub) ;
    RSA_free(rsa_pub) ;

    if(recomputed_key_id != key.keyId)
    {
        std::cerr << "(WW) GxsSecurity::checkPrivateKey(): key " << key.keyId << " has wrong fingerprint " << recomputed_key_id << "! This is unexpected." << std::endl;
        return false ;
    }

    return true ;
}
bool GxsSecurity::checkPublicKey(const RsTlvSecurityKey& key)
{
    std::cerr << "Checking public key " << key.keyId << " ..." << std::endl;

    if( (key.keyFlags & RSTLV_KEY_TYPE_MASK) != RSTLV_KEY_TYPE_PUBLIC_ONLY)
    {
        std::cerr << "(WW) GxsSecurity::checkPublicKey(): public key has wrong flags " << std::hex << (key.keyFlags & RSTLV_KEY_TYPE_MASK) << std::dec << ". This is unexpected." << std::endl;
        return false ;
    }
    RSA *rsa_pub = ::extractPublicKey(key) ;

    if(rsa_pub == NULL)
    {
        std::cerr << "(WW) GxsSecurity::checkPublicKey(): no public key can be extracted from key ID " << key.keyId << ". Key is corrupted?" << std::endl;
        return false ;
    }
    RsGxsId recomputed_key_id = getRsaKeyFingerprint(rsa_pub) ;
    RSA_free(rsa_pub) ;

    if(recomputed_key_id != key.keyId)
    {
        std::cerr << "(WW) GxsSecurity::checkPublicKey(): key " << key.keyId << " has wrong fingerprint " << recomputed_key_id << "! This is unexpected." << std::endl;
        return false ;
    }

    return true ;
}

static void setRSAPrivateKeyData(RsTlvSecurityKey & key, RSA *rsa_priv)
{
        unsigned char *data = NULL ;
        int reqspace = i2d_RSAPrivateKey(rsa_priv, &data);

        key.keyData.setBinData(data, reqspace);
        key.keyId = getRsaKeyFingerprint(rsa_priv);

		  free(data) ;
}

bool GxsSecurity::generateKeyPair(RsTlvSecurityKey& public_key,RsTlvSecurityKey& private_key)
{
	// admin keys
    RSA *rsa     = RSA_generate_key(2048, 65537, NULL, NULL);
    RSA *rsa_pub = RSAPublicKey_dup(rsa);

    setRSAPublicKeyData(public_key, rsa_pub);
    setRSAPrivateKeyData(private_key, rsa);

    public_key.startTS  = time(NULL);
    public_key.endTS    = public_key.startTS + 60 * 60 * 24 * 365 * 5; /* approx 5 years */
    public_key.keyFlags = RSTLV_KEY_TYPE_PUBLIC_ONLY ;

    private_key.startTS  = public_key.startTS;
    private_key.endTS    = 0; /* no end */
    private_key.keyFlags = RSTLV_KEY_TYPE_FULL ;

    // clean up
    RSA_free(rsa);
    RSA_free(rsa_pub);

	 return true ;
}

bool GxsSecurity::extractPublicKey(const RsTlvSecurityKey& private_key,RsTlvSecurityKey& public_key)
{
    public_key.TlvClear() ;

    if(!(private_key.keyFlags & RSTLV_KEY_TYPE_FULL))
        return false ;

    RSA *rsaPrivKey = extractPrivateKey(private_key);

    if(!rsaPrivKey)
        return false ;

    RSA *rsaPubKey = RSAPublicKey_dup(rsaPrivKey);
    RSA_free(rsaPrivKey);

    if(!rsaPubKey)
        return false ;

    setRSAPublicKeyData(public_key, rsaPubKey);
    RSA_free(rsaPubKey);

    public_key.keyFlags  = private_key.keyFlags & (RSTLV_KEY_DISTRIB_MASK) ;	// keep the distrib flags
    public_key.keyFlags |= RSTLV_KEY_TYPE_PUBLIC_ONLY;
    public_key.startTS   = private_key.startTS ;
    public_key.endTS     = public_key.startTS + 60 * 60 * 24 * 365 * 5; /* approx 5 years */

    // This code fixes a problem of old RSA keys where the fingerprint wasn't computed using SHA1(n,e) but
    // using the first bytes of n (ouuuuch!). Still, these keys are valid and should produce a correct
    // fingerprint. So we replace the public key fingerprint (that is normally recomputed) with the FP of
    // the private key.

    if(public_key.keyId != private_key.keyId)
    {
        std::cerr << std::endl;
        std::cerr << "WARNING: GXS ID key pair " << private_key.keyId << " has inconsistent fingerprint. This is an old key " << std::endl;
        std::cerr << "         that is unsecured (can be faked easily) should not be used anymore. Please delete it." << std::endl;
        std::cerr << std::endl;

        public_key.keyId = private_key.keyId ;
    }

    return true ;
}

bool GxsSecurity::getSignature(const char *data, uint32_t data_len, const RsTlvSecurityKey& privKey, RsTlvKeySignature& sign)
{
	RSA* rsa_pub = extractPrivateKey(privKey);

	if(!rsa_pub)
	{
		std::cerr << "GxsSecurity::getSignature(): Cannot create signature. Keydata is incomplete." << std::endl;
		return false ;
	}
	EVP_PKEY *key_pub = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(key_pub, rsa_pub);

	/* calc and check signature */
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	bool ok = EVP_SignInit(mdctx, EVP_sha1()) == 1;
	ok &= EVP_SignUpdate(mdctx, data, data_len) == 1;

	unsigned int siglen = EVP_PKEY_size(key_pub);
	unsigned char sigbuf[siglen];
	ok &= EVP_SignFinal(mdctx, sigbuf, &siglen, key_pub) == 1;

	// clean up
	EVP_MD_CTX_destroy(mdctx);
	EVP_PKEY_free(key_pub);

	sign.signData.setBinData(sigbuf, siglen);
	sign.keyId = RsGxsId(privKey.keyId);

	return ok;
}

bool GxsSecurity::validateSignature(const char *data, uint32_t data_len, const RsTlvSecurityKey& key, const RsTlvKeySignature& signature)
{
    RSA *tmpkey = (key.keyFlags & RSTLV_KEY_TYPE_FULL)?(::extractPrivateKey(key)):(::extractPublicKey(key)) ;

    RSA *rsakey = RSAPublicKey_dup(tmpkey) ;	// always extract public key
    RSA_free(tmpkey) ;

	if(!rsakey)
	{
		std::cerr << "GxsSecurity::validateSignature(): Cannot validate signature. Keydata is incomplete." << std::endl;
		key.print(std::cerr,0) ;
		return false ;
	}
	EVP_PKEY *signKey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(signKey, rsakey);

	/* calc and check signature */
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	EVP_VerifyInit(mdctx, EVP_sha1());
	EVP_VerifyUpdate(mdctx, data, data_len);

	int signOk = EVP_VerifyFinal(mdctx, (unsigned char*)signature.signData.bin_data, signature.signData.bin_len, signKey);

	/* clean up */
	EVP_PKEY_free(signKey);
	EVP_MD_CTX_destroy(mdctx);

	return signOk;
}

bool GxsSecurity::validateNxsMsg(const RsNxsMsg& msg, const RsTlvKeySignature& sign, const RsTlvSecurityKey& key)
{
    #ifdef GXS_SECURITY_DEBUG
            std::cerr << "GxsSecurity::validateNxsMsg()";
            std::cerr << std::endl;
            std::cerr << "RsNxsMsg :";
            std::cerr << std::endl;
            msg.print(std::cerr, 10);
            std::cerr << std::endl;
    #endif

            RsGxsMsgMetaData& msgMeta = *(msg.metaData);

    //        /********************* check signature *******************/

            /* check signature timeperiod */
            if ((msgMeta.mPublishTs < key.startTS) || (key.endTS != 0 && msgMeta.mPublishTs > key.endTS))
            {
    #ifdef GXS_SECURITY_DEBUG
                    std::cerr << " GxsSecurity::validateNxsMsg() TS out of range";
                    std::cerr << std::endl;
    #endif
                    return false;
            }

            /* decode key */
            const unsigned char *keyptr = (const unsigned char *) key.keyData.bin_data;
            long keylen = key.keyData.bin_len;
            unsigned int siglen = sign.signData.bin_len;
            unsigned char *sigbuf = (unsigned char *) sign.signData.bin_data;

    #ifdef DISTRIB_DEBUG
            std::cerr << "GxsSecurity::validateNxsMsg() Decode Key";
            std::cerr << " keylen: " << keylen << " siglen: " << siglen;
            std::cerr << std::endl;
    #endif

            /* extract admin key */

            RSA *rsakey = (key.keyFlags & RSTLV_KEY_TYPE_FULL)?  (d2i_RSAPrivateKey(NULL, &(keyptr), keylen)) : (d2i_RSAPublicKey(NULL, &(keyptr), keylen));

            if (!rsakey)
            {
    #ifdef GXS_SECURITY_DEBUG
                    std::cerr << "GxsSecurity::validateNxsMsg()";
                    std::cerr << " Invalid RSA Key";
                    std::cerr << std::endl;

                    key.print(std::cerr, 10);
    #endif
            }


            RsTlvKeySignatureSet signSet = msgMeta.signSet;
            msgMeta.signSet.TlvClear();

            RsGxsMessageId msgId = msgMeta.mMsgId, origMsgId = msgMeta.mOrigMsgId;
            msgMeta.mOrigMsgId.clear();
            msgMeta.mMsgId.clear();

            uint32_t metaDataLen = msgMeta.serial_size();
            uint32_t allMsgDataLen = metaDataLen + msg.msg.bin_len;
            char* metaData = new char[metaDataLen];
            char* allMsgData = new char[allMsgDataLen]; // msgData + metaData

            msgMeta.serialise(metaData, &metaDataLen);

            // copy msg data and meta in allmsgData buffer
            memcpy(allMsgData, msg.msg.bin_data, msg.msg.bin_len);
            memcpy(allMsgData+(msg.msg.bin_len), metaData, metaDataLen);

				delete[] metaData ;

            EVP_PKEY *signKey = EVP_PKEY_new();
            EVP_PKEY_assign_RSA(signKey, rsakey);

            /* calc and check signature */
            EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

            EVP_VerifyInit(mdctx, EVP_sha1());
            EVP_VerifyUpdate(mdctx, allMsgData, allMsgDataLen);
            int signOk = EVP_VerifyFinal(mdctx, sigbuf, siglen, signKey);

				delete[] allMsgData ;

            /* clean up */
            EVP_PKEY_free(signKey);
            EVP_MD_CTX_destroy(mdctx);

            msgMeta.mOrigMsgId = origMsgId;
            msgMeta.mMsgId = msgId;
            msgMeta.signSet = signSet;

            if (signOk == 1)
            {
    #ifdef GXS_SECURITY_DEBUG
                    std::cerr << "GxsSecurity::validateNxsMsg() Signature OK";
                    std::cerr << std::endl;
    #endif
                    return true;
            }

    #ifdef GXS_SECURITY_DEBUG
            std::cerr << "GxsSecurity::validateNxsMsg() Signature invalid";
            std::cerr << std::endl;
    #endif

	return false;
}


bool GxsSecurity::initEncryption(GxsSecurity::MultiEncryptionContext& encryption_context, const std::vector<RsTlvSecurityKey>& keys)
{
    // prepare an array of encrypted keys ek and public keys puk

    try
    {
	    encryption_context.clear() ;

	    encryption_context.ek  = new unsigned char *[keys.size()] ;
	    encryption_context.ekl = new int            [keys.size()] ;
	encryption_context.ids.resize(keys.size()) ;

	    EVP_PKEY      **pubk = new EVP_PKEY      *[keys.size()] ;
	    memset(pubk,0,keys.size()*sizeof(EVP_PKEY      *)) ;

	    memset(encryption_context.ek  ,0,keys.size()*sizeof(unsigned char *)) ;
	    memset(encryption_context.ekl ,0,keys.size()*sizeof(int            )) ;

	    for(uint32_t i=0;i<keys.size();++i)
	    {
		    RSA *tmpkey = ::extractPublicKey(keys[i]) ;
		    RSA *rsa_publish_pub = RSAPublicKey_dup(tmpkey) ;
		    RSA_free(tmpkey) ;

		    if(rsa_publish_pub == NULL)
			    throw std::runtime_error("Wrong key in input key table. Cannot extract public key.") ;

		    pubk[i] = EVP_PKEY_new();
		    EVP_PKEY_assign_RSA(pubk[i], rsa_publish_pub);

		    int max_evp_key_size = EVP_PKEY_size(pubk[i]);

		    encryption_context.ek [i] = (unsigned char*)malloc(max_evp_key_size);
		    encryption_context.ekl[i] = max_evp_key_size ;
		    encryption_context.ids[i] = keys[i].keyId ;
	    }

	    EVP_CIPHER_CTX_init(&encryption_context.ctx);

	    const EVP_CIPHER *cipher = EVP_aes_128_cbc();

	    // intialize context and send store encrypted cipher key in ek

	    if(!EVP_SealInit(&encryption_context.ctx, cipher, encryption_context.ek, encryption_context.ekl, encryption_context.iv, pubk, keys.size())) 
		    throw std::runtime_error("Error in EVP_SealInit. Cannot init encryption. Something's wrong.") ;
            
            return true ;
    }
    catch(std::exception& e)
    {
	    std::cerr << "(EE) cannot init encryption context: " << e.what() << std::endl;
	    encryption_context.clear() ;
	    return false ;
    }
}

bool GxsSecurity::encrypt(uint8_t *& out, uint32_t &outlen, const uint8_t *in, uint32_t inlen, GxsSecurity::MultiEncryptionContext& encryption_context)
{
    // encrypting for a single security key. This is a proxy function.
    
    out = NULL ;
    
    try
    {
	    int eklen, net_ekl;
	    int out_currOffset = 0;
	    int out_offset = 0;

	    int size_net_ekl = sizeof(net_ekl);

	    const EVP_CIPHER *cipher = EVP_CIPHER_CTX_cipher(&encryption_context.ctx) ;
	    int cipher_block_size = EVP_CIPHER_block_size(cipher);
	    int max_outlen = inlen + cipher_block_size ;

	    // now assign memory to out accounting for data, and cipher block size, key length, and key length val
        
	    out = (uint8_t*)malloc(max_outlen) ;

	    if(out == NULL)
		throw std::runtime_error("GxsSecurity::encrypt(): cannot allocate memory") ;

	    // now encrypt actual data
            
	    if(!EVP_SealUpdate(&encryption_context.ctx, (unsigned char*)out, &out_currOffset, (unsigned char*) in, inlen)) 
            	throw std::runtime_error("(EE) EVP_SealUpdate failed. Cannot encrypt.") ;

	    // move along to partial block space
	    out_offset += out_currOffset;
	    out_currOffset = 0 ;

	    // add padding
	    if(!EVP_SealFinal(&encryption_context.ctx, (unsigned char*)&out[out_offset], &out_currOffset)) 
            	throw std::runtime_error("(EE) EVP_SealFinal failed. Cannot encrypt.") ;

	    // move to end
	    out_offset += out_currOffset;

	    // make sure offset has not gone passed valid memory bounds
	    if(out_offset > max_outlen) 
            	throw std::runtime_error("(EE) GxsSecurity::encrypt(): exceeded memory bounds! This is a serious bug.") ;

	    outlen = out_offset;
        
	    return true;
    }
    catch(std::exception& e)
    {
        if(out)
            free(out) ;
        
        return false ;
    }
}

bool GxsSecurity::encrypt(uint8_t *& out, uint32_t &outlen, const uint8_t *in, uint32_t inlen, const RsTlvSecurityKey& key)
{
#ifdef DISTRIB_DEBUG
	std::cerr << "GxsSecurity::encrypt() " << std::endl;
#endif
        // Encrypts (in,inlen) into (out,outlen) using the given RSA public key.
        // The format of the encrypted data is:
        //
        //   [--- Encrypted session key length ---|--- Encrypted session key ---|--- IV ---|---- Encrypted data ---]
        //

	RSA *tmpkey = ::extractPublicKey(key) ;
	RSA *rsa_publish_pub = RSAPublicKey_dup(tmpkey) ;
	RSA_free(tmpkey) ;

	EVP_PKEY *public_key = NULL;

	//RSA* rsa_publish = EVP_PKEY_get1_RSA(privateKey);
	//rsa_publish_pub = RSAPublicKey_dup(rsa_publish);


	if(rsa_publish_pub  != NULL)
	{
		public_key = EVP_PKEY_new();
		EVP_PKEY_assign_RSA(public_key, rsa_publish_pub);
	}
	else
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "GxsSecurity(): Could not generate publish key " << grpId
		          << std::endl;
#endif
		return false;
	}

	EVP_CIPHER_CTX ctx;
	int eklen, net_ekl;
	unsigned char *ek;
	unsigned char iv[EVP_MAX_IV_LENGTH];
	EVP_CIPHER_CTX_init(&ctx);
	int out_currOffset = 0;
	int out_offset = 0;

	int max_evp_key_size = EVP_PKEY_size(public_key);
	ek = (unsigned char*)rs_malloc(max_evp_key_size);
    
    	if(ek == NULL)
            return false ;
        
	const EVP_CIPHER *cipher = EVP_aes_128_cbc();
	int cipher_block_size = EVP_CIPHER_block_size(cipher);
	int size_net_ekl = sizeof(net_ekl);

	int max_outlen = inlen + cipher_block_size + EVP_MAX_IV_LENGTH + max_evp_key_size + size_net_ekl;

	// intialize context and send store encrypted cipher in ek
	if(!EVP_SealInit(&ctx, EVP_aes_128_cbc(), &ek, &eklen, iv, &public_key, 1)) return false;

	// now assign memory to out accounting for data, and cipher block size, key length, and key length val
    out = (uint8_t*)rs_malloc(inlen + cipher_block_size + size_net_ekl + eklen + EVP_MAX_IV_LENGTH);

    if(out == NULL)
        return false ;

	net_ekl = htonl(eklen);
	memcpy((unsigned char*)out + out_offset, &net_ekl, size_net_ekl);
	out_offset += size_net_ekl;

	memcpy((unsigned char*)out + out_offset, ek, eklen);
	out_offset += eklen;

	memcpy((unsigned char*)out + out_offset, iv, EVP_MAX_IV_LENGTH);
	out_offset += EVP_MAX_IV_LENGTH;

	// now encrypt actual data
	if(!EVP_SealUpdate(&ctx, (unsigned char*) out + out_offset, &out_currOffset, (unsigned char*) in, inlen)) 
	{
		free(out) ;
		out = NULL ;
		return false;
	}

	// move along to partial block space
	out_offset += out_currOffset;

	// add padding
	if(!EVP_SealFinal(&ctx, (unsigned char*) out + out_offset, &out_currOffset)) 
	{
		free(out) ;
		out = NULL ;
		return false;
	}

	// move to end
	out_offset += out_currOffset;

	// make sure offset has not gone passed valid memory bounds
	if(out_offset > max_outlen) 
	{
		free(out) ;
		out = NULL ;
		return false;
	}

	// free encrypted key data
	free(ek);

	outlen = out_offset;
	return true;
}

bool GxsSecurity::initDecryption(GxsSecurity::MultiEncryptionContext& encryption_context, const RsTlvSecurityKey& key,unsigned char *IV,uint32_t IV_size,unsigned char *encrypted_session_key,uint32_t encrypted_session_key_size)
{
	// prepare an array of encrypted keys ek and public keys puk

	try
	{
		encryption_context.clear() ;

		encryption_context.ek  = new unsigned char *[1] ;
		encryption_context.ekl = new int            [1] ;

		RSA *rsa_publish = extractPrivateKey(key) ;

		if(rsa_publish == NULL)
		{
#ifdef DISTRIB_DEBUG
			std::cerr << "GxsSecurity(): Could not generate publish key " << grpId
			          << std::endl;
#endif
			return false;
		}

		EVP_PKEY *privateKey = EVP_PKEY_new();
		EVP_PKEY_assign_RSA(privateKey, rsa_publish);

		encryption_context.ek[0]  = (unsigned char*)malloc(EVP_PKEY_size(privateKey));
		encryption_context.ekl[0] = encrypted_session_key_size ;

		memcpy(encryption_context.ek[0],encrypted_session_key,encrypted_session_key_size) ;

		EVP_CIPHER_CTX_init(&encryption_context.ctx);

		const EVP_CIPHER* cipher = EVP_aes_128_cbc();

		if(!EVP_OpenInit(&encryption_context.ctx, cipher, encryption_context.ek[0], encryption_context.ekl[0], IV, privateKey)) 
		{
			std::cerr << "(EE) Cannot decrypt data. Most likely reason: private GXS key is missing." << std::endl;
			encryption_context.clear() ;
			return false;
		}
		return true ;
	}
	catch(std::exception& e)
	{
		std::cerr << "(EE) cannot init decryption context: " << e.what() << std::endl;
		encryption_context.clear() ;
		return false ;
	}
}

bool GxsSecurity::decrypt(uint8_t *&out, uint32_t &outlen, const uint8_t *in, uint32_t inlen, MultiEncryptionContext& encryption_context) 
{
    out = (uint8_t*)malloc(inlen);	// this is conservative

    if(out == NULL)
    {
	    std::cerr << "gxssecurity::decrypt(): cannot allocate memory of size " << inlen << " to decrypt data." << std::endl;
	    return false;
    }

    int out_currOffset = 0 ;

    if(!EVP_OpenUpdate(&encryption_context.ctx, (unsigned char*) out, &out_currOffset, (unsigned char*)in, inlen)) 
    {
	    std::cerr << "(EE) EVP_OpenUpdate failed! Decryption context is probably not inited correctly" << std::endl;
	    free(out) ;
	    out = NULL ;
	    outlen=0 ;
	    return false;
    }

    outlen = out_currOffset;

    if(!EVP_OpenFinal(&encryption_context.ctx, (unsigned char*)out + out_currOffset, &out_currOffset)) 
    {
	    free(out) ;
	    out = NULL ;
	    outlen=0 ;
	    return false;
    }

    outlen += out_currOffset;

    return true ;
}

bool GxsSecurity::decrypt(uint8_t *& out, uint32_t & outlen, const uint8_t *in, uint32_t inlen, const RsTlvSecurityKey& key)
{
	// Decrypts (in,inlen) into (out,outlen) using the given RSA public key.
	// The format of the encrypted data (in) is:
	//
	//   [--- Encrypted session key length ---|--- Encrypted session key ---|--- IV ---|---- Encrypted data ---]
	//
        // This method can be used to decrypt multi-encrypted data, if passing he correct encrypted key block (corresponding to the given key)

#ifdef DISTRIB_DEBUG
	std::cerr << "GxsSecurity::decrypt() " << std::endl;
#endif
	RSA *rsa_publish = extractPrivateKey(key) ;
	EVP_PKEY *privateKey = NULL;

	//RSA* rsa_publish = EVP_PKEY_get1_RSA(privateKey);
	//rsa_publish_pub = RSAPublicKey_dup(rsa_publish);

	if(rsa_publish != NULL)
	{
		privateKey = EVP_PKEY_new();
		EVP_PKEY_assign_RSA(privateKey, rsa_publish);
	}
	else
	{
#ifdef DISTRIB_DEBUG
		std::cerr << "GxsSecurity(): Could not generate publish key " << grpId
			<< std::endl;
#endif
		return false;
	}


    EVP_CIPHER_CTX ctx;
    int eklen = 0, net_ekl = 0;
	unsigned char *ek = (unsigned char*)rs_malloc(EVP_PKEY_size(privateKey));
    
    if(ek == NULL)
        return false ;
    
    unsigned char iv[EVP_MAX_IV_LENGTH];
    EVP_CIPHER_CTX_init(&ctx);

    int in_offset = 0, out_currOffset = 0;
    int size_net_ekl = sizeof(net_ekl);

    memcpy(&net_ekl, (unsigned char*)in, size_net_ekl);
    eklen = ntohl(net_ekl);
    in_offset += size_net_ekl;

	// Conservative limits to detect weird errors due to corrupted encoding.
	if(eklen < 0 || eklen > 512 || eklen+in_offset > (int)inlen)
	{
		std::cerr << "Error while deserialising encryption key length: eklen = " << std::dec << eklen << ". Giving up decryption." << std::endl;
		free(ek);
		return false;
	}

    memcpy(ek, (unsigned char*)in + in_offset, eklen);
    in_offset += eklen;

    memcpy(iv, (unsigned char*)in + in_offset, EVP_MAX_IV_LENGTH);
    in_offset += EVP_MAX_IV_LENGTH;

    const EVP_CIPHER* cipher = EVP_aes_128_cbc();

    if(!EVP_OpenInit(&ctx, cipher, ek, eklen, iv, privateKey)) 
    {
        std::cerr << "(EE) Cannot decrypt data. Most likely reason: private GXS key is missing." << std::endl;
        return false;
    }

	 if(inlen < in_offset)
	 {
		 std::cerr << "Severe error in " << __PRETTY_FUNCTION__ << ": cannot encrypt. " << std::endl;
		 return false ;
	 }
    out = (uint8_t*)rs_malloc(inlen - in_offset);

    if(out == NULL)
         return false;

    if(!EVP_OpenUpdate(&ctx, (unsigned char*) out, &out_currOffset, (unsigned char*)in + in_offset, inlen - in_offset)) 
	 {
         free(out) ;
		 out = NULL ;
		 return false;
	 }

    outlen = out_currOffset;

    if(!EVP_OpenFinal(&ctx, (unsigned char*)out + out_currOffset, &out_currOffset)) 
	 {
         free(out) ;
		 out = NULL ;
		 return false;
	 }

    outlen += out_currOffset;
    free(ek);

	 return true;
}


bool GxsSecurity::validateNxsGrp(const RsNxsGrp& grp, const RsTlvKeySignature& sign, const RsTlvSecurityKey& key)
{
#ifdef GXS_SECURITY_DEBUG
	std::cerr << "GxsSecurity::validateNxsGrp()";
	std::cerr << std::endl;
	std::cerr << "RsNxsGrp :";
	std::cerr << std::endl;
	grp.print(std::cerr, 10);
	std::cerr << std::endl;
#endif

	RsGxsGrpMetaData& grpMeta = *(grp.metaData);

	/********************* check signature *******************/

	/* check signature timeperiod */
	if ((grpMeta.mPublishTs < key.startTS) || (key.endTS != 0 && grpMeta.mPublishTs > key.endTS))
	{
#ifdef GXS_SECURITY_DEBUG
		std::cerr << " GxsSecurity::validateNxsMsg() TS out of range";
		std::cerr << std::endl;
#endif
		return false;
	}

	/* decode key */
	const unsigned char *keyptr = (const unsigned char *) key.keyData.bin_data;
	long keylen = key.keyData.bin_len;
	unsigned int siglen = sign.signData.bin_len;
	unsigned char *sigbuf = (unsigned char *) sign.signData.bin_data;

#ifdef DISTRIB_DEBUG
	std::cerr << "GxsSecurity::validateNxsMsg() Decode Key";
	std::cerr << " keylen: " << keylen << " siglen: " << siglen;
	std::cerr << std::endl;
#endif

	/* extract admin key */
	RSA *rsakey =  (key.keyFlags & RSTLV_KEY_TYPE_FULL)? d2i_RSAPrivateKey(NULL, &(keyptr), keylen): d2i_RSAPublicKey(NULL, &(keyptr), keylen);

	if (!rsakey)
	{
#ifdef GXS_SECURITY_DEBUG
		std::cerr << "GxsSecurity::validateNxsGrp()";
		std::cerr << " Invalid RSA Key";
		std::cerr << std::endl;

		key.print(std::cerr, 10);
#endif
	}

	std::vector<uint32_t> api_versions_to_check ;
	api_versions_to_check.push_back(RS_GXS_GRP_META_DATA_VERSION_ID_0002) ;	// put newest first, for debug info purpose
	api_versions_to_check.push_back(RS_GXS_GRP_META_DATA_VERSION_ID_0001) ;

	RsTlvKeySignatureSet signSet = grpMeta.signSet;
	grpMeta.signSet.TlvClear();
    
	int signOk =0;
	EVP_PKEY *signKey = EVP_PKEY_new();
	EVP_PKEY_assign_RSA(signKey, rsakey);


	for(uint32_t i=0;i<api_versions_to_check.size() && 0==signOk;++i)
	{
		uint32_t metaDataLen = grpMeta.serial_size(api_versions_to_check[i]);
		uint32_t allGrpDataLen = metaDataLen + grp.grp.bin_len;

		RsTemporaryMemory metaData(metaDataLen) ;
		RsTemporaryMemory allGrpData(allGrpDataLen) ;// msgData + metaData

		grpMeta.serialise(metaData, metaDataLen,api_versions_to_check[i]);

		// copy msg data and meta in allmsgData buffer
		memcpy(allGrpData, grp.grp.bin_data, grp.grp.bin_len);
		memcpy(allGrpData+(grp.grp.bin_len), metaData, metaDataLen);

		/* calc and check signature */
		EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

		EVP_VerifyInit(mdctx, EVP_sha1());
		EVP_VerifyUpdate(mdctx, allGrpData, allGrpDataLen);
		signOk = EVP_VerifyFinal(mdctx, sigbuf, siglen, signKey);
		EVP_MD_CTX_destroy(mdctx);

                if(i>0)
		std::cerr << "(WW) Checking group signature with old api version " << i+1 << " : tag " << std::hex << api_versions_to_check[i] << std::dec << " result: " << signOk << std::endl;
	}

	/* clean up */
	EVP_PKEY_free(signKey);

	// restore data

	grpMeta.signSet = signSet;

	if (signOk == 1)
	{
#ifdef GXS_SECURITY_DEBUG
		std::cerr << "GxsSecurity::validateNxsGrp() Signature OK";
		std::cerr << std::endl;
#endif
		return true;
	}

#ifdef GXS_SECURITY_DEBUG
	std::cerr << "GxsSecurity::validateNxsGrp() Signature invalid";
	std::cerr << std::endl;
#endif

	return false;
}



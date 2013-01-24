
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
#include "retroshare/rspeers.h"

#define GXS_SECURITY_DEBUG

GxsSecurity::GxsSecurity()
{
}

GxsSecurity::~GxsSecurity()
{
}

RSA *GxsSecurity::extractPublicKey(RsTlvSecurityKey& key)
{
        const unsigned char *keyptr = (const unsigned char *) key.keyData.bin_data;
        long keylen = key.keyData.bin_len;

        /* extract admin key */
        RSA *rsakey = d2i_RSAPublicKey(NULL, &(keyptr), keylen);

        return rsakey;
}

bool GxsSecurity::getSignature(char* data, uint32_t data_len, RsTlvSecurityKey* privKey, RsTlvKeySignature& sign)
{
	RSA* rsa_pub = extractPrivateKey(*privKey);
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
	sign.keyId = privKey->keyId;

	return ok;
}

bool GxsSecurity::validateNxsMsg(RsNxsMsg& msg, RsTlvKeySignature& sign, RsTlvSecurityKey& key)
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
            if ((msgMeta.mPublishTs < key.startTS) ||
                    (msgMeta.mPublishTs > key.endTS))
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
            RSA *rsakey = d2i_RSAPublicKey(NULL, &(keyptr), keylen);

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


            EVP_PKEY *signKey = EVP_PKEY_new();
            EVP_PKEY_assign_RSA(signKey, rsakey);

            /* calc and check signature */
            EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

            EVP_VerifyInit(mdctx, EVP_sha1());
            EVP_VerifyUpdate(mdctx, allMsgData, allMsgDataLen);
            int signOk = EVP_VerifyFinal(mdctx, sigbuf, siglen, signKey);

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



std::string GxsSecurity::getBinDataSign(void *data, int len)
{
        unsigned char *tmp = (unsigned char *) data;

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
}



bool GxsSecurity::encrypt(void *& out, int & outlen, const void *in, int inlen, EVP_PKEY *privateKey)
{


#ifdef DISTRIB_DEBUG
        std::cerr << "GxsSecurity::encrypt() " << std::endl;
#endif

        RSA *rsa_publish_pub = NULL;
        EVP_PKEY *public_key = NULL;

        RSA* rsa_publish = EVP_PKEY_get1_RSA(privateKey);
        rsa_publish_pub = RSAPublicKey_dup(rsa_publish);


        if(rsa_publish_pub  != NULL){
                public_key = EVP_PKEY_new();
                EVP_PKEY_assign_RSA(public_key, rsa_publish_pub);
        }else{
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
    ek = (unsigned char*)malloc(max_evp_key_size);
    const EVP_CIPHER *cipher = EVP_aes_128_cbc();
    int cipher_block_size = EVP_CIPHER_block_size(cipher);
    int size_net_ekl = sizeof(net_ekl);

    int max_outlen = inlen + cipher_block_size + EVP_MAX_IV_LENGTH + max_evp_key_size + size_net_ekl;

    // intialize context and send store encrypted cipher in ek
        if(!EVP_SealInit(&ctx, EVP_aes_128_cbc(), &ek, &eklen, iv, &public_key, 1)) return false;

        // now assign memory to out accounting for data, and cipher block size, key length, and key length val
    out = new unsigned char[inlen + cipher_block_size + size_net_ekl + eklen + EVP_MAX_IV_LENGTH];

        net_ekl = htonl(eklen);
        memcpy((unsigned char*)out + out_offset, &net_ekl, size_net_ekl);
        out_offset += size_net_ekl;

        memcpy((unsigned char*)out + out_offset, ek, eklen);
        out_offset += eklen;

        memcpy((unsigned char*)out + out_offset, iv, EVP_MAX_IV_LENGTH);
        out_offset += EVP_MAX_IV_LENGTH;

        // now encrypt actual data
        if(!EVP_SealUpdate(&ctx, (unsigned char*) out + out_offset, &out_currOffset, (unsigned char*) in, inlen)) return false;

        // move along to partial block space
        out_offset += out_currOffset;

        // add padding
        if(!EVP_SealFinal(&ctx, (unsigned char*) out + out_offset, &out_currOffset)) return false;

        // move to end
        out_offset += out_currOffset;

        // make sure offset has not gone passed valid memory bounds
        if(out_offset > max_outlen) return false;

        // free encrypted key data
        free(ek);

        outlen = out_offset;
        return true;

    delete[] ek;

#ifdef DISTRIB_DEBUG
    std::cerr << "GxsSecurity::encrypt() finished with outlen : " << outlen << std::endl;
#endif

    return true;
}


bool GxsSecurity::decrypt(void *& out, int & outlen, const void *in, int inlen, EVP_PKEY *privateKey)
{

#ifdef DISTRIB_DEBUG
        std::cerr << "GxsSecurity::decrypt() " << std::endl;
#endif

    EVP_CIPHER_CTX ctx;
    int eklen = 0, net_ekl = 0;
    unsigned char *ek = NULL;
    unsigned char iv[EVP_MAX_IV_LENGTH];
    ek = (unsigned char*)malloc(EVP_PKEY_size(privateKey));
    EVP_CIPHER_CTX_init(&ctx);

    int in_offset = 0, out_currOffset = 0;
    int size_net_ekl = sizeof(net_ekl);

    memcpy(&net_ekl, (unsigned char*)in, size_net_ekl);
    eklen = ntohl(net_ekl);
    in_offset += size_net_ekl;

    memcpy(ek, (unsigned char*)in + in_offset, eklen);
    in_offset += eklen;

    memcpy(iv, (unsigned char*)in + in_offset, EVP_MAX_IV_LENGTH);
    in_offset += EVP_MAX_IV_LENGTH;

    const EVP_CIPHER* cipher = EVP_aes_128_cbc();

    if(!EVP_OpenInit(&ctx, cipher, ek, eklen, iv, privateKey)) return false;

    out = new unsigned char[inlen - in_offset];

    if(!EVP_OpenUpdate(&ctx, (unsigned char*) out, &out_currOffset, (unsigned char*)in + in_offset, inlen - in_offset)) return false;

    in_offset += out_currOffset;
    outlen += out_currOffset;

    if(!EVP_OpenFinal(&ctx, (unsigned char*)out + out_currOffset, &out_currOffset)) return false;

    outlen += out_currOffset;

    free(ek);

        return true;
}

std::string GxsSecurity::getRsaKeySign(RSA *pubkey)
{
        int len = BN_num_bytes(pubkey -> n);
        unsigned char tmp[len];
        BN_bn2bin(pubkey -> n, tmp);

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
}


bool GxsSecurity::validateNxsGrp(RsNxsGrp *newGrp, RsTlvKeySignature& sign, RsTlvSecurityKey& key)
{
	return false;
}

void GxsSecurity::setRSAPublicKey(RsTlvSecurityKey & key, RSA *rsa_pub)
{
        unsigned char data[10240]; /* more than enough space */
        unsigned char *ptr = data;
        int reqspace = i2d_RSAPublicKey(rsa_pub, &ptr);

        key.keyData.setBinData(data, reqspace);

        std::string keyId = getRsaKeySign(rsa_pub);
        key.keyId = keyId;
}



void GxsSecurity::setRSAPrivateKey(RsTlvSecurityKey & key, RSA *rsa_priv)
{
        unsigned char data[10240]; /* more than enough space */
        unsigned char *ptr = data;
        int reqspace = i2d_RSAPrivateKey(rsa_priv, &ptr);

        key.keyData.setBinData(data, reqspace);

        std::string keyId = getRsaKeySign(rsa_priv);
        key.keyId = keyId;
}

RSA *GxsSecurity::extractPrivateKey(RsTlvSecurityKey & key)
{
        const unsigned char *keyptr = (const unsigned char *) key.keyData.bin_data;
        long keylen = key.keyData.bin_len;

        /* extract admin key */
        RSA *rsakey = d2i_RSAPrivateKey(NULL, &(keyptr), keylen);

        return rsakey;
}



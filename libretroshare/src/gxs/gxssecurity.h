#ifndef GXSSECURITY_H
#define GXSSECURITY_H

/*
 * libretroshare/src/gxs: gxssecurity.h
 *
 * Security functions for Gxs
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

#include "serialiser/rstlvkeys.h"
#include "serialiser/rsnxsitems.h"

#include <openssl/ssl.h>
#include <openssl/evp.h>


/*!
 * This contains functionality for performing security
 * operations needed to validate data received in RsGenExchange
 * Also has routine for creating security objects around msgs and groups
 */
class GxsSecurity {

public:

        GxsSecurity();
        ~GxsSecurity();

        /*!
         * extracts the public key from an RsTlvSecurityKey
         * @param key RsTlvSecurityKey to extract public RSA key from
         * @return pointer to the public RSA key if successful, null otherwise
         */
        static RSA *extractPublicKey(const RsTlvSecurityKey &key);

        /*!
         * extracts the public key from an RsTlvSecurityKey
         * @param key RsTlvSecurityKey to extract private RSA key from
         * @return pointer to the private RSA key if successful, null otherwise
         */
        static RSA *extractPrivateKey(const RsTlvSecurityKey &key);

        /*!
         * stores the rsa public key in a RsTlvSecurityKey
         * @param key RsTlvSecurityKey to store the public rsa key in
         * @param rsa_pub
         */
        static void setRSAPublicKey(RsTlvSecurityKey &key, RSA *rsa_pub);

        /*!
         * stores the rsa private key in a RsTlvSecurityKey
         * @param key stores the rsa private key in a RsTlvSecurityKey
         * @param rsa_priv the rsa private key to store
         */
        static void setRSAPrivateKey(RsTlvSecurityKey &key, RSA *rsa_priv);

        /*!
         * extracts signature from RSA key
         * @param pubkey
         * @return signature of RSA key in hex format
         */
        static std::string getRsaKeySign(RSA *pubkey);

        /*!
         * extracts the first CERTSIGNLEN bytes of signature and stores it in a string
         * in hex format
         * @param data signature
         * @param len the length of the signature data
         * @return returns the first CERTSIGNLEN of the signature as a string
         */
        static std::string getBinDataSign(void *data, int len);

        /*!
         * Encrypts data using envelope encryption (taken from open ssl's evp_sealinit )
         * only full publish key holders can encrypt data for given group
         *@param out
         *@param outlen
         *@param in
         *@param inlen
         */
        static bool encrypt(void *&out, int &outlen, const void *in, int inlen, const RsTlvSecurityKey& key) ;


        /**
         * Decrypts data using evelope decryption (taken from open ssl's evp_sealinit )
         * only full publish key holders can decrypt data for a group
         * @param out where decrypted data is written to
         * @param outlen
         * @param in
         * @param inlen
         * @return false if encryption failed
         */
        static bool decrypt(void *&out, int &outlen, const void *in, int inlen, const RsTlvSecurityKey& key) ;

        /*!
         * uses grp signature to check if group has been
         * tampered with
         * @param newGrp the Nxs group to be validated
         * @param sign the signature to validdate against
         * @param key the public key to use to check signature
         * @return true if group valid false otherwise
         */
        static bool validateNxsGrp(RsNxsGrp& grp, RsTlvKeySignature& sign, RsTlvSecurityKey& key);

        /*!
         * Validate a msg's signature using the given public key
         * @param msg the Nxs message to be validated
         * @param sign the signature to validdate against
         * @param key the public key to use to check signature
         * @return false if verfication of signature is not passed
         */
        static bool validateNxsMsg(RsNxsMsg& msg, RsTlvKeySignature& sign, RsTlvSecurityKey& key);


        /*!
         * @param data data to be signed
         * @param data_len length of data to be signed
         * @param privKey private key to used to make signature
         * @param sign the signature is stored here
         * @return false if signature creation failed, true is signature created
         */
        static bool getSignature(const char *data, uint32_t data_len, const RsTlvSecurityKey& privKey, RsTlvKeySignature& sign);

        /*!
         * @param data data that has been signed
         * @param data_len length of signed data 
         * @param privKey public key to used to check signature
         * @param sign Signature for the data 
         * @return true if signature checks
         */
        static bool validateSignature(const char *data, uint32_t data_len, const RsTlvSecurityKey& pubKey, const RsTlvKeySignature& sign);
};

#endif // GXSSECURITY_H

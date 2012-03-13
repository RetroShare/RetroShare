/*
 * libretroshare/src/distrib: p3distribverify.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2008-2010 by Robert Fernie
 *           2011 Christopher Evi-Parker
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

#ifndef P3DISTRIBVERIFY_H_
#define P3DISTRIBVERIFY_H_

#include "serialiser/rstlvkeys.h"
#include "distrib/p3distrib.h"

#include <openssl/ssl.h>
#include <openssl/evp.h>


/*!
 * This contains functionality for performing security
 * operations needed to validate data received in p3GroupDistrib
 * Also has functionality to receive data
 */
class p3DistribSecurity {

public:

	p3DistribSecurity();
	~p3DistribSecurity();

	/*!
	 * extracts the public key from an RsTlvSecurityKey
	 * @param key RsTlvSecurityKey to extract public RSA key from
	 * @return pointer to the public RSA key if successful, null otherwise
	 */
	static RSA *extractPublicKey(RsTlvSecurityKey &key);

	/*!
	 * extracts the public key from an RsTlvSecurityKey
	 * @param key RsTlvSecurityKey to extract private RSA key from
	 * @return pointer to the private RSA key if successful, null otherwise
	 */
	static RSA *extractPrivateKey(RsTlvSecurityKey &key);

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
	 * extracts the signature and stores it in a string
	 * in hex format
	 * @param data
	 * @param len
	 * @return
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
	static bool encrypt(void *&out, int &outlen, const void *in, int inlen, EVP_PKEY *privateKey);


	/**
	 * Decrypts data using evelope decryption (taken from open ssl's evp_sealinit )
	 * only full publish key holders can decrypt data for a group
	 * @param out where decrypted data is written to
	 * @param outlen
	 * @param in
	 * @param inlen
	 * @return false if encryption failed
	 */
	static bool decrypt(void *&out, int &outlen, const void *in, int inlen, EVP_PKEY *privateKey);

	/*!
	 * uses grp signature to check if group has been
	 * tampered with
	 * @param newGrp
	 * @return true if group valid false otherwise
	 */
	static bool validateDistribGrp(RsDistribGrp *newGrp);

	/*!
	 * uses groupinfo public key to verify signature of signed message
	 * @param info groupinfo for which msg is meant for
	 * @param msg
	 * @return false if verfication of signature is not passed
	 */
	static bool validateDistribSignedMsg(GroupInfo &info, RsDistribSignedMsg *msg);
};

#endif /* P3DISTRIBVERIFY_H_ */

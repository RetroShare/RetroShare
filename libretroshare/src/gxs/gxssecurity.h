/*******************************************************************************
 * libretroshare/src/gxs: gxssecurity.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008-2010 by Robert Fernie        <retroshare@lunamutt.com>       *
 *           2011-2012 Christopher Evi-Parker                                  *
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
#ifndef GXSSECURITY_H
#define GXSSECURITY_H

#include "serialiser/rstlvkeys.h"

#include "rsitems/rsnxsitems.h"

#include <openssl/ssl.h>
#include <openssl/evp.h>


/*!
 * This contains functionality for performing basic security operations needed
 * in RsGenExchange operations.
 * Also has routine for creating security objects around msgs and groups
 * TODO: Those functions doesn't do param checking!
 */
class GxsSecurity 
{
	public:
		/*!
		 * Extracts a public key from a private key.
		 */
		static bool extractPublicKey(const RsTlvPrivateRSAKey& private_key,RsTlvPublicRSAKey& public_key) ;

		/*!
		 * Generates a public/private RSA keypair. To be used for all GXS purposes.
		 * @param RsTlvSecurityKey public  RSA key 
		 * @param RsTlvSecurityKey private RSA key 
		 * @return true if the generate was successful, false otherwise.
		 */
		static bool generateKeyPair(RsTlvPublicRSAKey &public_key, RsTlvPrivateRSAKey &private_key) ;

		/*!
		 * Encrypts data using envelope encryption (taken from open ssl's evp_sealinit )
		 * only full publish key holders can encrypt data for given group
		 *@param out
		 *@param outlen
		 *@param in
		 *@param inlen
		 */
		static bool encrypt(uint8_t *&out, uint32_t &outlen, const uint8_t *in, uint32_t inlen, const RsTlvPublicRSAKey& key) ;
		static bool encrypt(uint8_t *&out, uint32_t &outlen, const uint8_t *in, uint32_t inlen, const std::vector<RsTlvPublicRSAKey>& keys) ;

		/**
		 * Decrypts data using evelope decryption (taken from open ssl's evp_sealinit )
		 * only full publish key holders can decrypt data for a group
		 * @param out where decrypted data is written to
		 * @param outlen
		 * @param in
		 * @param inlen
		 * @return false if encryption failed
		 */
		static bool decrypt(uint8_t *&out, uint32_t &outlen, const uint8_t *in, uint32_t inlen, const RsTlvPrivateRSAKey& key) ;
		static bool decrypt(uint8_t *& out, uint32_t & outlen, const uint8_t *in, uint32_t inlen, const std::vector<RsTlvPrivateRSAKey>& keys);

		/*!
		 * uses grp signature to check if group has been
		 * tampered with
		 * @param newGrp the Nxs group to be validated
		 * @param sign the signature to validdate against
		 * @param key the public key to use to check signature
		 * @return true if group valid false otherwise
		 */
        static bool validateNxsGrp(const RsNxsGrp& grp, const RsTlvKeySignature& sign, const RsTlvPublicRSAKey& key);

		/*!
		 * Validate a msg's signature using the given public key
		 * @param msg the Nxs message to be validated
		 * @param sign the signature to validdate against
		 * @param key the public key to use to check signature
		 * @return false if verfication of signature is not passed
		 */
        static bool validateNxsMsg(const RsNxsMsg& msg, const RsTlvKeySignature& sign, const RsTlvPublicRSAKey &key);


		/*!
		 * @param data data to be signed
		 * @param data_len length of data to be signed
		 * @param privKey private key to used to make signature
		 * @param sign the signature is stored here
		 * @return false if signature creation failed, true is signature created
		 */
		static bool getSignature(const char *data, uint32_t data_len, const RsTlvPrivateRSAKey& privKey, RsTlvKeySignature& sign);

		/*!
		 * @param data data that has been signed
		 * @param data_len length of signed data 
		 * @param privKey public key to used to check signature
		 * @param sign Signature for the data 
		 * @return true if signature checks
		 */
        static bool validateSignature(const char *data, uint32_t data_len, const RsTlvPublicRSAKey& pubKey, const RsTlvKeySignature& sign);

        /*!
         * Checks that the public key has correct fingerprint and correct flags.
         * @brief checkPublicKey
         * @param key
         * @return false if the key is invalid.
         */

        static bool checkPublicKey(const RsTlvPublicRSAKey &key);
        static bool checkPrivateKey(const RsTlvPrivateRSAKey &key);
	static bool checkFingerprint(const RsTlvPublicRSAKey& key);	// helper function to only check the fingerprint
        
        /*!
         * Adds possibly missing public keys when private keys are present.
         * 
         * \brief createPublicKeysForPrivateKeys
         * \param set   set of keys to consider
         * \return 
         */
        static void createPublicKeysFromPrivateKeys(RsTlvSecurityKeySet& set) ;
};

#endif // GXSSECURITY_H

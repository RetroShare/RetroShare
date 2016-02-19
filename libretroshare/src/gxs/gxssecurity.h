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
class GxsSecurity 
{
	public:
    	    /*!
             * \brief The MultiEncryptionContext struct
             * 
             * This structure is used to store encryption keys generated when encrypting for multiple keys at once, so that
             * the client doesn't need to know about all the libcrypto variables involved.
             * Typically, the client will first ask to init a MultiEncryptionContext by providing several GXS ids,
             * and then pass the structure as a parameter to encrypt some data with the same key.
             */
    		class MultiEncryptionContext
	    	{
	    	public:
		   	 MultiEncryptionContext()  { ekl=NULL;ek=NULL;}
		   	 ~MultiEncryptionContext() { clear() ;}

		   	 void clear() 
			 {
				 for(uint32_t i=0;i<ids.size();++i)
					 free(ek[i]) ;

				 if(ekl)
					 free(ekl) ;
				 if(ek)
					 free(ek) ;

				 ekl = NULL ;
				 ek = NULL ;

				 ids.clear() ;
			 }

		   	 // The functions below give access to the encrypted symmetric key to be used.
		   	 // 
		   	 int            n_encrypted_keys() const { return ids.size();}
		   	 RsGxsId        encrypted_key_id  (int i) const { return ids[i];}
		   	 unsigned char *encrypted_key_data(int i) const { return ek[i];}
		   	 int            encrypted_key_size(int i) { return ekl[i] ; }
		   	 const unsigned char *initialisation_vector() const { return iv ; }

	    	protected:
		   	 std::vector<RsGxsId> ids;		// array of destination ids
		   	 int *ekl ;                           // array of encrypted keys length 
		   	 unsigned char **ek ;                 // array of encrypted keys 
		   	 EVP_CIPHER_CTX ctx;                  // EVP encryption context
		   	 unsigned char iv[EVP_MAX_IV_LENGTH]; // initialization vector of the cipher.
             
             		friend class GxsSecurity ;
	    	};
		/*!
		 * Extracts a public key from a private key.
		 */
		static bool extractPublicKey(const RsTlvSecurityKey& private_key,RsTlvSecurityKey& public_key) ;

		/*!
		 * Generates a public/private RSA keypair. To be used for all GXS purposes.
		 * @param RsTlvSecurityKey public  RSA key 
		 * @param RsTlvSecurityKey private RSA key 
		 * @return true if the generate was successful, false otherwise.
		 */
		static bool generateKeyPair(RsTlvSecurityKey& public_key,RsTlvSecurityKey& private_key) ;

		/*!
		 * Encrypts data using envelope encryption (taken from open ssl's evp_sealinit )
		 * only full publish key holders can encrypt data for given group
		 *@param out
		 *@param outlen
		 *@param in
		 *@param inlen
		 */
		static bool encrypt(uint8_t *&out, uint32_t &outlen, const uint8_t *in, uint32_t inlen, const RsTlvSecurityKey& key) ;
		static bool encrypt(uint8_t *&out, uint32_t &outlen, const uint8_t *in, uint32_t inlen, const std::vector<RsTlvSecurityKey>& keys) ;
#ifdef TO_REMOVE
		/*!
		 * Encrypts/decrypt data using envelope encryption using the key pre-computed in the encryption context passed as
		 * parameter.
		 */
		static bool initEncryption(MultiEncryptionContext& encryption_context, const std::vector<RsTlvSecurityKey> &keys) ;
		static bool initDecryption(MultiEncryptionContext& encryption_context, const RsTlvSecurityKey& key, unsigned char *IV, uint32_t IV_size, unsigned char *encrypted_session_key, uint32_t encrypted_session_key_size) ;
        
		/*!
		 * Encrypts/decrypt data using envelope encryption using the key pre-computed in the encryption context passed as
		 * parameter.
		 */
		static bool encrypt(uint8_t *&out, uint32_t &outlen, const uint8_t *in, uint32_t inlen, MultiEncryptionContext& encryption_context) ;
		static bool decrypt(uint8_t *&out, uint32_t &outlen, const uint8_t *in, uint32_t inlen, MultiEncryptionContext& encryption_context) ;
#endif

		/**
		 * Decrypts data using evelope decryption (taken from open ssl's evp_sealinit )
		 * only full publish key holders can decrypt data for a group
		 * @param out where decrypted data is written to
		 * @param outlen
		 * @param in
		 * @param inlen
		 * @return false if encryption failed
		 */
		static bool decrypt(uint8_t *&out, uint32_t &outlen, const uint8_t *in, uint32_t inlen, const RsTlvSecurityKey& key) ;
		static bool decrypt(uint8_t *& out, uint32_t & outlen, const uint8_t *in, uint32_t inlen, const std::vector<RsTlvSecurityKey>& keys);

		/*!
		 * uses grp signature to check if group has been
		 * tampered with
		 * @param newGrp the Nxs group to be validated
		 * @param sign the signature to validdate against
		 * @param key the public key to use to check signature
		 * @return true if group valid false otherwise
		 */
        static bool validateNxsGrp(const RsNxsGrp& grp, const RsTlvKeySignature& sign, const RsTlvSecurityKey& key);

		/*!
		 * Validate a msg's signature using the given public key
		 * @param msg the Nxs message to be validated
		 * @param sign the signature to validdate against
		 * @param key the public key to use to check signature
		 * @return false if verfication of signature is not passed
		 */
        static bool validateNxsMsg(const RsNxsMsg& msg, const RsTlvKeySignature& sign, const RsTlvSecurityKey& key);


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

        /*!
         * Checks that the public key has correct fingerprint and correct flags.
         * @brief checkPublicKey
         * @param key
         * @return false if the key is invalid.
         */

        static bool checkPublicKey(const RsTlvSecurityKey &key);
        static bool checkPrivateKey(const RsTlvSecurityKey &key);
};

#endif // GXSSECURITY_H

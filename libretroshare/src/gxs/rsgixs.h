#ifndef RSGIXS_H
#define RSGIXS_H

/*
 * libretroshare/src/gxs: gxs.h
 *
 * General Exchange Protocol interface for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie, Christopher Evi-Prker
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

#include "gxs/rsgxs.h"

#include <openssl/ssl.h>
#include <set>

/*
 * GIXP: General Identity Exchange Protocol.
 *
 * As we're always running into troubles with GPG signatures... we are going to
 * create a layer of RSA Keys for the following properties:
 *
 * 1) RSA Keys can be Anonymous, Self-Signed with Pseudonym, Signed by GPG Key.
 *	- Anonymous & Pseudonym Keys will be shared network-wide (Hop by Hop).
        - GPG signed Keys will only be shared if we can validate the signature
                (providing similar behaviour to existing GPG Keys).
        - GPG signed Keys can optionally be marked for Network-wide sharing.
 * 2) These keys can be used anywhere, specifically in the protocols described below.
 * 3) These keys can be used to sign, encrypt, verify & decrypt
 * 4) Keys will never need to be directly accessed - stored in this class.
 * 5) They will be cached locally and exchanged p2p, by pull request.
 * 6) This class will use the generalised packet storage for efficient caching & loading.
 * 7) Data will be stored encrypted.
 */

    class GixsKey
    {
            KeyRef mKeyId;

            /// public key
            EVP_PKEY *mPubKey;

            ///  NULL if non-existant */
            EVP_PKEY *mPrivKey;
    };

    class KeyRef {



    };


    class KeyRefSet {
        std::set<KeyRef> mKeyRefSet;
    };

    class SignatureSet {
        std::set<Signature> mSignatureSet;
    };


    class Signature {

        KeyRef mKeyRef;
        Signature mSignature;
    };



    class RsGixsProfile {

    public:

        KeyRef mKeyRef;
        std::string mPseudonym;

        /// may be superseded by newer timestamps
        time_t mTimeStamp;
        uint32_t mProfileType;

        // TODO: add permissions members

        Signature mSignature;



    };

    /*!
     * Retroshare general identity exchange service
     * provides a means to distribute identities among peers
     * also provides encyption, decryption, verification,
     * and signing functionality using any created identities
     */
    class RsIdentityExchangeService
    {
    public:
        RsGixs();

        /*!
         * creates gixs profile and shares it
         * @param profile
         */
        bool createKey(RsGixsProfile& profile) = 0; /* fills in mKeyId, and signature */

        /*!
         * Use to query a whether given key is available by its key reference
         * @param keyref the keyref of key that is being checked for
         * @return true if available, false otherwise
         */
        bool haveKey(const KeyRef& keyref) = 0;

        /*!
         * Use to query whether private key member of the given key reference is available
         * @param keyref the KeyRef of the key being checked for
         * @return true if private key is held here, false otherwise
         */
        bool havePrivateKey(const KeyRef& keyref) = 0;

        /*!
         * Use to request a given key reference
         * @param keyref the KeyRef of the key being requested
         * @return will
         */
        bool requestKey(const KeyRef& keyref) = 0;

        /*!
         * Retrieves a key identity
         * @param keyref
         * @return a pointer to a valid profile if successful, otherwise NULL
         *
         */
        RsGixsProfile* getProfile(const KeyRef& keyref) = 0;


        /*** process data ***/

        /*!
         * Use to sign data with a given key
         * @param keyref the key to sign the data with
         * @param data the data to be signed
         * @param dataLen the length of the data
         * @param signature is set with the signature from signing with keyref
         * @return false if signing failed, true otherwise
         */
        bool sign(const KeyRef& keyref, unsigned char* data, uint32_t dataLen, std::string& signature) = 0;

        /*!
         * Verify that the data is signed by the key owner
         * @param keyref
         * @param data
         * @param dataLen
         * @param signature
         * @return false if verification failed, false otherwise
         */
        bool verify(const KeyRef& keyref, unsigned char* data, int dataLen, std::string& signature) = 0;

        /*!
         * Attempt to decrypt data with a given key
         * @param keyref
         * @param data data to be decrypted
         * @param dataLen length of data
         * @param decryptedData decrypted data
         * @param decryptDataLen length of decrypted data
         * @return false
         */
        bool decrypt(const KeyRef& keyref, unsigned char* data, int dataLen,
                     unsigned char*& decryptedData, uint32_t& decyptDataLen) = 0;

        /*!
         * Attempt to encrypt data with a given key
         * @param keyref
         * @param data data to be encrypted
         * @param dataLen length of data
         * @param encryptedData encrypted data
         * @param encryptDataLen length of encrypted data
         */
        bool encrypt(const KeyRef& keyref, unsigned char* data, int dataLen,
                     unsigned char*& encryptedData, uint32_t& encryptDataLen) = 0;

    };


#endif // RSGIXS_H

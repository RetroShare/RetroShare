/*******************************************************************************
 * libretroshare/src/pgp: pgphandler.h                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 Cyril Soler <csoler@users.sourceforge.net>                   *
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
#pragma once

#include <stdint.h>
#include <string>
#include <list>
#include <map>
#include <set>
#include <util/rsthreads.h>
#include <retroshare/rstypes.h>

typedef std::string (*PassphraseCallback)(void *data, const char *uid_title, const char *uid_hint, const char *passphrase_info, int prev_was_bad,bool *cancelled) ;

class PGPCertificateInfo
{
	public:
		PGPCertificateInfo() : _trustLvl(0), _validLvl(0), _flags(0), _type(0), _time_stamp(0), _key_index(0) {}

		std::string _name;
		std::string _email;
		std::string _comment;

		std::set<RsPgpId> signers;

		uint32_t _trustLvl;
		uint32_t _validLvl;
		uint32_t _flags ;
		uint32_t _type ;

		mutable rstime_t _time_stamp ;		// last time the key was used (received, used for signature verification, etc)

		PGPFingerprintType _fpr;           /* fingerprint */

        // Index to array of keys in the public keyring. Dependign on the specific implementation
        // of how the keyring is stored, this may be used differently.

        uint32_t _key_index ;

		static const uint32_t PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION      = 0x0001 ;
		static const uint32_t PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE     = 0x0002 ;
		static const uint32_t PGP_CERTIFICATE_FLAG_HAS_SIGNED_ME         = 0x0004 ;
		static const uint32_t PGP_CERTIFICATE_FLAG_UNSUPPORTED_ALGORITHM = 0x0008 ;	// set when the key is not RSA, so that RS avoids to use it.

		static const uint8_t PGP_CERTIFICATE_TRUST_UNDEFINED  = 0x00 ;
		static const uint8_t PGP_CERTIFICATE_TRUST_NEVER      = 0x02 ;
		static const uint8_t PGP_CERTIFICATE_TRUST_MARGINALLY = 0x03 ;
		static const uint8_t PGP_CERTIFICATE_TRUST_FULLY      = 0x04 ;
		static const uint8_t PGP_CERTIFICATE_TRUST_ULTIMATE   = 0x05 ;

		static const uint8_t PGP_CERTIFICATE_TYPE_UNKNOWN = 0x00 ;
		static const uint8_t PGP_CERTIFICATE_TYPE_DSA     = 0x01 ;
		static const uint8_t PGP_CERTIFICATE_TYPE_RSA     = 0x02 ;
};

/// This class offer an abstract pgp handler to be used in RetroShare.
class PGPHandler
{
public:
        PGPHandler(		const std::string& path_to_public_keyring,
						const std::string& path_to_secret_keyring, 
						const std::string& path_to_trust_database, 
						const std::string& pgp_lock_file) ;

		virtual ~PGPHandler() ;

        //=======================================================================================//
        //      Methods that needs to be derived depending on how PGP is implemented             //
        //=======================================================================================//

        // Removes the given keys from the keyring. Also backup the keyring to a file which name is automatically generated
        // and given pack for proper display.
        //
        virtual bool removeKeysFromPGPKeyring(const std::set<RsPgpId>& key_ids,std::string& backup_file,uint32_t& error_code) =0;
        virtual std::string makeRadixEncodedPGPKey(uint32_t key_index,bool include_signatures) =0;

        virtual bool availableGPGCertificatesWithPrivateKeys(std::list<RsPgpId>& ids)=0;
        virtual bool GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passwd, RsPgpId& pgpId, const int keynumbits, std::string& errString) =0;

        virtual std::string SaveCertificateToString(const RsPgpId& id,bool include_signatures) const=0;

        /** The caller is in charge of freeing `mem` once finished */
        virtual bool exportPublicKey( const RsPgpId& id, unsigned char*& mem_block, size_t& mem_size, bool armoured, bool include_signatures ) const =0;

        virtual bool exportGPGKeyPair(const std::string& filename,const RsPgpId& exported_key_id) const=0;
        virtual bool exportGPGKeyPairToString( std::string& data, const RsPgpId& exportedKeyId, bool includeSignatures, std::string& errorMsg ) const =0;

        // Gets info about the key. Who are the signers, what's the owner's name, etc.
        //
        virtual bool getGPGDetailsFromBinaryBlock(const unsigned char *mem,size_t mem_size,RsPgpId& key_id, std::string& name, std::list<RsPgpId>& signers) const =0;

        virtual bool importGPGKeyPair(const std::string& filename,RsPgpId& imported_id,std::string& import_error) =0;
        /**
		 * @param ids list of gpg certificate ids (note, not the actual certificates)
		 */

        virtual	bool importGPGKeyPairFromString(const std::string& data,RsPgpId& imported_id,std::string& import_error) =0;

        virtual bool LoadCertificateFromString(const std::string& pem, RsPgpId& gpg_id, std::string& error_string)=0;
        virtual bool LoadCertificateFromBinaryData(const unsigned char *bin_data,uint32_t bin_data_len, RsPgpId& gpg_id, std::string& error_string)=0;

        virtual bool encryptTextToFile(const RsPgpId& key_id,const std::string& text,const std::string& outfile) =0;
        virtual bool decryptTextFromFile(const RsPgpId& key_id,std::string& text,const std::string& encrypted_inputfile) =0;

        // The client should supply a memory chunk to store the data. The length will be updated to the real length of the data.
        //
        virtual bool encryptDataBin(const RsPgpId& key_id,const void *data, const uint32_t len , unsigned char *encrypted_data, unsigned int *encrypted_data_len) =0;
        virtual bool decryptDataBin(const RsPgpId& key_id,const void *encrypted_data, const uint32_t encrypted_len , unsigned char *data, unsigned int *data_len) =0;

        virtual bool SignDataBin(const RsPgpId& id, const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen, bool make_raw_signature=false, std::string reason = "") =0;
        virtual bool privateSignCertificate(const RsPgpId& own_id,const RsPgpId& id_of_key_to_sign) =0;
        virtual bool VerifySignBin(const void *data, uint32_t data_len, unsigned char *sign, unsigned int sign_len, const PGPFingerprintType& withfingerprint) =0;
        /**
         * @brief Get PGP fingerprint for the given key
         * @param id PGP 64bit key id
         * @param fp storage for the retrived key fingerpring, the contained value
         *	is meaningfull only if true is returned
         * @return true if the key was found, false if not
         */
        virtual bool getKeyFingerprint(const RsPgpId& id, RsPgpFingerprint& fp) const=0;

        virtual bool haveSecretKey(const RsPgpId& id) const =0;

        // Syncs the keyrings and trust database between memory and disk. The algorithm is:
        // 1 - lock the keyrings
        // 2 - compare file modification dates with last writing date
        // 		- if file is modified, load it, and merge with memory
        // 3 - look into memory modification flags
        // 		- if flag says keyring has changed, write to disk
        //
        virtual bool syncDatabase() =0;


        //=======================================================================================//
        //                              Common methods to PGPHandler                             //
        //=======================================================================================//

		bool getGPGFilteredList(std::list<RsPgpId>& list,bool (*filter)(const PGPCertificateInfo&) = NULL) const ;

        bool parseSignature(unsigned char *sign, unsigned int signlen,RsPgpId& issuer_id) ;

		void setAcceptConnexion(const RsPgpId&,bool) ;

		void updateOwnSignatureFlag(const RsPgpId& ownId) ;
		void updateOwnSignatureFlag(const RsPgpId& pgp_id,const RsPgpId& ownId) ;

		void locked_updateOwnSignatureFlag(PGPCertificateInfo&, const RsPgpId&, PGPCertificateInfo&, const RsPgpId&) ;

		//bool isKeySupported(const RsPgpId& id) const ;

		bool privateTrustCertificate(const RsPgpId& id,int valid_level) ;	

		// Write keyring

		//bool writeSecretKeyring() ;
		//bool writePublicKeyring() ;

		const PGPCertificateInfo *getCertificateInfo(const RsPgpId& id) const ;

		RS_DEPRECATED_FOR(isPgpPubKeyAvailable)
		bool isGPGId(const RsPgpId &id);
		bool isGPGSigned(const RsPgpId &id);
		bool isGPGAccepted(const RsPgpId &id);

		static void setPassphraseCallback(PassphraseCallback cb) ;
		static PassphraseCallback passphraseCallback() { return _passphrase_callback ; }

	/**
	 * @brief Check if a PGP publick key is available
	 * @param id id of the key to check
	 * @return true if the public key for the given id is available,
	 *	false otherwise
	 */
	bool isPgpPubKeyAvailable(const RsPgpId& id);

	/**
	 * @brief Convert PGP fingerprint to PGP 64bit id
	 * @param f PGP fingerprint to convert
	 * @return PGP 64bit id extracted from fingerprint
	 */
	static RsPgpId pgpIdFromFingerprint(const RsPgpFingerprint& f);

		// Debug stuff.
		virtual bool printKeys() const ;

    protected:
		void locked_readPrivateTrustDatabase() ;
		bool locked_writePrivateTrustDatabase() ;
        bool locked_syncTrustDatabase() ;

		// Members.
		//
		mutable RsMutex pgphandlerMtx ;

		std::map<RsPgpId,PGPCertificateInfo> _public_keyring_map ;	// used for fast access to keys. Gives the index in the keyring.
		std::map<RsPgpId,PGPCertificateInfo> _secret_keyring_map ;

		const std::string _pubring_path ;
		const std::string _secring_path ;
		const std::string _trustdb_path ;
		const std::string _pgp_lock_filename ;

		bool _pubring_changed ;
		mutable bool _trustdb_changed ;

		rstime_t _pubring_last_update_time ;
		rstime_t _secring_last_update_time ;
		rstime_t _trustdb_last_update_time ;

		static PassphraseCallback _passphrase_callback ;
};

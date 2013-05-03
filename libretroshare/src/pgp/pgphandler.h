#pragma once

// This class implements an abstract pgp handler to be used in RetroShare.
//
#include <stdint.h>
#include <string>
#include <list>
#include <map>
#include <set>
#include <util/rsthreads.h>
#include <util/rsid.h>

extern "C" {
#include <openpgpsdk/types.h>
#include <openpgpsdk/keyring.h>
#include <openpgpsdk/keyring_local.h>
}

static const int KEY_ID_SIZE          =  8 ;
static const int KEY_FINGERPRINT_SIZE = 20 ;

typedef std::string (*PassphraseCallback)(void *data, const char *uid_hint, const char *passphrase_info, int prev_was_bad) ;

typedef t_RsGenericIdType<KEY_ID_SIZE> 			 PGPIdType;
typedef t_RsGenericIdType<KEY_FINGERPRINT_SIZE>  PGPFingerprintType ;

class PGPCertificateInfo
{
	public:
		PGPCertificateInfo() {}

		std::string _name;
		std::string _email;
		std::string _comment;

		std::set<std::string> signers;

		uint32_t _trustLvl;
		uint32_t _validLvl;
		uint32_t _flags ;
		uint32_t _type ;

		PGPFingerprintType _fpr;           /* fingerprint */
		PGPIdType          _key_id ;

		uint32_t _key_index ;			// index to array of keys in the public keyring 

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

class PGPHandler
{
	public:
		PGPHandler(	const std::string& path_to_public_keyring, 
						const std::string& path_to_secret_keyring, 
						const std::string& path_to_trust_database, 
						const std::string& pgp_lock_file) ;

		virtual ~PGPHandler() ;

		/**
		 * @param ids list of gpg certificate ids (note, not the actual certificates)
		 */
		bool getGPGFilteredList(std::list<PGPIdType>& list,bool (*filter)(const PGPCertificateInfo&) = NULL) const ;
		bool haveSecretKey(const PGPIdType& id) const ;

		bool importGPGKeyPair(const std::string& filename,PGPIdType& imported_id,std::string& import_error) ;
		bool exportGPGKeyPair(const std::string& filename,const PGPIdType& exported_id) const ;

		bool availableGPGCertificatesWithPrivateKeys(std::list<PGPIdType>& ids);
		bool GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passwd, PGPIdType& pgpId, std::string& errString) ;

		bool LoadCertificateFromString(const std::string& pem, PGPIdType& gpg_id, std::string& error_string);

		std::string SaveCertificateToString(const PGPIdType& id,bool include_signatures) const ;
		bool exportPublicKey(const PGPIdType& id,unsigned char *& mem,size_t& mem_size,bool armoured,bool include_signatures) const ;

		bool SignDataBin(const PGPIdType& id,const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen,bool make_raw_signature=false) ;
		bool VerifySignBin(const void *data, uint32_t data_len, unsigned char *sign, unsigned int sign_len, const PGPFingerprintType& withfingerprint) ;
		bool privateSignCertificate(const PGPIdType& own_id,const PGPIdType& id_of_key_to_sign) ;

		// The client should supply a memory chunk to store the data. The length will be updated to the real length of the data.
		//
		bool encryptDataBin(const PGPIdType& key_id,const void *data, const uint32_t len, unsigned char *encrypted_data, unsigned int *encrypted_data_len) ;
		bool decryptDataBin(const PGPIdType& key_id,const void *data, const uint32_t len, unsigned char *decrypted_data, unsigned int *decrypted_data_len) ;

		bool encryptTextToFile(const PGPIdType& key_id,const std::string& text,const std::string& outfile) ;
		bool decryptTextFromFile(const PGPIdType& key_id,std::string& text,const std::string& encrypted_inputfile) ;
		//bool encryptTextToString(const PGPIdType& key_id,const std::string& text,std::string& outstring) ;
		//bool decryptTextFromString(const PGPIdType& key_id,const std::string& encrypted_text,std::string& outstring) ;

		bool getKeyFingerprint(const PGPIdType& id,PGPFingerprintType& fp) const ;
		void setAcceptConnexion(const PGPIdType&,bool) ;
		void updateOwnSignatureFlag(const PGPIdType& ownId) ;

		//bool isKeySupported(const PGPIdType& id) const ;

		bool privateTrustCertificate(const PGPIdType& id,int valid_level) ;	

		// Write keyring

		//bool writeSecretKeyring() ;
		//bool writePublicKeyring() ;

		const PGPCertificateInfo *getCertificateInfo(const PGPIdType& id) const ;

		bool isGPGId(const std::string &id);
		bool isGPGSigned(const std::string &id);
		bool isGPGAccepted(const std::string &id);

		static void setPassphraseCallback(PassphraseCallback cb) ;
		static PassphraseCallback passphraseCallback() { return _passphrase_callback ; }

		// Gets info about the key. Who are the signers, what's the owner's name, etc.
		//
		bool getGPGDetailsFromBinaryBlock(const unsigned char *mem,size_t mem_size,std::string& key_id, std::string& name, std::list<std::string>& signers) const ;

		// Debug stuff.
		virtual bool printKeys() const ;

		// Syncs the keyrings and trust database between memory and disk. The algorithm is:
		// 1 - lock the keyrings
		// 2 - compare file modification dates with last writing date
		// 		- if file is modified, load it, and merge with memory
		// 3 - look into memory modification flags
		// 		- if flag says keyring has changed, write to disk
		//
		bool syncDatabase() ;

	private:
		void initCertificateInfo(PGPCertificateInfo& cert,const ops_keydata_t *keydata,uint32_t i) ;

		// Returns true if the signatures have been updated
		//
		bool validateAndUpdateSignatures(PGPCertificateInfo& cert,const ops_keydata_t *keydata) ;

		const ops_keydata_t *getPublicKey(const PGPIdType&) const ;
		const ops_keydata_t *getSecretKey(const PGPIdType&) const ;

		void locked_readPrivateTrustDatabase() ;
		bool locked_writePrivateTrustDatabase() ;

		bool locked_syncPublicKeyring() ;
		bool locked_syncTrustDatabase() ;

		void mergeKeyringFromDisk(ops_keyring_t *keyring, std::map<std::string,PGPCertificateInfo>& kmap, const std::string& keyring_file) ;
		bool addOrMergeKey(ops_keyring_t *keyring,std::map<std::string,PGPCertificateInfo>& kmap,const ops_keydata_t *keydata) ;

		// Members.
		//
		mutable RsMutex pgphandlerMtx ;

		ops_keyring_t *_pubring ;
		ops_keyring_t *_secring ;

		std::map<std::string,PGPCertificateInfo> _public_keyring_map ;	// used for fast access to keys. Gives the index in the keyring.
		std::map<std::string,PGPCertificateInfo> _secret_keyring_map ;

		const std::string _pubring_path ;
		const std::string _secring_path ;
		const std::string _trustdb_path ;
		const std::string _pgp_lock_filename ;

		bool _pubring_changed ;
		bool _trustdb_changed ;

		time_t _pubring_last_update_time ;
		time_t _secring_last_update_time ;
		time_t _trustdb_last_update_time ;

		// Helper functions.
		//
		static std::string makeRadixEncodedPGPKey(const ops_keydata_t *key,bool include_signatures) ;
		static ops_keyring_t *allocateOPSKeyring() ;
		static void addNewKeyToOPSKeyring(ops_keyring_t*, const ops_keydata_t&) ;
		static PassphraseCallback _passphrase_callback ;
		static bool mergeKeySignatures(ops_keydata_t *dst,const ops_keydata_t *src) ;	// returns true if signature lists are different
};


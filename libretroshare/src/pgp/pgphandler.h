// This class implements an abstract pgp handler to be used in RetroShare.
//
#include <stdint.h>
#include <string>
#include <list>
#include <map>
#include <util/rsthreads.h>

extern "C" {
#include <openpgpsdk/types.h>
#include <openpgpsdk/keyring.h>
#include <openpgpsdk/keyring_local.h>
}

typedef std::string (*PassphraseCallback)(const std::string& display_msg) ;

class PGPIdType
{
	public:
		static const int KEY_ID_SIZE = 8 ;

		PGPIdType() {}
		PGPIdType(const std::string& hex_string) ;
		PGPIdType(const unsigned char bytes[]) ;

		std::string toStdString() const ;
		uint64_t toUInt64() const ;
		const unsigned char *toByteArray() const { return &bytes[0] ; }

	private:
		unsigned char bytes[KEY_ID_SIZE] ;
};

class PGPHandler
{
	public:
		PGPHandler(const std::string& path_to_public_keyring, const std::string& path_to_secret_keyring,PassphraseCallback cb) ;

		virtual ~PGPHandler() ;

		/**
		 * @param ids list of gpg certificate ids (note, not the actual certificates)
		 */

		bool availableGPGCertificatesWithPrivateKeys(std::list<PGPIdType>& ids);
		bool GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passwd, PGPIdType& pgpId, std::string& errString) ;

		bool LoadCertificateFromString(const std::string& pem, PGPIdType& gpg_id, std::string& error_string);
		std::string SaveCertificateToString(const PGPIdType& id,bool include_signatures) ;

		bool TrustCertificate(const PGPIdType& id, int trustlvl);

		bool SignDataBin(const PGPIdType& id,const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen) ;
		bool VerifySignBin(const void *data, uint32_t data_len, unsigned char *sign, unsigned int sign_len, const std::string &withfingerprint) ;

		// Debug stuff.
		virtual void printKeys() const ;

	private:
		static std::string makeRadixEncodedPGPKey(const ops_keydata_t *key) ;
		static ops_keyring_t *allocateOPSKeyring() ;
		static void addNewKeyToOPSKeyring(ops_keyring_t*, const ops_keydata_t&) ;

		const ops_keydata_t *getPublicKey(const PGPIdType&) const ;
		const ops_keydata_t *getSecretKey(const PGPIdType&) const ;

		RsMutex pgphandlerMtx ;

		ops_keyring_t *_pubring ;
		ops_keyring_t *_secring ;

		std::map<uint64_t,uint32_t> _public_keyring_map ;	// used for fast access to keys. Gives the index in the keyring.
		std::map<uint64_t,uint32_t> _secret_keyring_map ;

		const std::string _pubring_path ;
		const std::string _secring_path ;

		PassphraseCallback _passphrase_callback ;
};


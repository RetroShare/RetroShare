// This class implements an abstract pgp handler to be used in RetroShare.
//
#include <stdint.h>
#include <string>
#include <list>
#include <util/rsthreads.h>

extern "C" {
#include <openpgpsdk/types.h>
#include <openpgpsdk/keyring.h>
}

class PGPIdType
{
	public:
		static const int KEY_ID_SIZE = 8 ;

		PGPIdType(const std::string& hex_string) ;
		PGPIdType(const unsigned char bytes[]) ;

		std::string toStdString() const ;
		uint64_t toUInt64() const ;

	private:
		unsigned char bytes[KEY_ID_SIZE] ;
};

class PGPHandler
{
	public:
		PGPHandler(const std::string& path_to_public_keyring, const std::string& path_to_secret_keyring) ;

		virtual ~PGPHandler() ;

		/**
		 * @param ids list of gpg certificate ids (note, not the actual certificates)
		 */

		bool availableGPGCertificatesWithPrivateKeys(std::list<PGPIdType>& ids);
		bool GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passwd, PGPIdType& pgpId, std::string& errString) ;

		bool LoadCertificateFromString(const std::string& pem, PGPIdType& gpg_id, std::string& error_string);
		std::string SaveCertificateToString(const PGPIdType& id,bool include_signatures) ;

		bool TrustCertificate(const PGPIdType& id, int trustlvl);

		virtual bool SignDataBin(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen) { return false ; }
		virtual bool VerifySignBin(const void*, uint32_t, unsigned char*, unsigned int, const std::string &withfingerprint) { return false ; }

		// Debug stuff.
		virtual bool printKeys() { return false;}

	private:
		RsMutex pgphandlerMtx ;

		ops_keyring_t *_pubring ;
		ops_keyring_t *_secring ;

		const std::string _pubring_path ;
		const std::string _secring_path ;
};

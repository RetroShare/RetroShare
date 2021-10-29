/*******************************************************************************
 * libretroshare/src/pqi: authgpg.h                                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008-2009 by Raghu Dev R.                                         *
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

/****
 * Here's GPG policy :
 * By default, all pgpg keys imported via a RS user (make friend and accept friend action) are signed at level 0.
 * All signed keys by RS are set to be trusted marginally. You can change it to full or no trust in the friend profile
 * For a key to be marginaly valid, it has to be signed by one fully trusted key, or at least by 3 marginally trusted keys.
 * All keys that have at least marginal validity are designed as valid in RS. They are shown in the RS gui in order to be signed.
 * If there is no validity then the key is not shown.
 */

#ifndef RS_GPG_AUTH_HEADER
#define RS_GPG_AUTH_HEADER

#include "util/rsthreads.h"
#include "pqi/p3cfgmgr.h"
#include "pgp/openpgpsdkhandler.h"

#define MAX_GPG_SIGNATURE_SIZE  4096

struct RsPeerDetails;

/*!
 * gpgcert is the identifier for a person.
 * It is a wrapper class for a OpenPGP certificate.
 */

class AuthGPGOperation
{
public:
    explicit AuthGPGOperation(void *userdata)
    {
        m_userdata = userdata;
    }
    virtual ~AuthGPGOperation() {}

public:
    void *m_userdata;
};

class AuthGPGOperationLoadOrSave : public AuthGPGOperation
{
public:
    AuthGPGOperationLoadOrSave(bool load, const RsPgpId &gpgId, const std::string &gpgCert, void *userdata) 
    : AuthGPGOperation(userdata)
    {
        m_load = load;
        if (m_load) {
            m_certGpg = gpgCert;
	    m_certGpgId = gpgId;
        } else {
            m_certGpgId = gpgId;
        }
    }

public:
    bool m_load;
    RsPgpId m_certGpgId; // set for save & load.
	 std::string m_certGpg; // set for load
};

class AuthGPGService
{
public:
	AuthGPGService() {}
	~AuthGPGService() {}

    virtual AuthGPGOperation *getGPGOperation() = 0;
    virtual void setGPGOperation(AuthGPGOperation *operation) = 0;
};

class AuthPGP: public p3Config, public RsTickingThread
{
public:
    static void init(const std::string& path_to_pubring,
                     const std::string& path_to_secring,
                     const std::string& path_to_trustdb,
                     const std::string& pgp_lock_file);

    static void registerToConfigMgr(const std::string& fname,p3ConfigMgr *CfgMgr);
    static void exit();

    static bool isPGPId(const RsPgpId& id) ;
    static bool isPGPAccepted(const RsPgpId& id) ;

		/**
		 * @param ids list of gpg certificate ids (note, not the actual certificates)
		 */
		//virtual bool    availableGPGCertificatesWithPrivateKeys(std::list<RsPgpId> &ids);

		/*********************************************************************************/
		/************************* STAGE 1 ***********************************************/
		/*********************************************************************************/
		/*****
		 * STAGE 1: Initialisation.... As we are switching to OpenPGP the init functions
		 * will be different. Just move the initialisation functions over....
		 *
		 * As GPGMe requires external calls to the GPG executable, which could potentially
		 * be expensive, We'll want to cache the GPG keys in this class.
		 * This should be done at initialisation, and saved in a map.
		 * (see storage at the end of the class)
		 *
		 ****/
        static bool    active();

		// /* Initialize */
		// virtual bool    InitAuth ();
		// virtual bool    CloseAuth();

		/* Init by generating new Own PGP Cert, or selecting existing PGP Cert */

        static int  PgpInit(const RsPgpId &ownId);
        static bool GeneratePgpCertificate(const std::string& name, const std::string& email, const std::string& passwd, RsPgpId &pgpId, const int keynumbits, std::string &errString);

        static bool getPgpDetailsFromBinaryBlock(const unsigned char *mem,size_t mem_size,RsPgpId& key_id, std::string& name, std::list<RsPgpId>& signers) ;
        static int availablePgpCertificatesWithPrivateKeys(std::list<RsPgpId>& pgpIds);

        /*********************************************************************************/
		/************************* STAGE 3 ***********************************************/
		/*********************************************************************************/
		/*****
		 * STAGE 3: These are some of the most commonly used functions in Retroshare.
		 *
		 * More commonly used functions.
		 *
		 * provide access to details in cache list.
		 *
		 ****/
        static std::string getPgpName(const RsPgpId &pgp_id,bool *success = NULL);
        static std::string getPgpEmail(const RsPgpId &pgp_id,bool *success = NULL);

        static bool exportPublicKey( const RsPgpId& id, unsigned char*& mem_block, size_t& mem_size, bool armoured, bool include_signatures );

        /* PGP web of trust management */
        static const RsPgpId& getPgpOwnId();
        static std::string getPgpOwnName();

        //virtual std::string getGPGOwnEmail();
        static bool getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp) ;
        static bool isKeySupported(const RsPgpId &id) ;
        static bool isPgpPubKeyAvailable(const RsPgpId& pgp_id);
        static bool haveSecretKey(const RsPgpId &id) ;
        static bool getPgpDetails(const RsPgpId& id, RsPeerDetails &d);
        static bool getPgpAllList(std::list<RsPgpId> &ids);
        static bool getPgpValidList(std::list<RsPgpId> &ids);
        static bool getPgpAcceptedList(std::list<RsPgpId> &ids);
        static bool getPgpSignedList(std::list<RsPgpId> &ids);
        static bool importProfile(const std::string& filename,RsPgpId& gpg_id,std::string& import_error) ;
        static bool importProfileFromString(const std::string& data,RsPgpId& gpg_id,std::string& import_error) ;
        static bool exportProfile(const std::string& filename,const RsPgpId& gpg_id) ;
        static bool exportIdentityToString(
		        std::string& data, const RsPgpId& pgpId, bool includeSignatures,
		        std::string& errorMsg );

        static bool   removeKeysFromPGPKeyring(const std::set<RsPgpId> &pgp_ids,std::string& backup_file,uint32_t& error_code) ;

		/*********************************************************************************/
		/************************* STAGE 4 ***********************************************/
		/*********************************************************************************/
		/*****
		 * STAGE 4: Loading and Saving Certificates. (Strings and Files)
		 *
		 ****/
        static bool LoadCertificateFromString(const std::string &pem, RsPgpId& gpg_id,std::string& error_string);
        static bool LoadPGPKeyFromBinaryData(const unsigned char *data,uint32_t data_len, RsPgpId& gpg_id,std::string& error_string);
        static std::string SaveCertificateToString(const RsPgpId &id,bool include_signatures) ;

		// Cached certificates.
		//bool   getCachedGPGCertificate(const RsPgpId &id, std::string &certificate);

		/*********************************************************************************/
		/************************* STAGE 6 ***********************************************/
		/*********************************************************************************/
		/*****
		 * STAGE 6: Authentication, Trust and Signing.
		 *
		 * This is some of the harder functions, but they should have been 
		 * done in gpgroot already.
		 *
		 ****/
        static bool AllowConnection(const RsPgpId &gpg_id, bool accept);

        static bool SignCertificateLevel0(const RsPgpId &id);
        static bool RevokeCertificate(const RsPgpId &id);  /* Particularly hard - leave for later */

        static bool TrustCertificate(const RsPgpId& id,  int trustlvl); //trustlvl is 2 for none, 3 for marginal and 4 for full trust

		/*********************************************************************************/
		/************************* STAGE 7 ***********************************************/
		/*********************************************************************************/
		/*****
		 * STAGE 7: Signing Data.
		 *
		 * There should also be Encryption Functions... (do later).
		 *
		 ****/
        static bool SignDataBin(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen, std::string reason = "");
        static bool VerifySignBin(const void*, uint32_t, unsigned char*, unsigned int, const PGPFingerprintType& withfingerprint);
        static bool parseSignature(const void *sig, unsigned int siglen, RsPgpId& issuer_id);

        static bool encryptDataBin(const RsPgpId& pgp_id,const void *data, const uint32_t len, unsigned char *encr, unsigned int *encrlen);
        static bool decryptDataBin(const void *data, const uint32_t len, unsigned char *decr, unsigned int *decrlen);

        static bool decryptTextFromFile(      std::string& text,const std::string& filename);
        static bool encryptTextToFile  (const std::string& text,const std::string& filename);

//		virtual bool decryptTextFromString(      std::string& encrypted_text,std::string&     clear_string);
//		virtual bool encryptTextToString  (const std::string& pgp_id,const std::string&     clear_text,std::string& encrypted_string);

        static bool getGPGFilteredList(std::list<RsPgpId>& list,bool (*filter)(const PGPCertificateInfo&) = NULL) ;

		//END of PGP public functions

		/* GPG service */
        static bool addService(AuthGPGService *service) ;

		// This is for debug purpose only. Don't use it !!
        static void setAuthGPG_debug(AuthPGP *auth_gpg) { _instance = auth_gpg ; }

	protected:
        AuthPGP(const std::string& path_to_pubring, const std::string& path_to_secring,const std::string& path_to_trustdb,const std::string& pgp_lock_file);
        virtual ~AuthPGP();

		/*****************************************************************/
		/***********************  p3config  ******************************/
		/* Key Functions to be overloaded for Full Configuration */
        virtual RsSerialiser *setupSerialiser() override;
        virtual bool saveList(bool &cleanup, std::list<RsItem *>&) override;
        virtual bool    loadList(std::list<RsItem *>& load) override;
		/*****************************************************************/

private:
		// Gets the certificate pointer and returns NULL if the string is invalid, or the
		// cert was not found.
		//
		const PGPCertificateInfo *getCertInfoFromStdString(const std::string& ) const;

		/* SKTAN */
		//void showData(gpgme_data_t dh);
		//void createDummyFriends(void); //NYI

		/* Internal functions */
		bool DoOwnSignature(const void *, unsigned int, void *, unsigned int *, std::string reason);
		bool VerifySignature(const void *data, int datalen, const void *sig, unsigned int siglen, const PGPFingerprintType& withfingerprint);

		/* Sign/Trust stuff */
		int	privateSignCertificate(const RsPgpId &id);
		int	privateRevokeCertificate(const RsPgpId &id);		/* revoke the signature on Certificate */
		int	privateTrustCertificate(const RsPgpId& id, int trustlvl);

		// store all keys in map mKeyList to avoid calling gpgme exe repeatedly
		//bool    storeAllKeys();
		//bool    storeAllKeys_tick();

		// Not used anymore
		//        bool    updateTrustAllKeys_locked();

		/* GPG service */
		void    processServices();

		bool    printAllKeys_locked();
		bool    printOwnKeys_locked();

	void threadTick() override; /// @see RsTickingThread

private:
        static AuthPGP *instance();

		RsMutex gpgMtxService;
		RsMutex gpgMtxEngine;

		/* Below is protected via the mutex */

		// gpgme_engine_info_t INFO;
		// gpgme_ctx_t CTX;

		RsMutex gpgMtxData;
		/* Below is protected via the mutex */

		rstime_t mStoreKeyTime;

        PGPHandler *mPgpHandler;

		RsPgpId mOwnGpgId;
		bool gpgKeySelected;
        bool _force_sync_database ;
        uint32_t mCount ;

		std::list<AuthGPGService*> services ;

        static AuthPGP *_instance ;
};

#endif

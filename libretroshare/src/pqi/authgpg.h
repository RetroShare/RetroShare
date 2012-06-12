/*
 * libretroshare/src/   : gpgauthmgr.h
 *
 * GPG  interface for RetroShare.
 *
 * Copyright 2008-2009 by Raghu Dev R.
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
 * This is *THE* auth manager. It provides the web-of-trust via
 * gpgme, and authenticates the certificates that are managed
 * by the sublayer AuthSSL.
 *
 */

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

#include "util/rswin.h"
#include <gpgme.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include "util/rsthreads.h"
//#include "retroshare/rspeers.h"
#include <string>
#include <list>
#include <set>
#include <map>
#include "pqi/p3cfgmgr.h"
#include "pgp/pgphandler.h"

#define MAX_GPG_SIGNATURE_SIZE  4096

class RsPeerDetails;

/*!
 * gpgcert is the identifier for a person.
 * It is a wrapper class for a OpenPGP certificate.
 */

class AuthGPGOperation
{
public:
    AuthGPGOperation(void *userdata)
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
    AuthGPGOperationLoadOrSave(bool load, const std::string &gpgId, const std::string &gpgCert, void *userdata) 
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
    std::string m_certGpgId; // set for save & load.
    std::string m_certGpg; // set for load
};

class AuthGPGService
{
public:
    AuthGPGService() {};
    ~AuthGPGService() {};

    virtual AuthGPGOperation *getGPGOperation() = 0;
    virtual void setGPGOperation(AuthGPGOperation *operation) = 0;
};

class AuthGPG: public p3Config, public RsThread, public PGPHandler
{
	public:

		static void init(const std::string& path_to_pubring, const std::string& path_to_secring,const std::string& pgp_lock_file);
		static void exit();
		static AuthGPG *getAuthGPG() { return _instance ; }

		/**
		 * @param ids list of gpg certificate ids (note, not the actual certificates)
		 */
		virtual bool    availableGPGCertificatesWithPrivateKeys(std::list<std::string> &ids);

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
		virtual bool    active(); 

		// /* Initialize */
		// virtual bool    InitAuth ();
		// virtual bool    CloseAuth();

		/* Init by generating new Own PGP Cert, or selecting existing PGP Cert */

		virtual int  GPGInit(const std::string &ownId);
		virtual bool GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passwd, std::string &pgpId, std::string &errString);

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
		virtual std::string getGPGName(const std::string &pgp_id,bool *success = NULL);
		virtual std::string getGPGEmail(const std::string &pgp_id,bool *success = NULL);

		/* PGP web of trust management */
		virtual std::string getGPGOwnId();
		virtual std::string getGPGOwnName();

		//virtual std::string getGPGOwnEmail();
		virtual bool	getGPGDetails(const std::string &id, RsPeerDetails &d);
		virtual bool	getGPGAllList(std::list<std::string> &ids);
		virtual bool	getGPGValidList(std::list<std::string> &ids);
		virtual bool	getGPGAcceptedList(std::list<std::string> &ids);
		virtual bool	getGPGSignedList(std::list<std::string> &ids);

		/*********************************************************************************/
		/************************* STAGE 4 ***********************************************/
		/*********************************************************************************/
		/*****
		 * STAGE 4: Loading and Saving Certificates. (Strings and Files)
		 *
		 ****/
		virtual bool LoadCertificateFromString(const std::string &pem, std::string &gpg_id,std::string& error_string);
		virtual std::string SaveCertificateToString(const std::string &id,bool include_signatures) ;

		// Cached certificates.
		bool   getCachedGPGCertificate(const std::string &id, std::string &certificate);

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
		virtual bool AllowConnection(const std::string &gpg_id, bool accept);

		virtual bool SignCertificateLevel0(const std::string &id);
		virtual bool RevokeCertificate(const std::string &id);  /* Particularly hard - leave for later */

		virtual bool TrustCertificate(const std::string &id,  int trustlvl); //trustlvl is 2 for none, 3 for marginal and 4 for full trust

		/*********************************************************************************/
		/************************* STAGE 7 ***********************************************/
		/*********************************************************************************/
		/*****
		 * STAGE 7: Signing Data.
		 *
		 * There should also be Encryption Functions... (do later).
		 *
		 ****/
		virtual bool SignDataBin(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen);
		virtual bool VerifySignBin(const void*, uint32_t, unsigned char*, unsigned int, const std::string &withfingerprint);

		virtual bool decryptTextFromFile(      std::string& text,const std::string& filename);
		virtual bool encryptTextToFile  (const std::string& text,const std::string& filename);

		bool getGPGFilteredList(std::list<std::string>& list,bool (*filter)(const PGPCertificateInfo&) = NULL) ;

		//END of PGP public functions

		/* GPG service */
		virtual bool addService(AuthGPGService *service) ;

	protected:
		AuthGPG(const std::string& path_to_pubring, const std::string& path_to_secring,const std::string& pgp_lock_file);
		virtual ~AuthGPG();

		/*****************************************************************/
		/***********************  p3config  ******************************/
		/* Key Functions to be overloaded for Full Configuration */
		virtual RsSerialiser *setupSerialiser();
		virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
		virtual bool    loadList(std::list<RsItem *>& load);
		/*****************************************************************/

	private:

		/* SKTAN */
		//void showData(gpgme_data_t dh);
		//void createDummyFriends(void); //NYI

		/* Internal functions */
		bool DoOwnSignature(const void *, unsigned int, void *, unsigned int *);
		bool VerifySignature(const void *data, int datalen, const void *sig, unsigned int siglen, const std::string &withfingerprint);

		/* Sign/Trust stuff */
		int	privateSignCertificate(const std::string &id);
		int	privateRevokeCertificate(const std::string &id);		/* revoke the signature on Certificate */
		int	privateTrustCertificate(const std::string &id, int trustlvl);

		// store all keys in map mKeyList to avoid calling gpgme exe repeatedly
		//bool    storeAllKeys();
		//bool    storeAllKeys_tick();

		// Not used anymore
		//        bool    updateTrustAllKeys_locked();

		/* GPG service */
		void    processServices();

		bool    printAllKeys_locked();
		bool    printOwnKeys_locked();

		/* own thread */
		virtual void run();

	private:

		static AuthGPG *instance_gpg; // pointeur vers le singleton

		RsMutex gpgMtxService;
		RsMutex gpgMtxEngine;

		/* Below is protected via the mutex */

		// gpgme_engine_info_t INFO;
		// gpgme_ctx_t CTX;

		RsMutex gpgMtxData;
		/* Below is protected via the mutex */

		time_t mStoreKeyTime;

		PGPIdType mOwnGpgId;
		bool gpgKeySelected;

		std::list<AuthGPGService*> services ;

		static AuthGPG *_instance ;
};

 /*!
  *  Sign a key
  **/
 typedef enum
 {
   SIGN_START,
   SIGN_COMMAND,
   SIGN_UIDS,
   SIGN_SET_EXPIRE,
   SIGN_SET_CHECK_LEVEL,
   SIGN_ENTER_PASSPHRASE,
   SIGN_CONFIRM,
   SIGN_QUIT,
   SIGN_SAVE,
   SIGN_ERROR
 } SignState;
 
 
 /*!
  *  Change the key ownertrust
  **/
 typedef enum
 {
   TRUST_START,
   TRUST_COMMAND,
   TRUST_VALUE,
   TRUST_REALLY_ULTIMATE,
   TRUST_QUIT,
   TRUST_SAVE,
   TRUST_ERROR
 } TrustState;
 
 
 
 /*!
  * This is the generic data object passed to the
  * callback function in a gpgme_op_edit operation. 
  * The contents of this object are modified during 
  * each callback, to keep track of states, errors 
  * and other data.
  */
 class EditParams
 {
 	public: 
 	int state;
 
 	/*!
 	 *  The return code of gpgme_op_edit() is the return value of
 	 * the last invocation of the callback. But returning an error 
 	 * from the callback does not abort the edit operation, so we 
 	 * must remember any error.
 	 */		
 	gpg_error_t err;
 	
 	/// Parameters specific to the key operation
 	void *oParams;
 	
 	EditParams(int state, void *oParams) {
 		this->state = state;
 		this->err = gpgme_error(GPG_ERR_NO_ERROR);
 		this->oParams = oParams;
 	}
 			
 };
 
 /*!
  *  Data specific to key signing
  **/
 class SignParams 
 {
 	public:
 	
         std::string checkLvl;
 
         SignParams(std::string checkLvl) {
                 this->checkLvl = checkLvl;
 	}
 };
 
 /*!
  * Data specific to key signing
  **/
 class TrustParams
 {
         public:
 
         std::string trustLvl;
 
         TrustParams(std::string trustLvl) {
                 this->trustLvl = trustLvl;
         }
 };
 
#endif


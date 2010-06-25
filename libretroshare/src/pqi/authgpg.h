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

#include <gpgme.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include "util/rsthreads.h"
#include "rsiface/rspeers.h"
#include <string>
#include <list>
#include <set>
#include <map>
#include "pqi/p3cfgmgr.h"

#define GPG_id std::string

const time_t STORE_KEY_TIMEOUT = 60; //store key is call around every 60sec

/*!
 * gpgcert is the identifier for a person.
 * It is a wrapper class for a GPGme OpenPGP certificate.
 */
class gpgcert
{
	public:
		gpgcert();
		~gpgcert();

                std::string id;
                std::string name;
                std::string email;

                std::string fpr; /* fingerprint */
                std::list<std::string> signers;

                uint32_t trustLvl;
                uint32_t validLvl;

                bool ownsign;

                //This is not gpg, but RS data. A gpg peer can be accepted for connecting but not signed.
                bool accept_connection;

		gpgme_key_t key;
};

/*!
 * The certificate map type
 */
typedef std::map<std::string, gpgcert> certmap;
	
//! provides basic gpg functionality
/*!
 *
 * This provides retroshare basic gpg functionality and
 * key/web-of-trust management, also handle cert intialisation for retroshare
 */
class AuthGPG : public p3Config
{
	private:

	/* Internal functions */
        bool 	DoOwnSignature(const void *, unsigned int, void *, unsigned int *);
        bool    VerifySignature(const void *data, int datalen, const void *sig, unsigned int siglen, std::string withfingerprint);

        /* Sign/Trust stuff */
        int	privateSignCertificate(GPG_id id);
        int	privateRevokeCertificate(GPG_id id);		/* revoke the signature on Certificate */
        int	privateTrustCertificate(GPG_id id, int trustlvl);

        // store all keys in map mKeyList to avoid calling gpgme exe repeatedly
  	bool    storeAllKeys();
        bool    storeAllKeys_tick();

// Not used anymore
//        bool    updateTrustAllKeys_locked();

  	bool    printAllKeys_locked();
  	bool    printOwnKeys_locked();

	public:

        AuthGPG();
        ~AuthGPG();

        /**
         * @param ids list of gpg certificate ids (note, not the actual certificates)
         */
        bool    availableGPGCertificatesWithPrivateKeys(std::list<std::string> &ids);

	/* SKTAN */
	void showData(gpgme_data_t dh);
	void createDummyFriends(void); //NYI

  	bool    printKeys();

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
  bool    active(); 

  /* Initialize */
  bool    InitAuth ();

        /* Init by generating new Own PGP Cert, or selecting existing PGP Cert */
  int     GPGInit(std::string ownId);
  bool    CloseAuth();
  bool    GeneratePGPCertificate(std::string name, std::string email, std::string passwd, std::string &pgpId, std::string &errString);
  
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
    std::string getGPGName(GPG_id pgp_id);
    std::string getGPGEmail(GPG_id pgp_id);

    /* PGP web of trust management */
    std::string getGPGOwnId();
    std::string getGPGOwnName();
    std::string getGPGOwnEmail();
    bool	getGPGDetails(std::string id, RsPeerDetails &d);
    bool	getGPGAllList(std::list<std::string> &ids);
    bool	getGPGValidList(std::list<std::string> &ids);
    bool	getGPGAcceptedList(std::list<std::string> &ids);
    bool	getGPGSignedList(std::list<std::string> &ids);
    bool	isGPGValid(std::string id);
    bool	isGPGSigned(std::string id);
    bool	isGPGAccepted(std::string id);
    bool        isGPGId(GPG_id id);

/*********************************************************************************/
/************************* STAGE 4 ***********************************************/
/*********************************************************************************/
/*****
 * STAGE 4: Loading and Saving Certificates. (Strings and Files)
 *
 ****/
  bool LoadCertificateFromString(std::string pem, std::string &gpg_id);
  std::string SaveCertificateToString(std::string id);

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
  bool setAcceptToConnectGPGCertificate(std::string gpg_id, bool acceptance); //don't act on the gpg key, use a seperate set
  bool SignCertificateLevel0(std::string id);
  bool RevokeCertificate(std::string id);  /* Particularly hard - leave for later */
  bool TrustCertificateNone(std::string id);
  bool TrustCertificateMarginally(std::string id);
  bool TrustCertificateFully(std::string id);
  bool TrustCertificate(std::string id,  int trustlvl); //trustlvl is 2 for none, 3 for marginal and 4 for full trust

/*********************************************************************************/
/************************* STAGE 7 ***********************************************/
/*********************************************************************************/
/*****
 * STAGE 7: Signing Data.
 *
 * There should also be Encryption Functions... (do later).
 *
 ****/
  bool SignData(std::string input, std::string &sign);
  bool SignData(const void *data, const uint32_t len, std::string &sign);
  bool SignDataBin(std::string input, unsigned char *sign, unsigned int *signlen);
  bool SignDataBin(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen);
  bool VerifySignBin(const void*, uint32_t, unsigned char*, unsigned int, std::string withfingerprint);
  bool decryptText(gpgme_data_t CIPHER, gpgme_data_t PLAIN);
  bool encryptText(gpgme_data_t PLAIN, gpgme_data_t CIPHER);
//END of PGP public functions

  static AuthGPG *getAuthGPG() throw() // pour obtenir l'instance
      { return instance_gpg; }

        protected:
/*****************************************************************/
/***********************  p3config  ******************************/
        /* Key Functions to be overloaded for Full Configuration */
        virtual RsSerialiser *setupSerialiser();
        virtual std::list<RsItem *> saveList(bool &cleanup);
        virtual bool    loadList(std::list<RsItem *> load);
/*****************************************************************/

private:

    static AuthGPG *instance_gpg; // pointeur vers le singleton

    RsMutex gpgMtx;
    /* Below is protected via the mutex */

    certmap mKeyList;
    time_t mStoreKeyTime;

    bool gpgmeInit;

    bool gpgmeKeySelected;
    
    gpgme_engine_info_t INFO;
    gpgme_ctx_t CTX;

    std::string mOwnGpgId;
    gpgcert mOwnGpgCert;

    std::map<std::string, bool> mAcceptToConnectMap;

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

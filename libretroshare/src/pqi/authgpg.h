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

#ifndef RS_GPG_AUTH_HEADER
#define RS_GPG_AUTH_HEADER

#include <gpgme.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include "util/rsthreads.h"
#include <string>
#include <list>
#include <map>

#define GPG_id std::string

/* gpgcert is the identifier for a person.
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
//                std::string location;
//                std::string org;
//
//                std::string issuer;

                std::string fpr; /* fingerprint */
                std::list<std::string> signers;

                uint32_t trustLvl;
                uint32_t validLvl;

                bool ownsign;
                bool trusted; // means valid in pgp world.

		gpgme_key_t key;
};

/*
 * The certificate map type
 */
typedef std::map<std::string, gpgcert> certmap;
	
class AuthGPG
{
	private:

	/* Internal functions */
	bool 	setPGPPassword_locked(std::string pwd);
        bool 	DoOwnSignature_locked(const void *, unsigned int, void *, unsigned int *);
        bool    VerifySignature_locked(const void *data, int datalen, const void *sig, unsigned int siglen);

        /* Sign/Trust stuff */
        int	privateSignCertificate(GPG_id id);
        int	privateRevokeCertificate(GPG_id id);		/* revoke the signature on Certificate */
        int	privateTrustCertificate(GPG_id id, int trustlvl);

        // store all keys in map mKeyList to avoid calling gpgme exe repeatedly
  	bool    storeAllKeys_locked();
  	bool    updateTrustAllKeys_locked();

  	bool    printAllKeys_locked();
  	bool    printOwnKeys_locked();

	public:

        AuthGPG();
        ~AuthGPG();

        static AuthGPG *getAuthGPG();

        bool    availablePGPCertificates(std::list<std::string> &ids);

        //get the pgpg engine used by the pgp functions
        bool    getPGPEngineFileName(std::string &fileName);

	int	GPGInit(std::string ownId);
	int	GPGInit(std::string name, std::string comment, 
			std::string email, std::string passwd); /* create it */

	int	LoadGPGPassword(std::string pwd);

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

		/* initialisation -> done by derived classes */
  bool    active(); 

        /* Init by generating new Own PGP Cert, or selecting existing PGP Cert */
  int     InitAuth();
  bool    CloseAuth();

  
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

    /* get Details from the Certificates */
    std::string getPGPName(GPG_id pgp_id);
    std::string getPGPEmail(GPG_id pgp_id);


    /* PGP versions of Certificate Fns */
    GPG_id PGPOwnId();
    bool	getPGPAllList(std::list<std::string> &ids);
    bool	getPGPAuthenticatedList(std::list<std::string> &ids);
    bool	getPGPUnknownList(std::list<std::string> &ids);
    bool	isPGPValid(std::string id);
    bool	isPGPAuthenticated(std::string id);

/*********************************************************************************/
/************************* STAGE 4 ***********************************************/
/*********************************************************************************/
/*****
 * STAGE 4: Loading and Saving Certificates. (Strings and Files)
 *
 ****/


		/* Load/Save certificates */
  bool LoadCertificateFromString(std::string pem);
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

		/* Signatures */
  bool AuthCertificate(std::string uid);
  bool SignCertificate(std::string id);
  bool RevokeCertificate(std::string id);  /* Particularly hard - leave for later */
  bool TrustCertificate(std::string id, bool trust);

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
  bool VerifySignBin(const void*, uint32_t, unsigned char*, unsigned int);
  bool decryptText(gpgme_data_t CIPHER, gpgme_data_t PLAIN);
  bool encryptText(gpgme_data_t PLAIN, gpgme_data_t CIPHER);


/*********************************************************************************/
/************************* PGP Specific functions ********************************/
/*********************************************************************************/


bool checkSignature(std::string id, std::string hash, std::string signature);

        private:

        RsMutex pgpMtx;
	/* Below is protected via the mutex */

	certmap mKeyList;

	bool gpgmeInit;
	bool gpgmeKeySelected;
	bool gpgmeX509Selected;

	gpgme_engine_info_t INFO;
	gpgme_ctx_t CTX;

	std::string mOwnId;
	std::string mX509id;

	gpgcert mOwnGpgCert;

	std::string passphrase;
};

// the single instance of this
static AuthGPG instance_gpgroot;

/* Sign a key */

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


/* Change the key ownertrust */

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



/* This is the generic data object passed to the 
 * callback function in a gpgme_op_edit operation. 
 * The contents of this object are modified during 
 * each callback, to keep track of states, errors 
 * and other data.
 */

class EditParams
{
	public: 
	int state;
	/* The return code of gpgme_op_edit() is the return value of
	 * the last invocation of the callback. But returning an error 
	 * from the callback does not abort the edit operation, so we 
	 * must remember any error.
	 */		
	gpg_error_t err;
	
	/* Parameters specific to the key operation */	
	void *oParams;
	
	EditParams(int state, void *oParams) {
		this->state = state;
		this->err = gpgme_error(GPG_ERR_NO_ERROR);
		this->oParams = oParams;
	}
			
};

/* Data specific to key signing */

class SignParams 
{
	public:
	
	std::string checkLvl; 
	std::string passphrase;
		
	SignParams(std::string checkLvl, std::string passphrase) {
		this->checkLvl = checkLvl;
		this->passphrase = passphrase;
	}
};


#endif

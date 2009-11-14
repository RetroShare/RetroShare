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

//#include "p3authmgr.h"
#include "authssl.h"
#include <gpgme.h>

/* gpgcert is the identifier for a person.
 * It is a wrapper class for a GPGme OpenPGP certificate.
 */

class gpgcert
{
	public:
		gpgcert();
		~gpgcert();

		pqiAuthDetails user;
		gpgme_key_t key;
};

/*
 * The certificate map type
 */
typedef std::map<std::string, gpgcert> certmap;
	
class GPGAuthMgr: public AuthSSL
{
	private:

	/* Internal functions */
	bool 	setPGPPassword_locked(std::string pwd);
	bool 	DoOwnSignature_locked(void *, unsigned int, void *, unsigned int *);
	bool    VerifySignature_locked(std::string id, void *data, int datalen, 
							void *sig, unsigned int siglen);

	// store all keys in map mKeyList to avoid calling gpgme exe repeatedly
  	bool    storeAllKeys_locked();
  	bool    updateTrustAllKeys_locked();

  	bool    printAllKeys_locked();
  	bool    printOwnKeys_locked();

	public:

	GPGAuthMgr();
	~GPGAuthMgr();


	X509* 	SignX509Req(X509_REQ *req, long days, std::string);
	bool 	AuthX509(X509 *x509);


	bool    availablePGPCertificates(std::list<std::string> &ids);

        //get the pgpg engine used by the pgp functions
        bool    getPGPEngineFileName(std::string &fileName);

	int	GPGInit(std::string ownId);
	int	GPGInit(std::string name, std::string comment, 
			std::string email, std::string passwd); /* create it */

	int	LoadGPGPassword(std::string pwd);

	/* Sign/Trust stuff */
	int	signCertificate(std::string id);
	int	revokeCertificate(std::string id);		/* revoke the signature on Certificate */
	int	trustCertificate(std::string id, int trustlvl);

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

	/* Init by generating new Own PGP Cert, or selecting existing PGP Cert
	 */

	/* Arguments passed on to AuthSSL */
  int     InitAuth(const char *srvr_cert, const char *priv_key, 
                                        const char *passwd);
  bool    CloseAuth();
 // int     setConfigDirectories(std::string confFile, std::string neighDir);

  

/*********************************************************************************/
/************************* STAGE 2 ***********************************************/
/*********************************************************************************/
/*****
 * STAGE 2: These are some of the most commonly used functions in Retroshare.
 *
 * provide access to the cache list that was created in stage 1.
 * 
 ****/

		/* get Certificate Ids */

  std::string OwnId();
  bool	getAllList(std::list<std::string> &ids);
  bool	getAuthenticatedList(std::list<std::string> &ids);
  bool	getUnknownList(std::list<std::string> &ids);

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

 	bool	isValid(std::string id);
 	bool	isAuthenticated(std::string id);
 	std::string getPGPName(std::string pgp_id);
 	bool	getDetails(std::string id, pqiAuthDetails &details);

	virtual bool isTrustingMe(std::string);
	virtual void addTrustingPeer(std::string);


	/* PGP versions of Certificate Fns */

	std::string PGPOwnId();
  	bool	getPGPAllList(std::list<std::string> &ids);
  	bool	getPGPAuthenticatedList(std::list<std::string> &ids);
  	bool	getPGPUnknownList(std::list<std::string> &ids);
 	bool	isPGPValid(std::string id);
 	bool	isPGPAuthenticated(std::string id);
 	bool	getPGPDetails(std::string id, pqiAuthDetails &details);
	bool 	decryptText(gpgme_data_t CIPHER, gpgme_data_t PLAIN);
	bool	encryptText(gpgme_data_t PLAIN, gpgme_data_t CIPHER);

/*********************************************************************************/
/************************* STAGE 4 ***********************************************/
/*********************************************************************************/
/*****
 * STAGE 4: Loading and Saving Certificates. (Strings and Files)
 *
 ****/


		/* Load/Save certificates */
  bool LoadCertificateFromString(std::string pem, std::string &id);
  std::string SaveCertificateToString(std::string id);
  bool LoadCertificateFromFile(std::string filename, std::string &id);
  bool SaveCertificateToFile(std::string id, std::string filename);

/*********************************************************************************/
/************************* STAGE 5 ***********************************************/
/*********************************************************************************/
/*****
 * STAGE 5: Loading and Saving Certificates (Binary)
 *
 * The existing function arguments are based on OpenSSL functions.
 * Feel free to change this format if required.
 *
 ****/


  bool LoadCertificateFromBinary(const uint8_t *ptr, uint32_t len, std::string &id);
  bool SaveCertificateToBinary(std::string id, uint8_t **ptr, uint32_t *len);

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

#if 0
virtual  bool SignData(std::string input, std::string &sign);
virtual  bool SignData(const void *data, const uint32_t len, std::string &sign);
virtual  bool SignDataBin(std::string input, unsigned char *sign, unsigned int *signlen);
virtual bool  SignDataBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int *signlen);
virtual bool VerifySignBin(std::string, const void*, uint32_t, unsigned char*, unsigned int);

#endif


/*********************************************************************************/
/************************* PGP Specific functions ********************************/
/*********************************************************************************/

/*
 * These support the authentication process.
 *
 */

        /************* Virtual Functions from AuthSSL *************/
virtual bool 	ValidateCertificate(X509 *x509, std::string &peerId);
virtual int     VerifyX509Callback(int preverify_ok, X509_STORE_CTX *ctx);
        /************* Virtual Functions from AuthSSL *************/

/*
 *
 */

bool checkSignature(std::string id, std::string hash, std::string signature);




/*********************************************************************************/
/************************* OTHER FUNCTIONS ***************************************/
/*********************************************************************************/

		/* High Level Load/Save Configuration */
/*****
 * These functions call straight through to AuthSSL.
 * We don't need these functions here - as GPG stores the keys for us.
  bool FinalSaveCertificates();
  bool CheckSaveCertificates();
  bool saveCertificates();
  bool loadCertificates();
 ****/

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

/*****
 *
 * Support Functions for OpenSSL verification.
 *
 */

//int verify_pgp_callback(int preverify_ok, X509_STORE_CTX *ctx);


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

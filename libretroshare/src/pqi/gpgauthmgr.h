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
 */

#ifndef RS_GPG_AUTH_HEADER
#define RS_GPG_AUTH_HEADER

#include "p3authmgr.h"
#include <gpgme.h>

class GPGAuthMgr: public p3AuthMgr
{
	public:

	GPGAuthMgr();
	~GPGAuthMgr();

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
  int     InitAuth(const char *srvr_cert, const char *priv_key, 
                                        const char *passwd);
  bool    CloseAuth();
  int     setConfigDirectories(std::string confFile, std::string neighDir);

// store all keys in map mKeyList to avoid calling gpgme exe repeatedly
  bool    storeAllKeys();

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
 	std::string getName(std::string id);
 	bool	getDetails(std::string id, pqiAuthDetails &details);


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

virtual  bool SignData(std::string input, std::string &sign);
virtual  bool SignData(const void *data, const uint32_t len, std::string &sign);
virtual  bool SignDataBin(std::string input, unsigned char *sign, unsigned int *signlen);
virtual bool  SignDataBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int *signlen);

/*********************************************************************************/
/************************* PGP Specific functions ********************************/
/*********************************************************************************/

/*
 * These support the authentication process.
 *
 */

/*
 *
 */

bool checkSignature(std::string id, std::string hash, std::string signature);




/*********************************************************************************/
/************************* OTHER FUNCTIONS ***************************************/
/*********************************************************************************/
/*****
 * We don't need these functions - as GPG stores the keys for us.
 ****/

		/* High Level Load/Save Configuration */
  bool FinalSaveCertificates();
  bool CheckSaveCertificates();
  bool saveCertificates();
  bool loadCertificates();

	private:

	/* Example Storage - Change as needed */

	std::string mOwnId;
	std::map<std::string, pqiAuthDetails> mKeyList;

	bool gpgmeInit;
	gpgme_engine_info_t INFO;
	gpgme_ctx_t CTX;
};

/*****
 *
 * Support Functions for OpenSSL verification.
 *
 */

int verify_pgp_callback(int preverify_ok, X509_STORE_CTX *ctx);


#endif





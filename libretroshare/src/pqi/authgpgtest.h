/*
 * libretroshare/src/   : authgpgtest.h
 *
 * GPG  interface for RetroShare.
 *
 * Copyright 2009-2010 by Robert Fernie.
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

#ifndef RS_GPG_AUTH_TEST_HEADER
#define RS_GPG_AUTH_TEST_HEADER

#include "pqi/authgpg.h"

/* override the default AuthGPG */
void setAuthGPG(AuthGPG *newgpg);

class AuthGPGtest: public AuthGPG
{

	public:
        AuthGPGtest();

        /**
         * @param ids list of gpg certificate ids (note, not the actual certificates)
         */
virtual bool    availableGPGCertificatesWithPrivateKeys(std::list<std::string> &ids);
virtual	bool    printKeys();

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

  /* Initialize */
virtual bool    InitAuth ();

        /* Init by generating new Own PGP Cert, or selecting existing PGP Cert */
virtual int     GPGInit(std::string ownId);
virtual bool    CloseAuth();
virtual bool    GeneratePGPCertificate(std::string name, std::string email, std::string passwd, std::string &pgpId, std::string &errString);
  
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
virtual std::string getGPGName(GPG_id pgp_id);
virtual std::string getGPGEmail(GPG_id pgp_id);

    /* PGP web of trust management */
virtual std::string getGPGOwnId();
virtual std::string getGPGOwnName();
//virtual std::string getGPGOwnEmail();
virtual bool	getGPGDetails(std::string id, RsPeerDetails &d);
virtual bool	getGPGAllList(std::list<std::string> &ids);
virtual bool	getGPGValidList(std::list<std::string> &ids);
virtual bool	getGPGAcceptedList(std::list<std::string> &ids);
virtual bool	getGPGSignedList(std::list<std::string> &ids);
virtual bool	isGPGValid(std::string id);
virtual bool	isGPGSigned(std::string id);
virtual bool	isGPGAccepted(std::string id);
virtual bool        isGPGId(GPG_id id);

/*********************************************************************************/
/************************* STAGE 4 ***********************************************/
/*********************************************************************************/
/*****
 * STAGE 4: Loading and Saving Certificates. (Strings and Files)
 *
 ****/
virtual bool LoadCertificateFromString(std::string pem, std::string &gpg_id);
virtual std::string SaveCertificateToString(std::string id);

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
virtual bool setAcceptToConnectGPGCertificate(std::string gpg_id, bool acceptance); //don't act on the gpg key, use a seperate set
virtual bool SignCertificateLevel0(std::string id);
virtual bool RevokeCertificate(std::string id);  /* Particularly hard - leave for later */
//virtual bool TrustCertificateNone(std::string id);
//virtual bool TrustCertificateMarginally(std::string id);
//virtual bool TrustCertificateFully(std::string id);
virtual bool TrustCertificate(std::string id,  int trustlvl); //trustlvl is 2 for none, 3 for marginal and 4 for full trust

/*********************************************************************************/
/************************* STAGE 7 ***********************************************/
/*********************************************************************************/
/*****
 * STAGE 7: Signing Data.
 *
 * There should also be Encryption Functions... (do later).
 *
 ****/
//virtual bool SignData(std::string input, std::string &sign);
//virtual bool SignData(const void *data, const uint32_t len, std::string &sign);
//virtual bool SignDataBin(std::string input, unsigned char *sign, unsigned int *signlen);
virtual bool SignDataBin(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen);
virtual bool VerifySignBin(const void*, uint32_t, unsigned char*, unsigned int, std::string withfingerprint);
virtual bool decryptText(gpgme_data_t CIPHER, gpgme_data_t PLAIN);
virtual bool encryptText(gpgme_data_t PLAIN, gpgme_data_t CIPHER);
//END of PGP public functions

	private:

	std::string mOwnGPGId;
};


#endif

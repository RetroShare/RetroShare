/*
 * libretroshare/src/   : authgpgtest.cc
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

#include "pqi/authgpgtest.h"

AuthGPGtest::AuthGPGtest()
{
	mOwnGPGId = "TEST_DUMMY_OWN_GPG_ID";
	return;
}

        /**
         * @param ids list of gpg certificate ids (note, not the actual certificates)
         */
bool AuthGPGtest::availableGPGCertificatesWithPrivateKeys(std::list<std::string> &ids)
{
	std::cerr << "AuthGPGtest::availableGPGCertificatesWithPrivateKeys()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::printKeys()
{
	std::cerr << "AuthGPGtest::printKeys()";
	std::cerr << std::endl;
	return true;
}


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
bool AuthGPGtest::active() 
{
	std::cerr << "AuthGPGtest::active()";
	std::cerr << std::endl;
	return true;
}


  /* Initialize */
bool AuthGPGtest::InitAuth()
{
	std::cerr << "AuthGPGtest::InitAuth()";
	std::cerr << std::endl;
	return true;
}


        /* Init by generating new Own PGP Cert, or selecting existing PGP Cert */
int AuthGPGtest::GPGInit(std::string ownId)
{
	std::cerr << "AuthGPGtest::GPGInit(): new OwnId: " << ownId;
	std::cerr << std::endl;
	mOwnGPGId = ownId;
	return true;
}

bool AuthGPGtest::CloseAuth()
{
	std::cerr << "AuthGPGtest::CloseAuth()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::GeneratePGPCertificate(std::string name, std::string email, std::string passwd, std::string &pgpId, std::string &errString)
{
	std::cerr << "AuthGPGtest::GeneratePGPCertificate()";
	std::cerr << std::endl;
	return true;
}

  
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
std::string AuthGPGtest::getGPGName(GPG_id pgp_id)
{
	std::cerr << "AuthGPGtest::getGPGName()";
	std::cerr << std::endl;
	return "DUMMY_NAME";
}

std::string AuthGPGtest::getGPGEmail(GPG_id pgp_id)
{
	std::cerr << "AuthGPGtest::getGPGEmail()";
	std::cerr << std::endl;
	return "DUMMY_EMAIL";
}


    /* PGP web of trust management */
std::string AuthGPGtest::getGPGOwnId()
{
	std::cerr << "AuthGPGtest::getGPGOwnId()";
	std::cerr << std::endl;
	return mOwnGPGId;
}

std::string AuthGPGtest::getGPGOwnName()
{
	std::cerr << "AuthGPGtest::getGPGOwnName()";
	std::cerr << std::endl;
	return "DUMMY_OWN_NAME";
}

#if 0
std::string AuthGPGtest::getGPGOwnEmail()
{
	std::cerr << "AuthGPGtest::getGPGOwnEmail()";
	std::cerr << std::endl;
	return "DUMMY_OWN_EMAIL";
}
#endif

bool AuthGPGtest::getGPGDetails(std::string id, RsPeerDetails &d)
{
	std::cerr << "AuthGPGtest::getGPGDetails()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::getGPGAllList(std::list<std::string> &ids)
{
	std::cerr << "AuthGPGtest::getGPGAllList()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::getGPGValidList(std::list<std::string> &ids)
{
	std::cerr << "AuthGPGtest::getGPGValidList()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::getGPGAcceptedList(std::list<std::string> &ids)
{
	std::cerr << "AuthGPGtest::getGPGAcceptedList()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::getGPGSignedList(std::list<std::string> &ids)
{
	std::cerr << "AuthGPGtest::getGPGSignedList()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::isGPGValid(std::string id)
{
	std::cerr << "AuthGPGtest::isGPGValid()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::isGPGSigned(std::string id)
{
	std::cerr << "AuthGPGtest::isGPGSigned()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::isGPGAccepted(std::string id)
{
	std::cerr << "AuthGPGtest::isGPGAccepted()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::isGPGId(GPG_id id)
{
	std::cerr << "AuthGPGtest::isGPGId()";
	std::cerr << std::endl;
	return true;
}


/*********************************************************************************/
/************************* STAGE 4 ***********************************************/
/*********************************************************************************/
/*****
 * STAGE 4: Loading and Saving Certificates. (Strings and Files)
 *
 ****/
bool AuthGPGtest::LoadCertificateFromString(std::string pem, std::string &gpg_id)
{
	std::cerr << "AuthGPGtest::LoadCertificateFromString()";
	std::cerr << std::endl;
	return false;
}

std::string AuthGPGtest::SaveCertificateToString(std::string id)
{
	std::cerr << "AuthGPGtest::SaveCertificateToString()";
	std::cerr << std::endl;
	return "NOT_A_CERTIFICATE";
}


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
bool AuthGPGtest::setAcceptToConnectGPGCertificate(std::string gpg_id, bool acceptance) 
{
	std::cerr << "AuthGPGtest::setAcceptToConnectGPGCertificate()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::SignCertificateLevel0(std::string id)
{
	std::cerr << "AuthGPGtest::SignCertificateLevel0()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::RevokeCertificate(std::string id)
{
	std::cerr << "AuthGPGtest::RevokeCertificate()";
	std::cerr << std::endl;
	return true;
}

#if 0
bool AuthGPGtest::TrustCertificateNone(std::string id)
{
	std::cerr << "AuthGPGtest::TrustCertificateNone()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::TrustCertificateMarginally(std::string id)
{
	std::cerr << "AuthGPGtest::TrustCertificateMarginally()";
	std::cerr << std::endl;
	return true;
}

bool AuthGPGtest::TrustCertificateFully(std::string id)
{
	std::cerr << "AuthGPGtest::TrustCertificateFully()";
	std::cerr << std::endl;
	return true;
}

#endif

bool AuthGPGtest::TrustCertificate(std::string id,  int trustlvl)
{
	std::cerr << "AuthGPGtest::TrustCertificate()";
	std::cerr << std::endl;
	return true;
}


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
bool AuthGPGtest::SignData(std::string input, std::string &sign)
{
	std::cerr << "AuthGPGtest::SignData()";
	std::cerr << std::endl;
	return false;
}

bool AuthGPGtest::SignData(const void *data, const uint32_t len, std::string &sign)
{
	std::cerr << "AuthGPGtest::SignData()";
	std::cerr << std::endl;
	return false;
}

bool AuthGPGtest::SignDataBin(std::string input, unsigned char *sign, unsigned int *signlen)
{
	std::cerr << "AuthGPGtest::SignDataBin()";
	std::cerr << std::endl;
	return false;
}
#endif

bool AuthGPGtest::SignDataBin(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen)
{
	std::cerr << "AuthGPGtest::SignDataBin()";
	std::cerr << std::endl;
	return false;
}

bool AuthGPGtest::VerifySignBin(const void*, uint32_t, unsigned char*, unsigned int, std::string withfingerprint)
{
	std::cerr << "AuthGPGtest::VerifySignBin()";
	std::cerr << std::endl;
	return false;
}

bool AuthGPGtest::decryptText(gpgme_data_t CIPHER, gpgme_data_t PLAIN)
{
	std::cerr << "AuthGPGtest::decryptText()";
	std::cerr << std::endl;
	return false;
}

bool AuthGPGtest::encryptText(gpgme_data_t PLAIN, gpgme_data_t CIPHER)
{
	std::cerr << "AuthGPGtest::encryptText()";
	std::cerr << std::endl;
	return false;
}


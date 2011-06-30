/*
 * libretroshare/src/pqi: authssltest.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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

#include "pqi/authssltest.h"

AuthSSLtest::AuthSSLtest()
{
	mOwnId = "abcdtestid12345678";
}


        /* Initialisation Functions (Unique) */
bool    AuthSSLtest::validateOwnCertificate(X509 *x509, EVP_PKEY *pkey)
{
	std::cerr << "AuthSSLtest::validateOwnCertificate()";
	std::cerr << std::endl;
	return false;
}


bool	AuthSSLtest::active()
{
	std::cerr << "AuthSSLtest::active()";
	std::cerr << std::endl;
	return true;
}

int	AuthSSLtest::InitAuth(const char *srvr_cert, const char *priv_key, 
					const char *passwd)
{
	std::cerr << "AuthSSLtest::InitAuth()";
	std::cerr << std::endl;
	return 1;
}

bool	AuthSSLtest::CloseAuth()
{
	std::cerr << "AuthSSLtest::AuthSSLtest::CloseAuth()";
	std::cerr << std::endl;
	return 1;
}


	/*********** Overloaded Functions from p3AuthMgr **********/
	
        /* get Certificate Id */
std::string AuthSSLtest::OwnId()
{
	std::cerr << "AuthSSLtest::OwnId";
	std::cerr << std::endl;
	return mOwnId;
}

std::string AuthSSLtest::getOwnLocation()
{
	std::cerr << "AuthSSLtest::getOwnLocation";
	std::cerr << std::endl;
	return "TestVersion";
}

//bool    getAllList(std::list<std::string> &ids);
//bool    getAuthenticatedList(std::list<std::string> &ids);
//bool    getUnknownList(std::list<std::string> &ids);
//bool    getSSLChildListOfGPGId(std::string gpg_id, std::list<std::string> &ids);

	/* get Details from the Certificates */
//bool    isAuthenticated(std::string id);
//virtual	std::string getName(std::string id);
//std::string getIssuerName(std::string id);
//std::string getGPGId(SSL_id id);
//bool    getCertDetails(std::string id, sslcert &cert);

	/* Load/Save certificates */
std::string AuthSSLtest::SaveOwnCertificateToString()
{
	std::cerr << "AuthSSLtest::SaveOwnCertificateToString()";
	std::cerr << std::endl;
	return std::string();
}

	
	/* Sign / Encrypt / Verify Data */
bool 	AuthSSLtest::SignData(std::string input, std::string &sign)
{
	std::cerr << "AuthSSLtest::SignData()";
	std::cerr << std::endl;
	return false;
}

bool 	AuthSSLtest::SignData(const void *data, const uint32_t len, std::string &sign)
{
	std::cerr << "AuthSSLtest::SignData()";
	std::cerr << std::endl;
	return false;
}


bool 	AuthSSLtest::SignDataBin(std::string, unsigned char*, unsigned int*)
{
	std::cerr << "AuthSSLtest::SignDataBin()";
	std::cerr << std::endl;
	return false;
}

bool    AuthSSLtest::SignDataBin(const void*, uint32_t, unsigned char*, unsigned int*)
{
	std::cerr << "AuthSSLtest::SignDataBin()";
	std::cerr << std::endl;
	return false;
}

bool    AuthSSLtest::VerifyOwnSignBin(const void*, uint32_t, unsigned char*, unsigned int)
{
	std::cerr << "AuthSSLtest::VerifyOwnSignBin()";
	std::cerr << std::endl;
	return false;
}

bool	AuthSSLtest::VerifySignBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int signlen, SSL_id sslId)
{
	std::cerr << "AuthSSLtest::VerifySignBin()";
	std::cerr << std::endl;
	return false;
}



// return : false if encrypt failed
bool     AuthSSLtest::encrypt(void *&out, int &outlen, const void *in, int inlen, std::string peerId)
{
	std::cerr << "AuthSSLtest::encrypt()";
	std::cerr << std::endl;
	return false;
}

// return : false if decrypt fails
bool     AuthSSLtest::decrypt(void *&out, int &outlen, const void *in, int inlen)
{
	std::cerr << "AuthSSLtest::decrypt()";
	std::cerr << std::endl;
	return false;
}



X509* 	AuthSSLtest::SignX509ReqWithGPG(X509_REQ *req, long days)
{
	std::cerr << "AuthSSLtest::SignX509ReqWithGPG";
	std::cerr << std::endl;
	return NULL;
}

bool 	AuthSSLtest::AuthX509WithGPG(X509 *x509)
{
	std::cerr << "AuthSSLtest::AuthX509WithGPG()";
	std::cerr << std::endl;
	return false;
}



int 	AuthSSLtest::VerifyX509Callback(int preverify_ok, X509_STORE_CTX *ctx)
{
	std::cerr << "AuthSSLtest::VerifyX509Callback()";
	std::cerr << std::endl;
	return 0;
}

bool 	AuthSSLtest::ValidateCertificate(X509 *x509, std::string &peerId)
{
	std::cerr << "AuthSSLtest::ValidateCertificate()";
	std::cerr << std::endl;
	return false;
}



SSL_CTX *AuthSSLtest::getCTX()
{
	std::cerr << "AuthSSLtest::getCTX()";
	std::cerr << std::endl;
	return NULL;
}


/* Restored these functions: */
bool 	AuthSSLtest::FailedCertificate(X509 *x509, bool incoming)
{
	std::cerr << "AuthSSLtest::FailedCertificate()";
	std::cerr << std::endl;
	return false;
}

bool 	AuthSSLtest::CheckCertificate(std::string peerId, X509 *x509)
{
	std::cerr << "AuthSSLtest::CheckCertificate()";
	std::cerr << std::endl;
	return false;
}


/*
 * libretroshare/src/pqi: authssltest.h
 *
 * 3P/PQI network interface for RetroShare.
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
 */

#ifndef MRK_AUTH_SSL_TEST_HEADER
#define MRK_AUTH_SSL_TEST_HEADER

#include "pqi/authssl.h"

void setAuthSSL(AuthSSL *newssl);

class AuthSSLtest: public AuthSSL
{
	public:

	AuthSSLtest();
        /* Initialisation Functions (Unique) */
virtual bool    validateOwnCertificate(X509 *x509, EVP_PKEY *pkey);

virtual bool	active();
virtual int	InitAuth(const char *srvr_cert, const char *priv_key, 
					const char *passwd);
virtual bool	CloseAuth();

	/*********** Overloaded Functions from p3AuthMgr **********/
	
        /* get Certificate Id */
virtual	std::string OwnId();
virtual	std::string getOwnLocation();
//virtual bool    getAllList(std::list<std::string> &ids);
//virtual bool    getAuthenticatedList(std::list<std::string> &ids);
//virtual bool    getUnknownList(std::list<std::string> &ids);
//virtual bool    getSSLChildListOfGPGId(std::string gpg_id, std::list<std::string> &ids);

	/* get Details from the Certificates */
//virtual bool    isAuthenticated(std::string id);
//virtual	std::string getName(std::string id);
//virtual std::string getIssuerName(std::string id);
//virtual std::string getGPGId(SSL_id id);
//virtual bool    getCertDetails(std::string id, sslcert &cert);

	/* Load/Save certificates */
virtual	std::string SaveOwnCertificateToString();
	
	/* Sign / Encrypt / Verify Data */
virtual bool 	SignData(std::string input, std::string &sign);
virtual bool 	SignData(const void *data, const uint32_t len, std::string &sign);

virtual bool 	SignDataBin(std::string, unsigned char*, unsigned int*);
virtual bool    SignDataBin(const void*, uint32_t, unsigned char*, unsigned int*);
virtual bool    VerifyOwnSignBin(const void*, uint32_t, unsigned char*, unsigned int);
virtual bool	VerifySignBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int signlen, SSL_id sslId);

// return : false if encrypt failed
virtual bool     encrypt(void *&out, int &outlen, const void *in, int inlen, std::string peerId);
// return : false if decrypt fails
virtual bool     decrypt(void *&out, int &outlen, const void *in, int inlen);


virtual X509* 	SignX509ReqWithGPG(X509_REQ *req, long days);
virtual bool 	AuthX509WithGPG(X509 *x509);


virtual int 	VerifyX509Callback(int preverify_ok, X509_STORE_CTX *ctx);
virtual bool 	ValidateCertificate(X509 *x509, std::string &peerId); /* validate + get id */

	public: /* SSL specific functions used in pqissl/pqissllistener */
virtual SSL_CTX *getCTX();

/* Restored these functions: */
virtual bool 	FailedCertificate(X509 *x509, bool incoming);     /* store for discovery */
virtual bool 	CheckCertificate(std::string peerId, X509 *x509); /* check that they are exact match */

	private:

	std::string mOwnId;
};

#endif // MRK_AUTH_SSL_TEST_HEADER

/*
 * libretroshare/src/pqi: authssl.h
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

#ifndef MRK_AUTH_SSL_HEADER
#define MRK_AUTH_SSL_HEADER

/* 
 * This is an implementation of SSL certificate authentication, which is
 * overloaded with pgp style signatures, and web-of-trust authentication.
 *
 * only the owner ssl cert is store, the rest is jeus callback verification
 * 	
 * To use as an SSL authentication system, you must use a common CA certificate.
 *  * The pqissl stuff doesn't need to differentiate between SSL, SSL + PGP,
 *  as its X509 certs.
 *  * The rsserver stuff has to distinguish between all three types ;(
 * 
 */

#include "util/rswin.h"

#include <openssl/ssl.h>
#include <openssl/evp.h>

#include <string>
#include <map>

#include "util/rsthreads.h"

#include "pqi/pqi_base.h"
#include "pqi/pqinetwork.h"
#include "pqi/p3cfgmgr.h"

typedef std::string SSL_id;

/* This #define removes Connection Manager references in AuthSSL.
 * They should not be here. What about Objects and orthogonality?
 * This code is also stopping immediate reconnections from working.
 */

class AuthSSL;

class sslcert
{
        public:
        sslcert(X509* x509, std::string id);
        sslcert();

        /* certificate parameters */
        std::string id;
        std::string name;
        std::string location;
        std::string org;
        std::string email;

        std::string issuer;

        std::string fpr;
        //std::list<std::string> signers;

        /* Auth settings */
        bool authed;

        /* INTERNAL Parameters */
        X509* certificate;
};

/* required to install instance */
extern void    AuthSSLInit();

class AuthSSL
{
	public:
	AuthSSL();

static AuthSSL *getAuthSSL();

        /* Initialisation Functions (Unique) */
virtual bool    validateOwnCertificate(X509 *x509, EVP_PKEY *pkey) = 0;

virtual bool	active() = 0;
virtual int	InitAuth(const char *srvr_cert, const char *priv_key, 
					const char *passwd) = 0;
virtual bool	CloseAuth() = 0;

	/*********** Overloaded Functions from p3AuthMgr **********/
	
        /* get Certificate Id */
virtual	std::string OwnId() = 0;
virtual	std::string getOwnLocation() = 0;
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
virtual	std::string SaveOwnCertificateToString() = 0;
	
	/* Sign / Encrypt / Verify Data */
virtual bool 	SignData(std::string input, std::string &sign) = 0;
virtual bool 	SignData(const void *data, const uint32_t len, std::string &sign) = 0;

virtual bool 	SignDataBin(std::string, unsigned char*, unsigned int*) = 0;
virtual bool    SignDataBin(const void*, uint32_t, unsigned char*, unsigned int*) = 0;
virtual bool    VerifyOwnSignBin(const void*, uint32_t, unsigned char*, unsigned int) = 0;
virtual bool	VerifySignBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int signlen, SSL_id sslId) = 0;

// return : false if encrypt failed
virtual bool     encrypt(void *&out, int &outlen, const void *in, int inlen, std::string peerId) = 0;
// return : false if decrypt fails
virtual bool     decrypt(void *&out, int &outlen, const void *in, int inlen) = 0;


virtual X509* 	SignX509ReqWithGPG(X509_REQ *req, long days) = 0;
virtual bool 	AuthX509WithGPG(X509 *x509) = 0;


virtual int 	VerifyX509Callback(int preverify_ok, X509_STORE_CTX *ctx) = 0;
virtual bool 	ValidateCertificate(X509 *x509, std::string &peerId) = 0; /* validate + get id */

	public: /* SSL specific functions used in pqissl/pqissllistener */
virtual SSL_CTX *getCTX() = 0;

/* Restored these functions: */
virtual bool    FailedCertificate(X509 *x509, const struct sockaddr_in &addr, bool incoming) = 0; /* store for discovery */
virtual bool 	CheckCertificate(std::string peerId, X509 *x509) = 0; /* check that they are exact match */
};


class AuthSSLimpl : public AuthSSL, public p3Config
{
	public:

        /* Initialisation Functions (Unique) */
	AuthSSLimpl();
bool    validateOwnCertificate(X509 *x509, EVP_PKEY *pkey);

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


/*****************************************************************/
/***********************  p3config  ******************************/
        /* Key Functions to be overloaded for Full Configuration */
        virtual RsSerialiser *setupSerialiser();
        virtual bool saveList(bool &cleanup, std::list<RsItem *>& );
        virtual bool    loadList(std::list<RsItem *>& load);
/*****************************************************************/

	public: /* SSL specific functions used in pqissl/pqissllistener */
virtual SSL_CTX *getCTX();

/* Restored these functions: */
virtual bool    FailedCertificate(X509 *x509, const struct sockaddr_in &addr, bool incoming); /* store for discovery */
virtual bool 	CheckCertificate(std::string peerId, X509 *x509); /* check that they are exact match */


    private:

        // the single instance of this
        static AuthSSL *instance_ssl;

bool    LocalStoreCert(X509* x509);
bool 	RemoveX509(std::string id);

	/*********** LOCKED Functions ******/
bool 	locked_FindCert(std::string id, sslcert **cert);

	/* Data */
	/* these variables are constants -> don't need to protect */
	SSL_CTX *sslctx;
	std::string mOwnId;
	sslcert *mOwnCert;

        RsMutex sslMtx;  /* protects all below */


        EVP_PKEY *mOwnPrivateKey;
        EVP_PKEY *mOwnPublicKey;

	int init;

        std::map<std::string, sslcert *> mCerts;

};

#endif // MRK_AUTH_SSL_HEADER

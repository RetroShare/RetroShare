/*******************************************************************************
 * libretroshare/src/pqi: authssl.h                                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie, Retroshare Team.                      *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
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

#include <openssl/evp.h>
#include <openssl/x509.h>

#include <string>
#include <map>

#include "util/rsthreads.h"

#include "pqi/pqi_base.h"
#include "pqi/pqinetwork.h"
#include "pqi/p3cfgmgr.h"

/* This #define removes Connection Manager references in AuthSSL.
 * They should not be here. What about Objects and orthogonality?
 * This code is also stopping immediate reconnections from working.
 */

class AuthSSL;

class sslcert
{
        public:
        sslcert(X509* x509, const RsPeerId& id);
        sslcert();

        /* certificate parameters */
        RsPeerId id;
        std::string name;
        std::string location;
        std::string org;
        std::string email;

        RsPgpId issuer;
        PGPFingerprintType fpr;

        /* Auth settings */
        bool authed;

        /* INTERNAL Parameters */
        X509* certificate;
};

/* required to install instance */

class AuthSSL
{
	public:
	AuthSSL();

static AuthSSL *getAuthSSL();
static void    AuthSSLInit();

        /* Initialisation Functions (Unique) */
virtual bool    validateOwnCertificate(X509 *x509, EVP_PKEY *pkey) = 0;

virtual bool	active() = 0;
virtual int	InitAuth(const char *srvr_cert, const char *priv_key, 
                    const char *passwd, std::string alternative_location_name) = 0;
virtual bool	CloseAuth() = 0;

	/*********** Overloaded Functions from p3AuthMgr **********/
	
        /* get Certificate Id */
virtual	const RsPeerId& OwnId() = 0;
virtual	std::string getOwnLocation() = 0;

	/* Load/Save certificates */
virtual	std::string SaveOwnCertificateToString() = 0;
	
	/* Sign / Encrypt / Verify Data */
virtual bool 	SignData(std::string input, std::string &sign) = 0;
virtual bool 	SignData(const void *data, const uint32_t len, std::string &sign) = 0;

virtual bool 	SignDataBin(std::string, unsigned char*, unsigned int*) = 0;
virtual bool    SignDataBin(const void*, uint32_t, unsigned char*, unsigned int*) = 0;
virtual bool    VerifyOwnSignBin(const void*, uint32_t, unsigned char*, unsigned int) = 0;
virtual bool	VerifySignBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int signlen, const RsPeerId& sslId) = 0;

// return : false if encrypt failed
virtual bool     encrypt(void *&out, int &outlen, const void *in, int inlen, const RsPeerId& peerId) = 0;
// return : false if decrypt fails
virtual bool     decrypt(void *&out, int &outlen, const void *in, int inlen) = 0;


virtual X509* 	SignX509ReqWithGPG(X509_REQ *req, long days) = 0;
virtual bool 	AuthX509WithGPG(X509 *x509,uint32_t& auth_diagnostic)=0;


virtual int 	VerifyX509Callback(int preverify_ok, X509_STORE_CTX *ctx) = 0;
virtual bool 	ValidateCertificate(X509 *x509, RsPeerId& peerId) = 0; /* validate + get id */

	public: /* SSL specific functions used in pqissl/pqissllistener */
virtual SSL_CTX *getCTX() = 0;

/* Restored these functions: */
virtual void   setCurrentConnectionAttemptInfo(const RsPgpId& gpg_id,const RsPeerId& ssl_id,const std::string& ssl_cn) = 0 ;
virtual void   getCurrentConnectionAttemptInfo(      RsPgpId& gpg_id,      RsPeerId& ssl_id,      std::string& ssl_cn) = 0 ;

virtual bool    FailedCertificate(X509 *x509, const RsPgpId& gpgid,const RsPeerId& sslid,const std::string& sslcn,const struct sockaddr_storage &addr, bool incoming) = 0; /* store for discovery */
virtual bool 	CheckCertificate(const RsPeerId& peerId, X509 *x509) = 0; /* check that they are exact match */

static void setAuthSSL_debug(AuthSSL*) ;	// used for debug only. The real function is InitSSL()
static AuthSSL *instance_ssl ;
};


class AuthSSLimpl : public AuthSSL, public p3Config
{
	public:

        /* Initialisation Functions (Unique) */
	AuthSSLimpl();
bool    validateOwnCertificate(X509 *x509, EVP_PKEY *pkey);

virtual bool	active();
virtual int	InitAuth(const char *srvr_cert, const char *priv_key, 
                     const char *passwd, std::string alternative_location_name);
virtual bool	CloseAuth();

	/*********** Overloaded Functions from p3AuthMgr **********/
	
        /* get Certificate Id */
virtual	const RsPeerId& OwnId();
virtual	std::string getOwnLocation();

	/* Load/Save certificates */
virtual	std::string SaveOwnCertificateToString();
	
	/* Sign / Encrypt / Verify Data */
virtual bool 	SignData(std::string input, std::string &sign);
virtual bool 	SignData(const void *data, const uint32_t len, std::string &sign);

virtual bool 	SignDataBin(std::string, unsigned char*, unsigned int*);
virtual bool    SignDataBin(const void*, uint32_t, unsigned char*, unsigned int*);
virtual bool    VerifyOwnSignBin(const void*, uint32_t, unsigned char*, unsigned int);
virtual bool	VerifySignBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int signlen, const RsPeerId& sslId);

// return : false if encrypt failed
virtual bool     encrypt(void *&out, int &outlen, const void *in, int inlen, const RsPeerId& peerId);
// return : false if decrypt fails
virtual bool     decrypt(void *&out, int &outlen, const void *in, int inlen);


virtual X509* 	SignX509ReqWithGPG(X509_REQ *req, long days);
virtual bool 	AuthX509WithGPG(X509 *x509,uint32_t& auth_diagnostic);


virtual int 	VerifyX509Callback(int preverify_ok, X509_STORE_CTX *ctx);
virtual bool 	ValidateCertificate(X509 *x509, RsPeerId& peerId); /* validate + get id */


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
virtual void   setCurrentConnectionAttemptInfo(const RsPgpId& gpg_id,const RsPeerId& ssl_id,const std::string& ssl_cn) ;
virtual void   getCurrentConnectionAttemptInfo(      RsPgpId& gpg_id,      RsPeerId& ssl_id,      std::string& ssl_cn) ;
virtual bool    FailedCertificate(X509 *x509, const RsPgpId& gpgid,const RsPeerId& sslid,const std::string& sslcn,const struct sockaddr_storage &addr, bool incoming); /* store for discovery */
virtual bool 	CheckCertificate(const RsPeerId& peerId, X509 *x509); /* check that they are exact match */


    private:

bool    LocalStoreCert(X509* x509);
bool 	RemoveX509(const RsPeerId id);

	/*********** LOCKED Functions ******/
bool 	locked_FindCert(const RsPeerId& id, sslcert **cert);

	/* Data */
	/* these variables are constants -> don't need to protect */
	SSL_CTX *sslctx;
	RsPeerId mOwnId;
	sslcert *mOwnCert;

        RsMutex sslMtx;  /* protects all below */


        EVP_PKEY *mOwnPrivateKey;
        EVP_PKEY *mOwnPublicKey;

	int init;

        std::map<RsPeerId, sslcert *> mCerts;

		  RsPgpId _last_gpgid_to_connect ;
		  std::string _last_sslcn_to_connect ;
		  RsPeerId _last_sslid_to_connect ;
};

#endif // MRK_AUTH_SSL_HEADER

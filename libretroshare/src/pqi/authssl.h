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
 * and compilation should be done with PQI_USE_XPGP off, and PQI_USE_SSLONLY on
 *  * The pqissl stuff doesn't need to differentiate between SSL, SSL + PGP,
 *  as its X509 certs.
 *  * The rsserver stuff has to distinguish between all three types ;(
 * 
 */

#include <openssl/ssl.h>
#include <openssl/evp.h>

#include <string>
#include <map>

#include "util/rsthreads.h"

#include "pqi/pqi_base.h"
#include "pqi/pqinetwork.h"
#include "rsiface/rspeers.h"

typedef std::string SSL_id;

class AuthSSL;

class p3ConnectMgr;

class sslcert
{
        public:
        sslcert(X509 *x509, std::string id);
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
        X509 *certificate;
};


class AuthSSL
{
	public:

        /* Initialisation Functions (Unique) */
	AuthSSL();
bool    validateOwnCertificate(X509 *x509, EVP_PKEY *pkey);

virtual bool	active();
virtual int	InitAuth(const char *srvr_cert, const char *priv_key, 
					const char *passwd);
virtual bool	CloseAuth();
virtual int     setConfigDirectories(std::string confFile, std::string neighDir);
SSL_CTX *	getNewSslCtx();


	/*********** Overloaded Functions from p3AuthMgr **********/
	
        /* get Certificate Id */
virtual	std::string OwnId();
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

	/* High Level Load/Save Configuration */
//virtual bool FinalSaveCertificates();
//virtual bool CheckSaveCertificates();
//virtual bool saveCertificates();
//virtual bool loadCertificates();

	/* Load/Save certificates */

virtual bool LoadDetailsFromStringCert(std::string pem, RsPeerDetails &pd);
virtual	std::string SaveOwnCertificateToString();
//virtual bool LoadCertificateFromFile(std::string filename, std::string &id);
//virtual bool SaveCertificateToFile(std::string id, std::string filename);
//bool 	ProcessX509(X509 *x509, std::string &id);
//
//virtual bool LoadCertificateFromBinary(const uint8_t *ptr, uint32_t len, std::string &id);
//virtual	bool SaveCertificateToBinary(std::string id, uint8_t **ptr, uint32_t *len);
	
	/* Sign / Encrypt / Verify Data (TODO) */
virtual bool 	SignData(std::string input, std::string &sign);
virtual bool 	SignData(const void *data, const uint32_t len, std::string &sign);
virtual bool 	SignDataBin(std::string, unsigned char*, unsigned int*);
virtual bool    SignDataBin(const void*, uint32_t, unsigned char*, unsigned int*);
virtual bool    VerifySignBin(const void*, uint32_t, unsigned char*, unsigned int);

// return : false if encrypt failed
bool     encrypt(void *&out, int &outlen, const void *in, int inlen, std::string peerId);

// return : false if decrypt fails
bool     decrypt(void *&out, int &outlen, const void *in, int inlen);


	/*********** Overloaded Functions from p3AuthMgr **********/

	/************* Virtual Functions from AuthSSL *************/
X509* 	SignX509Req(X509_REQ *req, long days);
bool 	AuthX509(X509 *x509);


virtual int 	VerifyX509Callback(int preverify_ok, X509_STORE_CTX *ctx);
virtual bool 	ValidateCertificate(X509 *x509, std::string &peerId); /* validate + get id */

	/************* Virtual Functions from AuthSSL *************/


	public: /* SSL specific functions used in pqissl/pqissllistener */
SSL_CTX *getCTX();
static int ex_data_ctx_index; //used to pass the peer id in the ssl context


//bool 	FailedCertificate(X509 *x509, bool incoming);     /* store for discovery */
//bool 	CheckCertificate(std::string peerId, X509 *x509); /* check that they are exact match */

	/* Special Config Loading (backwards compatibility) */
//bool  	loadCertificates(bool &oldFormat, std::map<std::string, std::string> &keyValueMap);

  static AuthSSL *getAuthSSL() throw() // pour obtenir l'instance
      { return instance_ssl; }

        p3ConnectMgr *mConnMgr;

    private:

        // the single instance of this
        static AuthSSL *instance_ssl;

	/* Helper Functions */
X509 *	loadX509FromPEM(std::string pem);
X509 *	loadX509FromFile(std::string fname, std::string hash);
bool    saveX509ToFile(X509 *x509, std::string fname, std::string &hash);

X509 *	loadX509FromDER(const uint8_t *ptr, uint32_t len);
bool 	saveX509ToDER(X509 *x509, uint8_t **ptr, uint32_t *len);

	/*********** LOCKED Functions ******/
//bool 	locked_FindCert(std::string id, sslcert **cert);


	/* Data */
        RsMutex sslMtx;  /**** LOCKING */

	int init;
	std::string mCertConfigFile;
	std::string mNeighDir;

	SSL_CTX *sslctx;

	std::string mOwnId;
	sslcert *mOwnCert;
	EVP_PKEY *pkey;

	bool mToSaveCerts;
	bool mConfigSaveActive;
        //std::map<std::string, sslcert *> mCerts;

};

X509_REQ *GenerateX509Req(
                std::string pkey_file, std::string passwd,
                std::string name, std::string email, std::string org,
                std::string loc, std::string state, std::string country,
                int nbits_in, std::string &errString);

X509 *SignX509Certificate(X509_NAME *issuer, EVP_PKEY *privkey, X509_REQ *req, long days);


/* Helper Functions */
int printSSLError(SSL *ssl, int retval, int err, unsigned long err2, std::ostream &out);
std::string getX509NameString(X509_NAME *name);
std::string getX509CNString(X509_NAME *name);

std::string getX509OrgString(X509_NAME *name);
std::string getX509LocString(X509_NAME *name);
std::string getX509CountryString(X509_NAME *name);

std::string getX509Info(X509 *cert);
bool 	getX509id(X509 *x509, std::string &xid);

int     LoadCheckX509andGetIssuerName(const char *cert_file, 
			std::string &issuerName, std::string &userId);
int     LoadCheckX509andGetName(const char *cert_file, 
			std::string &userName, std::string &userId);

#endif // MRK_AUTH_SSL_HEADER

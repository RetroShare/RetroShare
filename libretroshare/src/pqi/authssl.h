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

/* This is a dummy auth header.... to 
 * work with the standard OpenSSL as opposed to the patched version.
 *
 * It is expected to be replaced by authpgp shortly.
 * (or provide the base OpenSSL iteraction for authpgp).
 *
 */

#include <openssl/ssl.h>
#include <openssl/evp.h>

#include <string>
#include <map>

#include "util/rsthreads.h"

#include "pqi/pqi_base.h"
#include "pqi/pqinetwork.h"
#include "pqi/p3authmgr.h"

class AuthXPGP;

class AuthSSL: public p3AuthMgr
{
	public:

	/* Initialisation Functions (Unique) */
	AuthSSL();
virtual bool	active();
virtual int	InitAuth(const char *srvr_cert, const char *priv_key, 
					const char *passwd);
virtual bool	CloseAuth();
virtual int     setConfigDirectories(std::string confFile, std::string neighDir);

	/*********** Overloaded Functions from p3AuthMgr **********/
	
	/* get Certificate Ids */
	
virtual	std::string OwnId();
virtual bool    getAllList(std::list<std::string> &ids);
virtual bool    getAuthenticatedList(std::list<std::string> &ids);
virtual bool    getUnknownList(std::list<std::string> &ids);
	
	/* get Details from the Certificates */
	
virtual bool    isValid(std::string id);
virtual bool    isAuthenticated(std::string id);
virtual	std::string getName(std::string id);
virtual bool    getDetails(std::string id, pqiAuthDetails &details);
	
	/* High Level Load/Save Configuration */
virtual bool FinalSaveCertificates();
virtual bool CheckSaveCertificates();
virtual bool saveCertificates();
virtual bool loadCertificates();

	/* Load/Save certificates */
virtual bool LoadCertificateFromString(std::string pem, std::string &id);
virtual	std::string SaveCertificateToString(std::string id);
virtual bool LoadCertificateFromFile(std::string filename, std::string &id);
virtual bool SaveCertificateToFile(std::string id, std::string filename);

virtual bool LoadCertificateFromBinary(const uint8_t *ptr, uint32_t len, std::string &id);	
virtual	bool SaveCertificateToBinary(std::string id, uint8_t **ptr, uint32_t *len);
	
	/* Signatures */

virtual bool AuthCertificate(std::string uid);
virtual bool SignCertificate(std::string id);
virtual bool RevokeCertificate(std::string id);
virtual bool TrustCertificate(std::string id, bool trust);
	
	/* Sign / Encrypt / Verify Data (TODO) */
virtual bool 	SignData(std::string input, std::string &sign);
virtual bool 	SignData(const void *data, const uint32_t len, std::string &sign);

	/*********** Overloaded Functions from p3AuthMgr **********/

	public: /* SSL specific functions used in pqissl/pqissllistener */
SSL_CTX *getCTX();

bool 	ValidateCertificate(X509 *x509, std::string &peerId); /* validate + get id */
bool 	FailedCertificate(X509 *x509, bool incoming);     /* store for discovery */
bool 	CheckCertificate(std::string peerId, X509 *x509); /* check that they are exact match */

	/* Special Config Loading (backwards compatibility) */
bool  	loadCertificates(bool &oldFormat, std::map<std::string, std::string> &keyValueMap);

#if 0
	private:

	/* Helper Functions */

bool 	ProcessXPGP(XPGP *xpgp, std::string &id);

XPGP *	loadXPGPFromPEM(std::string pem);
XPGP *	loadXPGPFromFile(std::string fname, std::string hash);
bool    saveXPGPToFile(XPGP *xpgp, std::string fname, std::string &hash);

XPGP *	loadXPGPFromDER(const uint8_t *ptr, uint32_t len);
bool 	saveXPGPToDER(XPGP *xpgp, uint8_t **ptr, uint32_t *len);

	/*********** LOCKED Functions ******/
bool 	locked_FindCert(std::string id, xpgpcert **cert);


	/* Data */
	RsMutex xpgpMtx;  /**** LOCKING */

	int init;
	std::string mCertConfigFile;
	std::string mNeighDir;

	SSL_CTX *sslctx;
	XPGP_KEYRING *pgp_keyring;

	std::string mOwnId;
	xpgpcert *mOwnCert;
	EVP_PKEY *pkey;

	bool mToSaveCerts;
	bool mConfigSaveActive;
	std::map<std::string, xpgpcert *> mCerts;
#endif

};

/* Helper Functions */
int printSSLError(SSL *ssl, int retval, int err, unsigned long err2, std::ostream &out);
std::string getX509NameString(X509_NAME *name);
std::string getX509CNString(X509_NAME *name);

std::string getX509OrgString(X509_NAME *name);
std::string getX509LocString(X509_NAME *name);
std::string getX509CountryString(X509_NAME *name);

#if 0
std::list<std::string> getXPGPsigners(XPGP *cert);
std::string getXPGPInfo(XPGP *cert);
std::string getXPGPAuthCode(XPGP *xpgp);

int     LoadCheckXPGPandGetName(const char *cert_file, 
			std::string &userName, std::string &userId);
bool 	getXPGPid(XPGP *xpgp, std::string &xpgpid);
#endif


#endif // MRK_SSL_XPGP_CERT_HEADER

/*
 * libretroshare/src/pqi: authxpgp.h
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

#ifndef MRK_AUTH_SSL_XPGP_HEADER
#define MRK_AUTH_SSL_XPGP_HEADER

/* This is the trial XPGP version 
 *
 * It has to be compiled against XPGP ssl version.
 * this is only a hacked up version, merging 
 * (so both can operate in parallel will happen later)
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


class xpgpcert
{
	public:
	xpgpcert(XPGP *xpgp, std::string id);

	/* certificate parameters */
	std::string id;
	std::string name; 
	std::string location;
	std::string org;
	std::string email;

	std::string fpr;
	std::list<std::string> signers;

	/* Auth settings */

	uint32_t trustLvl;
	bool ownsign;
	bool trusted;

	/* INTERNAL Parameters */
	XPGP *certificate;
};

class AuthXPGP: public p3AuthMgr
{
	public:

	/* Initialisation Functions (Unique) */
	AuthXPGP();
virtual bool	active();
virtual int	InitAuth(const char *srvr_cert, const char *priv_key, 
					const char *passwd);
virtual bool	CloseAuth();
virtual int     setConfigDirectories(const char *cdir, const char *ndir);

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
	
	/* Load/Save certificates */
	
virtual bool LoadCertificateFromString(std::string pem, std::string &id);
virtual	std::string SaveCertificateToString(std::string id);
virtual bool LoadCertificateFromFile(std::string filename, std::string &id);
virtual bool SaveCertificateToFile(std::string id, std::string filename);
	
	/* Signatures */

virtual bool AuthCertificate(std::string uid);
virtual bool SignCertificate(std::string id);
virtual bool RevokeCertificate(std::string id);
virtual bool TrustCertificate(std::string id, bool trust);
	
	/* Sign / Encrypt / Verify Data (TODO) */


	/**** NEW functions we've added ****/


	/*********** Overloaded Functions from p3AuthMgr **********/

	private:

	/* Helper Functions */
SSL_CTX *getCTX();

bool 	getXPGPid(XPGP *xpgp, std::string &xpgpid);
bool 	ProcessXPGP(XPGP *xpgp, std::string &id);
XPGP *	loadXPGPFromDER(char *data, uint32_t len);
XPGP *	loadXPGPFromPEM(std::string pem);
XPGP *	loadXPGPFromFile(std::string fname, std::string hash);
bool    saveXPGPToFile(XPGP *xpgp, std::string fname, std::string &hash);

	/*********** LOCKED Functions ******/
bool 	locked_FindCert(std::string id, xpgpcert **cert);


	/* Data */
	RsMutex xpgpMtx;  /**** LOCKING */

	int init;
	std::string mCertDir;
	std::string mNeighDir;

	SSL_CTX *sslctx;
	XPGP_KEYRING *pgp_keyring;

	std::string mOwnId;
	xpgpcert *mOwnCert;
	EVP_PKEY *pkey;

	std::map<std::string, xpgpcert *> mCerts;

};

/* Helper Functions */
int printSSLError(SSL *ssl, int retval, int err, unsigned long err2, std::ostream &out);
std::string getX509NameString(X509_NAME *name);
std::string getX509CNString(X509_NAME *name);

std::string getX509OrgString(X509_NAME *name);
std::string getX509LocString(X509_NAME *name);
std::string getX509CountryString(X509_NAME *name);

std::list<std::string> getXPGPsigners(XPGP *cert);
std::string getXPGPInfo(XPGP *cert);
std::string getXPGPAuthCode(XPGP *xpgp);

int     LoadCheckXPGPandGetName(const char *cert_file, std::string &userName);


#endif // MRK_SSL_XPGP_CERT_HEADER

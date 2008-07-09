/*
 * libretroshare/src/pqi: authxpgp.cc
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

#include "authxpgp.h"

#include "pqinetwork.h"

/******************** notify of new Cert **************************/
#include "pqinotify.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include <sstream>
#include <iomanip>

/******************************** TRUST LVLS
xPGP_vfy.h:#define TRUST_SIGN_OWN               6
xPGP_vfy.h:#define TRUST_SIGN_TRSTED    5
xPGP_vfy.h:#define TRUST_SIGN_AUTHEN    4
xPGP_vfy.h:#define TRUST_SIGN_BASIC     3
xPGP_vfy.h:#define TRUST_SIGN_UNTRUSTED 2
xPGP_vfy.h:#define TRUST_SIGN_UNKNOWN      1
xPGP_vfy.h:#define TRUST_SIGN_NONE              0
xPGP_vfy.h:#define TRUST_SIGN_BAD               -1
******************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/***********
 ** #define AUTHXPGP_DEBUG	1
 **********/

// the single instance of this.
static AuthXPGP instance_sslroot;

p3AuthMgr *getAuthMgr()
{
	return &instance_sslroot;
}


xpgpcert::xpgpcert(XPGP *xpgp, std::string pid)
{
	certificate = xpgp;
	id = pid;
	name = getX509CNString(xpgp->subject -> subject);
	org = getX509OrgString(xpgp->subject -> subject);
	location = getX509LocString(xpgp->subject -> subject);
	email = "";

	/* These should be filled in afterwards */
	fpr = pid;

	trustLvl = 0;
	ownsign = false;
	trusted = false;
}


AuthXPGP::AuthXPGP()
	:init(0), sslctx(NULL), pkey(NULL), mToSaveCerts(false), mConfigSaveActive(true)
{
}

bool AuthXPGP::active()
{
	return init;
}

// args: server cert, server private key, trusted certificates.

int	AuthXPGP::InitAuth(const char *cert_file, const char *priv_key_file, 
			const char *passwd)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::InitAuth()";
	std::cerr << std::endl;
#endif

static  int initLib = 0;
	if (!initLib)
	{
		initLib = 1;
		SSL_load_error_strings();
		SSL_library_init();
	}


	if (init == 1)
	{
		return 1;
	}

	if ((cert_file == NULL) ||
		(priv_key_file == NULL) ||
		(passwd == NULL))
	{
		fprintf(stderr, "sslroot::initssl() missing parameters!\n");
		return 0;
	}


	// XXX TODO
	// actions_to_seed_PRNG();

	std::cerr << "SSL Library Init!" << std::endl;

	// setup connection method
	sslctx = SSL_CTX_new(PGPv1_method());

	// setup cipher lists.
	SSL_CTX_set_cipher_list(sslctx, "DEFAULT");

	// certificates (Set Local Server Certificate).
	FILE *ownfp = fopen(cert_file, "r");
	if (ownfp == NULL)
	{
		std::cerr << "Couldn't open Own Certificate!" << std::endl;
		return -1;
	}



	// get xPGP certificate.
	XPGP *xpgp = PEM_read_XPGP(ownfp, NULL, NULL, NULL);
	fclose(ownfp);

	if (xpgp == NULL)
	{
		return -1;
	}
	SSL_CTX_use_pgp_certificate(sslctx, xpgp);

	// get private key
	FILE *pkfp = fopen(priv_key_file, "rb");
	if (pkfp == NULL)
	{
		std::cerr << "Couldn't Open PrivKey File!" << std::endl;
		CloseAuth();
		return -1;
	}

	pkey = PEM_read_PrivateKey(pkfp, NULL, NULL, (void *) passwd);
	fclose(pkfp);

	if (pkey == NULL)
	{
		return -1;
	}
	SSL_CTX_use_pgp_PrivateKey(sslctx, pkey);

	if (1 != SSL_CTX_check_pgp_private_key(sslctx))
	{
		std::cerr << "Issues With Private Key! - Doesn't match your Cert" << std::endl;
		std::cerr << "Check your input key/certificate:" << std::endl;
		std::cerr << priv_key_file << " & " << cert_file;
		std::cerr << std::endl;
		CloseAuth();
		return -1;
	}

	// make keyring.
	pgp_keyring = createPGPContext(xpgp, pkey);
	SSL_CTX_set_XPGP_KEYRING(sslctx, pgp_keyring);


	// Setup the certificate. (after keyring is made!).
	if (!XPGP_check_valid_certificate(xpgp))
	{
		/* bad certificate */
		CloseAuth();
		return -1;
	}
		
	if (!getXPGPid(xpgp, mOwnId))
	{
		/* bad certificate */
		CloseAuth();
		return -1;
	}


	// enable verification of certificates (PEER)
	SSL_CTX_set_verify(sslctx, SSL_VERIFY_PEER | 
			SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

	std::cerr << "SSL Verification Set" << std::endl;

	mOwnCert = new xpgpcert(xpgp, mOwnId);

	/* add to keyring */
	XPGP_add_certificate(pgp_keyring, mOwnCert->certificate);
        mOwnCert->trustLvl = XPGP_auth_certificate(pgp_keyring, mOwnCert->certificate);
	mOwnCert->trusted = true;
	mOwnCert->ownsign = true;

	init = 1;
	return 1;
}



bool	AuthXPGP::CloseAuth()
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::CloseAuth()";
	std::cerr << std::endl;
#endif
	SSL_CTX_free(sslctx);

	// clean up private key....
	// remove certificates etc -> opposite of initssl.
	init = 0;
	return 1;
}

/* Context handling  */
SSL_CTX *AuthXPGP::getCTX()
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::getCTX()";
	std::cerr << std::endl;
#endif
	return sslctx;
}

int     AuthXPGP::setConfigDirectories(std::string configfile, std::string neighdir)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::setConfigDirectories()";
	std::cerr << std::endl;
#endif
	xpgpMtx.lock();   /***** LOCK *****/

	mCertConfigFile = configfile;
	mNeighDir = neighdir;

	xpgpMtx.unlock(); /**** UNLOCK ****/
	return 1;
}
	
std::string AuthXPGP::OwnId()
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::OwnId()";
	std::cerr << std::endl;
#endif
	xpgpMtx.lock();   /***** LOCK *****/

	std::string id = mOwnId;

	xpgpMtx.unlock(); /**** UNLOCK ****/
	return id;
}

bool    AuthXPGP::getAllList(std::list<std::string> &ids)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::getAllList()";
	std::cerr << std::endl;
#endif
	xpgpMtx.lock();   /***** LOCK *****/

	/* iterate through both lists */
	std::map<std::string, xpgpcert *>::iterator it;

	for(it = mCerts.begin(); it != mCerts.end(); it++)
	{
		ids.push_back(it->first);
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/

	return true;
}

bool    AuthXPGP::getAuthenticatedList(std::list<std::string> &ids)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::getAuthenticatedList()";
	std::cerr << std::endl;
#endif
	xpgpMtx.lock();   /***** LOCK *****/

	/* iterate through both lists */
	std::map<std::string, xpgpcert *>::iterator it;

	for(it = mCerts.begin(); it != mCerts.end(); it++)
	{
		if (it->second->trustLvl > TRUST_SIGN_BASIC)
		{
			ids.push_back(it->first);
		}
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/

	return true;
}

bool    AuthXPGP::getUnknownList(std::list<std::string> &ids)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::getUnknownList()";
	std::cerr << std::endl;
#endif
	xpgpMtx.lock();   /***** LOCK *****/

	/* iterate through both lists */
	std::map<std::string, xpgpcert *>::iterator it;

	for(it = mCerts.begin(); it != mCerts.end(); it++)
	{
		if (it->second->trustLvl <= TRUST_SIGN_BASIC)
		{
			ids.push_back(it->first);
		}
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/

	return true;
}

	/* silly question really - only valid certs get saved to map
	 * so if in map its okay
	 */
bool    AuthXPGP::isValid(std::string id)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::isValid() " << id;
	std::cerr << std::endl;
#endif
	xpgpMtx.lock();   /***** LOCK *****/

	bool valid = (mCerts.end() != mCerts.find(id));

	xpgpMtx.unlock(); /**** UNLOCK ****/

	return valid;
}

bool    AuthXPGP::isAuthenticated(std::string id)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::isAuthenticated() " << id;
	std::cerr << std::endl;
#endif
	xpgpMtx.lock();   /***** LOCK *****/

	xpgpcert *cert = NULL;
	bool auth = false;

	if (locked_FindCert(id, &cert))
	{
		auth = (cert->trustLvl > TRUST_SIGN_BASIC);
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/

	return auth;
}

std::string AuthXPGP::getName(std::string id)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::getName() " << id;
	std::cerr << std::endl;
#endif
	std::string name;

	xpgpMtx.lock();   /***** LOCK *****/

	xpgpcert *cert = NULL;
	if (id == mOwnId)
	{
		name = mOwnCert->name;
	}
	else if (locked_FindCert(id, &cert))
	{
		name = cert->name;
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/

	return name;
}

bool    AuthXPGP::getDetails(std::string id, pqiAuthDetails &details)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::getDetails() " << id;
	std::cerr << std::endl;
#endif
	xpgpMtx.lock();   /***** LOCK *****/

	bool valid = false;
	xpgpcert *cert = NULL;
	if (id == mOwnId)
	{
		cert = mOwnCert;
		valid = true;
	}
	else if (locked_FindCert(id, &cert))
	{
		valid = true;
	}

	if (valid)
	{
		/* fill details */
		details.id 	= cert->id;
		details.name 	= cert->name;
		details.email 	= cert->email;
		details.location= cert->location;
		details.org 	= cert->org;

		details.fpr	= cert->fpr;
		details.signers = cert->signers;

		details.trustLvl= cert->trustLvl;
		details.ownsign = cert->ownsign;
		details.trusted = cert->trusted;
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/

	return valid;
}
	
	
	/* Load/Save certificates */
	
bool AuthXPGP::LoadCertificateFromString(std::string pem, std::string &id)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::LoadCertificateFromString() " << id;
	std::cerr << std::endl;
#endif

	XPGP *xpgp = loadXPGPFromPEM(pem);
	if (!xpgp)
		return false;

	return ProcessXPGP(xpgp, id);
}

std::string AuthXPGP::SaveCertificateToString(std::string id)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::SaveCertificateToString() " << id;
	std::cerr << std::endl;
#endif


	xpgpMtx.lock();   /***** LOCK *****/

	/* get the cert first */
	std::string certstr;
	xpgpcert *cert = NULL;
	bool valid = false;

	if (id == mOwnId)
	{
		cert = mOwnCert;
		valid = true;
	}
	else if (locked_FindCert(id, &cert))
	{
		valid = true;
	}

	if (valid)
	{
		BIO *bp = BIO_new(BIO_s_mem());

		PEM_write_bio_XPGP(bp, cert->certificate);

		/* translate the bp data to a string */
		char *data;
		int len = BIO_get_mem_data(bp, &data);
		for(int i = 0; i < len; i++)
		{
			certstr += data[i];
		}

		BIO_free(bp);
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/

	return certstr;
}



bool AuthXPGP::LoadCertificateFromFile(std::string filename, std::string &id)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::LoadCertificateFromFile() " << id;
	std::cerr << std::endl;
#endif

	std::string nullhash;

	XPGP *xpgp = loadXPGPFromFile(filename.c_str(), nullhash);
	if (!xpgp)
		return false;

	return ProcessXPGP(xpgp, id);
}

bool AuthXPGP::SaveCertificateToFile(std::string id, std::string filename)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::SaveCertificateToFile() " << id;
	std::cerr << std::endl;
#endif

	xpgpMtx.lock();   /***** LOCK *****/

	/* get the cert first */
	xpgpcert *cert = NULL;
	bool valid = false;
	std::string hash;

	if (id == mOwnId)
	{
		cert = mOwnCert;
		valid = true;
	}
	else if (locked_FindCert(id, &cert))
	{
		valid = true;
	}
	if (valid)
	{
		valid = saveXPGPToFile(cert->certificate, filename, hash);
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/
	return valid;
}

	/**** To/From DER format ***/

bool 	AuthXPGP::LoadCertificateFromBinary(const uint8_t *ptr, uint32_t len, std::string &id)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::LoadCertificateFromFile() " << id;
	std::cerr << std::endl;
#endif

	XPGP *xpgp = loadXPGPFromDER(ptr, len);
	if (!xpgp)
		return false;

	return ProcessXPGP(xpgp, id);

}

bool 	AuthXPGP::SaveCertificateToBinary(std::string id, uint8_t **ptr, uint32_t *len)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::SaveCertificateToBinary() " << id;
	std::cerr << std::endl;
#endif

	xpgpMtx.lock();   /***** LOCK *****/

	/* get the cert first */
	xpgpcert *cert = NULL;
	bool valid = false;
	std::string hash;

	if (id == mOwnId)
	{
		cert = mOwnCert;
		valid = true;
	}
	else if (locked_FindCert(id, &cert))
	{
		valid = true;
	}
	if (valid)
	{
		valid = saveXPGPToDER(cert->certificate, ptr, len);
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/
	return valid;
}


	/* Signatures */
bool AuthXPGP::SignCertificate(std::string id)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::SignCertificate() " << id;
	std::cerr << std::endl;
#endif

	xpgpMtx.lock();   /***** LOCK *****/

	/* get the cert first */
	xpgpcert *cert = NULL;
	xpgpcert *own = mOwnCert;
	bool valid = false;

	if (locked_FindCert(id, &cert))
	{
	  	if (0 < validateCertificateIsSignedByKey(
				cert->certificate, own->certificate))
		{
#ifdef AUTHXPGP_DEBUG
			std::cerr << "AuthXPGP::SignCertificate() Signed Already: " << id;
			std::cerr << std::endl;
#endif
			cert->ownsign=true;
		}
		else
		{
#ifdef AUTHXPGP_DEBUG
			std::cerr << "AuthXPGP::SignCertificate() Signing Cert: " << id;
			std::cerr << std::endl;
#endif
			/* sign certificate */
			XPGP_sign_certificate(pgp_keyring, cert->certificate, own->certificate);

			/* reevaluate the auth of the xpgp */
			cert->trustLvl = XPGP_auth_certificate(pgp_keyring, cert->certificate);
			cert->ownsign = true;

			mToSaveCerts = true;
		}
		valid = true;
	}


	xpgpMtx.unlock(); /**** UNLOCK ****/
	return valid;
}

bool AuthXPGP::TrustCertificate(std::string id, bool totrust)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::TrustCertificate() " << id;
	std::cerr << std::endl;
#endif

	xpgpMtx.lock();   /***** LOCK *****/

	/* get the cert first */
	xpgpcert *cert = NULL;
	bool valid = false;

	if (locked_FindCert(id, &cert))
	{

		/* if trusted -> untrust */
		if (!totrust)
		{
			XPGP_signer_untrusted(pgp_keyring, cert->certificate);
			cert->trusted = false;
		}
		else
		{
			/* if auth then we can trust them */
			if (XPGP_signer_trusted(pgp_keyring, cert->certificate))
			{
				cert->trusted = true;
			}
		}

		/* reevaluate the auth of the xpgp */
		cert->trustLvl = XPGP_auth_certificate(pgp_keyring, cert->certificate);
		valid = true;

		/* resave if changed trust setting */
		mToSaveCerts = true;
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/
	return valid;
}

bool AuthXPGP::RevokeCertificate(std::string id)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::RevokeCertificate() " << id;
	std::cerr << std::endl;
#endif

	xpgpMtx.lock();   /***** LOCK *****/
	xpgpMtx.unlock(); /**** UNLOCK ****/

	return false;
}


bool AuthXPGP::AuthCertificate(std::string id)
{

#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::AuthCertificate() " << id;
	std::cerr << std::endl;
#endif

	xpgpMtx.lock();   /***** LOCK *****/

	/* get the cert first */
	xpgpcert *cert = NULL;
	xpgpcert *own = mOwnCert;
	bool valid = false;

	if (locked_FindCert(id, &cert))
	{
		/* ADD IN LATER */
		//if (cert->trustLvl > TRUST_SIGN_BASIC)
		//{
#ifdef AUTHXPGP_DEBUG
		//	std::cerr << "AuthXPGP::AuthCertificate() Already Authed: " << id;
		//	std::cerr << std::endl;
#endif
		//}

	  	if (0 < validateCertificateIsSignedByKey(
				cert->certificate, own->certificate))
		{
#ifdef AUTHXPGP_DEBUG
			std::cerr << "AuthXPGP::AuthCertificate() Signed Already: " << id;
			std::cerr << std::endl;
#endif
			cert->ownsign=true;
		}
		else
		{
#ifdef AUTHXPGP_DEBUG
			std::cerr << "AuthXPGP::AuthCertificate() Signing Cert: " << id;
			std::cerr << std::endl;
#endif
			/* sign certificate */
			XPGP_sign_certificate(pgp_keyring, cert->certificate, own->certificate);

			/* reevaluate the auth of the xpgp */
			cert->trustLvl = XPGP_auth_certificate(pgp_keyring, cert->certificate);
			cert->ownsign = true;

			mToSaveCerts = true;
		}
		valid = true;
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/
	return valid;
}


	/* Sign / Encrypt / Verify Data (TODO) */
	
bool AuthXPGP::SignData(std::string input, std::string &sign)
{
	return SignData(input.c_str(), input.length(), sign);
}

bool AuthXPGP::SignData(const void *data, const uint32_t len, std::string &sign)
{

	RsStackMutex stack(xpgpMtx);   /***** STACK LOCK MUTEX *****/

	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	unsigned int signlen = EVP_PKEY_size(pkey);
	unsigned char signature[signlen];

	if (0 == EVP_SignInit(mdctx, EVP_sha1()))
	{
		std::cerr << "EVP_SignInit Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
		return false;
	}

	if (0 == EVP_SignUpdate(mdctx, data, len))
	{
		std::cerr << "EVP_SignUpdate Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
		return false;
	}

	if (0 == EVP_SignFinal(mdctx, signature, &signlen, pkey))
	{
		std::cerr << "EVP_SignFinal Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
		return false;
	}

	EVP_MD_CTX_destroy(mdctx);

	sign.clear();	
	std::ostringstream out;
	out << std::hex;
	for(uint32_t i = 0; i < signlen; i++) 
	{
		out << std::setw(2) << std::setfill('0');
		out << (uint32_t) (signature[i]);
	}

	sign = out.str();

	return true;
}



	/**** NEW functions we've added ****/




	/**** AUX Functions ****/
bool AuthXPGP::locked_FindCert(std::string id, xpgpcert **cert)
{
	std::map<std::string, xpgpcert *>::iterator it;

	if (mCerts.end() != (it = mCerts.find(id)))
	{
		*cert = it->second;
		return true;
	}
	return false;
}


XPGP *AuthXPGP::loadXPGPFromFile(std::string fname, std::string hash)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::LoadXPGPFromFile()";
	std::cerr << std::endl;
#endif

	// if there is a hash - check that the file matches it before loading.
	XPGP *pc = NULL;
	FILE *pcertfp = fopen(fname.c_str(), "rb");

	// load certificates from file.
	if (pcertfp == NULL)
	{
		std::cerr << "sslroot::loadcertificate() Bad File: " << fname;
		std::cerr << " Cannot be Hashed!" << std::endl;
		return NULL;
	}

	/* We only check a signature's hash if
	 * we are loading from a configuration file.
	 * Therefore we saved the file and it should be identical. 
	 * and a direct load + verify will work.
	 *
	 * If however it has been transported by email....
	 * Then we might have to correct the data (strip out crap)
	 * from the configuration at the end. (XPGP load should work!)
	 */

	if (hash.length() > 1)
	{

		unsigned int signlen = EVP_PKEY_size(pkey);
		unsigned char signature[signlen];

		int maxsize = 20480; /* should be enough for about 50 signatures */
		int rbytes;
		char inall[maxsize];
		if (0 == (rbytes = fread(inall, 1, maxsize, pcertfp)))
		{
			std::cerr << "Error Reading Peer Record!" << std::endl;
			return NULL;
		}
		//std::cerr << "Read " << rbytes << std::endl;


		EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	
		if (0 == EVP_SignInit_ex(mdctx, EVP_sha1(), NULL))
		{
			std::cerr << "EVP_SignInit Failure!" << std::endl;
		}
	
		if (0 == EVP_SignUpdate(mdctx, inall, rbytes))
		{
			std::cerr << "EVP_SignUpdate Failure!" << std::endl;
		}
	
		if (0 == EVP_SignFinal(mdctx, signature, &signlen, pkey))
		{
			std::cerr << "EVP_SignFinal Failure!" << std::endl;
		}

		EVP_MD_CTX_destroy(mdctx);
	
		bool same = true;
		if (signlen != hash.length())
		{
				std::cerr << "Different Length Signatures... ";
				std::cerr << "Cannot Load Certificate!" << std::endl;
				fclose(pcertfp);
				return NULL;
		}

		for(int i = 0; i < (signed) signlen; i++) 
		{
			if (signature[i] != (unsigned char) hash[i])
			{
				same = false;
				std::cerr << "Invalid Signature... ";
				std::cerr << "Cannot Load Certificate!" << std::endl;
				fclose(pcertfp);
				return NULL;
			}
		}
		std::cerr << "Verified Signature for: " << fname;
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "Not checking cert signature" << std::endl;
	}

	fseek(pcertfp, 0, SEEK_SET); /* rewind */
	pc = PEM_read_XPGP(pcertfp, NULL, NULL, NULL);
	fclose(pcertfp);

	if (pc != NULL)
	{
		// read a certificate.
		std::cerr << "Loaded Certificate: " << pc -> name << std::endl;
	}
	else // (pc == NULL)
	{
		unsigned long err = ERR_get_error();
		std::cerr << "Read Failed .... CODE(" << err << ")" << std::endl;
		std::cerr << ERR_error_string(err, NULL) << std::endl;
	
		return NULL;
	}
	return pc;
}

bool  	AuthXPGP::saveXPGPToFile(XPGP *xpgp, std::string fname, std::string &hash)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::saveXPGPToFile()";
	std::cerr << std::endl;
#endif

	// load certificates from file.
	FILE *setfp = fopen(fname.c_str(), "wb");
	if (setfp == NULL)
	{
		std::cerr << "sslroot::savecertificate() Bad File: " << fname;
		std::cerr << " Cannot be Written!" << std::endl;
		return false;
	}

	std::cerr << "Writing out Cert...:" << xpgp->name << std::endl;
	PEM_write_XPGP(setfp, xpgp);

	fclose(setfp);

	// then reopen to generate hash.
	setfp = fopen(fname.c_str(), "rb");
	if (setfp == NULL)
	{
		std::cerr << "sslroot::savecertificate() Bad File: " << fname;
		std::cerr << " Opened for ReHash!" << std::endl;
		return false;
	}

	unsigned int signlen = EVP_PKEY_size(pkey);
	unsigned char signature[signlen];

	int maxsize = 20480;
	int rbytes;
	char inall[maxsize];
	if (0 == (rbytes = fread(inall, 1, maxsize, setfp)))
	{
		std::cerr << "Error Writing Peer Record!" << std::endl;
		return -1;
	}
	std::cerr << "Read " << rbytes << std::endl;

	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	if (0 == EVP_SignInit_ex(mdctx, EVP_sha1(), NULL))
	{
		std::cerr << "EVP_SignInit Failure!" << std::endl;
	}

	if (0 == EVP_SignUpdate(mdctx, inall, rbytes))
	{
		std::cerr << "EVP_SignUpdate Failure!" << std::endl;
	}

	if (0 == EVP_SignFinal(mdctx, signature, &signlen, pkey))
	{
		std::cerr << "EVP_SignFinal Failure!" << std::endl;
	}

	std::cerr << "Saved Cert: " << xpgp->name;
	std::cerr << std::endl;

	std::cerr << "Cert + Setting Signature is(" << signlen << "): ";
	std::string signstr;
	for(uint32_t i = 0; i < signlen; i++) 
	{
		fprintf(stderr, "%02x", signature[i]);
		signstr += signature[i];
	}
	std::cerr << std::endl;

	hash = signstr;
	fclose(setfp);

	EVP_MD_CTX_destroy(mdctx);

	return true;
}


XPGP *AuthXPGP::loadXPGPFromPEM(std::string pem)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::LoadXPGPFromPEM()";
	std::cerr << std::endl;
#endif

	/* Put the data into a mem BIO */
	char *certstr = strdup(pem.c_str());

	BIO *bp = BIO_new_mem_buf(certstr, -1);

	XPGP *pc = PEM_read_bio_XPGP(bp, NULL, NULL, NULL);

	BIO_free(bp);
	free(certstr);

	return pc;
}

XPGP *AuthXPGP::loadXPGPFromDER(const uint8_t *ptr, uint32_t len)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::LoadXPGPFromDER()";
	std::cerr << std::endl;
#endif

        XPGP *tmp = NULL;
        unsigned char **certptr = (unsigned char **) &ptr;
        XPGP *xpgp = d2i_XPGP(&tmp, certptr, len);

	return xpgp;
}

bool AuthXPGP::saveXPGPToDER(XPGP *xpgp, uint8_t **ptr, uint32_t *len)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::saveXPGPToDER()";
	std::cerr << std::endl;
#endif

	int certlen = i2d_XPGP(xpgp, (unsigned char **) ptr);
	if (certlen > 0)
	{
		*len = certlen;
		return true;
	}
	else
	{
		*len = 0;
		return false;
	}
	return false;
}




bool AuthXPGP::ProcessXPGP(XPGP *xpgp, std::string &id)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::ProcessXPGP()";
	std::cerr << std::endl;
#endif

	/* extract id */
	std::string xpgpid;

	if (!XPGP_check_valid_certificate(xpgp))
	{
		/* bad certificate */
		XPGP_free(xpgp);
		return false;
	}
		
	if (!getXPGPid(xpgp, xpgpid))
	{
		/* bad certificate */
		XPGP_free(xpgp);
		return false;
	}

	xpgpcert *cert = NULL;
	bool duplicate = false;

	xpgpMtx.lock();   /***** LOCK *****/

	if (xpgpid == mOwnId)
	{
		cert = mOwnCert;
		duplicate = true;
	}
	else if (locked_FindCert(xpgpid, &cert))
	{
		duplicate = true;
	}

	if (duplicate)
	{
		/* have a duplicate */
		/* check that they are exact */
		if (0 != XPGP_cmp(cert->certificate, xpgp))
		{
			/* MAJOR ERROR */
			XPGP_free(xpgp);
			xpgpMtx.unlock(); /**** UNLOCK ****/
			return false;
		}

		/* transfer new signatures */
		XPGP_copy_known_signatures(pgp_keyring, cert->certificate, xpgp);
		XPGP_free(xpgp);

		/* we accepted it! */
		id = xpgpid;

		/* update signers */
		cert->signers = getXPGPsigners(cert->certificate);

		xpgpMtx.unlock(); /**** UNLOCK ****/
		return true;
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/

	/* if we get here -> its a new certificate */
	cert = new xpgpcert(xpgp, xpgpid);

	xpgpMtx.lock();   /***** LOCK *****/

	/* add to keyring */
	XPGP_add_certificate(pgp_keyring, cert->certificate);
	mCerts[xpgpid] = cert;	

        cert -> trustLvl = XPGP_auth_certificate(pgp_keyring, cert->certificate);
	if (cert -> trustLvl == TRUST_SIGN_TRSTED)
	{
		cert->trusted = true;
		cert->ownsign = true;
	}
	else if (cert->trustLvl == TRUST_SIGN_OWN)
	{
		cert->ownsign = true;
	}

	cert->signers = getXPGPsigners(xpgp);

	/* resave if new certificate */
	mToSaveCerts = true;
	xpgpMtx.unlock(); /**** UNLOCK ****/

#if 0
	/******************** notify of new Cert **************************/
	pqiNotify *pqinotify = getPqiNotify();
	if (pqinotify)
	{
		pqinotify->AddFeedItem(RS_FEED_ITEM_PEER_NEW, xpgpid, "","");
	}
	/******************** notify of new Cert **************************/
#endif

	id = xpgpid;

	return true;
}


bool getXPGPid(XPGP *xpgp, std::string &xpgpid)
{
#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::getXPGPid()";
	std::cerr << std::endl;
#endif

	xpgpid = "";
	if (xpgp == NULL)
	{
#ifdef XPGP_DEBUG
		std::cerr << "AuthXPGP::getXPGPid() NULL pointer";
		std::cerr << std::endl;
#endif
		return false;
	}

	// get the first signature....
	if (sk_XPGP_SIGNATURE_num(xpgp->signs) < 1)
	{
#ifdef XPGP_DEBUG
		std::cerr << "AuthXPGP::getXPGPid() ERROR: No Signature";
		std::cerr << std::endl;
#endif
		return false;
	}
	XPGP_SIGNATURE *xpgpsign = sk_XPGP_SIGNATURE_value(xpgp->signs, 0);

	// Validate that it is a self signature.
	// (Already Done - but not in this function)

	// get the signature from the cert, and copy to the array.
	ASN1_BIT_STRING *signature = xpgpsign->signature;
	int signlen = ASN1_STRING_length(signature);
	if (signlen < CERTSIGNLEN)
	{
#ifdef XPGP_DEBUG
		std::cerr << "AuthXPGP::getXPGPid() ERROR: Short Signature";
		std::cerr << std::endl;
#endif
		return false;
	}

	// else copy in the first CERTSIGNLEN.
	unsigned char *signdata = ASN1_STRING_data(signature);
	
        std::ostringstream id;
	for(uint32_t i = 0; i < CERTSIGNLEN; i++)
	{
		id << std::hex << std::setw(2) << std::setfill('0') 
			<< (uint16_t) (((uint8_t *) (signdata))[i]);
	}
	xpgpid = id.str();
	return true;
}



	/* validate + get id */
bool    AuthXPGP::ValidateCertificateXPGP(XPGP *xpgp, std::string &peerId)
{
	/* check self signed */
	if (!XPGP_check_valid_certificate(xpgp))
	{
		/* bad certificate */
		return false;
	}

	return getXPGPid(xpgp, peerId);
}

/* store for discovery */
bool    AuthXPGP::FailedCertificateXPGP(XPGP *xpgp, bool incoming)
{
	std::string id;
	return ProcessXPGP(xpgp, id);
}

/* check that they are exact match */
bool    AuthXPGP::CheckCertificateXPGP(std::string xpgpId, XPGP *xpgp)
{
	xpgpMtx.lock();   /***** LOCK *****/

	xpgpcert *cert = NULL;
	if (!locked_FindCert(xpgpId, &cert))
	{
		/* not there -> error */
		XPGP_free(xpgp);

		xpgpMtx.unlock(); /**** UNLOCK ****/
		return false;
	}
	else
	{
		/* have a duplicate */
		/* check that they are exact */
		if (0 != XPGP_cmp(cert->certificate, xpgp))
		{
			/* MAJOR ERROR */
			XPGP_free(xpgp);
			xpgpMtx.unlock(); /**** UNLOCK ****/
			return false;
		}

		/* transfer new signatures */
		XPGP_copy_known_signatures(pgp_keyring, cert->certificate, xpgp);
		XPGP_free(xpgp);

		/* update signers */
		cert->signers = getXPGPsigners(cert->certificate);

		xpgpMtx.unlock(); /**** UNLOCK ****/
		return true;
	}
}




/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

int pem_passwd_cb(char *buf, int size, int rwflag, void *password)
{
	strncpy(buf, (char *)(password), size);
	buf[size - 1] = '\0';
	return(strlen(buf));
}

// Not dependent on sslroot. load, and detroys the XPGP memory.

int	LoadCheckXPGPandGetName(const char *cert_file, std::string &userName, std::string &userId)
{
	/* This function loads the XPGP certificate from the file, 
	 * and checks the certificate 
	 */

	FILE *tmpfp = fopen(cert_file, "r");
	if (tmpfp == NULL)
	{
#ifdef XPGP_DEBUG
		std::cerr << "sslroot::LoadCheckAndGetXPGPName()";
		std::cerr << " Failed to open Certificate File:" << cert_file;
		std::cerr << std::endl;
#endif
		return 0;
	}

	// get xPGP certificate.
	XPGP *xpgp = PEM_read_XPGP(tmpfp, NULL, NULL, NULL);
	fclose(tmpfp);

	// check the certificate.
	bool valid = false;
	if (xpgp)
	{
		valid = XPGP_check_valid_certificate(xpgp);
	}

	if (valid)
	{
		// extract the name.
		userName = getX509CNString(xpgp->subject->subject);
	}

	if (!getXPGPid(xpgp, userId))
	{
		valid = false;
	}

	// clean up.
	XPGP_free(xpgp);

	if (valid)
	{
		// happy!
		return 1;
	}
	else
	{
		// something went wrong!
		return 0;
	}
}

std::string getX509NameString(X509_NAME *name)
{
	std::string namestr;
	for(int i = 0; i < X509_NAME_entry_count(name); i++)
	{
		X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, i);
		ASN1_STRING *entry_data = X509_NAME_ENTRY_get_data(entry);
		ASN1_OBJECT *entry_obj = X509_NAME_ENTRY_get_object(entry);

		namestr += "\t";
		namestr += OBJ_nid2ln(OBJ_obj2nid(entry_obj));
		namestr += " : ";

		//namestr += entry_obj -> flags;
		//namestr += entry_data -> length;
		//namestr += entry_data -> type;

		//namestr += entry_data -> flags;
		//entry -> set; 

		if (entry_data -> data != NULL)
		{
			namestr += (char *) entry_data -> data;
		}
		else
		{
			namestr += "NULL";
		}

		if (i + 1 < X509_NAME_entry_count(name))
		{
			namestr += "\n";
		}

	}
	return namestr;
}


std::string getX509CNString(X509_NAME *name)
{
	std::string namestr;
	for(int i = 0; i < X509_NAME_entry_count(name); i++)
	{
		X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, i);
		ASN1_STRING *entry_data = X509_NAME_ENTRY_get_data(entry);
		ASN1_OBJECT *entry_obj = X509_NAME_ENTRY_get_object(entry);

		if (0 == strncmp("CN", OBJ_nid2sn(OBJ_obj2nid(entry_obj)), 2))
		{
			if (entry_data -> data != NULL)
			{
				namestr += (char *) entry_data -> data;
			}
			else
			{
				namestr += "Unknown";
			}
			return namestr;
		}
	}
	return namestr;
}


std::string getX509TypeString(X509_NAME *name, const char *type, int len)
{
	std::string namestr;
	for(int i = 0; i < X509_NAME_entry_count(name); i++)
	{
		X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, i);
		ASN1_STRING *entry_data = X509_NAME_ENTRY_get_data(entry);
		ASN1_OBJECT *entry_obj = X509_NAME_ENTRY_get_object(entry);

		if (0 == strncmp(type, OBJ_nid2sn(OBJ_obj2nid(entry_obj)), len))
		{
			if (entry_data -> data != NULL)
			{
				namestr += (char *) entry_data -> data;
			}
			else
			{
				namestr += "Unknown";
			}
			return namestr;
		}
	}
	return namestr;
}

		
std::string getX509LocString(X509_NAME *name)
{
	return getX509TypeString(name, "L", 2);
}

std::string getX509OrgString(X509_NAME *name)
{
	return getX509TypeString(name, "O", 2);
}
	
		
std::string getX509CountryString(X509_NAME *name)
{
	return getX509TypeString(name, "C", 2);
}


std::string getXPGPInfo(XPGP *cert)
{
	std::stringstream out;
	long l;
	int i,j;

	out << "XPGP Certificate:" << std::endl;
	l=XPGP_get_version(cert);
	out << "     Version: " << l+1 << "(0x" << l << ")" << std::endl;
	out << "     Subject: " << std::endl;
	out << "  " << getX509NameString(cert -> subject -> subject);
	out << std::endl;
	out << std::endl;
	out << "     Signatures:" << std::endl;

	for(i = 0; i < sk_XPGP_SIGNATURE_num(cert->signs); i++)
	{
		out << "Sign[" << i << "] -> [";

		XPGP_SIGNATURE *sig = sk_XPGP_SIGNATURE_value(cert->signs,i);
	        ASN1_BIT_STRING *signature = sig->signature;
	        int signlen = ASN1_STRING_length(signature);
	        unsigned char *signdata = ASN1_STRING_data(signature);

		/* only show the first 8 bytes */
		if (signlen > 8)
			signlen = 8;
		for(j=0;j<signlen;j++)
		{
			out << std::hex << std::setw(2) << (int) (signdata[j]);
			if ((j+1)%16==0)
			{
				out << std::endl;
			}
			else
			{
				out << ":";
			}
		}
		out << "] by:";
		out << std::endl;
		out << getX509NameString(sig->issuer);
		out << std::endl;
		out << std::endl;
	}

	return out.str();
}



std::string getXPGPAuthCode(XPGP *xpgp)
{
	/* get the self signature -> the first signature */

	std::stringstream out;
	if (1 >  sk_XPGP_SIGNATURE_num(xpgp->signs))
	{
		out.str();
	}

	XPGP_SIGNATURE *sig = sk_XPGP_SIGNATURE_value(xpgp->signs,0);
	ASN1_BIT_STRING *signature = sig->signature;
	int signlen = ASN1_STRING_length(signature);
	unsigned char *signdata = ASN1_STRING_data(signature);

	/* extract the authcode from the signature */
	/* convert it to a string, inverse of 2 bytes of signdata */
	if (signlen > 2)
		signlen = 2;
	int j;
	for(j=0;j<signlen;j++)
	{
		out << std::hex << std::setprecision(2) << std::setw(2) 
		<< std::setfill('0') << (unsigned int) (signdata[j]);
	}
	return out.str();
}

std::list<std::string> getXPGPsigners(XPGP *cert)
{
	std::list<std::string> signers;
	int i;

	for(i = 0; i < sk_XPGP_SIGNATURE_num(cert->signs); i++)
	{
		XPGP_SIGNATURE *sig = sk_XPGP_SIGNATURE_value(cert->signs,i);
		std::string str = getX509CNString(sig->issuer);
		signers.push_back(str);
#ifdef XPGP_DEBUG
		std::cerr << "XPGPsigners(" << i << ")" << str << std::endl;
#endif
	}
	return signers;
}

// other fns
std::string getCertName(XPGP *xpgp)
{
	std::string name = xpgp->name;
	// strip out bad chars.
	for(int i = 0; i < (signed) name.length(); i++)
	{
		if ((name[i] == '/') || (name[i] == ' ') || (name[i] == '=') ||
			(name[i] == '\\') || (name[i] == '\t') || (name[i] == '\n'))
		{
			name[i] = '_';
		}
	}
	return name;
}
	

/********** SSL ERROR STUFF ******************************************/

int printSSLError(SSL *ssl, int retval, int err, unsigned long err2, 
		std::ostream &out)
{
	std::string reason;

	std::string mainreason = std::string("UNKNOWN ERROR CODE");
	if (err == SSL_ERROR_NONE)
	{
		mainreason =  std::string("SSL_ERROR_NONE");
	}
	else if (err == SSL_ERROR_ZERO_RETURN)
	{
		mainreason =  std::string("SSL_ERROR_ZERO_RETURN");
	}
	else if (err == SSL_ERROR_WANT_READ)
	{
		mainreason =  std::string("SSL_ERROR_WANT_READ");
	}
	else if (err == SSL_ERROR_WANT_WRITE)
	{
		mainreason =  std::string("SSL_ERROR_WANT_WRITE");
	}
	else if (err == SSL_ERROR_WANT_CONNECT)
	{
		mainreason =  std::string("SSL_ERROR_WANT_CONNECT");
	}
	else if (err == SSL_ERROR_WANT_ACCEPT)
	{
		mainreason =  std::string("SSL_ERROR_WANT_ACCEPT");
	}
	else if (err == SSL_ERROR_WANT_X509_LOOKUP)
	{
		mainreason =  std::string("SSL_ERROR_WANT_X509_LOOKUP");
	}
	else if (err == SSL_ERROR_SYSCALL)
	{
		mainreason =  std::string("SSL_ERROR_SYSCALL");
	}
	else if (err == SSL_ERROR_SSL)
	{
		mainreason =  std::string("SSL_ERROR_SSL");
	}
	out << "RetVal(" << retval;
	out << ") -> SSL Error: " << mainreason << std::endl;
	out << "\t + ERR Error: " << ERR_error_string(err2, NULL) << std::endl;
	return 1;
}


/***************************** OLD STORAGE of CERTS *************************
 * We will retain the existing CERT storage format for the moment....
 * This will enable the existing certs to be loaded in.
 *
 * BUT Save will change the format - removing the options from 
 * the configuration file. This will mean that we can catch NEW/OLD formats.
 *
 * We only want to load old format ONCE. as we'll use it to get 
 * the list of existing friends...
 *
 *
 *
 */

bool	AuthXPGP::FinalSaveCertificates()
{
	CheckSaveCertificates();

	RsStackMutex stack(xpgpMtx); /***** LOCK *****/
	mConfigSaveActive = false;
	return true;
}

bool	AuthXPGP::CheckSaveCertificates()
{
	xpgpMtx.lock();   /***** LOCK *****/

	if ((mConfigSaveActive) && (mToSaveCerts))
	{
		mToSaveCerts = false;
		xpgpMtx.unlock(); /**** UNLOCK ****/

		saveCertificates();
		return true;
	}

	xpgpMtx.unlock(); /**** UNLOCK ****/

	return false;
}

bool    AuthXPGP::saveCertificates()
{
	// construct file name.
	// create the file in memory - hash + sign.
	// write out data to a file.
	
	xpgpMtx.lock();   /***** LOCK *****/

	std::string configfile = mCertConfigFile;
	std::string neighdir = mNeighDir;

	xpgpMtx.unlock(); /**** UNLOCK ****/

	/* add on the slash */
	if (neighdir != "")
	{
		neighdir += "/";
	}

	std::map<std::string, std::string>::iterator mit;

	std::string conftxt;
	std::string empty("");
	unsigned int i;

#ifdef AUTHXPGP_DEBUG
	std::cerr << "AuthXPGP::saveCertificates()";
	std::cerr << std::endl;
#endif
	xpgpMtx.lock();   /***** LOCK *****/

	/* iterate through both lists */
	std::map<std::string, xpgpcert *>::iterator it;

	for(it = mCerts.begin(); it != mCerts.end(); it++)
	{
		if (it->second->trustLvl > TRUST_SIGN_BASIC)
		{
			XPGP *xpgp = it->second->certificate;
			std::string hash;
			std::string neighfile = neighdir + getCertName(xpgp) + ".pqi";

			if (saveXPGPToFile(xpgp, neighfile, hash))
			{
				conftxt += "CERT ";
				conftxt += getCertName(xpgp);
				conftxt += "\n";
				conftxt += hash;
				conftxt += "\n";
			}
		}
	}


	// now work out signature of it all. This relies on the 
	// EVP library of openSSL..... We are going to use signing
	// for the moment.

	unsigned int signlen = EVP_PKEY_size(pkey);
	unsigned char signature[signlen];

	//OpenSSL_add_all_digests();

	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	if (0 == EVP_SignInit_ex(mdctx, EVP_sha1(), NULL))
	{
#ifdef XPGP_DEBUG
		std::cerr << "EVP_SignInit Failure!" << std::endl;
#endif
	}

	if (0 == EVP_SignUpdate(mdctx, conftxt.c_str(), conftxt.length()))
	{
#ifdef XPGP_DEBUG
		std::cerr << "EVP_SignUpdate Failure!" << std::endl;
#endif
	}


	if (0 == EVP_SignFinal(mdctx, signature, &signlen, pkey))
	{
#ifdef XPGP_DEBUG
		std::cerr << "EVP_SignFinal Failure!" << std::endl;
#endif
	}

#ifdef XPGP_DEBUG
	std::cerr << "Conf Signature is(" << signlen << "): ";
#endif
	for(i = 0; i < signlen; i++) 
	{
#ifdef XPGP_DEBUG
		fprintf(stderr, "%02x", signature[i]);
#endif
		conftxt += signature[i];
	}
#ifdef XPGP_DEBUG
	std::cerr << std::endl;
#endif

	FILE *cfd = fopen(configfile.c_str(), "wb");
	int wrec;
	if (1 != (wrec = fwrite(conftxt.c_str(), conftxt.length(), 1, cfd)))
	{
#ifdef XPGP_DEBUG
		std::cerr << "Error writing: " << configfile << std::endl;
		std::cerr << "Wrote: " << wrec << "/" << 1 << " Records" << std::endl;
#endif
	}

	EVP_MD_CTX_destroy(mdctx);
	fclose(cfd);

	xpgpMtx.unlock(); /**** UNLOCK ****/

	return true;
}


/****** 
 * Special version for backwards compatibility
 *
 * has two extra parameters.
 * bool oldFormat & std::map<std::string, std::string> keyvaluemap
 *
 * We'll leave these in for the next couple of months...
 * so that old versions will automatically be converted to the 
 * new format!
 *
 */

bool    AuthXPGP::loadCertificates()
{
	bool oldFormat;
	std::map<std::string, std::string> keyValueMap;

	return loadCertificates(oldFormat, keyValueMap);
}

/*********************
 * NOTE no need to Lock here. locking handled in ProcessXPGP()
 */
static const uint32_t OPT_LEN = 16;
static const uint32_t VAL_LEN = 1000;

bool    AuthXPGP::loadCertificates(bool &oldFormat, std::map<std::string, std::string> &keyValueMap)
{

	/*******************************************
	 * open the configuration file.
	 * read in CERT + Hash.
	 *
	 * construct file name.
	 * create the file in memory - hash + sign.
	 * write out data to a file.
	 *****************************************/

	xpgpMtx.lock();   /***** LOCK *****/

	std::string configfile = mCertConfigFile;
	std::string neighdir = mNeighDir;

	xpgpMtx.unlock(); /**** UNLOCK ****/

	/* add on the slash */
	if (neighdir != "")
	{
		neighdir += "/";
	}

	oldFormat = false;
	
	std::string conftxt;

	unsigned int maxnamesize = 1024;
	char name[maxnamesize];

	int c;
	unsigned int i;

	FILE *cfd = fopen(configfile.c_str(), "rb");
	if (cfd == NULL)
	{
#ifdef XPGP_DEBUG
		std::cerr << "Unable to Load Configuration File!" << std::endl;
		std::cerr << "File: " << configfile << std::endl;
#endif
		return false;
	}

	std::list<std::string> fnames;
	std::list<std::string> hashes;
	std::map<std::string, std::string>::iterator mit;
	std::map<std::string, std::string> tmpsettings;

	unsigned int signlen = EVP_PKEY_size(pkey);
	unsigned char conf_signature[signlen];
	char *ret = NULL;

	for(ret = fgets(name, maxnamesize, cfd); 
			((ret != NULL) && (!strncmp(name, "CERT ", 5)));
				ret = fgets(name, maxnamesize, cfd))
	{
		for(i = 5; (name[i] != '\n') && (i < (unsigned) maxnamesize); i++);

		if (name[i] == '\n')
		{
			name[i] = '\0';
		}

		// so the name is first....
		std::string fname = &(name[5]);

		// now read the 
		std::string hash;
		std::string signature;

		for(i = 0; i < signlen; i++)
		{
			if (EOF == (c = fgetc(cfd)))
			{
#ifdef XPGP_DEBUG
				std::cerr << "Error Reading Signature of: ";
				std::cerr << fname;
				std::cerr << std::endl;
				std::cerr << "ABorting Load!";
				std::cerr << std::endl;
#endif
				return -1;
			}
			unsigned char uc = (unsigned char) c;
			signature += (unsigned char) uc;
		}
		if ('\n' != (c = fgetc(cfd)))
		{
#ifdef XPGP_DEBUG
			std::cerr << "Warning Mising seperator" << std::endl;
#endif
		}

#ifdef XPGP_DEBUG
		std::cerr << "Read fname:" << fname << std::endl;
		std::cerr << "Signature:" << std::endl;
		for(i = 0; i < signlen; i++) 
		{
			fprintf(stderr, "%02x", (unsigned char) signature[i]);
		}
		std::cerr << std::endl;
		std::cerr << std::endl;
#endif

		// push back.....
		fnames.push_back(fname);
		hashes.push_back(signature);

		conftxt += "CERT ";
		conftxt += fname;
		conftxt += "\n";
		conftxt += signature;
		conftxt += "\n";

		// be sure to write over a bit...
		name[0] = 'N';
		name[1] = 'O';
	}

	// string already waiting!
	for(; ((ret != NULL) && (!strncmp(name, "OPT ", 4)));
				ret = fgets(name, maxnamesize, cfd))
	{
		for(i = 4; (name[i] != '\n') && (i < OPT_LEN); i++);
		// terminate the string.
		name[i] = '\0';

		// so the name is first....
		std::string opt = &(name[4]);

		// now read the 
		std::string val;     // cleaned up value.
		std::string valsign; // value in the file.
		for(i = 0; i < VAL_LEN; i++)
		{
			if (EOF == (c = fgetc(cfd)))
			{
#ifdef XPGP_DEBUG
				std::cerr << "Error Reading Value of: ";
				std::cerr << opt;
				std::cerr << std::endl;
				std::cerr << "ABorting Load!";
				std::cerr << std::endl;
#endif
				return -1;
			}
			// remove zeros on strings...
			if (c != '\0')
			{
				val += (unsigned char) c;
			}
			valsign += (unsigned char) c;
		}
		if ('\n' != (c = fgetc(cfd)))
		{
#ifdef XPGP_DEBUG
			std::cerr << "Warning Mising seperator" << std::endl;
#endif
		}

#ifdef XPGP_DEBUG
		std::cerr << "Read OPT:" << opt;
		std::cerr << " Val:" << val << std::endl;
#endif

		// push back.....
		tmpsettings[opt] = val;

		conftxt += "OPT ";
		conftxt += opt;
		conftxt += "\n";
		conftxt += valsign;
		conftxt += "\n";

		// be sure to write over a bit...
		name[0] = 'N';
		name[1] = 'O';
	}

	// only read up to the first newline symbol....
	// continue...
	for(i = 0; (name[i] != '\n') && (i < signlen); i++);

	if (i != signlen)
	{
		for(i++; i < signlen; i++)
		{
			c = fgetc(cfd);
			if (c == EOF)
			{
#ifdef XPGP_DEBUG
				std::cerr << "Error Reading Conf Signature:";
				std::cerr << std::endl;
#endif
				return 1;
			}
			unsigned char uc = (unsigned char) c;
			name[i] = uc;
		}
	}

#ifdef XPGP_DEBUG
	std::cerr << "Configuration File Signature: " << std::endl;
	for(i = 0; i < signlen; i++) 
	{
		fprintf(stderr, "%02x", (unsigned char) name[i]);
	}
	std::cerr << std::endl;
#endif


	// when we get here - should have the final signature in the buffer.
	// check.
	//
	// compare signatures.
	// instead of verifying with the public key....
	// we'll sign it again - and compare .... FIX LATER...
	
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

	if (0 == EVP_SignInit(mdctx, EVP_sha1()))
	{
#ifdef XPGP_DEBUG
#endif
		std::cerr << "EVP_SignInit Failure!" << std::endl;
	}

	if (0 == EVP_SignUpdate(mdctx, conftxt.c_str(), conftxt.length()))
	{
#ifdef XPGP_DEBUG
		std::cerr << "EVP_SignUpdate Failure!" << std::endl;
#endif
	}

	if (0 == EVP_SignFinal(mdctx, conf_signature, &signlen, pkey))
	{
#ifdef XPGP_DEBUG
		std::cerr << "EVP_SignFinal Failure!" << std::endl;
#endif
	}

	EVP_MD_CTX_destroy(mdctx);
	fclose(cfd);

#ifdef XPGP_DEBUG
	std::cerr << "Recalced File Signature: " << std::endl;
	for(i = 0; i < signlen; i++) 
	{
		fprintf(stderr, "%02x", conf_signature[i]);
	}
	std::cerr << std::endl;
#endif

	bool same = true;
	for(i = 0; i < signlen; i++)
	{
		if ((unsigned char) name[i] != conf_signature[i])
		{
			same = false;
		}
	}

	if (same == false)
	{
#ifdef XPGP_DEBUG
		std::cerr << "ERROR VALIDATING CONFIGURATION!" << std::endl;
		std::cerr << "PLEASE FIX!" << std::endl;
#endif
		return false;
	}

	std::list<std::string>::iterator it;
	std::list<std::string>::iterator it2;
	for(it = fnames.begin(), it2 = hashes.begin(); it != fnames.end(); it++, it2++)
	{
		std::string neighfile = neighdir + (*it) + ".pqi";
		XPGP *xpgp = loadXPGPFromFile(neighfile, (*it2));
		if (xpgp != NULL)
		{
			std::string id;
			if (ProcessXPGP(xpgp, id))
			{
#ifdef XPGP_DEBUG
				std::cerr << "Loaded Certificate: " << id;
				std::cerr << std::endl;
#endif
			}
		}
	}
	for(mit = tmpsettings.begin(); mit != tmpsettings.end(); mit++)
	{
		keyValueMap[mit -> first] = mit -> second;
	}

	mToSaveCerts = false;

	if (keyValueMap.size() > 0)
	{
		oldFormat = true;
		mToSaveCerts = true;
	}

	return true;
}


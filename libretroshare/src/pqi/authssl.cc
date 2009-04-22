/*
 * libretroshare/src/pqi: authssl.cc
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
 *
 * This class is designed to provide authentication using ssl certificates
 * only. It is intended to be wrapped by an gpgauthmgr to provide
 * pgp + ssl web-of-trust authentication.
 *
 */

#include "authssl.h"
//#include "cleanupx509.h"

#include "pqinetwork.h"

/******************** notify of new Cert **************************/
#include "pqinotify.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

#include <sstream>
#include <iomanip>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/***********
 ** #define AUTHSSL_DEBUG	1
 **********/
#define AUTHSSL_DEBUG	1

// the single instance of this.
static AuthSSL instance_sslroot;

p3AuthMgr *getAuthMgr()
{
	return &instance_sslroot;
}


sslcert::sslcert(X509 *x509, std::string pid)
{
	certificate = x509;
	id = pid;
	name = getX509CNString(x509->cert_info->subject);
	org = getX509OrgString(x509->cert_info->subject);
	location = getX509LocString(x509->cert_info->subject);
	email = "";

	authed = false;
}


AuthSSL::AuthSSL()
	:init(0), sslctx(NULL), pkey(NULL), mToSaveCerts(false), mConfigSaveActive(true)
{
}

bool AuthSSL::active()
{
	return init;
}

// args: server cert, server private key, trusted certificates.

int	AuthSSL::InitAuth(const char *cert_file, const char *priv_key_file, 
			const char *passwd)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::InitAuth()";
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


	// actions_to_seed_PRNG();
	RAND_seed(passwd, strlen(passwd));

	std::cerr << "SSL Library Init!" << std::endl;

	// setup connection method
	sslctx = SSL_CTX_new(TLSv1_method());

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
	X509 *x509 = PEM_read_X509(ownfp, NULL, NULL, NULL);
	fclose(ownfp);

	if (x509 == NULL)
	{
		return -1;
	}
	SSL_CTX_use_certificate(sslctx, x509);

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
	SSL_CTX_use_PrivateKey(sslctx, pkey);

	if (1 != SSL_CTX_check_private_key(sslctx))
	{
		std::cerr << "Issues With Private Key! - Doesn't match your Cert" << std::endl;
		std::cerr << "Check your input key/certificate:" << std::endl;
		std::cerr << priv_key_file << " & " << cert_file;
		std::cerr << std::endl;
		CloseAuth();
		return -1;
	}

	if (!getX509id(x509, mOwnId))
	{
		/* bad certificate */
		CloseAuth();
		return -1;
	}

	/* Check that Certificate is Ok ( virtual function )
	 * for gpg/pgp or CA verification
	 */

  	validateOwnCertificate(x509, pkey);

	// enable verification of certificates (PEER)
	SSL_CTX_set_verify(sslctx, SSL_VERIFY_PEER | 
			SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

	std::cerr << "SSL Verification Set" << std::endl;

	mOwnCert = new sslcert(x509, mOwnId);

	init = 1;
	return 1;
}



bool	AuthSSL::CloseAuth()
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::CloseAuth()";
	std::cerr << std::endl;
#endif
	SSL_CTX_free(sslctx);

	// clean up private key....
	// remove certificates etc -> opposite of initssl.
	init = 0;
	return 1;
}

/* Context handling  */
SSL_CTX *AuthSSL::getCTX()
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::getCTX()";
	std::cerr << std::endl;
#endif
	return sslctx;
}

int     AuthSSL::setConfigDirectories(std::string configfile, std::string neighdir)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::setConfigDirectories()";
	std::cerr << std::endl;
#endif
	sslMtx.lock();   /***** LOCK *****/

	mCertConfigFile = configfile;
	mNeighDir = neighdir;

	sslMtx.unlock(); /**** UNLOCK ****/
	return 1;
}

/* no trust in SSL certs */	
bool AuthSSL::isTrustingMe(std::string id) 
{
	return false;
}
void AuthSSL::addTrustingPeer(std::string id)
{
	return;
}

std::string AuthSSL::OwnId()
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::OwnId()";
	std::cerr << std::endl;
#endif
	sslMtx.lock();   /***** LOCK *****/

	std::string id = mOwnId;

	sslMtx.unlock(); /**** UNLOCK ****/
	return id;
}

bool    AuthSSL::getAllList(std::list<std::string> &ids)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::getAllList()";
	std::cerr << std::endl;
#endif
	sslMtx.lock();   /***** LOCK *****/

	/* iterate through both lists */
	std::map<std::string, sslcert *>::iterator it;

	for(it = mCerts.begin(); it != mCerts.end(); it++)
	{
		ids.push_back(it->first);
	}

	sslMtx.unlock(); /**** UNLOCK ****/

	return true;
}

bool    AuthSSL::getAuthenticatedList(std::list<std::string> &ids)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::getAuthenticatedList()";
	std::cerr << std::endl;
#endif
	sslMtx.lock();   /***** LOCK *****/

	/* iterate through both lists */
	std::map<std::string, sslcert *>::iterator it;

	for(it = mCerts.begin(); it != mCerts.end(); it++)
	{
		if (it->second->authed)
		{
			ids.push_back(it->first);
		}
	}

	sslMtx.unlock(); /**** UNLOCK ****/

	return true;
}

bool    AuthSSL::getUnknownList(std::list<std::string> &ids)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::getUnknownList()";
	std::cerr << std::endl;
#endif
	sslMtx.lock();   /***** LOCK *****/

	/* iterate through both lists */
	std::map<std::string, sslcert *>::iterator it;

	for(it = mCerts.begin(); it != mCerts.end(); it++)
	{
		if (!it->second->authed)
		{
			ids.push_back(it->first);
		}
	}

	sslMtx.unlock(); /**** UNLOCK ****/

	return true;
}

	/* silly question really - only valid certs get saved to map
	 * so if in map its okay
	 */
bool    AuthSSL::isValid(std::string id)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::isValid() " << id;
	std::cerr << std::endl;
#endif
	sslMtx.lock();   /***** LOCK *****/
	bool valid = false;

	if (id == mOwnId)
	{
		valid = true;
	}
	else
	{
		valid = (mCerts.end() != mCerts.find(id));
	}

	sslMtx.unlock(); /**** UNLOCK ****/

	return valid;
}

bool    AuthSSL::isAuthenticated(std::string id)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::isAuthenticated() " << id;
	std::cerr << std::endl;
#endif
	sslMtx.lock();   /***** LOCK *****/

	sslcert *cert = NULL;
	bool auth = false;

	if (locked_FindCert(id, &cert))
	{
		auth = it->second->authed;
	}

	sslMtx.unlock(); /**** UNLOCK ****/

	return auth;
}

std::string AuthSSL::getName(std::string id)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::getName() " << id;
	std::cerr << std::endl;
#endif
	std::string name;

	sslMtx.lock();   /***** LOCK *****/

	sslcert *cert = NULL;
	if (id == mOwnId)
	{
		name = mOwnCert->name;
	}
	else if (locked_FindCert(id, &cert))
	{
		name = cert->name;
	}

	sslMtx.unlock(); /**** UNLOCK ****/

	return name;
}

bool    AuthSSL::getDetails(std::string id, pqiAuthDetails &details)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::getDetails() " << id;
	std::cerr << std::endl;
#endif
	sslMtx.lock();   /***** LOCK *****/

	bool valid = false;
	sslcert *cert = NULL;
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

	sslMtx.unlock(); /**** UNLOCK ****/

	return valid;
}
	
	
	/* Load/Save certificates */
	
bool AuthSSL::LoadCertificateFromString(std::string pem, std::string &id)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::LoadCertificateFromString() " << id;
	std::cerr << std::endl;
#endif

#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::LoadCertificateFromString() Cleaning up Certificate First!";
	std::cerr << std::endl;
#endif

	std::string cleancert = cleanUpX509Certificate(pem);

	X509 *x509 = loadX509FromPEM(cleancert);
	if (!x509)
		return false;

	return ProcessX509(x509, id);
}

std::string AuthSSL::SaveCertificateToString(std::string id)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::SaveCertificateToString() " << id;
	std::cerr << std::endl;
#endif


	sslMtx.lock();   /***** LOCK *****/

	/* get the cert first */
	std::string certstr;
	sslcert *cert = NULL;
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

		PEM_write_bio_X509(bp, cert->certificate);

		/* translate the bp data to a string */
		char *data;
		int len = BIO_get_mem_data(bp, &data);
		for(int i = 0; i < len; i++)
		{
			certstr += data[i];
		}

		BIO_free(bp);
	}

	sslMtx.unlock(); /**** UNLOCK ****/

	return certstr;
}



bool AuthSSL::LoadCertificateFromFile(std::string filename, std::string &id)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::LoadCertificateFromFile() " << id;
	std::cerr << std::endl;
#endif

	std::string nullhash;

	X509 *x509 = loadX509FromFile(filename.c_str(), nullhash);
	if (!x509)
		return false;

	return ProcessX509(x509, id);
}

//============================================================================

//! Saves something to filename

//! \returns true on success, false on failure
bool AuthSSL::SaveCertificateToFile(std::string id, std::string filename)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::SaveCertificateToFile() " << id;
	std::cerr << std::endl;
#endif

	sslMtx.lock();   /***** LOCK *****/

	/* get the cert first */
	sslcert *cert = NULL;
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
		valid = saveX509ToFile(cert->certificate, filename, hash);
	}

	sslMtx.unlock(); /**** UNLOCK ****/
	return valid;
}

	/**** To/From DER format ***/

bool 	AuthSSL::LoadCertificateFromBinary(const uint8_t *ptr, uint32_t len, std::string &id)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::LoadCertificateFromFile() " << id;
	std::cerr << std::endl;
#endif

	X509 *x509 = loadX509FromDER(ptr, len);
	if (!x509)
		return false;

	return ProcessX509(x509, id);

}

bool 	AuthSSL::SaveCertificateToBinary(std::string id, uint8_t **ptr, uint32_t *len)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::SaveCertificateToBinary() " << id;
	std::cerr << std::endl;
#endif

	sslMtx.lock();   /***** LOCK *****/

	/* get the cert first */
	sslcert *cert = NULL;
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
		valid = saveX509ToDER(cert->certificate, ptr, len);
	}

	sslMtx.unlock(); /**** UNLOCK ****/
	return valid;
}


	/* Signatures */
	/* NO Signatures in SSL Certificates */

bool AuthSSL::SignCertificate(std::string id)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::SignCertificate() NULL " << id;
	std::cerr << std::endl;
#endif
	bool valid = false;
	return valid;
}

bool AuthSSL::TrustCertificate(std::string id, bool totrust)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::TrustCertificate() NULL " << id;
	std::cerr << std::endl;
#endif
	bool valid = false;
	return valid;
}

bool AuthSSL::RevokeCertificate(std::string id)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::RevokeCertificate() NULL " << id;
	std::cerr << std::endl;
#endif

	sslMtx.lock();   /***** LOCK *****/
	sslMtx.unlock(); /**** UNLOCK ****/

	return false;
}


bool AuthSSL::AuthCertificate(std::string id)
{

#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::AuthCertificate() " << id;
	std::cerr << std::endl;
#endif

	sslMtx.lock();   /***** LOCK *****/

	/* get the cert first */
	sslcert *cert = NULL;
	sslcert *own = mOwnCert;
	bool valid = false;

	if (locked_FindCert(id, &cert))
	{
		cert->authed=true;
		mToSaveCerts = true;
	}

	sslMtx.unlock(); /**** UNLOCK ****/
	return valid;
}


	/* Sign / Encrypt / Verify Data (TODO) */
	
bool AuthSSL::SignData(std::string input, std::string &sign)
{
	return SignData(input.c_str(), input.length(), sign);
}

bool AuthSSL::SignData(const void *data, const uint32_t len, std::string &sign)
{

	RsStackMutex stack(sslMtx);   /***** STACK LOCK MUTEX *****/

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

	
bool AuthSSL::SignDataBin(std::string input, unsigned char *sign, unsigned int *signlen)
{
	return SignDataBin(input.c_str(), input.length(), sign, signlen);
}

bool AuthSSL::SignDataBin(const void *data, const uint32_t len, 
			unsigned char *sign, unsigned int *signlen)
{

	RsStackMutex stack(sslMtx);   /***** STACK LOCK MUTEX *****/

	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	unsigned int req_signlen = EVP_PKEY_size(pkey);
	if (req_signlen > *signlen)
	{
		/* not enough space */
		std::cerr << "SignDataBin() Not Enough Sign SpacegnInit Failure!" << std::endl;
		return false;
	}
	

		

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

	if (0 == EVP_SignFinal(mdctx, sign, signlen, pkey))
	{
		std::cerr << "EVP_SignFinal Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
		return false;
	}

	EVP_MD_CTX_destroy(mdctx);
	return true;
}


bool AuthSSL::VerifySignBin(std::string pid, 
			const void *data, const uint32_t len,
                       	unsigned char *sign, unsigned int signlen)
{
	RsStackMutex stack(sslMtx);   /***** STACK LOCK MUTEX *****/

	/* find the peer */
	
	sslcert *peer;
	if (pid == mOwnId)
	{
		peer = mOwnCert;
	}
	else if (!locked_FindCert(pid, &peer))
	{
		std::cerr << "VerifySignBin() no peer" << std::endl;
		return false;
	}

	EVP_PKEY *peerkey = peer->certificate->key->key->pkey;
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	
	if (0 == EVP_VerifyInit(mdctx, EVP_sha1()))
	{
		std::cerr << "EVP_VerifyInit Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
		return false;
	}

	if (0 == EVP_VerifyUpdate(mdctx, data, len))
	{
		std::cerr << "EVP_VerifyUpdate Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
		return false;
	}

	if (0 == EVP_VerifyFinal(mdctx, sign, signlen, peerkey))
	{
		std::cerr << "EVP_VerifyFinal Failure!" << std::endl;

		EVP_MD_CTX_destroy(mdctx);
		return false;
	}

	EVP_MD_CTX_destroy(mdctx);
	return true;
}






	/**** NEW functions we've added ****/


	/**** AUX Functions ****/
bool AuthSSL::locked_FindCert(std::string id, sslcert **cert)
{
	std::map<std::string, sslcert *>::iterator it;

	if (mCerts.end() != (it = mCerts.find(id)))
	{
		*cert = it->second;
		return true;
	}
	return false;
}


X509 *AuthSSL::loadX509FromFile(std::string fname, std::string hash)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::LoadX509FromFile()";
	std::cerr << std::endl;
#endif

	// if there is a hash - check that the file matches it before loading.
	X509 *pc = NULL;
	FILE *pcertfp = fopen(fname.c_str(), "rb");

	// load certificates from file.
	if (pcertfp == NULL)
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "sslroot::loadcertificate() Bad File: " << fname;
		std::cerr << " Cannot be Hashed!" << std::endl;
#endif
		return NULL;
	}

	/* We only check a signature's hash if
	 * we are loading from a configuration file.
	 * Therefore we saved the file and it should be identical. 
	 * and a direct load + verify will work.
	 *
	 * If however it has been transported by email....
	 * Then we might have to correct the data (strip out crap)
	 * from the configuration at the end. (X509 load should work!)
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
#ifdef AUTHSSL_DEBUG
			std::cerr << "Error Reading Peer Record!" << std::endl;
#endif
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
#ifdef AUTHSSL_DEBUG
				std::cerr << "Different Length Signatures... ";
				std::cerr << "Cannot Load Certificate!" << std::endl;
#endif
				fclose(pcertfp);
				return NULL;
		}

		for(int i = 0; i < (signed) signlen; i++) 
		{
			if (signature[i] != (unsigned char) hash[i])
			{
				same = false;
#ifdef AUTHSSL_DEBUG
				std::cerr << "Invalid Signature... ";
				std::cerr << "Cannot Load Certificate!" << std::endl;
#endif
				fclose(pcertfp);
				return NULL;
			}
		}
#ifdef AUTHSSL_DEBUG
		std::cerr << "Verified Signature for: " << fname;
		std::cerr << std::endl;
#endif
	}
	else
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "Not checking cert signature" << std::endl;
#endif
	}

	fseek(pcertfp, 0, SEEK_SET); /* rewind */
	pc = PEM_read_X509(pcertfp, NULL, NULL, NULL);
	fclose(pcertfp);

	if (pc != NULL)
	{
		// read a certificate.
#ifdef AUTHSSL_DEBUG
		std::cerr << "Loaded Certificate: " << pc -> name << std::endl;
#endif
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

bool  	AuthSSL::saveX509ToFile(X509 *x509, std::string fname, std::string &hash)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::saveX509ToFile()";
	std::cerr << std::endl;
#endif

	// load certificates from file.
	FILE *setfp = fopen(fname.c_str(), "wb");
	if (setfp == NULL)
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "sslroot::savecertificate() Bad File: " << fname;
		std::cerr << " Cannot be Written!" << std::endl;
#endif
		return false;
	}

#ifdef AUTHSSL_DEBUG
	std::cerr << "Writing out Cert...:" << x509->name << std::endl;
#endif
	PEM_write_X509(setfp, x509);

	fclose(setfp);

	// then reopen to generate hash.
	setfp = fopen(fname.c_str(), "rb");
	if (setfp == NULL)
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "sslroot::savecertificate() Bad File: " << fname;
		std::cerr << " Opened for ReHash!" << std::endl;
#endif
		return false;
	}

	unsigned int signlen = EVP_PKEY_size(pkey);
	unsigned char signature[signlen];

	int maxsize = 20480;
	int rbytes;
	char inall[maxsize];
	if (0 == (rbytes = fread(inall, 1, maxsize, setfp)))
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "Error Writing Peer Record!" << std::endl;
#endif
		return -1;
	}
#ifdef AUTHSSL_DEBUG
	std::cerr << "Read " << rbytes << std::endl;
#endif

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

#ifdef AUTHSSL_DEBUG
	std::cerr << "Saved Cert: " << x509->name;
	std::cerr << std::endl;
#endif

#ifdef AUTHSSL_DEBUG
	std::cerr << "Cert + Setting Signature is(" << signlen << "): ";
#endif
	std::string signstr;
	for(uint32_t i = 0; i < signlen; i++) 
	{
#ifdef AUTHSSL_DEBUG
		fprintf(stderr, "%02x", signature[i]);
#endif
		signstr += signature[i];
	}
#ifdef AUTHSSL_DEBUG
	std::cerr << std::endl;
#endif

	hash = signstr;
	fclose(setfp);

	EVP_MD_CTX_destroy(mdctx);

	return true;
}


X509 *AuthSSL::loadX509FromPEM(std::string pem)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::LoadX509FromPEM()";
	std::cerr << std::endl;
#endif

	/* Put the data into a mem BIO */
	char *certstr = strdup(pem.c_str());

	BIO *bp = BIO_new_mem_buf(certstr, -1);

	X509 *pc = PEM_read_bio_X509(bp, NULL, NULL, NULL);

	BIO_free(bp);
	free(certstr);

	return pc;
}

X509 *AuthSSL::loadX509FromDER(const uint8_t *ptr, uint32_t len)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::LoadX509FromDER()";
	std::cerr << std::endl;
#endif

        X509 *tmp = NULL;
        unsigned char **certptr = (unsigned char **) &ptr;
        X509 *x509 = d2i_X509(&tmp, certptr, len);

	return x509;
}

bool AuthSSL::saveX509ToDER(X509 *x509, uint8_t **ptr, uint32_t *len)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::saveX509ToDER()";
	std::cerr << std::endl;
#endif

	int certlen = i2d_X509(x509, (unsigned char **) ptr);
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




bool AuthSSL::ProcessX509(X509 *x509, std::string &id)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::ProcessX509()";
	std::cerr << std::endl;
#endif

	/* extract id */
	std::string xid;

	if (!X509_check_valid_certificate(x509))
	{
		/* bad certificate */
		X509_free(x509);
		return false;
	}
		
	if (!getX509id(x509, xid))
	{
		/* bad certificate */
		X509_free(x509);
		return false;
	}

	sslcert *cert = NULL;
	bool duplicate = false;

	sslMtx.lock();   /***** LOCK *****/

	if (xid == mOwnId)
	{
		cert = mOwnCert;
		duplicate = true;
	}
	else if (locked_FindCert(xid, &cert))
	{
		duplicate = true;
	}

	if (duplicate)
	{
		/* have a duplicate */
		/* check that they are exact */
		if (0 != X509_cmp(cert->certificate, x509))
		{
			/* MAJOR ERROR */
			X509_free(x509);
			sslMtx.unlock(); /**** UNLOCK ****/
			return false;
		}

		X509_free(x509);

		/* we accepted it! */
		id = xid;

		sslMtx.unlock(); /**** UNLOCK ****/
		return true;
	}

	sslMtx.unlock(); /**** UNLOCK ****/

	/* if we get here -> its a new certificate */
	cert = new sslcert(x509, xid);

	sslMtx.lock();   /***** LOCK *****/

	mCerts[xid] = cert;	

	/* resave if new certificate */
	mToSaveCerts = true;
	sslMtx.unlock(); /**** UNLOCK ****/

#if 0
	/******************** notify of new Cert **************************/
	pqiNotify *pqinotify = getPqiNotify();
	if (pqinotify)
	{
		pqinotify->AddFeedItem(RS_FEED_ITEM_PEER_NEW, xid, "","");
	}
	/******************** notify of new Cert **************************/
#endif

	id = xid;

	return true;
}


bool getX509id(X509 *x509, std::string &xid)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::getX509id()";
	std::cerr << std::endl;
#endif

	xid = "";
	if (x509 == NULL)
	{
#ifdef X509_DEBUG
		std::cerr << "AuthSSL::getX509id() NULL pointer";
		std::cerr << std::endl;
#endif
		return false;
	}

	// get the signature from the cert, and copy to the array.
	ASN1_BIT_STRING *signature = x509->signature;
	int signlen = ASN1_STRING_length(signature);
	if (signlen < CERTSIGNLEN)
	{
#ifdef X509_DEBUG
		std::cerr << "AuthSSL::getX509id() ERROR: Short Signature";
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
	xid = id.str();
	return true;
}



	/* validate + get id */
bool    AuthSSL::ValidateCertificateX509(X509 *x509, std::string &peerId)
{
	/* check self signed */
	if (!X509_check_valid_certificate(x509))
	{
		/* bad certificate */
		return false;
	}

	return getX509id(x509, peerId);
}

/* store for discovery */
bool    AuthSSL::FailedCertificateX509(X509 *x509, bool incoming)
{
	std::string id;
	return ProcessX509(x509, id);
}

/* check that they are exact match */
bool    AuthSSL::CheckCertificateX509(std::string x509Id, X509 *x509)
{
	sslMtx.lock();   /***** LOCK *****/

	sslcert *cert = NULL;
	if (!locked_FindCert(x509Id, &cert))
	{
		/* not there -> error */
		X509_free(x509);

		sslMtx.unlock(); /**** UNLOCK ****/
		return false;
	}
	else
	{
		/* have a duplicate */
		/* check that they are exact */
		if (0 != X509_cmp(cert->certificate, x509))
		{
			/* MAJOR ERROR */
			X509_free(x509);
			sslMtx.unlock(); /**** UNLOCK ****/
			return false;
		}

		/* transfer new signatures */
		X509_copy_known_signatures(pgp_keyring, cert->certificate, x509);
		X509_free(x509);

		/* update signers */
		cert->signers = getX509signers(cert->certificate);

		sslMtx.unlock(); /**** UNLOCK ****/
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

// Not dependent on sslroot. load, and detroys the X509 memory.

int	LoadCheckX509andGetName(const char *cert_file, std::string &userName, std::string &userId)
{
	/* This function loads the X509 certificate from the file, 
	 * and checks the certificate 
	 */

	FILE *tmpfp = fopen(cert_file, "r");
	if (tmpfp == NULL)
	{
#ifdef X509_DEBUG
		std::cerr << "sslroot::LoadCheckAndGetX509Name()";
		std::cerr << " Failed to open Certificate File:" << cert_file;
		std::cerr << std::endl;
#endif
		return 0;
	}

	// get xPGP certificate.
	X509 *x509 = PEM_read_X509(tmpfp, NULL, NULL, NULL);
	fclose(tmpfp);

	// check the certificate.
	bool valid = false;
	if (x509)
	{
		valid = X509_check_valid_certificate(x509);
	}

	if (valid)
	{
		// extract the name.
		userName = getX509CNString(x509->subject->subject);
	}

	if (!getX509id(x509, userId))
	{
		valid = false;
	}

	std::cout << getX509Info(x509) << std::endl ;
	// clean up.
	X509_free(x509);

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


std::string getX509Info(X509 *cert)
{
	std::stringstream out;
	long l;
	int i,j;

	out << "X509 Certificate:" << std::endl;
	l=X509_get_version(cert);
	out << "     Version: " << l+1 << "(0x" << l << ")" << std::endl;
	out << "     Subject: " << std::endl;
	out << "  " << getX509NameString(cert -> subject -> subject);
	out << std::endl;
	out << std::endl;
	out << "     Signatures:" << std::endl;
	return out.str();
}



std::string getX509AuthCode(X509 *x509)
{
	/* get the self signature -> the first signature */

	std::stringstream out;

	ASN1_BIT_STRING *signature = x509->signature;
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

// other fns
#if 0
std::string getCertName(X509 *x509)
{
	std::string name = x509->name;
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

#endif
	

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

bool	AuthSSL::FinalSaveCertificates()
{
	CheckSaveCertificates();

	RsStackMutex stack(sslMtx); /***** LOCK *****/
	mConfigSaveActive = false;
	return true;
}

bool	AuthSSL::CheckSaveCertificates()
{
	sslMtx.lock();   /***** LOCK *****/

	if ((mConfigSaveActive) && (mToSaveCerts))
	{
		mToSaveCerts = false;
		sslMtx.unlock(); /**** UNLOCK ****/

		saveCertificates();
		return true;
	}

	sslMtx.unlock(); /**** UNLOCK ****/

	return false;
}

bool    AuthSSL::saveCertificates()
{
	// construct file name.
	// create the file in memory - hash + sign.
	// write out data to a file.
	
	sslMtx.lock();   /***** LOCK *****/

	std::string configfile = mCertConfigFile;
	std::string neighdir = mNeighDir;

	sslMtx.unlock(); /**** UNLOCK ****/

	/* add on the slash */
	if (neighdir != "")
	{
		neighdir += "/";
	}

	std::map<std::string, std::string>::iterator mit;

	std::string conftxt;
	std::string empty("");
	unsigned int i;

#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::saveCertificates()";
	std::cerr << std::endl;
#endif
	sslMtx.lock();   /***** LOCK *****/

	/* iterate through both lists */
	std::map<std::string, sslcert *>::iterator it;

	for(it = mCerts.begin(); it != mCerts.end(); it++)
	{
		if (it->second->trustLvl > TRUST_SIGN_BASIC)
		{
			X509 *x509 = it->second->certificate;
			std::string hash;
			std::string neighfile = neighdir + getCertName(x509) + ".pqi";

			if (saveX509ToFile(x509, neighfile, hash))
			{
				conftxt += "CERT ";
				conftxt += getCertName(x509);
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
#ifdef X509_DEBUG
		std::cerr << "EVP_SignInit Failure!" << std::endl;
#endif
	}

	if (0 == EVP_SignUpdate(mdctx, conftxt.c_str(), conftxt.length()))
	{
#ifdef X509_DEBUG
		std::cerr << "EVP_SignUpdate Failure!" << std::endl;
#endif
	}


	if (0 == EVP_SignFinal(mdctx, signature, &signlen, pkey))
	{
#ifdef X509_DEBUG
		std::cerr << "EVP_SignFinal Failure!" << std::endl;
#endif
	}

#ifdef X509_DEBUG
	std::cerr << "Conf Signature is(" << signlen << "): ";
#endif
	for(i = 0; i < signlen; i++) 
	{
#ifdef X509_DEBUG
		fprintf(stderr, "%02x", signature[i]);
#endif
		conftxt += signature[i];
	}
#ifdef X509_DEBUG
	std::cerr << std::endl;
#endif

	FILE *cfd = fopen(configfile.c_str(), "wb");
	int wrec;
	if (1 != (wrec = fwrite(conftxt.c_str(), conftxt.length(), 1, cfd)))
	{
#ifdef X509_DEBUG
		std::cerr << "Error writing: " << configfile << std::endl;
		std::cerr << "Wrote: " << wrec << "/" << 1 << " Records" << std::endl;
#endif
	}

	EVP_MD_CTX_destroy(mdctx);
	fclose(cfd);

	sslMtx.unlock(); /**** UNLOCK ****/

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

bool    AuthSSL::loadCertificates()
{
	bool oldFormat;
	std::map<std::string, std::string> keyValueMap;

	return loadCertificates(oldFormat, keyValueMap);
}

/*********************
 * NOTE no need to Lock here. locking handled in ProcessX509()
 */
static const uint32_t OPT_LEN = 16;
static const uint32_t VAL_LEN = 1000;

bool    AuthSSL::loadCertificates(bool &oldFormat, std::map<std::string, std::string> &keyValueMap)
{

	/*******************************************
	 * open the configuration file.
	 * read in CERT + Hash.
	 *
	 * construct file name.
	 * create the file in memory - hash + sign.
	 * write out data to a file.
	 *****************************************/

	sslMtx.lock();   /***** LOCK *****/

	std::string configfile = mCertConfigFile;
	std::string neighdir = mNeighDir;

	sslMtx.unlock(); /**** UNLOCK ****/

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
#ifdef X509_DEBUG
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
#ifdef X509_DEBUG
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
#ifdef X509_DEBUG
			std::cerr << "Warning Mising seperator" << std::endl;
#endif
		}

#ifdef X509_DEBUG
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
#ifdef X509_DEBUG
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
#ifdef X509_DEBUG
			std::cerr << "Warning Mising seperator" << std::endl;
#endif
		}

#ifdef X509_DEBUG
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
#ifdef X509_DEBUG
				std::cerr << "Error Reading Conf Signature:";
				std::cerr << std::endl;
#endif
				return 1;
			}
			unsigned char uc = (unsigned char) c;
			name[i] = uc;
		}
	}

#ifdef X509_DEBUG
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
#ifdef X509_DEBUG
#endif
		std::cerr << "EVP_SignInit Failure!" << std::endl;
	}

	if (0 == EVP_SignUpdate(mdctx, conftxt.c_str(), conftxt.length()))
	{
#ifdef X509_DEBUG
		std::cerr << "EVP_SignUpdate Failure!" << std::endl;
#endif
	}

	if (0 == EVP_SignFinal(mdctx, conf_signature, &signlen, pkey))
	{
#ifdef X509_DEBUG
		std::cerr << "EVP_SignFinal Failure!" << std::endl;
#endif
	}

	EVP_MD_CTX_destroy(mdctx);
	fclose(cfd);

#ifdef X509_DEBUG
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
#ifdef X509_DEBUG
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
		X509 *x509 = loadX509FromFile(neighfile, (*it2));
		if (x509 != NULL)
		{
			std::string id;
			if (ProcessX509(x509, id))
			{
#ifdef X509_DEBUG
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


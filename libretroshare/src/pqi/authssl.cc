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
#include "cleanupxpgp.h"

#include "pqinetwork.h"
#include "authgpg.h"

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

// initialisation du pointeur de singleton à zéro
AuthSSL *AuthSSL::instance_ssl = new AuthSSL();

sslcert::sslcert(X509 *x509, std::string pid)
{
	certificate = x509;
	id = pid;
	name = getX509CNString(x509->cert_info->subject);
	org = getX509OrgString(x509->cert_info->subject);
	location = getX509LocString(x509->cert_info->subject);
	email = "";

	issuer = getX509CNString(x509->cert_info->issuer);

	authed = false;
}

sslcert::sslcert()
{
        email = "";
        authed = false;
}

X509_REQ *GenerateX509Req(
		std::string pkey_file, std::string passwd,
		std::string name, std::string email, std::string org, 
		std::string loc, std::string state, std::string country, 
		int nbits_in, std::string &errString)
{
	/* generate request */
	X509_REQ *req=X509_REQ_new();

        // setup output.
        BIO *bio_out = NULL;
        bio_out = BIO_new(BIO_s_file());
        BIO_set_fp(bio_out,stdout,BIO_NOCLOSE);

        EVP_PKEY *pkey = NULL;

        // first generate a key....
        if ((pkey=EVP_PKEY_new()) == NULL)
        {
                fprintf(stderr,"GenerateX509Req: Couldn't Create Key\n");
                return 0;
        }

        int nbits = 2048;
        unsigned long e = 0x10001;

        if ((nbits_in >= 512) && (nbits_in <= 4096))
        {
                nbits = nbits_in;
        }
        else
        {
                fprintf(stderr,"GenerateX509Req: strange num of nbits: %d\n", nbits_in);
                fprintf(stderr,"GenerateX509Req: reverting to %d\n", nbits);
        }


        RSA *rsa = RSA_generate_key(nbits, e, NULL, NULL);
        if ((rsa == NULL) || !EVP_PKEY_assign_RSA(pkey, rsa))
        {
                if(rsa) RSA_free(rsa);
                fprintf(stderr,"GenerateX509Req: Couldn't Generate RSA Key!\n");
                return 0;
        }


        // open the file.
        FILE *out;
        if (NULL == (out = fopen(pkey_file.c_str(), "w")))
        {
                fprintf(stderr,"GenerateX509Req: Couldn't Create Key File!");
                fprintf(stderr," : %s\n", pkey_file.c_str());
                return 0;
        }

        const EVP_CIPHER *cipher = EVP_des_ede3_cbc();

        if (!PEM_write_PrivateKey(out,pkey,cipher,
                        NULL,0,NULL,(void *) passwd.c_str()))
        {
                fprintf(stderr,"GenerateX509Req() Couldn't Save Private Key");
                fprintf(stderr," : %s\n", pkey_file.c_str());
                return 0;
        }
        fclose(out);

        // We have now created a private key....
        fprintf(stderr,"GenerateX509Req() Saved Private Key");
        fprintf(stderr," : %s\n", pkey_file.c_str());

        /********** Test Loading the private Key.... ************/
        FILE *tst_in = NULL;
        EVP_PKEY *tst_pkey = NULL;
        if (NULL == (tst_in = fopen(pkey_file.c_str(), "rb")))
        {
                fprintf(stderr,"GenerateX509Req() Couldn't Open Private Key");
                fprintf(stderr," : %s\n", pkey_file.c_str());
                return 0;
        }

        if (NULL == (tst_pkey =
                PEM_read_PrivateKey(tst_in,NULL,NULL,(void *) passwd.c_str())))
        {
                fprintf(stderr,"GenerateX509Req() Couldn't Read Private Key");
                fprintf(stderr," : %s\n", pkey_file.c_str());
                return 0;
        }
        fclose(tst_in);
        EVP_PKEY_free(tst_pkey);
        /********** Test Loading the private Key.... ************/

	/* Fill in details: fields. 
	req->req_info;
	req->req_info->enc;
	req->req_info->version;
	req->req_info->subject;
	req->req_info->pubkey;
	 ****************************/

	long version = 0x00;
        unsigned long chtype = MBSTRING_ASC;
	X509_NAME *x509_name = X509_NAME_new();

        // fill in the request.

        /**** X509_REQ -> Version ********************************/
        if (!X509_REQ_set_version(req,version)) /* version 1 */
        {
                fprintf(stderr,"GenerateX509Req(): Couldn't Set Version!\n");
                return 0;
        }
        /**** X509_REQ -> Version ********************************/
	/**** X509_REQ -> Key     ********************************/

	if (!X509_REQ_set_pubkey(req,pkey)) 
	{
		fprintf(stderr,"GenerateX509Req() Couldn't Set PUBKEY Version!\n");
		return 0;
	}

	/**** SUBJECT         ********************************/
        // create the name.

        // fields to add.
        // commonName CN
        // emailAddress (none)
        // organizationName O
        // localityName L
        // stateOrProvinceName ST
        // countryName C

        if (0 < strlen(name.c_str()))
        {
                X509_NAME_add_entry_by_txt(x509_name, "CN", chtype,
                        (unsigned char *) name.c_str(), -1, -1, 0);
        }
        else
        {
                fprintf(stderr,"GenerateX509Req(): No Name -> Not creating X509 Cert Req\n");
                return 0;
        }

	if (0 < strlen(email.c_str()))
	{
		//X509_NAME_add_entry_by_txt(x509_name, "Email", 0, 
		//  (unsigned char *) ui -> gen_email -> value(), -1, -1, 0);
		X509_NAME_add_entry_by_NID(x509_name, 48, 0, 
			(unsigned char *) email.c_str(), -1, -1, 0);
	}

	if (0 < strlen(org.c_str()))
	{
		X509_NAME_add_entry_by_txt(x509_name, "O", chtype, 
			(unsigned char *) org.c_str(), -1, -1, 0);
	}

	if (0 < strlen(loc.c_str()))
	{
		X509_NAME_add_entry_by_txt(x509_name, "L", chtype, 
			(unsigned char *) loc.c_str(), -1, -1, 0);
	}

	if (0 < strlen(state.c_str()))
	{
		X509_NAME_add_entry_by_txt(x509_name, "ST", chtype, 
			(unsigned char *) state.c_str(), -1, -1, 0);
	}

	if (0 < strlen(country.c_str()))
	{
		X509_NAME_add_entry_by_txt(x509_name, "C", chtype, 
			(unsigned char *) country.c_str(), -1, -1, 0);
	}

	if (!X509_REQ_set_subject_name(req,x509_name))
	{
		fprintf(stderr,"GenerateX509Req() Couldn't Set Name to Request!\n");
		X509_NAME_free(x509_name);
		return 0;
	}

	X509_NAME_free(x509_name);
	/**** SUBJECT         ********************************/

	if (!X509_REQ_sign(req,pkey,EVP_sha1()))
	{
		fprintf(stderr,"GenerateX509Req() Failed to Sign REQ\n");
		return 0;
	}

	return req;
}

#define SERIAL_RAND_BITS 	64

X509 *SignX509Certificate(X509_NAME *issuer, EVP_PKEY *privkey, X509_REQ *req, long days)
{
	const EVP_MD *digest = EVP_sha1();
	ASN1_INTEGER *serial = ASN1_INTEGER_new();
	EVP_PKEY *tmppkey;
	X509 *x509 = X509_new();
	if (x509 == NULL)
		return NULL;

        BIGNUM *btmp = BN_new();
        if (!BN_pseudo_rand(btmp, SERIAL_RAND_BITS, 0, 0))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," pseudo_rand\n");

		return NULL;
	}
        if (!BN_to_ASN1_INTEGER(btmp, serial))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," int\n");

		return NULL;
	}
        BN_free(btmp);

	if (!X509_set_serialNumber(x509, serial)) 
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," serialNumber\n");

		return NULL;
	}
	ASN1_INTEGER_free(serial);

	if (!X509_set_issuer_name(x509, issuer))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," issuer\n");

		return NULL;
	}

        if (!X509_gmtime_adj(x509->cert_info->validity->notBefore, 0))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," notBefore\n");

		return NULL;
	}

	//x509->cert_info->validity->notAfter
        //if (!X509_gmtime_adj(X509_get_notAfter(x509), (long)60*60*24*days))
        if (!X509_gmtime_adj(x509->cert_info->validity->notAfter, (long)60*60*24*days))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," notAfter\n");

		return NULL;
	}

        if (!X509_set_subject_name(x509, X509_REQ_get_subject_name(req)))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," subject_name\n");

		return NULL;
	}


        tmppkey = X509_REQ_get_pubkey(req);
        if (!tmppkey || !X509_set_pubkey(x509,tmppkey))
	{
		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," pubkey\n");

		return NULL;
	}


	/* Cleanup Algorithm part */

        X509_ALGOR *algor1 = x509->cert_info->signature;
        X509_ALGOR *algor2 = x509->sig_alg;

        X509_ALGOR *a;

        a = algor1;
        ASN1_TYPE_free(a->parameter);
        a->parameter=ASN1_TYPE_new();
        a->parameter->type=V_ASN1_NULL;

        ASN1_OBJECT_free(a->algorithm);
        a->algorithm=OBJ_nid2obj(digest->pkey_type);

        a = algor2;
        ASN1_TYPE_free(a->parameter);
        a->parameter=ASN1_TYPE_new();
        a->parameter->type=V_ASN1_NULL;

        ASN1_OBJECT_free(a->algorithm);
        a->algorithm=OBJ_nid2obj(digest->pkey_type);
  

        if (!X509_sign(x509,privkey,digest))
	{
		long e = ERR_get_error();

		fprintf(stderr,"SignX509Certificate() Failed: ");
		fprintf(stderr," signing Error: %ld\n", e);

		fprintf(stderr,"ERR: %s, %s, %s\n", 
			ERR_lib_error_string(e),
			ERR_func_error_string(e),
			ERR_reason_error_string(e));

        	int inl=i2d_X509(x509,NULL);
        	int outl=EVP_PKEY_size(privkey);
		fprintf(stderr,"Size Check: inl: %d, outl: %d\n", inl, outl);

		return NULL;
	}

	fprintf(stderr,"SignX509Certificate() Success\n");

	return x509;
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

static int verify_x509_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
#ifdef AUTHSSL_DEBUG
        std::cerr << "static verify_x509_callback called.";
        std::cerr << std::endl;
#endif
        return AuthSSL::getAuthSSL()->VerifyX509Callback(preverify_ok, ctx);

}

int	AuthSSL::InitAuth(const char *cert_file, const char *priv_key_file,
			const char *passwd)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::InitAuth()";
	std::cerr << std::endl;
#endif
	if ( passwd == NULL || strlen(passwd) == 0) {
	    std::cerr << "Warning : AuthSSL::InitAuth passwd empty." << std::endl;
	}

static  int initLib = 0;
	if (!initLib)
	{
		initLib = 1;
		SSL_load_error_strings();
		SSL_library_init();
	}


	if (init == 1)
	{
                std::cerr << "AuthSSL::InitAuth already initialized." << std::endl;
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
                std::cerr << "AuthSSL::InitAuth() PEM_read_X509() Failed";
		std::cerr << std::endl;
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
                std::cerr << "AuthSSL::InitAuth() PEM_read_PrivateKey() Failed";
		std::cerr << std::endl;
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
		std::cerr << "AuthSSL::InitAuth() getX509id() Failed";
		std::cerr << std::endl;

		/* bad certificate */
		CloseAuth();
		return -1;
	}

	/* Check that Certificate is Ok ( virtual function )
	 * for gpg/pgp or CA verification
	 */

	if (!validateOwnCertificate(x509, pkey))
	{
		std::cerr << "AuthSSL::InitAuth() validateOwnCertificate() Failed";
		std::cerr << std::endl;

		/* bad certificate */
		CloseAuth();
		return -1;
	}


	// enable verification of certificates (PEER)
	// and install verify callback.
	SSL_CTX_set_verify(sslctx, SSL_VERIFY_PEER | 
			SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 
				verify_x509_callback);

	std::cerr << "SSL Verification Set" << std::endl;

	mOwnCert = new sslcert(x509, mOwnId);

	init = 1;
	return 1;
}

/* Dummy function to be overloaded by real implementation */
bool	AuthSSL::validateOwnCertificate(X509 *x509, EVP_PKEY *pkey)
{
	return true;
	//return false;
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
	std::cerr << "AuthSSL::setConfigDirectories() ";
	std::cerr << " configfile: " << configfile;
	std::cerr << " neighdir: " << neighdir;
	std::cerr << std::endl;
#endif
	sslMtx.lock();   /***** LOCK *****/

	mCertConfigFile = configfile;
	mNeighDir = neighdir;

	sslMtx.unlock(); /**** UNLOCK ****/
	return 1;
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

bool    AuthSSL::getSSLChildListOfGPGId(std::string gpg_id, std::list<std::string> &ids)
{
#ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSL::getChildListOfGPGId() called for gpg id : " << gpg_id << std::endl;
#endif
        sslMtx.lock();   /***** LOCK *****/

        /* iterate through both lists */
        std::map<std::string, sslcert *>::iterator it;

        for(it = mCerts.begin(); it != mCerts.end(); it++)
        {
#ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSL::getChildListOfGPGId() it->second->authed : " << it->second->authed << "; it->second->issuer : " << it->second->issuer << std::endl;
#endif
                if (it->second->authed && it->second->issuer == gpg_id)
                {
                        ids.push_back(it->first);
                }
        }

        sslMtx.unlock(); /**** UNLOCK ****/

        return true;
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
		auth = cert->authed;
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

std::string AuthSSL::getIssuerName(std::string id)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::getIssuerName() " << id;
	std::cerr << std::endl;
#endif
	std::string issuer;

	sslMtx.lock();   /***** LOCK *****/

	sslcert *cert = NULL;
	if (id == mOwnId)
	{
		issuer = mOwnCert->issuer;
	}
	else if (locked_FindCert(id, &cert))
	{
		issuer = cert->issuer;
	}

	sslMtx.unlock(); /**** UNLOCK ****/

#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::getIssuerName() => " << issuer;
	std::cerr << std::endl;
#endif

	return issuer;
}

GPG_id AuthSSL::getGPGId(SSL_id id) {
    return getIssuerName(id);
}

bool    AuthSSL::getCertDetails(SSL_id id, sslcert &cert)
{
#ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSL::getCertDetails() \"" << id << "\"";
	std::cerr << std::endl;
#endif
	sslMtx.lock();   /***** LOCK *****/

	bool valid = false;
        sslcert *tcert = NULL;
        if (id == mOwnId) {
                cert.authed = mOwnCert->authed;
                cert.certificate = mOwnCert->certificate;
                cert.email = mOwnCert->email;
                cert.fpr = mOwnCert->fpr;
                cert.id = mOwnCert->id;
                cert.issuer = mOwnCert->issuer;
                cert.location = mOwnCert->location;
                cert.name = mOwnCert->name;
                cert.org = mOwnCert->org;
                cert.signers = mOwnCert->signers;
                valid = true;
        } else if (locked_FindCert(id, &tcert))	{
                cert.authed = tcert->authed;
                cert.certificate = tcert->certificate;
                cert.email = tcert->email;
                cert.fpr = tcert->fpr;
                cert.id = tcert->id;
                cert.issuer = tcert->issuer;
                cert.location = tcert->location;
                cert.name = tcert->name;
                cert.org = tcert->org;
                cert.signers = tcert->signers;
                valid = true;
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

	std::string cleancert = cleanUpCertificate(pem);

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

	EVP_PKEY *peerkey = peer->certificate->cert_info->key->pkey;

	if(peerkey == NULL)
		return false ;

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

	if(signlen == 0 || sign == NULL)
	{
		EVP_MD_CTX_destroy(mdctx);
		return false ;
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
        const unsigned char **certptr = (const unsigned char **) &ptr;
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

	bool valid = ValidateCertificate(x509, xid);

	if (!valid)
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSL::ProcessX509() ValidateCertificate FAILED";
                std::cerr << std::endl;
#endif
	}

	sslcert *cert = NULL;
	bool duplicate = false;

	sslMtx.lock();   /***** LOCK *****/

	if (xid == mOwnId)
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSL::ProcessX509() Cert is own id (dup)";
		std::cerr << std::endl;
#endif

		cert = mOwnCert;
		duplicate = true;
	}
	else if (locked_FindCert(xid, &cert))
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSL::ProcessX509() Found Duplicate";
		std::cerr << std::endl;
#endif

		duplicate = true;
	}

	if (duplicate)
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSL::ProcessX509() Processing as dup";
		std::cerr << std::endl;
#endif

		/* have a duplicate */
		/* check that they are exact */
		if (0 != X509_cmp(cert->certificate, x509))
		{

#ifdef AUTHSSL_DEBUG
			std::cerr << "AuthSSL::ProcessX509() Not the same: MAJOR ERROR";
			std::cerr << std::endl;
#endif

			/* MAJOR ERROR */
			X509_free(x509);
			sslMtx.unlock(); /**** UNLOCK ****/
			return false;
		}

		X509_free(x509);

		/* we accepted it! */
		id = xid;

		if (!cert->authed)
		{
			cert->authed = valid;

			/* resave newly authed certificate */
			mToSaveCerts = true;

#ifdef AUTHSSL_DEBUG
			std::cerr << "AuthSSL::ProcessX509() ";
			std::cerr << "Updating Unauthed duplicate: ";
			std::cerr << (valid ? "true" : "false");
			std::cerr << std::endl;
#endif
		}
		else
		{
#ifdef AUTHSSL_DEBUG
			std::cerr << "AuthSSL::ProcessX509() ";
			std::cerr << "Original already Valid";
			std::cerr << std::endl;
#endif
		}

#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSL::ProcessX509() Accepted Dup";
		std::cerr << std::endl;
#endif


		sslMtx.unlock(); /**** UNLOCK ****/
		return true;
	}

	sslMtx.unlock(); /**** UNLOCK ****/

	/* if we get here -> its a new certificate */
	cert = new sslcert(x509, xid);
	cert->authed = valid;

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

#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::ProcessX509() Accepted New Cert";
	std::cerr << std::endl;
#endif
	return true;
}


bool getX509id(X509 *x509, std::string &xid) {
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL::getX509id()";
	std::cerr << std::endl;
#endif

	xid = "";
	if (x509 == NULL)
	{
#ifdef AUTHSSL_DEBUG
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
#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSL::getX509id() ERROR: Short Signature";
		std::cerr << std::endl;
#endif
		return false;
	}

	// else copy in the first CERTSIGNLEN.
	unsigned char *signdata = ASN1_STRING_data(signature);
	
        std::ostringstream id;
	/* switched to the other end of the signature. for
	 * more randomness
	 */
	for(uint32_t i = signlen - CERTSIGNLEN; i < signlen; i++)
	{
		id << std::hex << std::setw(2) << std::setfill('0') 
			<< (uint16_t) (((uint8_t *) (signdata))[i]);
	}
	xid = id.str();
	return true;
}

X509 *AuthSSL::SignX509Req(X509_REQ *req, long days)
{
        RsStackMutex stack(sslMtx); /******* LOCKED ******/

        /* Transform the X509_REQ into a suitable format to
         * generate DIGEST hash. (for SSL to do grunt work)
         */


#define SERIAL_RAND_BITS 64

        const EVP_MD *digest = EVP_sha1();
        ASN1_INTEGER *serial = ASN1_INTEGER_new();
        EVP_PKEY *tmppkey;
        X509 *x509 = X509_new();
        if (x509 == NULL)
        {
                std::cerr << "AuthSSL::SignX509Req() FAIL" << std::endl;
                return NULL;
        }

        long version = 0x00;
        unsigned long chtype = MBSTRING_ASC;
        X509_NAME *issuer_name = X509_NAME_new();
        X509_NAME_add_entry_by_txt(issuer_name, "CN", chtype,
                        (unsigned char *) AuthGPG::getAuthGPG()->PGPOwnId().c_str(), -1, -1, 0);
/****
        X509_NAME_add_entry_by_NID(issuer_name, 48, 0,
                        (unsigned char *) "email@email.com", -1, -1, 0);
        X509_NAME_add_entry_by_txt(issuer_name, "O", chtype,
                        (unsigned char *) "org", -1, -1, 0);
        X509_NAME_add_entry_by_txt(x509_name, "L", chtype,
                        (unsigned char *) "loc", -1, -1, 0);
****/

        std::cerr << "AuthSSL::SignX509Req() Issuer name: " << AuthGPG::getAuthGPG()->PGPOwnId() << std::endl;

        BIGNUM *btmp = BN_new();
        if (!BN_pseudo_rand(btmp, SERIAL_RAND_BITS, 0, 0))
        {
                std::cerr << "AuthSSL::SignX509Req() rand FAIL" << std::endl;
                return NULL;
        }
        if (!BN_to_ASN1_INTEGER(btmp, serial))
        {
                std::cerr << "AuthSSL::SignX509Req() asn1 FAIL" << std::endl;
                return NULL;
        }
        BN_free(btmp);

        if (!X509_set_serialNumber(x509, serial))
        {
                std::cerr << "AuthSSL::SignX509Req() serial FAIL" << std::endl;
                return NULL;
        }
        ASN1_INTEGER_free(serial);

        /* Generate SUITABLE issuer name.
         * Must reference OpenPGP key, that is used to verify it
         */

        if (!X509_set_issuer_name(x509, issuer_name))
        {
                std::cerr << "AuthSSL::SignX509Req() issue FAIL" << std::endl;
                return NULL;
        }
        X509_NAME_free(issuer_name);


        if (!X509_gmtime_adj(X509_get_notBefore(x509),0))
        {
                std::cerr << "AuthSSL::SignX509Req() notbefore FAIL" << std::endl;
                return NULL;
        }

        if (!X509_gmtime_adj(X509_get_notAfter(x509), (long)60*60*24*days))
        {
                std::cerr << "AuthSSL::SignX509Req() notafter FAIL" << std::endl;
                return NULL;
        }

        if (!X509_set_subject_name(x509, X509_REQ_get_subject_name(req)))
        {
                std::cerr << "AuthSSL::SignX509Req() sub FAIL" << std::endl;
                return NULL;
        }

        tmppkey = X509_REQ_get_pubkey(req);
        if (!tmppkey || !X509_set_pubkey(x509,tmppkey))
        {
                std::cerr << "AuthSSL::SignX509Req() pub FAIL" << std::endl;
                return NULL;
        }

        std::cerr << "X509 Cert, prepared for signing" << std::endl;

        /*** NOW The Manual signing bit (HACKED FROM asn1/a_sign.c) ***/
        int (*i2d)(X509_CINF*, unsigned char**) = i2d_X509_CINF;
        X509_ALGOR *algor1 = x509->cert_info->signature;
        X509_ALGOR *algor2 = x509->sig_alg;
        ASN1_BIT_STRING *signature = x509->signature;
        X509_CINF *data = x509->cert_info;
        EVP_PKEY *pkey = NULL;
        const EVP_MD *type = EVP_sha1();

        EVP_MD_CTX ctx;
        unsigned char *p,*buf_in=NULL;
        unsigned char *buf_hashout=NULL,*buf_sigout=NULL;
        int i,inl=0,hashoutl=0,hashoutll=0;
        int sigoutl=0,sigoutll=0;
        X509_ALGOR *a;

        EVP_MD_CTX_init(&ctx);

        /* FIX ALGORITHMS */

        a = algor1;
        ASN1_TYPE_free(a->parameter);
        a->parameter=ASN1_TYPE_new();
        a->parameter->type=V_ASN1_NULL;

        ASN1_OBJECT_free(a->algorithm);
        a->algorithm=OBJ_nid2obj(type->pkey_type);

        a = algor2;
        ASN1_TYPE_free(a->parameter);
        a->parameter=ASN1_TYPE_new();
        a->parameter->type=V_ASN1_NULL;

        ASN1_OBJECT_free(a->algorithm);
        a->algorithm=OBJ_nid2obj(type->pkey_type);


        std::cerr << "Algorithms Fixed" << std::endl;

        /* input buffer */
        inl=i2d(data,NULL);
        buf_in=(unsigned char *)OPENSSL_malloc((unsigned int)inl);

        hashoutll=hashoutl=EVP_MD_size(type);
        buf_hashout=(unsigned char *)OPENSSL_malloc((unsigned int)hashoutl);

        sigoutll=sigoutl=2048; // hashoutl; //EVP_PKEY_size(pkey);
        buf_sigout=(unsigned char *)OPENSSL_malloc((unsigned int)sigoutl);

        if ((buf_in == NULL) || (buf_hashout == NULL) || (buf_sigout == NULL))
                {
                hashoutl=0;
                sigoutl=0;
                fprintf(stderr, "AuthSSL::SignX509Req: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_MALLOC_FAILURE)\n");
                goto err;
                }
        p=buf_in;

        std::cerr << "Buffers Allocated" << std::endl;

        i2d(data,&p);
        /* data in buf_in, ready to be hashed */
        EVP_DigestInit_ex(&ctx,type, NULL);
        EVP_DigestUpdate(&ctx,(unsigned char *)buf_in,inl);
        if (!EVP_DigestFinal(&ctx,(unsigned char *)buf_hashout,
                        (unsigned int *)&hashoutl))
                {
                hashoutl=0;
                fprintf(stderr, "AuthSSL::SignX509Req: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_EVP_LIB)\n");
                goto err;
                }

        std::cerr << "Digest Applied: len: " << hashoutl << std::endl;

        /* NOW Sign via GPG Functions */
        if (!AuthGPG::getAuthGPG()->SignDataBin(buf_hashout, hashoutl, buf_sigout, (unsigned int *) &sigoutl))
        {
                sigoutl = 0;
                goto err;
        }

        std::cerr << "Buffer Sizes: in: " << inl;
        std::cerr << "  HashOut: " << hashoutl;
        std::cerr << "  SigOut: " << sigoutl;
        std::cerr << std::endl;

        //passphrase = "NULL";

        std::cerr << "Signature done: len:" << sigoutl << std::endl;

        /* ADD Signature back into Cert... Signed!. */

        if (signature->data != NULL) OPENSSL_free(signature->data);
        signature->data=buf_sigout;
        buf_sigout=NULL;
        signature->length=sigoutl;
        /* In the interests of compatibility, I'll make sure that
         * the bit string has a 'not-used bits' value of 0
         */
        signature->flags&= ~(ASN1_STRING_FLAG_BITS_LEFT|0x07);
        signature->flags|=ASN1_STRING_FLAG_BITS_LEFT;

        std::cerr << "Certificate Complete" << std::endl;

        return x509;


  err:
        /* cleanup */
        std::cerr << "GPGAuthMgr::SignX509Req() err: FAIL" << std::endl;

        return NULL;
}

bool AuthSSL::AuthX509(X509 *x509)
{
        RsStackMutex stack(sslMtx); /******* LOCKED ******/

        /* extract CN for peer Id */
        X509_NAME *issuer = X509_get_issuer_name(x509);
        std::string id = "";

        /* verify signature */

        /*** NOW The Manual signing bit (HACKED FROM asn1/a_sign.c) ***/
        int (*i2d)(X509_CINF*, unsigned char**) = i2d_X509_CINF;
        ASN1_BIT_STRING *signature = x509->signature;
        X509_CINF *data = x509->cert_info;
        EVP_PKEY *pkey = NULL;
        const EVP_MD *type = EVP_sha1();

        EVP_MD_CTX ctx;
        unsigned char *p,*buf_in=NULL;
        unsigned char *buf_hashout=NULL,*buf_sigout=NULL;
        int i,inl=0,hashoutl=0,hashoutll=0;
        int sigoutl=0,sigoutll=0;
        X509_ALGOR *a;

        EVP_MD_CTX_init(&ctx);

        /* input buffer */
        inl=i2d(data,NULL);
        buf_in=(unsigned char *)OPENSSL_malloc((unsigned int)inl);

        hashoutll=hashoutl=EVP_MD_size(type);
        buf_hashout=(unsigned char *)OPENSSL_malloc((unsigned int)hashoutl);

        sigoutll=sigoutl=2048; //hashoutl; //EVP_PKEY_size(pkey);
        buf_sigout=(unsigned char *)OPENSSL_malloc((unsigned int)sigoutl);

        std::cerr << "Buffer Sizes: in: " << inl;
        std::cerr << "  HashOut: " << hashoutl;
        std::cerr << "  SigOut: " << sigoutl;
        std::cerr << std::endl;

        if ((buf_in == NULL) || (buf_hashout == NULL) || (buf_sigout == NULL)) {
                hashoutl=0;
                sigoutl=0;
                fprintf(stderr, "AuthSSL::AuthX509: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_MALLOC_FAILURE)\n");
                goto err;
        }
        p=buf_in;

        std::cerr << "Buffers Allocated" << std::endl;

        i2d(data,&p);
        /* data in buf_in, ready to be hashed */
        EVP_DigestInit_ex(&ctx,type, NULL);
        EVP_DigestUpdate(&ctx,(unsigned char *)buf_in,inl);
        if (!EVP_DigestFinal(&ctx,(unsigned char *)buf_hashout,
                        (unsigned int *)&hashoutl))
                {
                hashoutl=0;
                fprintf(stderr, "AuthSSL::AuthX509: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_EVP_LIB)\n");
                goto err;
                }

        std::cerr << "Digest Applied: len: " << hashoutl << std::endl;

        /* copy data into signature */
        sigoutl = signature->length;
        memmove(buf_sigout, signature->data, sigoutl);

        /* NOW Sign via GPG Functions */
        if (!AuthGPG::getAuthGPG()->VerifySignBin(buf_hashout, hashoutl, buf_sigout, (unsigned int) sigoutl)) {
                sigoutl = 0;
                goto err;
        }
        //TODO implement a way to check that the sign KEY is the same as the issuer id in the ssl cert

        std::cerr << "AuthSSL::AuthX509() X509 authenticated" << std::endl;
        return true;

  err:
        std::cerr << "AuthSSL::AuthX509() X509 NOT authenticated" << std::endl;
        return false;
}
	/* validate + get id */
bool    AuthSSL::ValidateCertificate(X509 *x509, std::string &peerId)
{
	/* check self signed */
        if (!AuthX509(x509) || !getX509id(x509, peerId)) {
#ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSL::ValidateCertificate() bad certificate.";
        std::cerr << std::endl;
#endif
                return false;
	}

#ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSL::ValidateCertificate() good certificate.";
	std::cerr << std::endl;
#endif

        return true;
}

/* store for discovery */
bool    AuthSSL::FailedCertificate(X509 *x509, bool incoming)
{
	std::string id;
	return ProcessX509(x509, id);
}

bool    AuthSSL::encrypt(void *&out, int &outlen, const void *in, int inlen, std::string peerId)
{
#ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSL::encrypt() called with inlen : " << inlen << std::endl;
#endif
        //TODO : use ssl to crypt the binary input buffer
        out = malloc(inlen);
        memcpy(out, in, inlen);
        outlen = inlen;
}

bool    AuthSSL::decrypt(void *&out, int &outlen, const void *in, int inlen)
{
#ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSL::decrypt() called with inlen : " << inlen << std::endl;
#endif
        //TODO : use ssl to decrypt the binary input buffer
        out = malloc(inlen);
        memcpy(out, in, inlen);
        outlen = inlen;
}


/* check that they are exact match */
bool    AuthSSL::CheckCertificate(std::string x509Id, X509 *x509)
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
		//X509_copy_known_signatures(pgp_keyring, cert->certificate, x509);
		X509_free(x509);

		/* update signers */
		//cert->signers = getX509signers(cert->certificate);

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

int AuthSSL::VerifyX509Callback(int preverify_ok, X509_STORE_CTX *ctx)
{
        char    buf[256];
        X509   *err_cert;
        int     err, depth;

        err_cert = X509_STORE_CTX_get_current_cert(ctx);
        err = X509_STORE_CTX_get_error(ctx);
        depth = X509_STORE_CTX_get_error_depth(ctx);

        std::cerr << "AuthSSL::VerifyX509Callback(preverify_ok: " << preverify_ok
                                 << " Err: " << err << " Depth: " << depth;
        std::cerr << std::endl;

        /*
        * Retrieve the pointer to the SSL of the connection currently treated
        * and the application specific data stored into the SSL object.
        */

        X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

        std::cerr << "AuthSSL::VerifyX509Callback: depth: " << depth << ":" << buf;
        std::cerr << std::endl;


        if (!preverify_ok) {
                fprintf(stderr, "Verify error:num=%d:%s:depth=%d:%s\n", err,
                X509_verify_cert_error_string(err), depth, buf);
        }

        /*
        * At this point, err contains the last verification error. We can use
        * it for something special
        */

        if (!preverify_ok)
        {
                if ((err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT) ||
                                (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY))
                {
                        X509_NAME_oneline(X509_get_issuer_name(X509_STORE_CTX_get_current_cert(ctx)), buf, 256);
                        printf("issuer= %s\n", buf);

                        fprintf(stderr, "Doing REAL PGP Certificates\n");
                        /* do the REAL Authentication */
                        if (!AuthX509(X509_STORE_CTX_get_current_cert(ctx)))
                        {
                                fprintf(stderr, "AuthSSL::VerifyX509Callback() X509 not authenticated.\n");
                                return false;
                        }
                        std::string pgpid = getX509CNString(X509_STORE_CTX_get_current_cert(ctx)->cert_info->issuer);
                        if (!AuthGPG::getAuthGPG()->isPGPSigned(pgpid))
                        {
                                fprintf(stderr, "AuthSSL::VerifyX509Callback() pgp key not signed by ourself.\n");
                                return false;
                        }
                        preverify_ok = true;
                }
                else if ((err == X509_V_ERR_CERT_UNTRUSTED) ||
                        (err == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE))
                {
                        std::string pgpid = getX509CNString(X509_STORE_CTX_get_current_cert(ctx)->cert_info->issuer);
                        if (!AuthGPG::getAuthGPG()->isPGPSigned(pgpid))
                        {
                                fprintf(stderr, "AuthSSL::VerifyX509Callback() pgp key not signed by ourself.\n");
                                return false;
                        }
                        preverify_ok = true;
                }
        }
        else
        {
                fprintf(stderr, "Failing Normal Certificate!!!\n");
                preverify_ok = false;
        }
        if (preverify_ok) {
            fprintf(stderr, "AuthSSL::VerifyX509Callback returned true.\n");
        } else {
            fprintf(stderr, "AuthSSL::VerifyX509Callback returned false.\n");
        }
        return preverify_ok;
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
#ifdef AUTHSSL_DEBUG
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
                valid =AuthSSL::getAuthSSL()->ValidateCertificate(x509, userId);
	}

	if (valid)
	{
		// extract the name.
		userName = getX509CNString(x509->cert_info->subject);
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


// Not dependent on sslroot. load, and detroys the X509 memory.

int	LoadCheckX509andGetIssuerName(const char *cert_file, std::string &issuerName, std::string &userId)
{
	/* This function loads the X509 certificate from the file, 
	 * and checks the certificate 
	 */

	FILE *tmpfp = fopen(cert_file, "r");
	if (tmpfp == NULL)
	{
#ifdef AUTHSSL_DEBUG
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
                valid = AuthSSL::getAuthSSL()->ValidateCertificate(x509, userId);
	}

	if (valid)
	{
		// extract the name.
		issuerName = getX509CNString(x509->cert_info->issuer);
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
	out << "  " << getX509NameString(cert->cert_info->subject);
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

// filename of certificate. (SSL Only)
std::string getCertName(X509 *x509)
{

	std::string name = getX509NameString(x509->cert_info->subject);
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
// SAVE ALL CERTS
#if PQI_USE_PQISSL
#endif
// Save only Authed Certs;
		if (it->second->authed)
		{
			X509 *x509 = it->second->certificate;
			std::string hash;
#if PQI_SSLONLY
			std::string neighfile = neighdir + getCertName(x509) + ".pqi";
#else
			std::string neighfile = neighdir + (it->first) + ".pqi";
#endif

			if (saveX509ToFile(x509, neighfile, hash))
			{
				conftxt += "CERT ";
#if PQI_SSLONLY
				conftxt += getCertName(x509);
#else
				conftxt += (it->first);
#endif
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
#ifdef AUTHSSL_DEBUG
		std::cerr << "EVP_SignInit Failure!" << std::endl;
#endif
	}

	if (0 == EVP_SignUpdate(mdctx, conftxt.c_str(), conftxt.length()))
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "EVP_SignUpdate Failure!" << std::endl;
#endif
	}


	if (0 == EVP_SignFinal(mdctx, signature, &signlen, pkey))
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "EVP_SignFinal Failure!" << std::endl;
#endif
	}

#ifdef AUTHSSL_DEBUG
	std::cerr << "Conf Signature is(" << signlen << "): ";
#endif
	for(i = 0; i < signlen; i++) 
	{
#ifdef AUTHSSL_DEBUG
		fprintf(stderr, "%02x", signature[i]);
#endif
		conftxt += signature[i];
	}
#ifdef AUTHSSL_DEBUG
	std::cerr << std::endl;
#endif
	EVP_MD_CTX_destroy(mdctx);

	FILE *cfd = fopen(configfile.c_str(), "wb");
	if (cfd == NULL)
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "Failed to open: " << configfile << std::endl;
#endif
		sslMtx.unlock(); /**** UNLOCK ****/

		return false;
	}

	int wrec;
	if (1 != (wrec = fwrite(conftxt.c_str(), conftxt.length(), 1, cfd)))
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "Error writing: " << configfile << std::endl;
		std::cerr << "Wrote: " << wrec << "/" << 1 << " Records" << std::endl;
#endif
	}

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
#ifdef AUTHSSL_DEBUG
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
#ifdef AUTHSSL_DEBUG
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
#ifdef AUTHSSL_DEBUG
			std::cerr << "Warning Mising seperator" << std::endl;
#endif
		}

#ifdef AUTHSSL_DEBUG
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
#ifdef AUTHSSL_DEBUG
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
#ifdef AUTHSSL_DEBUG
			std::cerr << "Warning Mising seperator" << std::endl;
#endif
		}

#ifdef AUTHSSL_DEBUG
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
#ifdef AUTHSSL_DEBUG
				std::cerr << "Error Reading Conf Signature:";
				std::cerr << std::endl;
#endif
				return 1;
			}
			unsigned char uc = (unsigned char) c;
			name[i] = uc;
		}
	}

#ifdef AUTHSSL_DEBUG
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
#ifdef AUTHSSL_DEBUG
#endif
		std::cerr << "EVP_SignInit Failure!" << std::endl;
	}

	if (0 == EVP_SignUpdate(mdctx, conftxt.c_str(), conftxt.length()))
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "EVP_SignUpdate Failure!" << std::endl;
#endif
	}

	if (0 == EVP_SignFinal(mdctx, conf_signature, &signlen, pkey))
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "EVP_SignFinal Failure!" << std::endl;
#endif
	}

	EVP_MD_CTX_destroy(mdctx);
	fclose(cfd);

#ifdef AUTHSSL_DEBUG
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
#ifdef AUTHSSL_DEBUG
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
#ifdef AUTHSSL_DEBUG
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


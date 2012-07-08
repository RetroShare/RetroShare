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
#include "sslfns.h"
#include "cleanupxpgp.h"

#include "pqinetwork.h"
#include "authgpg.h"
#include "serialiser/rsconfigitems.h"
#include "util/rsdir.h"

#include "retroshare/rspeers.h" // for RsPeerDetails structure 

/******************** notify of new Cert **************************/
#include "pqinotify.h"

#include <openssl/err.h>
//#include <openssl/evp.h>
//#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/x509.h>

#include <sstream>
#include <iomanip>

/****
 * #define AUTHSSL_DEBUG 1
 ***/
 #define AUTHSSL_DEBUG 1

// initialisation du pointeur de singleton
static AuthSSL *instance_ssl = NULL;

/* hidden function - for testing purposes() */
void setAuthSSL(AuthSSL *newssl)
{
	instance_ssl = newssl;
}

void AuthSSLInit()
{
	if (instance_ssl == NULL)
	{
		instance_ssl = new AuthSSLimpl();
	}
}
  
AuthSSL *AuthSSL::getAuthSSL()
{
	return instance_ssl;
}

AuthSSL::AuthSSL()
{
	return;
}

  
/********************************************************************************/
/********************************************************************************/
/*********************   Cert Search / Add / Remove    **************************/
/********************************************************************************/
/********************************************************************************/

static int verify_x509_callback(int preverify_ok, X509_STORE_CTX *ctx);


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

/************************************************************************
 *
 *
 * CODE IS DIVIDED INTO
 *
 * 1) SSL Setup.
 * 3) Cert Access.
 * 4) Cert Sign / Verify.
 * 5) Cert Authentication
 * 2) Cert Add / Remove
 * 6) Cert Storage
 */

/********************************************************************************/
/********************************************************************************/
/*********************   Cert Search / Add / Remove    **************************/
/********************************************************************************/
/********************************************************************************/


AuthSSLimpl::AuthSSLimpl()
	: p3Config(CONFIG_TYPE_AUTHSSL), sslctx(NULL),
	mOwnCert(NULL), sslMtx("AuthSSL"), mOwnPrivateKey(NULL), mOwnPublicKey(NULL), init(0)
{
}

bool AuthSSLimpl::active()
{
	return init;
}


int	AuthSSLimpl::InitAuth(const char *cert_file, const char *priv_key_file,
			const char *passwd)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSLimpl::InitAuth()";
	std::cerr << std::endl;
#endif

	/* single call here si don't need to invoke mutex yet */
static  int initLib = 0;
	if (!initLib)
	{
		initLib = 1;
		SSL_load_error_strings();
		SSL_library_init();
	}


	if (init == 1)
	{
                std::cerr << "AuthSSLimpl::InitAuth already initialized." << std::endl;
		return 1;
	}

	if ((cert_file == NULL) ||
		(priv_key_file == NULL) ||
		(passwd == NULL))
	{
                //fprintf(stderr, "sslroot::initssl() missing parameters!\n");
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
	FILE *ownfp = RsDirUtil::rs_fopen(cert_file, "r");
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
                std::cerr << "AuthSSLimpl::InitAuth() PEM_read_X509() Failed";
		std::cerr << std::endl;
		return -1;
	}
	SSL_CTX_use_certificate(sslctx, x509);
        mOwnPublicKey = X509_get_pubkey(x509);

	// get private key
	FILE *pkfp = RsDirUtil::rs_fopen(priv_key_file, "rb");
	if (pkfp == NULL)
	{
		std::cerr << "Couldn't Open PrivKey File!" << std::endl;
		CloseAuth();
		return -1;
	}

        mOwnPrivateKey = PEM_read_PrivateKey(pkfp, NULL, NULL, (void *) passwd);
	fclose(pkfp);

        if (mOwnPrivateKey == NULL)
	{
                std::cerr << "AuthSSLimpl::InitAuth() PEM_read_PrivateKey() Failed";
		std::cerr << std::endl;
		return -1;
	}
        SSL_CTX_use_PrivateKey(sslctx, mOwnPrivateKey);

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
		std::cerr << "AuthSSLimpl::InitAuth() getX509id() Failed";
		std::cerr << std::endl;

		/* bad certificate */
		CloseAuth();
		return -1;
	}

	/* Check that Certificate is Ok ( virtual function )
	 * for gpg/pgp or CA verification
	 */

        if (!validateOwnCertificate(x509, mOwnPrivateKey))
	{
		std::cerr << "AuthSSLimpl::InitAuth() validateOwnCertificate() Failed";
		std::cerr << std::endl;

		/* bad certificate */
		CloseAuth();
		exit(1);
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
bool	AuthSSLimpl::validateOwnCertificate(X509 *x509, EVP_PKEY *pkey)
{
	(void) pkey; /* remove unused parameter warning */

	/* standard authentication */
	if (!AuthX509WithGPG(x509))
	{
		return false;
	}
	return true;
}

bool	AuthSSLimpl::CloseAuth()
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSLimpl::CloseAuth()";
	std::cerr << std::endl;
#endif
	SSL_CTX_free(sslctx);

	// clean up private key....
	// remove certificates etc -> opposite of initssl.
	init = 0;
	return 1;
}

/* Context handling  */
SSL_CTX *AuthSSLimpl::getCTX()
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSLimpl::getCTX()";
	std::cerr << std::endl;
#endif
	return sslctx;
}

std::string AuthSSLimpl::OwnId()
{
#ifdef AUTHSSL_DEBUG
//	std::cerr << "AuthSSLimpl::OwnId()" << std::endl;
#endif
        return mOwnId;
}

std::string AuthSSLimpl::getOwnLocation()
{
#ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSLimpl::OwnId()" << std::endl;
#endif
        return mOwnCert->location;
}

std::string AuthSSLimpl::SaveOwnCertificateToString()
{
#ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSLimpl::SaveOwnCertificateToString() " << std::endl;
#endif
        return saveX509ToPEM(mOwnCert->certificate);
}

/********************************************************************************/
/********************************************************************************/
/*********************   Cert Search / Add / Remove    **************************/
/********************************************************************************/
/********************************************************************************/

bool AuthSSLimpl::SignData(std::string input, std::string &sign)
{
	return SignData(input.c_str(), input.length(), sign);
}

bool AuthSSLimpl::SignData(const void *data, const uint32_t len, std::string &sign)
{

	RsStackMutex stack(sslMtx);   /***** STACK LOCK MUTEX *****/

	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
        unsigned int signlen = EVP_PKEY_size(mOwnPrivateKey);
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

        if (0 == EVP_SignFinal(mdctx, signature, &signlen, mOwnPrivateKey))
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

bool AuthSSLimpl::SignDataBin(std::string input, unsigned char *sign, unsigned int *signlen)
{
	return SignDataBin(input.c_str(), input.length(), sign, signlen);
}

bool AuthSSLimpl::SignDataBin(const void *data, const uint32_t len, 
			unsigned char *sign, unsigned int *signlen)
{
	RsStackMutex stack(sslMtx);   /***** STACK LOCK MUTEX *****/
	return SSL_SignDataBin(data, len, sign, signlen, mOwnPrivateKey);
}


bool AuthSSLimpl::VerifySignBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int signlen, SSL_id sslId)
{
	/* find certificate.
	 * if we don't have - fail.
         */

	RsStackMutex stack(sslMtx);   /***** STACK LOCK MUTEX *****/
	
	/* find the peer */
	sslcert *peer;
	if (sslId == mOwnId)
	{
		peer = mOwnCert;
	}
	else if (!locked_FindCert(sslId, &peer))
	{
		std::cerr << "VerifySignBin() no peer" << std::endl;
		return false;
	}

	return SSL_VerifySignBin(data, len, sign, signlen, peer->certificate);
}

bool AuthSSLimpl::VerifyOwnSignBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int signlen) 
{
    return SSL_VerifySignBin(data, len, sign, signlen, mOwnCert->certificate);
}


/********************************************************************************/
/********************************************************************************/
/*********************   Sign and Auth with GPG        **************************/
/********************************************************************************/
/********************************************************************************/

/* Note these functions don't need Mutexes - 
 * only using GPG functions - which lock themselves
 */

X509 *AuthSSLimpl::SignX509ReqWithGPG(X509_REQ *req, long days)
{
        /* Transform the X509_REQ into a suitable format to
         * generate DIGEST hash. (for SSL to do grunt work)
         */

#define SERIAL_RAND_BITS 64

        //const EVP_MD *digest = EVP_sha1();
        ASN1_INTEGER *serial = ASN1_INTEGER_new();
        EVP_PKEY *tmppkey;
        X509 *x509 = X509_new();
        if (x509 == NULL)
        {
                std::cerr << "AuthSSLimpl::SignX509Req() FAIL" << std::endl;
                return NULL;
        }

        //long version = 0x00;
        unsigned long chtype = MBSTRING_ASC;
        X509_NAME *issuer_name = X509_NAME_new();
        X509_NAME_add_entry_by_txt(issuer_name, "CN", chtype,
                        (unsigned char *) AuthGPG::getAuthGPG()->getGPGOwnId().c_str(), -1, -1, 0);
/****
        X509_NAME_add_entry_by_NID(issuer_name, 48, 0,
                        (unsigned char *) "email@email.com", -1, -1, 0);
        X509_NAME_add_entry_by_txt(issuer_name, "O", chtype,
                        (unsigned char *) "org", -1, -1, 0);
        X509_NAME_add_entry_by_txt(x509_name, "L", chtype,
                        (unsigned char *) "loc", -1, -1, 0);
****/

        std::cerr << "AuthSSLimpl::SignX509Req() Issuer name: " << AuthGPG::getAuthGPG()->getGPGOwnId() << std::endl;

        BIGNUM *btmp = BN_new();
        if (!BN_pseudo_rand(btmp, SERIAL_RAND_BITS, 0, 0))
        {
                std::cerr << "AuthSSLimpl::SignX509Req() rand FAIL" << std::endl;
                return NULL;
        }
        if (!BN_to_ASN1_INTEGER(btmp, serial))
        {
                std::cerr << "AuthSSLimpl::SignX509Req() asn1 FAIL" << std::endl;
                return NULL;
        }
        BN_free(btmp);

        if (!X509_set_serialNumber(x509, serial))
        {
                std::cerr << "AuthSSLimpl::SignX509Req() serial FAIL" << std::endl;
                return NULL;
        }
        ASN1_INTEGER_free(serial);

        /* Generate SUITABLE issuer name.
         * Must reference OpenPGP key, that is used to verify it
         */

        if (!X509_set_issuer_name(x509, issuer_name))
        {
                std::cerr << "AuthSSLimpl::SignX509Req() issue FAIL" << std::endl;
                return NULL;
        }
        X509_NAME_free(issuer_name);


        if (!X509_gmtime_adj(X509_get_notBefore(x509),0))
        {
                std::cerr << "AuthSSLimpl::SignX509Req() notbefore FAIL" << std::endl;
                return NULL;
        }

        if (!X509_gmtime_adj(X509_get_notAfter(x509), (long)60*60*24*days))
        {
                std::cerr << "AuthSSLimpl::SignX509Req() notafter FAIL" << std::endl;
                return NULL;
        }

        if (!X509_set_subject_name(x509, X509_REQ_get_subject_name(req)))
        {
                std::cerr << "AuthSSLimpl::SignX509Req() sub FAIL" << std::endl;
                return NULL;
        }

        tmppkey = X509_REQ_get_pubkey(req);
        if (!tmppkey || !X509_set_pubkey(x509,tmppkey))
        {
                std::cerr << "AuthSSLimpl::SignX509Req() pub FAIL" << std::endl;
                return NULL;
        }

        std::cerr << "X509 Cert, prepared for signing" << std::endl;

        /*** NOW The Manual signing bit (HACKED FROM asn1/a_sign.c) ***/
        int (*i2d)(X509_CINF*, unsigned char**) = i2d_X509_CINF;
        X509_ALGOR *algor1 = x509->cert_info->signature;
        X509_ALGOR *algor2 = x509->sig_alg;
        ASN1_BIT_STRING *signature = x509->signature;
        X509_CINF *data = x509->cert_info;
        //EVP_PKEY *pkey = NULL;
        const EVP_MD *type = EVP_sha1();

        EVP_MD_CTX ctx;
        unsigned char *p,*buf_in=NULL;
        unsigned char *buf_hashout=NULL,*buf_sigout=NULL;
        int inl=0,hashoutl=0,hashoutll=0;
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
                fprintf(stderr, "AuthSSLimpl::SignX509Req: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_MALLOC_FAILURE)\n");
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
                fprintf(stderr, "AuthSSLimpl::SignX509Req: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_EVP_LIB)\n");
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

	/* XXX CLEANUP */
  err:
        /* cleanup */
        std::cerr << "GPGAuthMgr::SignX509Req() err: FAIL" << std::endl;

        return NULL;
}


/* This function, checks that the X509 is signed by a known GPG key,
 * NB: we do not have to have approved this person as a friend.
 * this is important - as it allows non-friends messages to be validated.
 */

bool AuthSSLimpl::AuthX509WithGPG(X509 *x509)
{
        #ifdef AUTHSSL_DEBUG
        fprintf(stderr, "AuthSSLimpl::AuthX509WithGPG() called\n");
        #endif

        if (!CheckX509Certificate(x509))
	{
            std::cerr << "AuthSSLimpl::AuthX509() X509 NOT authenticated : Certificate failed basic checks" << std::endl;
            return false;
        }

        /* extract CN for peer Id */
        std::string issuer = getX509CNString(x509->cert_info->issuer);
        RsPeerDetails pd;
        #ifdef AUTHSSL_DEBUG
        std::cerr << "Checking GPG issuer : " << issuer << std::endl ;
        #endif
        if (!AuthGPG::getAuthGPG()->getGPGDetails(issuer, pd)) {
            std::cerr << "AuthSSLimpl::AuthX509() X509 NOT authenticated : AuthGPG::getAuthGPG()->getGPGDetails() returned false." << std::endl;
            return false;
        }

        /* verify GPG signature */

        /*** NOW The Manual signing bit (HACKED FROM asn1/a_sign.c) ***/
        int (*i2d)(X509_CINF*, unsigned char**) = i2d_X509_CINF;
        ASN1_BIT_STRING *signature = x509->signature;
        X509_CINF *data = x509->cert_info;
        const EVP_MD *type = EVP_sha1();

        EVP_MD_CTX ctx;
        unsigned char *p,*buf_in=NULL;
        unsigned char *buf_hashout=NULL,*buf_sigout=NULL;
        int inl=0,hashoutl=0,hashoutll=0;
        int sigoutl=0,sigoutll=0;
        //X509_ALGOR *a;

        EVP_MD_CTX_init(&ctx);

        /* input buffer */
        inl=i2d(data,NULL);
        buf_in=(unsigned char *)OPENSSL_malloc((unsigned int)inl);

        hashoutll=hashoutl=EVP_MD_size(type);
        buf_hashout=(unsigned char *)OPENSSL_malloc((unsigned int)hashoutl);

        sigoutll=sigoutl=2048; //hashoutl; //EVP_PKEY_size(pkey);
        buf_sigout=(unsigned char *)OPENSSL_malloc((unsigned int)sigoutl);

        #ifdef AUTHSSL_DEBUG
        std::cerr << "Buffer Sizes: in: " << inl;
        std::cerr << "  HashOut: " << hashoutl;
        std::cerr << "  SigOut: " << sigoutl;
        std::cerr << std::endl;
        #endif

        if ((buf_in == NULL) || (buf_hashout == NULL) || (buf_sigout == NULL)) {
                hashoutl=0;
                sigoutl=0;
                fprintf(stderr, "AuthSSLimpl::AuthX509: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_MALLOC_FAILURE)\n");
                goto err;
        }
        p=buf_in;

        #ifdef AUTHSSL_DEBUG
        std::cerr << "Buffers Allocated" << std::endl;
        #endif

        i2d(data,&p);
        /* data in buf_in, ready to be hashed */
        EVP_DigestInit_ex(&ctx,type, NULL);
        EVP_DigestUpdate(&ctx,(unsigned char *)buf_in,inl);
        if (!EVP_DigestFinal(&ctx,(unsigned char *)buf_hashout,
                        (unsigned int *)&hashoutl))
                {
                hashoutl=0;
                fprintf(stderr, "AuthSSLimpl::AuthX509: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_EVP_LIB)\n");
                goto err;
                }

        #ifdef AUTHSSL_DEBUG
        std::cerr << "Digest Applied: len: " << hashoutl << std::endl;
        #endif

        /* copy data into signature */
        sigoutl = signature->length;
        memmove(buf_sigout, signature->data, sigoutl);

        /* NOW check sign via GPG Functions */
        //get the fingerprint of the key that is supposed to sign
        #ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSLimpl::AuthX509() verifying the gpg sig with keyprint : " << pd.fpr << std::endl;
		  std::cerr << "Sigoutl = " << sigoutl << std::endl ;
		  std::cerr << "pd.fpr = " << pd.fpr << std::endl ;
		  std::cerr << "hashoutl = " << hashoutl << std::endl ;
        #endif

        if (!AuthGPG::getAuthGPG()->VerifySignBin(buf_hashout, hashoutl, buf_sigout, (unsigned int) sigoutl, pd.fpr)) {
                sigoutl = 0;
                goto err;
        }

        #ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSLimpl::AuthX509() X509 authenticated" << std::endl;
        #endif

		  OPENSSL_free(buf_in) ;
		  OPENSSL_free(buf_hashout) ;
        return true;

  err:
        std::cerr << "AuthSSLimpl::AuthX509() X509 NOT authenticated" << std::endl;

		  if(buf_in != NULL)
			  OPENSSL_free(buf_in) ;
		  if(buf_hashout != NULL)
			  OPENSSL_free(buf_hashout) ;
        return false;
}



	/* validate + get id */
bool    AuthSSLimpl::ValidateCertificate(X509 *x509, std::string &peerId)
{
	/* check self signed */
	if (!AuthX509WithGPG(x509))
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSLimpl::ValidateCertificate() bad certificate.";
		std::cerr << std::endl;
#endif
		return false;
	}
	if(!getX509id(x509, peerId)) 
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSLimpl::ValidateCertificate() Cannot retrieve peer id from certificate..";
		std::cerr << std::endl;
#endif
		return false;
	}

#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSLimpl::ValidateCertificate() good certificate.";
	std::cerr << std::endl;
#endif

	return true;
}


/********************************************************************************/
/********************************************************************************/
/****************************  encrypt / decrypt fns ****************************/
/********************************************************************************/
/********************************************************************************/

static int verify_x509_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
#ifdef AUTHSSL_DEBUG
        std::cerr << "static verify_x509_callback called.";
        std::cerr << std::endl;
#endif
        int verify = AuthSSL::getAuthSSL()->VerifyX509Callback(preverify_ok, ctx);
	if (!verify)
	{
		/* Process as FAILED Certificate */
		/* Start as INCOMING, as outgoing is already captured */
		struct sockaddr_in addr;
		sockaddr_clear(&addr);
		
		AuthSSL::getAuthSSL()->FailedCertificate(X509_STORE_CTX_get_current_cert(ctx), addr, true); 
	}

        return verify;
}

int AuthSSLimpl::VerifyX509Callback(int preverify_ok, X509_STORE_CTX *ctx)
{
        char    buf[256];
        X509   *err_cert;
        int     err, depth;

        err_cert = X509_STORE_CTX_get_current_cert(ctx);
        err = X509_STORE_CTX_get_error(ctx);
        depth = X509_STORE_CTX_get_error_depth(ctx);

        #ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSLimpl::VerifyX509Callback(preverify_ok: " << preverify_ok
                                 << " Err: " << err << " Depth: " << depth << std::endl;
        #endif

        /*
        * Retrieve the pointer to the SSL of the connection currently treated
        * and the application specific data stored into the SSL object.
        */

        X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

        #ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSLimpl::VerifyX509Callback: depth: " << depth << ":" << buf << std::endl;
        #endif


        if (!preverify_ok) {
                #ifdef AUTHSSL_DEBUG
                fprintf(stderr, "Verify error:num=%d:%s:depth=%d:%s\n", err,
                X509_verify_cert_error_string(err), depth, buf);
                #endif
        }

        /*
        * At this point, err contains the last verification error. We can use
        * it for something special
        */

        if (!preverify_ok)
        {

            X509_NAME_oneline(X509_get_issuer_name(X509_STORE_CTX_get_current_cert(ctx)), buf, 256);
            #ifdef AUTHSSL_DEBUG
            printf("issuer= %s\n", buf);
            #endif

            #ifdef AUTHSSL_DEBUG
            fprintf(stderr, "Doing REAL PGP Certificates\n");
            #endif
            /* do the REAL Authentication */
            if (!AuthX509WithGPG(X509_STORE_CTX_get_current_cert(ctx)))
            {
                    #ifdef AUTHSSL_DEBUG
                    fprintf(stderr, "AuthSSLimpl::VerifyX509Callback() X509 not authenticated.\n");
                    #endif
                    return false;
            }
            std::string pgpid = getX509CNString(X509_STORE_CTX_get_current_cert(ctx)->cert_info->issuer);

            if (pgpid != AuthGPG::getAuthGPG()->getGPGOwnId() && !AuthGPG::getAuthGPG()->isGPGAccepted(pgpid))
            {
                    #ifdef AUTHSSL_DEBUG
                    fprintf(stderr, "AuthSSLimpl::VerifyX509Callback() pgp key not accepted : \n");
                    fprintf(stderr, "issuer pgpid : ");
                    fprintf(stderr, "%s\n",pgpid.c_str());
                    fprintf(stderr, "\n AuthGPG::getAuthGPG()->getGPGOwnId() : ");
                    fprintf(stderr, "%s\n",AuthGPG::getAuthGPG()->getGPGOwnId().c_str());
                    fprintf(stderr, "\n");
                    #endif
                    return false;
            }

            preverify_ok = true;

        } else {
                #ifdef AUTHSSL_DEBUG
                fprintf(stderr, "A normal certificate is probably a security breach attempt. We sould fail it !!!\n");
                #endif
                preverify_ok = false;
        }

        if (preverify_ok) {

            //sslcert *cert = NULL;
            std::string certId;
            getX509id(X509_STORE_CTX_get_current_cert(ctx), certId);

        }

        #ifdef AUTHSSL_DEBUG
        if (preverify_ok) {
            fprintf(stderr, "AuthSSLimpl::VerifyX509Callback returned true.\n");
        } else {
            fprintf(stderr, "AuthSSLimpl::VerifyX509Callback returned false.\n");
        }
        #endif

        return preverify_ok;
}


/********************************************************************************/
/********************************************************************************/
/****************************  encrypt / decrypt fns ****************************/
/********************************************************************************/
/********************************************************************************/


bool    AuthSSLimpl::encrypt(void *&out, int &outlen, const void *in, int inlen, std::string peerId)
{
	RsStackMutex stack(sslMtx); /******* LOCKED ******/

#ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSLimpl::encrypt() called for peerId : " << peerId << " with inlen : " << inlen << std::endl;
#endif
        //TODO : use ssl to crypt the binary input buffer
//        out = malloc(inlen);
//        memcpy(out, in, inlen);
//        outlen = inlen;

        EVP_PKEY *public_key;
        if (peerId == mOwnId) {
            public_key = mOwnPublicKey;
        } else {
            if (!mCerts[peerId]) {
                #ifdef AUTHSSL_DEBUG
                std::cerr << "AuthSSLimpl::encrypt() public key not found." << std::endl;
                #endif
                return false;
            } else {
                public_key = mCerts[peerId]->certificate->cert_info->key->pkey;
            }
        }

        EVP_CIPHER_CTX ctx;
        int eklen, net_ekl;
        unsigned char *ek;
        unsigned char iv[EVP_MAX_IV_LENGTH];
        EVP_CIPHER_CTX_init(&ctx);
        int out_currOffset = 0;
        int out_offset = 0;

        int max_evp_key_size = EVP_PKEY_size(public_key);
        ek = (unsigned char*)malloc(max_evp_key_size);
        const EVP_CIPHER *cipher = EVP_aes_128_cbc();
        int cipher_block_size = EVP_CIPHER_block_size(cipher);
        int size_net_ekl = sizeof(net_ekl);

        int max_outlen = inlen + cipher_block_size + EVP_MAX_IV_LENGTH + max_evp_key_size + size_net_ekl;

        // intialize context and send store encrypted cipher in ek
        if(!EVP_SealInit(&ctx, EVP_aes_128_cbc(), &ek, &eklen, iv, &public_key, 1)) {
            free(ek);
            return false;
        }

    	// now assign memory to out accounting for data, and cipher block size, key length, and key length val
        out = new unsigned char[inlen + cipher_block_size + size_net_ekl + eklen + EVP_MAX_IV_LENGTH];

    	net_ekl = htonl(eklen);
    	memcpy((unsigned char*)out + out_offset, &net_ekl, size_net_ekl);
    	out_offset += size_net_ekl;

    	memcpy((unsigned char*)out + out_offset, ek, eklen);
    	out_offset += eklen;

    	memcpy((unsigned char*)out + out_offset, iv, EVP_MAX_IV_LENGTH);
    	out_offset += EVP_MAX_IV_LENGTH;

    	// now encrypt actual data
        if(!EVP_SealUpdate(&ctx, (unsigned char*) out + out_offset, &out_currOffset, (unsigned char*) in, inlen)) {
            free(ek);
            delete[] (unsigned char*) out;
            out = NULL;
            return false;
        }

    	// move along to partial block space
    	out_offset += out_currOffset;

    	// add padding
        if(!EVP_SealFinal(&ctx, (unsigned char*) out + out_offset, &out_currOffset)) {
            free(ek);
            delete[] (unsigned char*) out;
            out = NULL;
            return false;
        }

    	// move to end
    	out_offset += out_currOffset;

    	// make sure offset has not gone passed valid memory bounds
    	if(out_offset > max_outlen) return false;

    	// free encrypted key data
    	free(ek);

        EVP_CIPHER_CTX_cleanup(&ctx);

    	outlen = out_offset;

    #ifdef DISTRIB_DEBUG
        std::cerr << "Authssl::encrypt() finished with outlen : " << outlen << std::endl;
    #endif

        return true;
}

bool    AuthSSLimpl::decrypt(void *&out, int &outlen, const void *in, int inlen)
{
	RsStackMutex stack(sslMtx); /******* LOCKED ******/



#ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSLimpl::decrypt() called with inlen : " << inlen << std::endl;
#endif
        //TODO : use ssl to decrypt the binary input buffer
//        out = malloc(inlen);
//        memcpy(out, in, inlen);
//        outlen = inlen;
        EVP_CIPHER_CTX ctx;
        int eklen = 0, net_ekl = 0;
        unsigned char *ek = NULL;
        unsigned char iv[EVP_MAX_IV_LENGTH];
        int ek_mkl = EVP_PKEY_size(mOwnPrivateKey);
        ek = (unsigned char*)malloc(ek_mkl);
        EVP_CIPHER_CTX_init(&ctx);

        int in_offset = 0, out_currOffset = 0;
        int size_net_ekl = sizeof(net_ekl);

        if(size_net_ekl > inlen) {
            free(ek);
            return false;
        }

        memcpy(&net_ekl, (unsigned char*)in, size_net_ekl);
        eklen = ntohl(net_ekl);
        in_offset += size_net_ekl;

        if(eklen > (inlen-in_offset)) {
            free(ek);
            return false;
        }

        memcpy(ek, (unsigned char*)in + in_offset, eklen);
        in_offset += eklen;

        if(EVP_MAX_IV_LENGTH > (inlen-in_offset)) {
            free(ek);
            return false;
        }

        memcpy(iv, (unsigned char*)in + in_offset, EVP_MAX_IV_LENGTH);
        in_offset += EVP_MAX_IV_LENGTH;

        const EVP_CIPHER* cipher = EVP_aes_128_cbc();

        if(0 == EVP_OpenInit(&ctx, cipher, ek, eklen, iv, mOwnPrivateKey)) {
            free(ek);
            return false;
        }

        out = new unsigned char[inlen - in_offset];

        if(!EVP_OpenUpdate(&ctx, (unsigned char*) out, &out_currOffset, (unsigned char*)in + in_offset, inlen - in_offset)) {
            free(ek);
            delete[] (unsigned char*) out;
            out = NULL;
            return false;
        }

        in_offset += out_currOffset;
        outlen += out_currOffset;

        if(!EVP_OpenFinal(&ctx, (unsigned char*)out + out_currOffset, &out_currOffset)) {
            free(ek);
            delete[] (unsigned char*) out;
            out = NULL;
            return false;
        }

        outlen += out_currOffset;

        if(ek != NULL)
        	free(ek);

        EVP_CIPHER_CTX_cleanup(&ctx);

        #ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSLimpl::decrypt() finished with outlen : " << outlen << std::endl;
        #endif

        return true;
}


/********************************************************************************/
/********************************************************************************/
/*********************   Cert Search / Add / Remove    **************************/
/********************************************************************************/
/********************************************************************************/

/* store for discovery */
bool    AuthSSLimpl::FailedCertificate(X509 *x509, const struct sockaddr_in &addr, bool incoming)
{
        std::string peerId = "UnknownSSLID";
	if(!getX509id(x509, peerId)) 
	{
		std::cerr << "AuthSSLimpl::FailedCertificate() ERROR cannot extract X509id from certificate";
		std::cerr << std::endl;
	}

        std::string gpgid = getX509CNString(x509->cert_info->issuer);
        std::string sslcn = getX509CNString(x509->cert_info->subject);

	std::cerr << "AuthSSLimpl::FailedCertificate() ";
	if (incoming)
	{
		std::cerr << " Incoming from: ";
	}
	else
	{
		std::cerr << " Outgoing to: ";
	}

	std::cerr << "GpgId: " << gpgid << " SSLcn: " << sslcn << " peerId: " << peerId;
	std::cerr << std::endl;

	{
		// Hacky - adding IpAddress to SSLId.
		std::ostringstream out;
		out << "/" << rs_inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
		peerId += out.str();
	}

	uint32_t notifyType = 0;

	/* if auths -> store */
	if (AuthX509WithGPG(x509))
	{
		std::cerr << "AuthSSLimpl::FailedCertificate() Cert Checked Out, so passing to Notify";
		std::cerr << std::endl;

		if (incoming)
		{
			notifyType = RS_FEED_ITEM_SEC_CONNECT_ATTEMPT;
		}
		else
		{
			notifyType = RS_FEED_ITEM_SEC_AUTH_DENIED;
		}

		getPqiNotify()->AddFeedItem(notifyType, gpgid, peerId, sslcn);

		LocalStoreCert(x509);
		return true;
	}
	else
	{
		/* unknown peer! */
		if (incoming)
		{
			notifyType = RS_FEED_ITEM_SEC_UNKNOWN_IN;
		}
		else
		{
			notifyType = RS_FEED_ITEM_SEC_UNKNOWN_OUT;
		}

		getPqiNotify()->AddFeedItem(notifyType, gpgid, peerId, sslcn);

	}

	return false;
}

bool    AuthSSLimpl::CheckCertificate(std::string id, X509 *x509)
{
	(void) id; /* remove unused parameter warning */

	/* if auths -> store */
	if (AuthX509WithGPG(x509))
	{
		LocalStoreCert(x509);
		return true;
	}
	return false;
}



/* Locked search -> internal help function */
bool AuthSSLimpl::locked_FindCert(std::string id, sslcert **cert)
{
	std::map<std::string, sslcert *>::iterator it;
	
	if (mCerts.end() != (it = mCerts.find(id)))
	{
		*cert = it->second;
		return true;
	}
	return false;
}


/* Remove Certificate */

bool AuthSSLimpl::RemoveX509(std::string id)
{
	std::map<std::string, sslcert *>::iterator it;
	
	RsStackMutex stack(sslMtx); /******* LOCKED ******/

	if (mCerts.end() != (it = mCerts.find(id)))
	{
		sslcert *cert = it->second;

		/* clean up */
		X509_free(cert->certificate);
		cert->certificate = NULL;
		delete cert;

		mCerts.erase(it);

		return true;
	}
	return false;
}


bool AuthSSLimpl::LocalStoreCert(X509* x509) 
{
	//store the certificate in the local cert list
	std::string peerId;
	if(!getX509id(x509, peerId))
	{
		std::cerr << "AuthSSLimpl::LocalStoreCert() Cannot retrieve peer id from certificate." << std::endl;
#ifdef AUTHSSL_DEBUG
#endif
		return false;
	}


	RsStackMutex stack(sslMtx); /******* LOCKED ******/

	if (peerId == mOwnId) 
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSLimpl::LocalStoreCert() not storing own certificate" << std::endl;
#endif
		return false;
	}

	/* do a search */
	std::map<std::string, sslcert *>::iterator it;
	
	if (mCerts.end() != (it = mCerts.find(peerId)))
	{
		sslcert *cert = it->second;

		/* found something */
		/* check that they are exact */
		if (0 != X509_cmp(cert->certificate, x509))
		{
			/* MAJOR ERROR */
			std::cerr << "ERROR : AuthSSLimpl::LocalStoreCert() got two ssl certificates with identical ids -> dropping second";
			std::cerr << std::endl;
			return false;
		}
		/* otherwise - we have it already */
		return false;
	}

#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSLimpl::LocalStoreCert() storing certificate for " << peerId << std::endl;
#endif
	mCerts[peerId] = new sslcert(X509_dup(x509), peerId);

	/* flag for saving config */
	IndicateConfigChanged();
	return true ;
}


/********************************************************************************/
/********************************************************************************/
/************************   Config Functions   **********************************/
/********************************************************************************/
/********************************************************************************/


RsSerialiser *AuthSSLimpl::setupSerialiser()
{
        RsSerialiser *rss = new RsSerialiser();
        rss->addSerialType(new RsGeneralConfigSerialiser());
        return rss ;
}

bool AuthSSLimpl::saveList(bool& cleanup, std::list<RsItem*>& lst)
{
        #ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSLimpl::saveList() called" << std::endl ;
        #endif

        RsStackMutex stack(sslMtx); /******* LOCKED ******/

        cleanup = true ;

        // Now save config for network digging strategies
        RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
        std::map<std::string, sslcert*>::iterator mapIt;
        for (mapIt = mCerts.begin(); mapIt != mCerts.end(); mapIt++) {
            if (mapIt->first == mOwnId) {
                continue;
            }
            RsTlvKeyValue kv;
            kv.key = mapIt->first;
            #ifdef AUTHSSL_DEBUG
            std::cerr << "AuthSSLimpl::saveList() called (mapIt->first) : " << (mapIt->first) << std::endl ;
            #endif
            kv.value = saveX509ToPEM(mapIt->second->certificate);
            vitem->tlvkvs.pairs.push_back(kv) ;
        }
        lst.push_back(vitem);

        return true ;
}

bool AuthSSLimpl::loadList(std::list<RsItem*>& load)
{
        #ifdef AUTHSSL_DEBUG
        std::cerr << "AuthSSLimpl::loadList() Item Count: " << load.size() << std::endl;
        #endif

        /* load the list of accepted gpg keys */
        std::list<RsItem *>::iterator it;
        for(it = load.begin(); it != load.end(); it++) {
                RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet *>(*it);

                if(vitem) {
                        #ifdef AUTHSSL_DEBUG
                        std::cerr << "AuthSSLimpl::loadList() General Variable Config Item:" << std::endl;
                        vitem->print(std::cerr, 10);
                        std::cerr << std::endl;
                        #endif

                        std::list<RsTlvKeyValue>::iterator kit;
                        for(kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); kit++) {
                            if (kit->key == mOwnId) {
                                continue;
                            }

                            X509 *peer = loadX509FromPEM(kit->value);
			    /* authenticate it */
			    if (AuthX509WithGPG(peer))
			    {
				LocalStoreCert(peer);
			    }
                        }
                }
                delete (*it);
        }
        return true;
}

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


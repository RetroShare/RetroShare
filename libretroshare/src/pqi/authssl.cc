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

#ifdef WINDOWS_SYS
#include "util/rswin.h"
#endif // WINDOWS_SYS

#include "authssl.h"
#include "sslfns.h"

#include "pqinetwork.h"
#include "authgpg.h"
#include "serialiser/rsconfigitems.h"
#include "util/rsdir.h"
#include "util/rsstring.h"

#include "retroshare/rspeers.h" // for RsPeerDetails structure 
#include "retroshare/rsids.h" // for RsPeerDetails structure 
#include "rsserver/p3face.h" 

/******************** notify of new Cert **************************/

#include <openssl/rand.h>
#include <openssl/ssl.h>

#include <iomanip>

/* SSL connection diagnostic */

const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_UNKNOWN               = 0x00 ; 
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_OK                    = 0x01 ; 
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_CERTIFICATE_NOT_VALID = 0x02 ; 
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_ISSUER_UNKNOWN        = 0x03 ; 
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_MALLOC_ERROR          = 0x04 ; 
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_WRONG_SIGNATURE       = 0x05 ; 
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_CERTIFICATE_MISSING   = 0x06 ; 

/****
 * #define AUTHSSL_DEBUG 1
 ***/

// initialisation du pointeur de singleton
AuthSSL *AuthSSL::instance_ssl = NULL;
static pthread_mutex_t *mutex_buf = NULL;

struct CRYPTO_dynlock_value
{
	pthread_mutex_t mutex;
};

/**
 * OpenSSL locking function.
 *
 * @param    mode    lock mode
 * @param    n       lock number
 * @param    file    source file name
 * @param    line    source file line number
 * @return   none
 */
static void locking_function(int mode, int n, const char */*file*/, int /*line*/)
{
	if (mode & CRYPTO_LOCK) {
		pthread_mutex_lock(&mutex_buf[n]);
	} else {
		pthread_mutex_unlock(&mutex_buf[n]);
	}
}

/**
 * OpenSSL uniq id function.
 *
 * @return    thread id
 */
static unsigned long id_function(void)
{
#if defined( WINDOWS_SYS) && !defined(WIN_PTHREADS_H)
	return (unsigned long) pthread_self().p;
#else
	return (unsigned long) pthread_self();
#endif
}

/**
 * OpenSSL allocate and initialize dynamic crypto lock.
 *
 * @param    file    source file name
 * @param    line    source file line number
 */
static struct CRYPTO_dynlock_value *dyn_create_function(const char */*file*/, int /*line*/)
{
	struct CRYPTO_dynlock_value *value;

	value = (struct CRYPTO_dynlock_value*) malloc(sizeof(struct CRYPTO_dynlock_value));
	if (!value) {
		return NULL;
	}
	pthread_mutex_init(&value->mutex, NULL);

	return value;
}

/**
 * OpenSSL dynamic locking function.
 *
 * @param    mode    lock mode
 * @param    l       lock structure pointer
 * @param    file    source file name
 * @param    line    source file line number
 * @return   none
 */
static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l, const char */*file*/, int /*line*/)
{
	if (mode & CRYPTO_LOCK) {
		pthread_mutex_lock(&l->mutex);
	} else {
		pthread_mutex_unlock(&l->mutex);
	}
}

/**
 * OpenSSL destroy dynamic crypto lock.
 *
 * @param    l       lock structure pointer
 * @param    file    source file name
 * @param    line    source file line number
 * @return   none
 */
static void dyn_destroy_function(struct CRYPTO_dynlock_value *l, const char */*file*/, int /*line*/)
{
	pthread_mutex_destroy(&l->mutex);
	free(l);
}

/**
 * Initialize TLS library.
 *
 * @return    true on success, false on error
 */
bool tls_init()
{
	/* static locks area */
	mutex_buf = (pthread_mutex_t*) malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
	if (mutex_buf == NULL) {
		return false;
	}
	for (int i = 0; i < CRYPTO_num_locks(); i++) {
		pthread_mutex_init(&mutex_buf[i], NULL);
	}
	/* static locks callbacks */
	CRYPTO_set_locking_callback(locking_function);
	CRYPTO_set_id_callback(id_function);
	/* dynamic locks callbacks */
	CRYPTO_set_dynlock_create_callback(dyn_create_function);
	CRYPTO_set_dynlock_lock_callback(dyn_lock_function);
	CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function);

    return true;
}

/**
 * Cleanup TLS library.
 *
 * @return    0
 */
void tls_cleanup()
{
	CRYPTO_set_dynlock_create_callback(NULL);
	CRYPTO_set_dynlock_lock_callback(NULL);
	CRYPTO_set_dynlock_destroy_callback(NULL);

	CRYPTO_set_locking_callback(NULL);
	CRYPTO_set_id_callback(NULL);

	if (mutex_buf != NULL) {
		for (int i = 0; i < CRYPTO_num_locks(); i++) {
			pthread_mutex_destroy(&mutex_buf[i]);
		}
		free(mutex_buf);
		mutex_buf = NULL;
	}
}

/* hidden function - for testing purposes() */
void AuthSSL::setAuthSSL_debug(AuthSSL *newssl)
{
	instance_ssl = newssl;
}

void AuthSSL::AuthSSLInit()
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


sslcert::sslcert(X509 *x509, const RsPeerId& pid)
{
	certificate = x509;
	id = pid;
	name = getX509CNString(x509->cert_info->subject);
	org = getX509OrgString(x509->cert_info->subject);
	location = getX509LocString(x509->cert_info->subject);
	email = "";

	issuer = RsPgpId(std::string(getX509CNString(x509->cert_info->issuer)));

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
	: p3Config(), sslctx(NULL),
	mOwnCert(NULL), sslMtx("AuthSSL"), mOwnPrivateKey(NULL), mOwnPublicKey(NULL), init(0)
{
}

bool AuthSSLimpl::active()
{
	return init;
}


int	AuthSSLimpl::InitAuth(const char *cert_file, const char *priv_key_file,
            const char *passwd, std::string alternative_location_name)
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

		if (!tls_init()) {
			return 0;
		}

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
	sslctx = SSL_CTX_new(SSLv23_method());
	SSL_CTX_set_options(sslctx,SSL_OP_NO_SSLv3) ;

	// Setup cipher lists:
	//
	//       std::string cipherString = "HIGH:!DSS:!aNULL:!3DES";
	//       std::string cipherString = "DEFAULT";
	//
	// The current cipher list  asks in priority for EDH which provides PFS. However EDH needs proper
	// parameters to be set on the server side, so if we're a client for a RS instance that has no DH params,
	// the connection will be refused. So we happend the HIGH cipher suite just after. In oder to force 
	// PFS, at the risk of not always connecting, one should use:
	//
	// 		std::string cipherString = "kEDH:HIGH:!DSS:!aNULL:!3DES";
	//
	// The following safe primes are 2048/4096 bits long. Should be enough.
	//
	// std::string dh_prime_2048_dec = "30651576830996935311378276950670996791883170963804289256203421500259588715033040934547350194073369837229137842804826417332761673984632102152477971341551955103053338169949165519208562998954887445690136488713010579430413255432398961330773637820158790237012997356731669148258317860643591694814197514454546928317578771868379525705082166818553884557266645700906836702542808787791878865135741211056957383668479369231868698451684633965462539374994559481908068730787128654626819903401038534403722014687647173327537458614224702967073490136394698912372792187651228785689025073104374674728645661275001416541267543884923191810923";
	//
	std::string dh_prime_2048_hex = "B3B86A844550486C7EA459FA468D3A8EFD71139593FE1C658BBEFA9B2FC0AD2628242C2CDC2F91F5B220ED29AAC271192A7374DFA28CDDCA70252F342D0821273940344A7A6A3CB70C7897A39864309F6CAC5C7EA18020EF882693CA2C12BB211B7BA8367D5A7C7252A5B5E840C9E8F081469EBA0B98BCC3F593A4D9C4D5DF539362084F1B9581316C1F80FDAD452FD56DBC6B8ED0775F596F7BB22A3FE2B4753764221528D33DB4140DE58083DB660E3E105123FC963BFF108AC3A268B7380FFA72005A1515C371287C5706FFA6062C9AC73A9B1A6AC842C2764CDACFC85556607E86611FDF486C222E4896CDF6908F239E177ACC641FCBFF72A758D1C10CBB" ;

	std::string dh_prime_4096_hex = "A6F5777292D9E6BB95559C9124B9119E6771F11F2048C8FE74F4E8140494520972A087EF1D60B73894F1C5D509DD15D96CF379E9DDD46CE51B748085BACB440D915565782C73AF3A9580CE788441D1DA4D114E3D302CAB45A061ABCFC1F7E9200AE019CB923B77E096FA9377454A16FFE91D86535FF23E075B3E714F785CD7606E9CBD9D06F01CAFA2271883D649F13ABE170D714F6B6EC064C5BF35C4F4BDA5EF5ED5E70D5DC78F1AC1CDC04EEDAE8ADD65C4A9E27368E0B2C8595DD7626D763BFFB15364B3CCA9FCE814B9226B35FE652F4B041F0FF6694D6A482B0EF48CA41163D083AD2DE7B7A068BB05C0453E9D008551C7F67993A3EF2C4874F0244F78C4E0997BD31AB3BD88446916B499B2513DD5BA002063BD38D2CE55D29D071399D5CEE99458AF6FDC104A61CA3FACDAC803CBDE62B4C0EAC946D0E12F05CE9E94497110D64E611D957423B8AA412D84EC83E6E70E0977A31D6EE056D0527D4667D7242A77C9B679D191562E4026DA9C35FF85666296D872ED548E0FFE1A677FCC373C1F490CAB4F53DFD8735C0F1DF02FEAD824A217FDF4E3404D38A5BBC719C6622630FCD34F6F1968AF1B66A4AB1A9FCF653DA96EB3A42AF6FCFEA0547B8F314A527C519949007D7FA1726FF3D33EC46393B0207AA029E5EA574BDAC94D78894B22A2E3303E65A3F820DF57DB44951DE4E973C016C57F7A242D0BC53BC563AF" ;

	std::string cipherString = "kEDH+HIGH:!DSS:!aNULL:!3DES:!EXP";

	SSL_CTX_set_cipher_list(sslctx, cipherString.c_str());

	DH* dh = DH_new();
	int codes = 0;
	bool pfs_enabled = true ;

	if (dh)
	{
		BN_hex2bn(&dh->p,dh_prime_4096_hex.c_str()) ;
		BN_hex2bn(&dh->g,"5") ;

		std::cout.flush() ;

		if(DH_check(dh, &codes) && codes == 0)
			SSL_CTX_set_tmp_dh(sslctx, dh);	
		else
			pfs_enabled = false ;
	}
	else
		pfs_enabled = false ;

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

	RsPeerId mownidstr ;

	if (!getX509id(x509, mownidstr))
	{
		std::cerr << "AuthSSLimpl::InitAuth() getX509id() Failed";
		std::cerr << std::endl;

		/* bad certificate */
		CloseAuth();
		return -1;
	}
	mOwnId = mownidstr ;

	assert(!mOwnId.isNull()) ;

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

    // new locations don't store the name in the cert
    // if empty, use the external supplied value
    if(mOwnCert->location == "")
        mOwnCert->location = alternative_location_name;

	std::cerr << "Inited SSL context: " << std::endl;
	std::cerr << "    Certificate: " << mOwnId << std::endl;
	std::cerr << "    cipher list: " << cipherString << std::endl;
	std::cerr << "    PFS enabled: " << (pfs_enabled?"YES":"NO") ;
	if(codes > 0)
	{
		std::cerr << " (reason: " ;
		if(codes & DH_CHECK_P_NOT_PRIME     ) std::cerr << "Not a prime number, " ;
		if(codes & DH_CHECK_P_NOT_SAFE_PRIME) std::cerr << "Not a safe prime number, " ;
		if(codes & DH_UNABLE_TO_CHECK_GENERATOR) std::cerr << "unable to check generator, " ;
		if(codes & DH_NOT_SUITABLE_GENERATOR) std::cerr << "not a suitable generator" ;
		std::cerr << ")" << std::endl;
	}
	else
		std::cerr << std::endl;

	init = 1;
	return 1;
}

/* Dummy function to be overloaded by real implementation */
bool	AuthSSLimpl::validateOwnCertificate(X509 *x509, EVP_PKEY *pkey)
{
	(void) pkey; /* remove unused parameter warning */

	uint32_t diagnostic ;

	/* standard authentication */
	if (!AuthX509WithGPG(x509,diagnostic))
	{
		return false;
	}
	return true;
}

bool	AuthSSLimpl::CloseAuth()
{
	tls_cleanup();

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

const RsPeerId& AuthSSLimpl::OwnId()
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
	for(uint32_t i = 0; i < signlen; i++) 
	{
		rs_sprintf_append(sign, "%02x", (uint32_t) (signature[i]));
	}

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
                        unsigned char *sign, unsigned int signlen, const RsPeerId& sslId)
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

X509 *AuthSSLimpl::SignX509ReqWithGPG(X509_REQ *req, long /*days*/)
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
        unsigned long chtype = MBSTRING_UTF8;
        X509_NAME *issuer_name = X509_NAME_new();
        X509_NAME_add_entry_by_txt(issuer_name, "CN", chtype,
                        (unsigned char *) AuthGPG::getAuthGPG()->getGPGOwnId().toStdString().c_str(), -1, -1, 0);
/****
        X509_NAME_add_entry_by_NID(issuer_name, 48, 0,
                        (unsigned char *) "email@email.com", -1, -1, 0);
        X509_NAME_add_entry_by_txt(issuer_name, "O", chtype,
                        (unsigned char *) "org", -1, -1, 0);
        X509_NAME_add_entry_by_txt(x509_name, "L", chtype,
                        (unsigned char *) "loc", -1, -1, 0);
****/

        std::cerr << "AuthSSLimpl::SignX509Req() Issuer name: " << AuthGPG::getAuthGPG()->getGPGOwnId().toStdString() << std::endl;

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

        // NEW code, set validity time between null and null
        // (does not leak the key creation date to the outside anymore. for more privacy)
        ASN1_TIME_set(X509_get_notBefore(x509), 0);
        ASN1_TIME_set(X509_get_notAfter(x509), 0);

        // OLD code, sets validity time of cert to be between now and some days in the future
        /*
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
        */

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
        int inl=0,hashoutl=0;
        int sigoutl=0;
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

        hashoutl=EVP_MD_size(type);
        buf_hashout=(unsigned char *)OPENSSL_malloc((unsigned int)hashoutl);

        sigoutl=2048; // hashoutl; //EVP_PKEY_size(pkey);
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
        if(buf_in != NULL)
            OPENSSL_free(buf_in) ;
        if(buf_hashout != NULL)
            OPENSSL_free(buf_hashout) ;
        if(buf_sigout != NULL)
            OPENSSL_free(buf_sigout) ;
        std::cerr << "GPGAuthMgr::SignX509Req() err: FAIL" << std::endl;

        return NULL;
}


/* This function, checks that the X509 is signed by a known GPG key,
 * NB: we do not have to have approved this person as a friend.
 * this is important - as it allows non-friends messages to be validated.
 */

bool AuthSSLimpl::AuthX509WithGPG(X509 *x509,uint32_t& diagnostic)
{
#ifdef AUTHSSL_DEBUG
	fprintf(stderr, "AuthSSLimpl::AuthX509WithGPG() called\n");
#endif

	if (!CheckX509Certificate(x509))
	{
		std::cerr << "AuthSSLimpl::AuthX509() X509 NOT authenticated : Certificate failed basic checks" << std::endl;
		diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_CERTIFICATE_NOT_VALID ;
		return false;
	}

	/* extract CN for peer Id */
	RsPgpId issuer(std::string(getX509CNString(x509->cert_info->issuer)));
	RsPeerDetails pd;
#ifdef AUTHSSL_DEBUG
	std::cerr << "Checking GPG issuer : " << issuer.toStdString() << std::endl ;
#endif
	if (!AuthGPG::getAuthGPG()->getGPGDetails(issuer, pd)) {
		std::cerr << "AuthSSLimpl::AuthX509() X509 NOT authenticated : AuthGPG::getAuthGPG()->getGPGDetails() returned false." << std::endl;
		diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_ISSUER_UNKNOWN ;
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
	int inl=0,hashoutl=0;
	int sigoutl=0;
	//X509_ALGOR *a;

	EVP_MD_CTX_init(&ctx);

	/* input buffer */
	inl=i2d(data,NULL);
	buf_in=(unsigned char *)OPENSSL_malloc((unsigned int)inl);

	hashoutl=EVP_MD_size(type);
	buf_hashout=(unsigned char *)OPENSSL_malloc((unsigned int)hashoutl);

	sigoutl=2048; //hashoutl; //EVP_PKEY_size(pkey);
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
		diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_MALLOC_ERROR ;
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
		diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_MALLOC_ERROR ;
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
		diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_WRONG_SIGNATURE ;
		goto err;
	}

#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSLimpl::AuthX509() X509 authenticated" << std::endl;
#endif

	OPENSSL_free(buf_in) ;
	OPENSSL_free(buf_hashout) ;
	OPENSSL_free(buf_sigout) ;

	diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_OK ;

	return true;

err:
	std::cerr << "AuthSSLimpl::AuthX509() X509 NOT authenticated" << std::endl;

	if(buf_in != NULL)
		OPENSSL_free(buf_in) ;
	if(buf_hashout != NULL)
		OPENSSL_free(buf_hashout) ;
	if(buf_sigout != NULL)
		OPENSSL_free(buf_sigout) ;
	return false;
}



	/* validate + get id */
bool    AuthSSLimpl::ValidateCertificate(X509 *x509, RsPeerId &peerId)
{
	uint32_t auth_diagnostic ;

	/* check self signed */
	if (!AuthX509WithGPG(x509,auth_diagnostic))
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSLimpl::ValidateCertificate() bad certificate.";
		std::cerr << std::endl;
#endif
		return false;
	}
	RsPeerId peerIdstr ;

	if(!getX509id(x509, peerIdstr)) 
	{
#ifdef AUTHSSL_DEBUG
		std::cerr << "AuthSSLimpl::ValidateCertificate() Cannot retrieve peer id from certificate..";
		std::cerr << std::endl;
#endif
		return false;
	}
	peerId = peerIdstr ;

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

	X509 *x509 = X509_STORE_CTX_get_current_cert(ctx) ;

	if(x509 != NULL)
	{
		RsPgpId gpgid (std::string(getX509CNString(x509->cert_info->issuer)));
		if(gpgid.isNull()) 
		{
			std::cerr << "verify_x509_callback(): wrong PGP id \"" << std::string(getX509CNString(x509->cert_info->issuer)) << "\"" << std::endl;
			return false ;
		}

		std::string sslcn = getX509CNString(x509->cert_info->subject);
		RsPeerId sslid ;

		getX509id(x509,sslid);

		if(sslid.isNull()) 
		{
			std::cerr << "verify_x509_callback(): wrong SSL id \"" << std::string(getX509CNString(x509->cert_info->subject)) << "\"" << std::endl;
			return false ;
		}

		AuthSSL::getAuthSSL()->setCurrentConnectionAttemptInfo(gpgid,sslid,sslcn) ;
	} 

	return verify;
}

int AuthSSLimpl::VerifyX509Callback(int preverify_ok, X509_STORE_CTX *ctx)
{
    char    buf[256];
    X509   *err_cert;

    err_cert = X509_STORE_CTX_get_current_cert(ctx);
#ifdef AUTHSSL_DEBUG
    int err, depth;
    err = X509_STORE_CTX_get_error(ctx);
    depth = X509_STORE_CTX_get_error_depth(ctx);
#endif

    if(err_cert == NULL)
    {
        std::cerr << "AuthSSLimpl::VerifyX509Callback(): Cannot get certificate. Error!" << std::endl;
        return false ;
    }
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
        uint32_t auth_diagnostic ;

        /* do the REAL Authentication */
        if (!AuthX509WithGPG(X509_STORE_CTX_get_current_cert(ctx),auth_diagnostic))
        {
#ifdef AUTHSSL_DEBUG
            fprintf(stderr, "AuthSSLimpl::VerifyX509Callback() X509 not authenticated.\n");
#endif
            std::cerr << "(WW) Certificate was rejected because authentication failed. Diagnostic = " << auth_diagnostic << std::endl;
            return false;
        }
        RsPgpId pgpid = RsPgpId(std::string(getX509CNString(X509_STORE_CTX_get_current_cert(ctx)->cert_info->issuer)));

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
        RsPeerId certId;
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


bool    AuthSSLimpl::encrypt(void *&out, int &outlen, const void *in, int inlen, const RsPeerId& peerId)
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
        out = (unsigned char*)malloc(inlen + cipher_block_size + size_net_ekl + eklen + EVP_MAX_IV_LENGTH);

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
            free(out);
            out = NULL;
            return false;
        }

    	// move along to partial block space
    	out_offset += out_currOffset;

    	// add padding
        if(!EVP_SealFinal(&ctx, (unsigned char*) out + out_offset, &out_currOffset)) {
            free(ek);
				free(out) ;
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

        out = (unsigned char*)malloc(inlen - in_offset);

        if(!EVP_OpenUpdate(&ctx, (unsigned char*) out, &out_currOffset, (unsigned char*)in + in_offset, inlen - in_offset)) {
            free(ek);
				free(out) ;
            out = NULL;
            return false;
        }

        in_offset += out_currOffset;
        outlen += out_currOffset;

        if(!EVP_OpenFinal(&ctx, (unsigned char*)out + out_currOffset, &out_currOffset)) {
            free(ek);
				free(out) ;
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

void AuthSSLimpl::setCurrentConnectionAttemptInfo(const RsPgpId& gpg_id,const RsPeerId& ssl_id,const std::string& ssl_cn)
{
#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSL: registering connection attempt from:" << std::endl;
	std::cerr << "    GPG id: " << gpg_id << std::endl;
	std::cerr << "    SSL id: " << ssl_id << std::endl;
	std::cerr << "    SSL cn: " << ssl_cn << std::endl;
#endif
	_last_gpgid_to_connect = gpg_id ;
	_last_sslid_to_connect = ssl_id ;
	_last_sslcn_to_connect = ssl_cn ;
}
void AuthSSLimpl::getCurrentConnectionAttemptInfo(RsPgpId& gpg_id,RsPeerId& ssl_id,std::string& ssl_cn)
{
	gpg_id = _last_gpgid_to_connect ;
	ssl_id = _last_sslid_to_connect ;
	ssl_cn = _last_sslcn_to_connect ;
}

/* store for discovery */
bool    AuthSSLimpl::FailedCertificate(X509 *x509, const RsPgpId& gpgid,
													const RsPeerId& sslid,
													const std::string& sslcn,
													const struct sockaddr_storage& addr, 
													bool incoming)
{
	std::string ip_address = sockaddr_storage_tostring(addr);

	uint32_t auth_diagnostic = 0 ;
	bool authed ;

	if(x509 == NULL)
	{
		auth_diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_CERTIFICATE_MISSING ;
		authed = false ;
	}
	else
		authed = AuthX509WithGPG(x509,auth_diagnostic) ;

	if(authed)
		LocalStoreCert(x509);

#ifdef AUTHSSL_DEBUG
	std::cerr << "AuthSSLimpl::FailedCertificate() ";
#endif
	if (incoming)
	{
		RsServer::notify()->AddPopupMessage(RS_POPUP_CONNECT_ATTEMPT, gpgid.toStdString(), sslcn, sslid.toStdString());

		switch(auth_diagnostic)
		{
			case RS_SSL_HANDSHAKE_DIAGNOSTIC_CERTIFICATE_MISSING: 	RsServer::notify()->AddFeedItem(RS_FEED_ITEM_SEC_MISSING_CERTIFICATE, gpgid.toStdString(), sslid.toStdString(), sslcn, ip_address);
																					  	break ;
			case RS_SSL_HANDSHAKE_DIAGNOSTIC_CERTIFICATE_NOT_VALID: 	RsServer::notify()->AddFeedItem(RS_FEED_ITEM_SEC_BAD_CERTIFICATE, gpgid.toStdString(), sslid.toStdString(), sslcn, ip_address);
																					  	break ;
			case RS_SSL_HANDSHAKE_DIAGNOSTIC_ISSUER_UNKNOWN: 			RsServer::notify()->AddFeedItem(RS_FEED_ITEM_SEC_UNKNOWN_IN     , gpgid.toStdString(), sslid.toStdString(), sslcn, ip_address);
																					  	break ;
			case RS_SSL_HANDSHAKE_DIAGNOSTIC_MALLOC_ERROR:				RsServer::notify()->AddFeedItem(RS_FEED_ITEM_SEC_INTERNAL_ERROR , gpgid.toStdString(), sslid.toStdString(), sslcn, ip_address);
																					  	break ;
			case RS_SSL_HANDSHAKE_DIAGNOSTIC_WRONG_SIGNATURE: 			RsServer::notify()->AddFeedItem(RS_FEED_ITEM_SEC_WRONG_SIGNATURE, gpgid.toStdString(), sslid.toStdString(), sslcn, ip_address);
																					  	break ;
			case RS_SSL_HANDSHAKE_DIAGNOSTIC_OK: 							
			case RS_SSL_HANDSHAKE_DIAGNOSTIC_UNKNOWN:
			default:
																						RsServer::notify()->AddFeedItem(RS_FEED_ITEM_SEC_CONNECT_ATTEMPT, gpgid.toStdString(), sslid.toStdString(), sslcn, ip_address);
		}

#ifdef AUTHSSL_DEBUG
		std::cerr << " Incoming from: ";
#endif
	}
	else 
	{
		if(authed)
			RsServer::notify()->AddFeedItem(RS_FEED_ITEM_SEC_AUTH_DENIED, gpgid.toStdString(), sslid.toStdString(), sslcn, ip_address);
		else
			RsServer::notify()->AddFeedItem(RS_FEED_ITEM_SEC_UNKNOWN_OUT, gpgid.toStdString(), sslid.toStdString(), sslcn, ip_address);

#ifdef AUTHSSL_DEBUG
		std::cerr << " Outgoing to: ";
#endif
	}
	
#ifdef AUTHSSL_DEBUG
	std::cerr << "GpgId: " << gpgid << " SSLcn: " << sslcn << " peerId: " << sslid << ", ip address: " << ip_address;
	std::cerr << std::endl;
#endif

	return false;
}

bool    AuthSSLimpl::CheckCertificate(const RsPeerId& id, X509 *x509)
{
	(void) id; /* remove unused parameter warning */

	uint32_t diagnos ;
	/* if auths -> store */
	if (AuthX509WithGPG(x509,diagnos))
	{
		LocalStoreCert(x509);
		return true;
	}
	return false;
}



/* Locked search -> internal help function */
bool AuthSSLimpl::locked_FindCert(const RsPeerId& id, sslcert **cert)
{
	std::map<RsPeerId, sslcert *>::iterator it;
	
	if (mCerts.end() != (it = mCerts.find(id)))
	{
		*cert = it->second;
		return true;
	}
	return false;
}


/* Remove Certificate */

bool AuthSSLimpl::RemoveX509(RsPeerId id)
{
	std::map<RsPeerId, sslcert *>::iterator it;
	
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
	RsPeerId peerId ;
	if(!getX509id(x509, peerId))
	{
		std::cerr << "AuthSSLimpl::LocalStoreCert() Cannot retrieve peer id from certificate." << std::endl;
#ifdef AUTHSSL_DEBUG
#endif
		return false;
	}

	if(peerId.isNull())
	{
		std::cerr << "AuthSSLimpl::LocalStoreCert(): invalid peer id \"" << peerId << "\"" << std::endl;
		return false ;
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
	std::map<RsPeerId, sslcert *>::iterator it;
	
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
        std::map<RsPeerId, sslcert*>::iterator mapIt;
        for (mapIt = mCerts.begin(); mapIt != mCerts.end(); ++mapIt) {
            if (mapIt->first == mOwnId) {
                continue;
            }
            RsTlvKeyValue kv;
            kv.key = mapIt->first.toStdString();
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
        for(it = load.begin(); it != load.end(); ++it) {
                RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet *>(*it);

                if(vitem) {
                        #ifdef AUTHSSL_DEBUG
                        std::cerr << "AuthSSLimpl::loadList() General Variable Config Item:" << std::endl;
                        vitem->print(std::cerr, 10);
                        std::cerr << std::endl;
                        #endif

                        std::list<RsTlvKeyValue>::iterator kit;
                        for(kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit) {
                            if (RsPeerId(kit->key) == mOwnId) {
                                continue;
                            }

                            X509 *peer = loadX509FromPEM(kit->value);
			    /* authenticate it */
				uint32_t diagnos ;
			    if (AuthX509WithGPG(peer,diagnos))
			    {
				LocalStoreCert(peer);
			    }
                        }
                }
                delete (*it);
        }
        load.clear() ;
        return true;
}

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


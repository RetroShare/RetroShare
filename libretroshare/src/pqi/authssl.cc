/*******************************************************************************
 * libretroshare/src/pqi: authssl.cc                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2008  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#ifdef WINDOWS_SYS
#include "util/rswin.h"
#endif // WINDOWS_SYS

#include "authssl.h"
#include "sslfns.h"

#include "pqinetwork.h"
#include "authgpg.h"
#include "rsitems/rsconfigitems.h"
#include "util/rsdir.h"
#include "util/rsstring.h"
#include "pgp/pgpkeyutil.h"

#include "retroshare/rspeers.h" // for RsPeerDetails structure 
#include "retroshare/rsids.h" // for RsPeerDetails structure
#include "rsserver/p3face.h" 

/******************** notify of new Cert **************************/

#include <openssl/rand.h>
#include <openssl/ssl.h>

#include <iomanip>

/* SSL connection diagnostic */

const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_UNKNOWN                     = 0x00 ;
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_OK                          = 0x01 ;
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_CERTIFICATE_NOT_VALID       = 0x02 ;
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_ISSUER_UNKNOWN              = 0x03 ;
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_MALLOC_ERROR                = 0x04 ;
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_WRONG_SIGNATURE             = 0x05 ;
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_CERTIFICATE_MISSING         = 0x06 ;
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_HASH_ALGORITHM_NOT_ACCEPTED = 0x07 ;
//const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_KEY_ALGORITHM_NOT_ACCEPTED  = 0x08 ;
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_WRONG_SIGNATURE_TYPE        = 0x09 ;
const uint32_t RS_SSL_HANDSHAKE_DIAGNOSTIC_WRONG_SIGNATURE_VERSION     = 0x0a ;

/****
 * #define AUTHSSL_DEBUG 1
 ***/

static pthread_mutex_t* mutex_buf = nullptr;

struct CRYPTO_dynlock_value
{
	pthread_mutex_t mutex;
};

# if OPENSSL_VERSION_NUMBER < 0x10100000L
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
#endif

# if OPENSSL_VERSION_NUMBER < 0x10100000L
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
#endif

# if OPENSSL_VERSION_NUMBER < 0x10100000L
/**
 * OpenSSL allocate and initialize dynamic crypto lock.
 *
 * @param    file    source file name
 * @param    line    source file line number
 */
static struct CRYPTO_dynlock_value *dyn_create_function(const char */*file*/, int /*line*/)
{
	struct CRYPTO_dynlock_value *value;

	value = (struct CRYPTO_dynlock_value*) rs_malloc(sizeof(struct CRYPTO_dynlock_value));
	if (!value) 
		return NULL;
	
	pthread_mutex_init(&value->mutex, NULL);

	return value;
}
#endif

# if OPENSSL_VERSION_NUMBER < 0x10100000L
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
#endif

# if OPENSSL_VERSION_NUMBER < 0x10100000L
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
#endif

/**
 * Initialize TLS library.
 *
 * @return    true on success, false on error
 */
bool tls_init()
{
	/* static locks area */
	mutex_buf = (pthread_mutex_t*) rs_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
	if (mutex_buf == NULL) 
		return false;
	
	for (int i = 0; i < CRYPTO_num_locks(); i++) {
		pthread_mutex_init(&mutex_buf[i], NULL);
	}
# if OPENSSL_VERSION_NUMBER < 0x10100000L
	/* static locks callbacks */
	CRYPTO_set_locking_callback(locking_function);
	CRYPTO_set_id_callback(id_function);
	/* dynamic locks callbacks */
	CRYPTO_set_dynlock_create_callback(dyn_create_function);
	CRYPTO_set_dynlock_lock_callback(dyn_lock_function);
	CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function);
#endif
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

/*static*/ AuthSSL& AuthSSL::instance()
{
	static AuthSSLimpl mInstance;
	return mInstance;
}

AuthSSL* AuthSSL::getAuthSSL() { return &instance(); }

AuthSSL::~AuthSSL() = default;


/********************************************************************************/
/********************************************************************************/
/*********************   Cert Search / Add / Remove    **************************/
/********************************************************************************/
/********************************************************************************/

static int verify_x509_callback(int preverify_ok, X509_STORE_CTX *ctx);

std::string RsX509Cert::getCertName(const X509& x509)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	return getX509CNString(x509.cert_info->subject);
#else
	return getX509CNString(X509_get_subject_name(&x509));
#endif
}

std::string RsX509Cert::getCertLocation(const X509& x509)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	return getX509LocString(x509.cert_info->subject);
#else
	return getX509LocString(X509_get_subject_name(&x509));
#endif
}

std::string RsX509Cert::getCertOrg(const X509& x509)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	return getX509OrgString(x509.cert_info->subject);
#else
	return getX509OrgString(X509_get_subject_name(&x509));
#endif
}

/*static*/ RsPgpId RsX509Cert::getCertIssuer(const X509& x509)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	return RsPgpId(getX509CNString(x509.cert_info->issuer));
#else
	return RsPgpId(getX509CNString(X509_get_issuer_name(&x509)));
#endif
}

/*static*/ std::string RsX509Cert::getCertIssuerString(const X509& x509)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	return getX509CNString(x509.cert_info->issuer);
#else
	return getX509CNString(X509_get_issuer_name(&x509));
#endif
}

/*static*/ RsPeerId RsX509Cert::getCertSslId(const X509& x509)
{
	RsPeerId sslid;
	return getX509id(const_cast<X509*>(&x509), sslid) ? sslid : RsPeerId();
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


AuthSSLimpl::AuthSSLimpl() :
    p3Config(), sslctx(nullptr), mOwnCert(nullptr), sslMtx("AuthSSL"),
    mOwnPrivateKey(nullptr), mOwnPublicKey(nullptr), init(0) {}

bool AuthSSLimpl::active() { return init; }

int AuthSSLimpl::InitAuth(
        const char* cert_file, const char* priv_key_file, const char* passwd,
        std::string locationName )
{
	/* single call here si don't need to invoke mutex yet */
	static int initLib = 0;
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

	//SSL_OP_SINGLE_DH_USE 	CVE-2016-0701
	//https://www.openssl.org/docs/manmaster/ssl/SSL_CTX_set_options.html
	//If "strong" primes were used, it is not strictly necessary to generate a new DH key during each handshake but it is also recommended. SSL_OP_SINGLE_DH_USE should therefore be enabled whenever temporary/ephemeral DH parameters are used.
	//SSL_CTX_set_options() adds the options set via bitmask in options to ctx. Options already set before are not cleared!
        SSL_CTX_set_options(sslctx,SSL_OP_SINGLE_DH_USE) ;


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
	//Not used (should be here: /libretroshare/src/gxstunnel/p3gxstunnel.cc:1131
	//std::string dh_prime_2048_hex = "B3B86A844550486C7EA459FA468D3A8EFD71139593FE1C658BBEFA9B2FC0AD2628242C2CDC2F91F5B220ED29AAC271192A7374DFA28CDDCA70252F342D0821273940344A7A6A3CB70C7897A39864309F6CAC5C7EA18020EF882693CA2C12BB211B7BA8367D5A7C7252A5B5E840C9E8F081469EBA0B98BCC3F593A4D9C4D5DF539362084F1B9581316C1F80FDAD452FD56DBC6B8ED0775F596F7BB22A3FE2B4753764221528D33DB4140DE58083DB660E3E105123FC963BFF108AC3A268B7380FFA72005A1515C371287C5706FFA6062C9AC73A9B1A6AC842C2764CDACFC85556607E86611FDF486C222E4896CDF6908F239E177ACC641FCBFF72A758D1C10CBB" ;

	std::string dh_prime_4096_hex = "A6F5777292D9E6BB95559C9124B9119E6771F11F2048C8FE74F4E8140494520972A087EF1D60B73894F1C5D509DD15D96CF379E9DDD46CE51B748085BACB440D915565782C73AF3A9580CE788441D1DA4D114E3D302CAB45A061ABCFC1F7E9200AE019CB923B77E096FA9377454A16FFE91D86535FF23E075B3E714F785CD7606E9CBD9D06F01CAFA2271883D649F13ABE170D714F6B6EC064C5BF35C4F4BDA5EF5ED5E70D5DC78F1AC1CDC04EEDAE8ADD65C4A9E27368E0B2C8595DD7626D763BFFB15364B3CCA9FCE814B9226B35FE652F4B041F0FF6694D6A482B0EF48CA41163D083AD2DE7B7A068BB05C0453E9D008551C7F67993A3EF2C4874F0244F78C4E0997BD31AB3BD88446916B499B2513DD5BA002063BD38D2CE55D29D071399D5CEE99458AF6FDC104A61CA3FACDAC803CBDE62B4C0EAC946D0E12F05CE9E94497110D64E611D957423B8AA412D84EC83E6E70E0977A31D6EE056D0527D4667D7242A77C9B679D191562E4026DA9C35FF85666296D872ED548E0FFE1A677FCC373C1F490CAB4F53DFD8735C0F1DF02FEAD824A217FDF4E3404D38A5BBC719C6622630FCD34F6F1968AF1B66A4AB1A9FCF653DA96EB3A42AF6FCFEA0547B8F314A527C519949007D7FA1726FF3D33EC46393B0207AA029E5EA574BDAC94D78894B22A2E3303E65A3F820DF57DB44951DE4E973C016C57F7A242D0BC53BC563AF" ;

	std::string cipherString = "kEDH+HIGH:!DSS:!aNULL:!3DES:!EXP";

	SSL_CTX_set_cipher_list(sslctx, cipherString.c_str());

	DH* dh = DH_new();
	int codes = 0;
	bool pfs_enabled = true ;

	if (dh)
	{
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
		BN_hex2bn(&dh->p,dh_prime_4096_hex.c_str()) ;
		BN_hex2bn(&dh->g,"5") ;
#else
        BIGNUM *pp=NULL,*gg=NULL ;

        BN_hex2bn(&pp,dh_prime_4096_hex.c_str()) ;
        BN_hex2bn(&gg,"5");

        DH_set0_pqg(dh,pp,NULL,gg) ;
#endif

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

	int result = SSL_CTX_use_certificate(sslctx, x509);

#if OPENSSL_VERSION_NUMBER >= 0x10101000L
	if(result != 1)
	{
		// In debian Buster, openssl security level is set to 2 which preclude the use of SHA1, originally used to sign RS certificates.
		// As a consequence, on these systems, locations created with RS previously to Jan.2020 will not start unless we revert the
		// security level to a value of 1.

		int save_sec = SSL_CTX_get_security_level(sslctx);
		SSL_CTX_set_security_level(sslctx,1);
		result = SSL_CTX_use_certificate(sslctx, x509);

		if(result == 1)
		{
			std::cerr << std::endl;
			std::cerr << "     Your Retroshare certificate uses low security settings that are incompatible " << std::endl;
			std::cerr << "(WW) with current security level " << save_sec << " of the OpenSSL library. Retroshare will still start " << std::endl;
			std::cerr << "     (with security level set to 1), but you should probably create a new location." << std::endl;
			std::cerr << std::endl;
		}
	}
#endif

    if(result != 1)
    {
        std::cerr << "(EE) Cannot use your Retroshare certificate. Some error occured in SSL_CTX_use_certificate()" << std::endl;
        return -1;
    }

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

	mOwnCert = x509;

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

	mOwnLocationName = locationName;

	init = 1;
	return 1;
}

/* Dummy function to be overloaded by real implementation */
bool	AuthSSLimpl::validateOwnCertificate(X509 *x509, EVP_PKEY *pkey)
{
	(void) pkey; /* remove unused parameter warning */

	uint32_t diagnostic ;

	/* standard authentication */
	if (!AuthX509WithGPG(x509,true,diagnostic))
	{
		std::cerr << "Validate Own certificate ERROR: diagnostic = " << diagnostic << std::endl;
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
{ return mOwnLocationName; }

std::string AuthSSLimpl::SaveOwnCertificateToString()
{ return saveX509ToPEM(mOwnCert); }

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
	unsigned char signature[signlen] ;
	memset(signature,0,signlen) ;

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
	X509* peercert;
	if (sslId == mOwnId) peercert = mOwnCert;
	else if (!locked_FindCert(sslId, &peercert))
	{
		std::cerr << "VerifySignBin() no peer" << std::endl;
		return false;
	}

	return SSL_VerifySignBin(data, len, sign, signlen, peercert);
}

bool AuthSSLimpl::VerifyOwnSignBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int signlen) 
{ return SSL_VerifySignBin(data, len, sign, signlen, mOwnCert); }


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

#ifdef V07_NON_BACKWARD_COMPATIBLE_CHANGE_002
		static const uint64_t CERTIFICATE_SERIAL_NUMBER = RS_CERTIFICATE_VERSION_NUMBER_07_0001 ;
#else
#ifdef V07_NON_BACKWARD_COMPATIBLE_CHANGE_001
		static const uint64_t CERTIFICATE_SERIAL_NUMBER = RS_CERTIFICATE_VERSION_NUMBER_06_0001 ;
#else
		static const uint64_t CERTIFICATE_SERIAL_NUMBER = RS_CERTIFICATE_VERSION_NUMBER_06_0000 ;
#endif
#endif

        BIGNUM *btmp = BN_new();
		BN_set_word(btmp,CERTIFICATE_SERIAL_NUMBER) ;

#ifdef OLD_CODE
        if (!BN_pseudo_rand(btmp, SERIAL_RAND_BITS, 0, 0))
        {
                std::cerr << "AuthSSLimpl::SignX509Req() rand FAIL" << std::endl;
                return NULL;
        }
#endif
        ASN1_INTEGER *serial = ASN1_INTEGER_new();

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
        //
        // The code has been copied in order to use the PGP signing instead of supplying the
        // private EVP_KEY to ASN1_sign(), which would be another alternative.

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
        int (*i2d)(X509_CINF*, unsigned char**) = i2d_X509_CINF;
        X509_ALGOR *algor1 = x509->cert_info->signature;
        X509_ALGOR *algor2 = x509->sig_alg;
        ASN1_BIT_STRING *signature = x509->signature;
        X509_CINF *data = x509->cert_info;
#else
        const X509_ALGOR *algor1 = X509_get0_tbs_sigalg(x509) ;
        const X509_ALGOR *algor2 = NULL ;

        const ASN1_BIT_STRING *tmp_signature = NULL ;

        X509_get0_signature(&tmp_signature,&algor2,x509);

        ASN1_BIT_STRING *signature = const_cast<ASN1_BIT_STRING*>(tmp_signature);
#endif
        //EVP_PKEY *pkey = NULL;
#ifdef V07_NON_BACKWARD_COMPATIBLE_CHANGE_002
        const EVP_MD *type = EVP_sha256();
#else
        const EVP_MD *type = EVP_sha1();
#endif

        EVP_MD_CTX *ctx = EVP_MD_CTX_create();
        int inl=0;
        X509_ALGOR *a;

        /* FIX ALGORITHMS */

        a = const_cast<X509_ALGOR*>(algor1);
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
        ASN1_TYPE_free(a->parameter);
        a->parameter=ASN1_TYPE_new();
        a->parameter->type=V_ASN1_NULL;

        ASN1_OBJECT_free(a->algorithm);
        a->algorithm=OBJ_nid2obj(type->pkey_type);
#else
        X509_ALGOR_set0(a,OBJ_nid2obj(EVP_MD_pkey_type(type)),V_ASN1_NULL,NULL);
#endif

        a = const_cast<X509_ALGOR*>(algor2);
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
        ASN1_TYPE_free(a->parameter);
        a->parameter=ASN1_TYPE_new();
        a->parameter->type=V_ASN1_NULL;

        ASN1_OBJECT_free(a->algorithm);
		a->algorithm=OBJ_nid2obj(type->pkey_type);
#else
        X509_ALGOR_set0(a,OBJ_nid2obj(EVP_MD_pkey_type(type)),V_ASN1_NULL,NULL);
#endif


        std::cerr << "Algorithms Fixed" << std::endl;

        unsigned int sigoutl=2048; // hashoutl; //EVP_PKEY_size(pkey);
        unsigned char *buf_sigout=(unsigned char *)OPENSSL_malloc((unsigned int)sigoutl);

        /* input buffer */
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
        inl=i2d(data,NULL);
        unsigned char *buf_in=(unsigned char *)OPENSSL_malloc((unsigned int)inl);

		if(buf_in == NULL)
		{
			sigoutl=0;
			fprintf(stderr, "AuthSSLimpl::SignX509Req: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_MALLOC_FAILURE)\n");
			return NULL ;
		}
		unsigned char *p=buf_in;	// This because i2d modifies the pointer after writing to it.
		i2d(data,&p);
#else
        unsigned char *buf_in=NULL;
        inl=i2d_re_X509_tbs(x509,&buf_in) ;	// this does the i2d over x509->cert_info
#endif

#ifdef V07_NON_BACKWARD_COMPATIBLE_CHANGE_003
        if((buf_in == NULL) || (buf_sigout == NULL))
		{
			sigoutl=0;
			fprintf(stderr, "AuthSSLimpl::SignX509Req: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_MALLOC_FAILURE)\n");
			goto err;
		}
        std::cerr << "Buffers Allocated" << std::endl;

        /* NOW Sign via GPG Functions */
        if (!AuthGPG::getAuthGPG()->SignDataBin(buf_in, inl, buf_sigout, (unsigned int *) &sigoutl,"AuthSSLimpl::SignX509ReqWithGPG()"))
        {
                sigoutl = 0;
                goto err;
        }
#else
        unsigned int hashoutl=EVP_MD_size(type);
        unsigned char *buf_hashout=(unsigned char *)OPENSSL_malloc((unsigned int)hashoutl);

        if((buf_hashout == NULL) || (buf_sigout == NULL))
		{
			hashoutl=0;
			sigoutl=0;
			fprintf(stderr, "AuthSSLimpl::SignX509Req: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_MALLOC_FAILURE)\n");
			goto err;
		}
        std::cerr << "Buffers Allocated" << std::endl;

        /* data in buf_in, ready to be hashed */
        EVP_DigestInit_ex(ctx,type, NULL);
        EVP_DigestUpdate(ctx,(unsigned char *)buf_in,inl);
        if (!EVP_DigestFinal(ctx,(unsigned char *)buf_hashout, (unsigned int *)&hashoutl))
		{
			hashoutl=0;
			fprintf(stderr, "AuthSSLimpl::SignX509Req: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_EVP_LIB)\n");
			goto err;
		}

		std::cerr << "Digest Applied: len: " << hashoutl << std::endl;

        /* NOW Sign via GPG Functions */
        if (!AuthGPG::getAuthGPG()->SignDataBin(buf_hashout, hashoutl, buf_sigout, (unsigned int *) &sigoutl,"AuthSSLimpl::SignX509ReqWithGPG()"))
        {
                sigoutl = 0;
                goto err;
        }
#endif

        std::cerr << "Buffer Sizes: in: " << inl;
#ifndef V07_NON_BACKWARD_COMPATIBLE_CHANGE_003
        std::cerr << "  HashOut: " << hashoutl;
#endif
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

        EVP_MD_CTX_destroy(ctx) ;

		// debug
		// {
		// int pkey_nid = OBJ_obj2nid(x509->sig_alg->algorithm);
		// const char* sslbuf = OBJ_nid2ln(pkey_nid);
		// std::cerr << "Signature hash algorithm: " << sslbuf << std::endl;
		// }

        return x509;

	/* XXX CLEANUP */
  err:
        /* cleanup */
        if(buf_in != NULL)
            OPENSSL_free(buf_in) ;
#ifndef V07_NON_BACKWARD_COMPATIBLE_CHANGE_003
        if(buf_hashout != NULL)
            OPENSSL_free(buf_hashout) ;
#endif
        if(buf_sigout != NULL)
            OPENSSL_free(buf_sigout) ;
        std::cerr << "GPGAuthMgr::SignX509Req() err: FAIL" << std::endl;

        return NULL;
}


bool AuthSSLimpl::AuthX509WithGPG(X509 *x509,bool verbose, uint32_t& diagnostic)
{
	RsPgpId issuer = RsX509Cert::getCertIssuer(*x509);
	RsPeerDetails pd;
	if (!AuthGPG::getAuthGPG()->getGPGDetails(issuer, pd))
	{
		RsInfo() << __PRETTY_FUNCTION__ << " X509 NOT authenticated : "
		         << "AuthGPG::getAuthGPG()->getGPGDetails(" << issuer
		         << ",...) returned false." << std::endl;
		diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_ISSUER_UNKNOWN;
		return false;
	}
	else
		Dbg3() << __PRETTY_FUNCTION__ << " issuer: " << issuer << " found"
		       << std::endl;

	/* verify GPG signature */
	/*** NOW The Manual signing bit (HACKED FROM asn1/a_sign.c) ***/

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	int (*i2d)(X509_CINF*, unsigned char**) = i2d_X509_CINF;
	ASN1_BIT_STRING* signature = x509->signature;
	X509_CINF* data = x509->cert_info;
#else
	const ASN1_BIT_STRING* signature = nullptr;
	const X509_ALGOR* algor2 = nullptr;
	X509_get0_signature(&signature,&algor2,x509);
#endif

	uint32_t certificate_version = getX509RetroshareCertificateVersion(x509);

	EVP_MD_CTX* ctx = EVP_MD_CTX_create();
	int inl = 0;

	const unsigned char* signed_data = nullptr;
	uint32_t signed_data_length = 0;

	/* input buffer */
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	inl = i2d(data, nullptr);
	unsigned char* buf_in = static_cast<unsigned char *>(
	            OPENSSL_malloc(static_cast<unsigned int>(inl)) );
	unsigned char* p = nullptr;
#else
	unsigned char* buf_in = nullptr;
	inl=i2d_re_X509_tbs(x509,&buf_in) ;	// this does the i2d over x509->cert_info
#endif

	if(buf_in == nullptr)
	{
		RsErr() << __PRETTY_FUNCTION__
		        << " ASN1err(ASN1_F_ASN1_SIGN,ERR_R_MALLOC_FAILURE)" << std::endl;
		diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_MALLOC_ERROR ;
		return false;
	}

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	p = buf_in;
	i2d(data,&p);
#endif

	{ // this scope is to avoid cross-initialization jumps to err.

		const Sha1CheckSum sha1 = RsDirUtil::sha1sum(
		            buf_in, static_cast<uint32_t>(inl) );

		if(certificate_version < RS_CERTIFICATE_VERSION_NUMBER_07_0001)
		{
			/* If the certificate belongs to 0.6 version, we hash it here, and
			 * then re-hash the hash it in the PGP signature */
			signed_data = sha1.toByteArray();
			signed_data_length = sha1.SIZE_IN_BYTES;
		}
		else
		{
			signed_data = buf_in ;
			signed_data_length = static_cast<uint32_t>(inl);
		}

		/* NOW check sign via GPG Functions */

		Dbg2() << __PRETTY_FUNCTION__
		       << " verifying the PGP Key signature with finger print: "
		       << pd.fpr << std::endl;

		/* Take a early look at signature parameters. In particular we dont
		 * accept signatures with unsecure hash algorithms */

		PGPSignatureInfo signature_info ;
		PGPKeyManagement::parseSignature(
		            signature->data, static_cast<size_t>(signature->length),
		            signature_info );

		if(signature_info.signature_version != PGP_PACKET_TAG_SIGNATURE_VERSION_V4)
		{
			diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_WRONG_SIGNATURE_VERSION;
			goto err;
		}

		std::string sigtypestring;

		switch(signature_info.signature_type)
		{
		case PGP_PACKET_TAG_SIGNATURE_TYPE_BINARY_DOCUMENT: break;
		case PGP_PACKET_TAG_SIGNATURE_TYPE_STANDALONE_SIG: /*fallthrough*/
		case PGP_PACKET_TAG_SIGNATURE_TYPE_CANONICAL_TEXT: /*fallthrough*/
		case PGP_PACKET_TAG_SIGNATURE_TYPE_UNKNOWN: /*fallthrough*/
		default:
			diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_WRONG_SIGNATURE_TYPE;
			goto err;
		}

		switch(signature_info.public_key_algorithm)
		{
		case PGP_PACKET_TAG_PUBLIC_KEY_ALGORITHM_RSA_ES: /*fallthrough*/
		case PGP_PACKET_TAG_PUBLIC_KEY_ALGORITHM_RSA_S:
			sigtypestring = "RSA";
			break ;
		case PGP_PACKET_TAG_PUBLIC_KEY_ALGORITHM_DSA:
			sigtypestring = "DSA";
			break ;
		case PGP_PACKET_TAG_PUBLIC_KEY_ALGORITHM_RSA_E: /*fallthrough*/
		case PGP_PACKET_TAG_PUBLIC_KEY_ALGORITHM_UNKNOWN: /*fallthrough*/
		default:
			diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_HASH_ALGORITHM_NOT_ACCEPTED ;
			goto err ;
		}

		switch(signature_info.hash_algorithm)
		{
		case PGP_PACKET_TAG_HASH_ALGORITHM_SHA1:
			sigtypestring += "+SHA1";
			break;
		case PGP_PACKET_TAG_HASH_ALGORITHM_SHA256:
			sigtypestring += "+SHA256";
			break;
		case PGP_PACKET_TAG_HASH_ALGORITHM_SHA512:
			sigtypestring += "+SHA512";
			break;
		// We dont accept signatures with unknown or weak hash algorithms.
		case  PGP_PACKET_TAG_HASH_ALGORITHM_MD5: /*fallthrough*/
		case  PGP_PACKET_TAG_HASH_ALGORITHM_UNKNOWN: /*fallthrough*/
		default:
			diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_HASH_ALGORITHM_NOT_ACCEPTED;
			goto err;
		}

		// passed, verify the signature itself

		if (!AuthGPG::getAuthGPG()->VerifySignBin(
		            signed_data, signed_data_length, signature->data,
		            static_cast<unsigned int>(signature->length), pd.fpr ))
		{
			diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_WRONG_SIGNATURE;
			goto err;
		}

        if(verbose)
			RsInfo() << " Verified: " << sigtypestring
			         << " signature of certificate sslId: "
			         << RsX509Cert::getCertSslId(*x509)
			         << ", Version " << std::hex << certificate_version << std::dec
			         << " using PGP key " << pd.fpr << " " << pd.name << std::endl;
	}

	EVP_MD_CTX_destroy(ctx);

	OPENSSL_free(buf_in);

	diagnostic = RS_SSL_HANDSHAKE_DIAGNOSTIC_OK;

	return true;

err: // TODO: this label is very short and might collide every easly
	RsInfo() << __PRETTY_FUNCTION__ << " X509 PGP authentication failed with "
	         << "diagnostic: " << diagnostic << std::endl;

	if(buf_in) OPENSSL_free(buf_in);

	return false;
}

/********************************************************************************/
/********************************************************************************/
/****************************  encrypt / decrypt fns ****************************/
/********************************************************************************/
/********************************************************************************/

static int verify_x509_callback(int preverify_ok, X509_STORE_CTX *ctx)
{ return AuthSSL::instance().VerifyX509Callback(preverify_ok, ctx); }

int AuthSSLimpl::VerifyX509Callback(int /*preverify_ok*/, X509_STORE_CTX* ctx)
{
	/* According to OpenSSL documentation must return 0 if verification failed
	 * and 1 if succeded (aka can continue connection).
	 * About preverify_ok OpenSSL documentation doesn't tell which value is
	 * passed to the first callback in the authentication chain, it just says
	 * that the result of previous step is passed down, so I have tested it
	 * and we get passed 0 always so in our case as there is no other
	 * verifications step vefore we ignore it completely */

	constexpr int verificationFailed = 0;
	constexpr int verificationSuccess = 1;

	using Evt_t = RsAuthSslConnectionAutenticationEvent;
	std::unique_ptr<Evt_t> ev = std::unique_ptr<Evt_t>(new Evt_t);

	X509* x509Cert = X509_STORE_CTX_get_current_cert(ctx);
	if(!x509Cert)
	{
		std::string errMsg = "Cannot get certificate! OpenSSL error: " +
		        std::to_string(X509_STORE_CTX_get_error(ctx)) + " depth: " +
		        std::to_string(X509_STORE_CTX_get_error_depth(ctx));

		RsErr() << __PRETTY_FUNCTION__ << " " << errMsg << std::endl;

//		if(rsEvents)
//		{
//			ev->mErrorMsg = errMsg;
//			ev->mErrorCode = RsAuthSslConnectionAutenticationEvent::NO_CERTIFICATE_SUPPLIED;
//
//			rsEvents->postEvent(std::move(ev));
//		}

		return verificationFailed;
	}

	RsPeerId sslId = RsX509Cert::getCertSslId(*x509Cert);
	std::string sslCn = RsX509Cert::getCertIssuerString(*x509Cert);

	RsPgpId pgpId(sslCn);

    if(sslCn.length() == RsPgpFingerprint::SIZE_IN_BYTES*2)
	{
		RsPgpFingerprint pgpFpr(sslCn);	// we also accept fingerprint format, so that in the future we can switch to fingerprints without backward compatibility issues

		if(!pgpFpr.isNull())
			pgpId = PGPHandler::pgpIdFromFingerprint(pgpFpr);	// in the future, we drop PGP ids and keep the fingerprint all along
	}

	if(sslId.isNull())
	{
		std::string errMsg = "x509Cert has invalid sslId!";

		RsInfo() << __PRETTY_FUNCTION__ << " " << errMsg << std::endl;

		if(rsEvents)
		{
			ev->mSslCn = sslCn;
			ev->mSslId = sslId;
			ev->mPgpId = pgpId;
			ev->mErrorMsg = errMsg;
			ev->mErrorCode = RsAuthSslError::MISSING_AUTHENTICATION_INFO;

			rsEvents->postEvent(std::move(ev));
		}

		return verificationFailed;
	}

	if(pgpId.isNull())
	{
		std::string errMsg = "x509Cert has invalid pgpId! sslCn >>>" + sslCn +
		        "<<<";

		RsInfo() << __PRETTY_FUNCTION__ << " " << errMsg << std::endl;

		if(rsEvents)
		{
			ev->mSslId = sslId;
			ev->mSslCn = sslCn;
			ev->mErrorMsg = errMsg;
			ev->mErrorCode = RsAuthSslError::MISSING_AUTHENTICATION_INFO;

			rsEvents->postEvent(std::move(ev));
		}

		return verificationFailed;
	}

	bool isSslOnlyFriend = false;

    // For SSL only friends (ones added through short invites) we check that the fingerprint
    // in the key (det.gpg_id) matches the one of the handshake.
	{
		RsPeerDetails det;

		if(rsPeers->getPeerDetails(sslId,det))
			isSslOnlyFriend = det.skip_pgp_signature_validation;

		if(det.skip_pgp_signature_validation && det.gpg_id != pgpId)// in the future, we should compare fingerprints instead
		{
			std::string errorMsg = "Peer " + sslId.toStdString() + " trying to connect with issuer ID " + pgpId.toStdString()
                    + " whereas key ID " + det.gpg_id.toStdString() + " was expected! Refusing connection." ;

			RsErr() << __PRETTY_FUNCTION__ << errorMsg << std::endl;

			if(rsEvents)
			{
				ev->mSslId = sslId;
				ev->mSslCn = sslCn;
				ev->mPgpId = pgpId;
				ev->mErrorMsg = errorMsg;
				ev->mErrorCode = RsAuthSslError::MISMATCHED_PGP_ID;
				rsEvents->postEvent(std::move(ev));
			}

			return verificationFailed;
		}
	}

	uint32_t auth_diagnostic;
	if(!isSslOnlyFriend && !AuthX509WithGPG(x509Cert,true, auth_diagnostic))
	{
		std::string errMsg = "Certificate was rejected because PGP "
		                     "signature verification failed with diagnostic: "
		        + std::to_string(auth_diagnostic) + " certName: " +
		        RsX509Cert::getCertName(*x509Cert) + " sslId: " +
		        RsX509Cert::getCertSslId(*x509Cert).toStdString() +
		        " issuerString: " + RsX509Cert::getCertIssuerString(*x509Cert);

		RsInfo() << __PRETTY_FUNCTION__ << " " << errMsg << std::endl;

		if(rsEvents)
		{
			ev->mSslId = sslId;
			ev->mSslCn = sslCn;
			ev->mPgpId = pgpId;

			switch(auth_diagnostic)
			{
			case RS_SSL_HANDSHAKE_DIAGNOSTIC_ISSUER_UNKNOWN:
				ev->mErrorCode = RsAuthSslError::NOT_A_FRIEND;
				break;
			case RS_SSL_HANDSHAKE_DIAGNOSTIC_WRONG_SIGNATURE:
				ev->mErrorCode = RsAuthSslError::PGP_SIGNATURE_VALIDATION_FAILED;
				break;
			default:
				ev->mErrorCode = RsAuthSslError::MISSING_AUTHENTICATION_INFO;
				break;
			}

			ev->mErrorMsg = errMsg;
			rsEvents->postEvent(std::move(ev));
		}

		return verificationFailed;
	}
#ifdef AUTHSSL_DEBUG
    std::cerr << "******* VerifyX509Callback cert: " << std::hex << ctx->cert <<std::dec << std::endl;
#endif

	if ( !isSslOnlyFriend && pgpId != AuthGPG::getAuthGPG()->getGPGOwnId() && !AuthGPG::getAuthGPG()->isGPGAccepted(pgpId) )
	{
		std::string errMsg = "Connection attempt signed by PGP key id: " +
		        pgpId.toStdString() + " not accepted because it is not"
		                              " a friend.";

		Dbg1() << __PRETTY_FUNCTION__ << " " << errMsg << std::endl;

		if(rsEvents)
		{
			ev->mSslId = sslId;
			ev->mSslCn = sslCn;
			ev->mPgpId = pgpId;
			ev->mErrorMsg = errMsg;
			ev->mErrorCode = RsAuthSslError::NOT_A_FRIEND;
			rsEvents->postEvent(std::move(ev));
		}

		return verificationFailed;
	}

	//setCurrentConnectionAttemptInfo(pgpId, sslId, sslCn);
	LocalStoreCert(x509Cert);

	RsInfo() << __PRETTY_FUNCTION__ << " authentication successfull for "
	         << "sslId: " << sslId << " isSslOnlyFriend: " << isSslOnlyFriend
	         << std::endl;

	return verificationSuccess;
}

bool AuthSSLimpl::parseX509DetailsFromFile(
        const std::string& certFilePath, RsPeerId& certId,
        RsPgpId& issuer, std::string& location )
{
	FILE* tmpfp = RsDirUtil::rs_fopen(certFilePath.c_str(), "r");
	if(!tmpfp)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Failed to open Certificate File: "
		        << certFilePath << std::endl;
		return false;
	}

	// get xPGP certificate.
	X509* x509 = PEM_read_X509(tmpfp, nullptr, nullptr, nullptr);
	fclose(tmpfp);

	if(!x509)
	{
		RsErr() << __PRETTY_FUNCTION__ << " PEM_read_X509 failed!" << std::endl;
		return false;
	}

	uint32_t diagnostic = 0;
	if(!AuthX509WithGPG(x509,false, diagnostic))
	{
		RsErr() << __PRETTY_FUNCTION__ << " AuthX509WithGPG failed with "
		        << "diagnostic: " << diagnostic << std::endl;
		return false;
	}

	certId = RsX509Cert::getCertSslId(*x509);
	issuer = RsX509Cert::getCertIssuer(*x509);
	location = RsX509Cert::getCertLocation(*x509);

	X509_free(x509);

	if(certId.isNull() || issuer.isNull()) return false;

	return true;
}


/********************************************************************************/
/********************************************************************************/
/****************************  encrypt / decrypt fns ****************************/
/********************************************************************************/
/********************************************************************************/


bool    AuthSSLimpl::encrypt(void *&out, int &outlen, const void *in, int inlen, const RsPeerId& peerId)
{
	RS_STACK_MUTEX(sslMtx);

	/*const*/ EVP_PKEY* public_key = nullptr;
	if (peerId == mOwnId) { public_key = mOwnPublicKey; }
	else
	{
		if (!mCerts[peerId])
		{
			RsErr() << __PRETTY_FUNCTION__ << " public key not found."
			        << std::endl;
			return false;
		}
		else public_key = const_cast<EVP_PKEY*>(
		            RsX509Cert::getPubKey(*mCerts[peerId]) );
	}

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        int eklen, net_ekl;
        unsigned char *ek;
        unsigned char iv[EVP_MAX_IV_LENGTH];
        int out_currOffset = 0;
        int out_offset = 0;

        int max_evp_key_size = EVP_PKEY_size(public_key);
        ek = (unsigned char*)rs_malloc(max_evp_key_size);
        
        if(ek == NULL)
            return false ;
        
        const EVP_CIPHER *cipher = EVP_aes_128_cbc();
        int cipher_block_size = EVP_CIPHER_block_size(cipher);
        int size_net_ekl = sizeof(net_ekl);

        int max_outlen = inlen + cipher_block_size + EVP_MAX_IV_LENGTH + max_evp_key_size + size_net_ekl;

        // intialize context and send store encrypted cipher in ek
        if(!EVP_SealInit(ctx, EVP_aes_128_cbc(), &ek, &eklen, iv, &public_key, 1)) {
            free(ek);
            return false;
        }

    	// now assign memory to out accounting for data, and cipher block size, key length, and key length val
        out = (unsigned char*)rs_malloc(inlen + cipher_block_size + size_net_ekl + eklen + EVP_MAX_IV_LENGTH);

        if(out == NULL)
        {
            free(ek) ;
            return false ;
        }
    	net_ekl = htonl(eklen);
    	memcpy((unsigned char*)out + out_offset, &net_ekl, size_net_ekl);
    	out_offset += size_net_ekl;

    	memcpy((unsigned char*)out + out_offset, ek, eklen);
    	out_offset += eklen;

    	memcpy((unsigned char*)out + out_offset, iv, EVP_MAX_IV_LENGTH);
    	out_offset += EVP_MAX_IV_LENGTH;

    	// now encrypt actual data
        if(!EVP_SealUpdate(ctx, (unsigned char*) out + out_offset, &out_currOffset, (unsigned char*) in, inlen)) {
            free(ek);
            free(out);
            out = NULL;
            return false;
        }

    	// move along to partial block space
    	out_offset += out_currOffset;

    	// add padding
        if(!EVP_SealFinal(ctx, (unsigned char*) out + out_offset, &out_currOffset)) {
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

        EVP_CIPHER_CTX_free(ctx);

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
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        int eklen = 0, net_ekl = 0;
        unsigned char iv[EVP_MAX_IV_LENGTH];
        int ek_mkl = EVP_PKEY_size(mOwnPrivateKey);
        unsigned char *ek = (unsigned char*)malloc(ek_mkl);
        
        if(ek == NULL)
        {
            std::cerr << "(EE) Cannot allocate memory for " << ek_mkl << " bytes in " << __PRETTY_FUNCTION__ << std::endl;
            return false ;
        }

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

        if(0 == EVP_OpenInit(ctx, cipher, ek, eklen, iv, mOwnPrivateKey)) {
            free(ek);
            return false;
        }

        out = (unsigned char*)rs_malloc(inlen - in_offset);

        if(out == NULL)
        {
            free(ek) ;
            return false ;
        }
        if(!EVP_OpenUpdate(ctx, (unsigned char*) out, &out_currOffset, (unsigned char*)in + in_offset, inlen - in_offset)) {
            free(ek);
				free(out) ;
            out = NULL;
            return false;
        }

        //in_offset += out_currOffset;
        outlen += out_currOffset;

        if(!EVP_OpenFinal(ctx, (unsigned char*)out + out_currOffset, &out_currOffset)) {
            free(ek);
				free(out) ;
            out = NULL;
            return false;
        }

        outlen += out_currOffset;

        if(ek != NULL)
        	free(ek);

        EVP_CIPHER_CTX_free(ctx);

#ifdef AUTHSSL_DEBUG
		  std::cerr << "AuthSSLimpl::decrypt() finished with outlen : " << outlen << std::endl;
#endif

        return true;
}

/* Locked search -> internal help function */
bool AuthSSLimpl::locked_FindCert(const RsPeerId& id, X509** cert)
{
	std::map<RsPeerId, X509*>::iterator it;
	
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
	std::map<RsPeerId, X509*>::iterator it;
	
	RsStackMutex stack(sslMtx); /******* LOCKED ******/

	if (mCerts.end() != (it = mCerts.find(id)))
	{
		X509* cert = it->second;
		X509_free(cert);
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
	std::map<RsPeerId, X509*>::iterator it;
	
	if (mCerts.end() != (it = mCerts.find(peerId)))
	{
		X509* cert = it->second;

		/* found something */
		/* check that they are exact */
		if (0 != X509_cmp(cert, x509))
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
	mCerts[peerId] = X509_dup(x509);

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
		std::map<RsPeerId, X509*>::iterator mapIt;
        for (mapIt = mCerts.begin(); mapIt != mCerts.end(); ++mapIt) {
            if (mapIt->first == mOwnId) {
                continue;
            }
            RsTlvKeyValue kv;
            kv.key = mapIt->first.toStdString();
            #ifdef AUTHSSL_DEBUG
            std::cerr << "AuthSSLimpl::saveList() called (mapIt->first) : " << (mapIt->first) << std::endl ;
            #endif
			kv.value = saveX509ToPEM(mapIt->second);
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
			    if (AuthX509WithGPG(peer,false,diagnos))
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

const EVP_PKEY*RsX509Cert::getPubKey(const X509& x509)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
	return x509.cert_info->key->pkey;
#else
	return X509_get0_pubkey(&x509);
#endif
}

/*******************************************************************************
 * libretroshare/src/pqi: authssl.h                                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2008 Robert Fernie, Retroshare Team.                     *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@altermundi.net>                *
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
#pragma once

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
#include "util/rsdebug.h"

#include "pqi/pqi_base.h"
#include "pqi/pqinetwork.h"
#include "pqi/p3cfgmgr.h"

#include "retroshare/rsevents.h"


#define RS_DEBUG_AUTHSSL 1

class AuthSSL;

struct sslcert
{
	sslcert(X509* x509, const RsPeerId& id);

	/* certificate parameters */
	RsPeerId id;
	std::string name;
	std::string location;
	std::string org;

	RsPgpId issuer;

	/* INTERNAL Parameters */
	X509* certificate;

	static std::string getCertName(const X509& x509);
	static std::string getCertLocation(const X509& x509);
	static std::string getCertOrg(const X509& x509);
	static RsPgpId getCertIssuer(const X509& x509);
	static std::string getCertIssuerString(const X509& x509);
	static RsPeerId getCertSslId(const X509& x509);
};

/* required to install instance */

class AuthSSL
{
public:
	static AuthSSL* getAuthSSL();
	static void AuthSSLInit();

	/* Initialisation Functions (Unique) */
	virtual bool validateOwnCertificate(X509 *x509, EVP_PKEY *pkey) = 0;

	virtual bool active() = 0;
	virtual int InitAuth(
	        const char* srvr_cert, const char* priv_key, const char* passwd,
	        std::string alternative_location_name ) = 0;
	virtual bool CloseAuth() = 0;

	/*********** Overloaded Functions from p3AuthMgr **********/

	/* get Certificate Id */
	virtual const RsPeerId& OwnId() = 0;
	virtual std::string getOwnLocation() = 0;

	/* Load/Save certificates */
	virtual std::string SaveOwnCertificateToString() = 0;
	
	/* Sign / Encrypt / Verify Data */
	virtual bool SignData(std::string input, std::string &sign) = 0;
	virtual bool SignData(
	        const void* data, const uint32_t len, std::string& sign ) = 0;

	virtual bool SignDataBin(std::string, unsigned char*, unsigned int*) = 0;
	virtual bool SignDataBin(
	        const void*, uint32_t, unsigned char*, unsigned int* ) = 0;
	virtual bool VerifyOwnSignBin(
	        const void*, uint32_t, unsigned char*, unsigned int ) = 0;
	virtual bool VerifySignBin(
	        const void* data, const uint32_t len, unsigned char* sign,
	        unsigned int signlen, const RsPeerId& sslId ) = 0;

	/// return false if failed
	virtual bool encrypt(
	        void*& out, int& outlen, const void* in, int inlen,
	        const RsPeerId& peerId ) = 0;
	/// return false if fails
	virtual bool decrypt(void*& out, int& outlen, const void* in, int inlen) = 0;

	virtual X509* SignX509ReqWithGPG(X509_REQ* req, long days) = 0;

	/**
	 * @brief Verify PGP signature correcteness on given X509 certificate
	 * Beware this doesn't check if the PGP signer is friend or not, just if the
	 * signature is valid!
	 * @param[in] x509 pointer ti the X509 certificate to check
	 * @param[out] diagnostic one of RS_SSL_HANDSHAKE_DIAGNOSTIC_* diagnostic
	 *	codes
	 * @return true if correctly signed, false otherwise
	 */
	virtual bool checkX509PgpSignature(X509* x509, uint32_t& diagnostic) = 0;

	/**
	 * @brief Callback provided to OpenSSL to authenticate connections
	 * This is the ultimate place where connection attempts get accepted
	 * if authenticated or refused if not authenticated.
	 * Emits @see RsAuthSslConnectionAutenticationEvent.
	 * @param preverify_ok passed by OpenSSL ignored as this call is the first
	 *	in the authentication callback chain
	 * @param ctx OpenSSL connection context
	 * @return 0 if authentication failed, 1 if success see OpenSSL
	 *	documentation
	 */
	virtual int VerifyX509Callback(int preverify_ok, X509_STORE_CTX* ctx) = 0;

	/* SSL specific functions used in pqissl/pqissllistener */
	virtual SSL_CTX* getCTX() = 0;

	virtual void setCurrentConnectionAttemptInfo(
	        const RsPgpId& gpg_id, const RsPeerId& ssl_id,
	        const std::string& ssl_cn ) = 0;
	virtual void getCurrentConnectionAttemptInfo(
	        RsPgpId& gpg_id, RsPeerId& ssl_id, std::string& ssl_cn ) = 0;

	/**
	 * @deprecated When this function get called the information is usually not
	 * available anymore because the authentication is completely handled by
	 * OpenSSL through @see VerifyX509Callback, so debug messages and
	 * notifications generated from this functions are most of the time
	 * misleading saying that the certificate wasn't provided by the peer, while
	 * it have been provided but already deleted by OpenSSL.
	 */
	RS_DEPRECATED
	virtual bool FailedCertificate(
	        X509* x509, const RsPgpId& gpgid, const RsPeerId& sslid,
	        const std::string& sslcn, const struct sockaddr_storage& addr,
	        bool incoming ) = 0;

	virtual ~AuthSSL();

protected:
	static AuthSSL* instance_ssl;

#if defined(RS_DEBUG_AUTHSSL) && RS_DEBUG_AUTHSSL == 1
	using Dbg1 = RsDbg;
	using Dbg2 = RsNoDbg;
	using Dbg3 = RsNoDbg;
#elif defined(RS_DEBUG_AUTHSSL) && RS_DEBUG_AUTHSSL == 2
	using Dbg1 = RsDbg;
	using Dbg2 = RsDbg;
	using Dbg3 = RsNoDbg;
#elif defined(RS_DEBUG_AUTHSSL) && RS_DEBUG_AUTHSSL >= 3
	using Dbg1 = RsDbg;
	using Dbg2 = RsDbg;
	using Dbg3 = RsDbg;
#else // RS_DEBUG_AUTHSSL
	using Dbg1 = RsNoDbg;
	using Dbg2 = RsNoDbg;
	using Dbg3 = RsNoDbg;
#endif // RS_DEBUG_AUTHSSL
};

struct RsAuthSslConnectionAutenticationEvent : RsEvent
{
	RsAuthSslConnectionAutenticationEvent();

	bool mSuccess;
	bool mIsPendingPpg;
	RsPeerId mSslId;
	std::string mSslCn;
	RsPgpId mPgpId;
	std::string mErrorMsg;

	///* @see RsEvent @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RsEvent::serial_process(j, ctx);
		RS_SERIAL_PROCESS(mSuccess);
		RS_SERIAL_PROCESS(mIsPendingPpg);
		RS_SERIAL_PROCESS(mSslId);
		RS_SERIAL_PROCESS(mSslCn);
		RS_SERIAL_PROCESS(mPgpId);
		RS_SERIAL_PROCESS(mErrorMsg);
	}
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
virtual bool 	checkX509PgpSignature(X509 *x509,uint32_t& auth_diagnostic);


    virtual int VerifyX509Callback(int /*preverify_ok*/, X509_STORE_CTX* ctx);


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

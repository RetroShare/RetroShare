/*******************************************************************************
 * libretroshare/src/pqi: authssl.h                                            *
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
#pragma once


#include <openssl/evp.h>
#include <openssl/x509.h>

#include <string>
#include <map>

#include "util/rsthreads.h"
#include "pqi/pqi_base.h"
#include "pqi/pqinetwork.h"
#include "pqi/p3cfgmgr.h"
#include "util/rsmemory.h"
#include "retroshare/rsevents.h"

/**
 * Functions to interact elegantly with X509 certificates, using this functions
 * you can avoid annoying #ifdef *SSL_VERSION_NUMBER all around the code.
 * Function names should be self descriptive.
 */
namespace RsX509Cert
{
std::string getCertName(const X509& x509);
std::string getCertLocation(const X509& x509);
std::string getCertOrg(const X509& x509);
RsPgpId getCertIssuer(const X509& x509);
std::string getCertIssuerString(const X509& x509);
RsPeerId getCertSslId(const X509& x509);
const EVP_PKEY* getPubKey(const X509& x509);
};

/**
 * This is an implementation of SSL certificate authentication with PGP
 * signatures, instead of centralized certification authority.
 */
class AuthSSL
{
public:
	static AuthSSL& instance();

	RS_DEPRECATED_FOR(AuthSSL::instance())
	static AuthSSL* getAuthSSL();

	/* Initialisation Functions (Unique) */
	virtual bool validateOwnCertificate(X509 *x509, EVP_PKEY *pkey) = 0;

	virtual bool active() = 0;
	virtual int InitAuth(
	        const char* srvr_cert, const char* priv_key, const char* passwd,
	        std::string locationName ) = 0;
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
	/// return false if failed
	virtual bool decrypt(void*& out, int& outlen, const void* in, int inlen) = 0;

	virtual X509* SignX509ReqWithGPG(X509_REQ* req, long days) = 0;

	/**
	 * @brief Verify PGP signature correcteness on given X509 certificate
	 * Beware this doesn't check if the PGP signer is friend or not, just if the
	 * signature is valid!
	 * @param[in] x509 pointer ti the X509 certificate to check
	 * @param[out] diagnostic one of RS_SSL_HANDSHAKE_DIAGNOSTIC_* diagnostic
	 *	codes
	 * @param[in] verbose if true, prints the authentication result to screen.
	 * @return true if correctly signed, false otherwise
	 */
	virtual bool AuthX509WithGPG(
	        X509* x509,
            bool verbose,
	        uint32_t& diagnostic = RS_DEFAULT_STORAGE_PARAM(uint32_t)
	        ) = 0;

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

	/// SSL specific functions used in pqissl/pqissllistener
	virtual SSL_CTX* getCTX() = 0;

//	virtual void setCurrentConnectionAttemptInfo(
//	        const RsPgpId& gpg_id, const RsPeerId& ssl_id,
//	        const std::string& ssl_cn ) = 0;
//	virtual void getCurrentConnectionAttemptInfo(
//	        RsPgpId& gpg_id, RsPeerId& ssl_id, std::string& ssl_cn ) = 0;


	/**
	 * This function parse X509 certificate from the file and return some
	 * verified informations, like ID and signer
	 * @return false on error, true otherwise
	 */
	virtual bool parseX509DetailsFromFile(
	        const std::string& certFilePath, RsPeerId& certId, RsPgpId& issuer,
	        std::string& location ) = 0;

	virtual ~AuthSSL();

protected:
	AuthSSL() {}

	RS_SET_CONTEXT_DEBUG_LEVEL(2)
};


class AuthSSLimpl : public AuthSSL, public p3Config
{
public:

	/** Initialisation Functions (Unique) */
	AuthSSLimpl();
	bool validateOwnCertificate(X509 *x509, EVP_PKEY *pkey) override;

	bool active() override;
	int InitAuth( const char *srvr_cert, const char *priv_key,
	              const char *passwd, std::string locationName )
	override;

	bool CloseAuth() override;

	/*********** Overloaded Functions from p3AuthMgr **********/

	const RsPeerId& OwnId() override;
	virtual std::string getOwnLocation() override;

	/* Load/Save certificates */
	virtual std::string SaveOwnCertificateToString() override;

	/* Sign / Encrypt / Verify Data */
	bool SignData(std::string input, std::string &sign) override;
	bool SignData(
	        const void *data, const uint32_t len, std::string &sign) override;

	bool SignDataBin(std::string, unsigned char*, unsigned int*) override;
	virtual bool SignDataBin(
	        const void*, uint32_t, unsigned char*, unsigned int*) override;
	bool VerifyOwnSignBin(
	        const void*, uint32_t, unsigned char*, unsigned int) override;
	virtual bool VerifySignBin(
	        const void *data, const uint32_t len, unsigned char *sign,
	        unsigned int signlen, const RsPeerId& sslId) override;

	bool encrypt(
	        void*& out, int& outlen, const void* in, int inlen,
	        const RsPeerId& peerId ) override;
	bool decrypt(void *&out, int &outlen, const void *in, int inlen) override;

	virtual X509* SignX509ReqWithGPG(X509_REQ *req, long days) override;

	/// @see AuthSSL
	bool AuthX509WithGPG(X509 *x509, bool verbose, uint32_t& auth_diagnostic) override;

	/// @see AuthSSL
	int VerifyX509Callback(int preverify_ok, X509_STORE_CTX *ctx) override;

	/// @see AuthSSL
	bool parseX509DetailsFromFile(
	        const std::string& certFilePath, RsPeerId& certId,
	        RsPgpId& issuer, std::string& location ) override;


/*****************************************************************/
/***********************  p3config  ******************************/
	/* Key Functions to be overloaded for Full Configuration */
	RsSerialiser* setupSerialiser() override;
	bool saveList(bool &cleanup, std::list<RsItem *>& ) override;
	bool loadList(std::list<RsItem *>& load) override;
/*****************************************************************/

public:
	/* SSL specific functions used in pqissl/pqissllistener */
	SSL_CTX* getCTX() override;

	/* Restored these functions: */
//	void setCurrentConnectionAttemptInfo(
//	        const RsPgpId& gpg_id, const RsPeerId& ssl_id,
//	        const std::string& ssl_cn ) override;
//	void getCurrentConnectionAttemptInfo(
//	        RsPgpId& gpg_id, RsPeerId& ssl_id, std::string& ssl_cn ) override;


private:

	bool LocalStoreCert(X509* x509);
	bool RemoveX509(const RsPeerId id);

	/*********** LOCKED Functions ******/
	bool locked_FindCert(const RsPeerId& id, X509** cert);

	/* Data */
	/* these variables are constants -> don't need to protect */
	SSL_CTX *sslctx;
	RsPeerId mOwnId;
	X509* mOwnCert;

	/**
	 * If the location name is included in SSL certificate it becomes a public
	 * information, because anyone able to open an SSL connection to the host is
	 * able to read it. To avoid that location name is now stored separately and
	 * and not included in the SSL certificate.
	 */
	std::string mOwnLocationName;

	RsMutex sslMtx;  /* protects all below */

	EVP_PKEY* mOwnPrivateKey;
	EVP_PKEY* mOwnPublicKey;

	int init;
	std::map<RsPeerId, X509*> mCerts;

	RsPgpId _last_gpgid_to_connect;
	std::string _last_sslcn_to_connect;
	RsPeerId _last_sslid_to_connect;
};

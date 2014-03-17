#ifndef RS_PQI_SSL_HELPER_H 
#define RS_PQI_SSL_HELPER_H 

/*
 * libretroshare/src/pqi: sslfns.cc
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

/* Functions in this file are SSL only, 
 * and have no dependence on SSLRoot() etc.
 * might need SSL_Init() to be called - thats it!
 */

/******************** notify of new Cert **************************/

#include <openssl/evp.h>
#include <openssl/x509.h>

#include <inttypes.h>
#include <retroshare/rstypes.h>
#include <string>

/****
 * #define AUTHSSL_DEBUG 1
 ***/

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


// IF we are compiling against ssl0.9.7 - these functions don't exist.

#if (OPENSSL_VERSION_NUMBER & 0xfffff000) < 0x00908000
	#define SSLFNS_ADD_CIPHER_CTX_RAND_KEY  1
#endif

#if defined(SSLFNS_ADD_CIPHER_CTX_RAND_KEY)

int EVP_CIPHER_CTX_rand_key(EVP_CIPHER_CTX *ctx, unsigned char *key);

#endif



X509_REQ *GenerateX509Req(
		std::string pkey_file, std::string passwd,
		std::string name, std::string email, std::string org, 
		std::string loc, std::string state, std::string country, 
		int nbits_in, std::string &errString);

X509 *SignX509Certificate(X509_NAME *issuer, EVP_PKEY *privkey, X509_REQ *req, long days);

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


	/* Sign / Encrypt / Verify Data */
bool SSL_SignDataBin(const void *data, const uint32_t len, 
		unsigned char *sign, unsigned int *signlen, EVP_PKEY *pkey);

bool SSL_VerifySignBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int signlen, X509 *cert);

bool SSL_VerifySignBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int signlen, EVP_PKEY *peerkey);

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

X509 *loadX509FromPEM(std::string pem);
std::string saveX509ToPEM(X509* x509);
X509 *loadX509FromDER(const uint8_t *ptr, uint32_t len);
bool saveX509ToDER(X509 *x509, uint8_t **ptr, uint32_t *len);

bool getX509id(X509 *x509, RsPeerId &xid);

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

int pem_passwd_cb(char *buf, int size, int rwflag, void *password);

bool CheckX509Certificate(X509 *x509);
// Not dependent on sslroot. load, and detroys the X509 memory.
int	LoadCheckX509(const char *cert_file, RsPgpId& issuer, std::string &location, RsPeerId& userId);


std::string getX509NameString(X509_NAME *name);
std::string getX509CNString(X509_NAME *name);
std::string getX509TypeString(X509_NAME *name, const char *type, int len);
std::string getX509LocString(X509_NAME *name);
std::string getX509OrgString(X509_NAME *name);
std::string getX509CountryString(X509_NAME *name);
std::string getX509Info(X509 *cert);

/********** SSL ERROR STUFF ******************************************/

int printSSLError(SSL *ssl, int retval, int err, unsigned long err2, std::string &out);

#endif /* RS_PQI_SSL_HELPER_H */


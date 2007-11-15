/*
 * "$Id: xpgpcert.h,v 1.9 2007-04-15 18:45:18 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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



#ifndef MRK_SSL_XPGP_CERT_HEADER
#define MRK_SSL_XPGP_CERT_HEADER

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

#include "pqi_base.h"
#include "pqinetwork.h"

#include "pqiindic.h"


// helper fns.
int printSSLError(SSL *ssl, int retval, int err, unsigned long err2, std::ostream &out);
std::string getX509NameString(X509_NAME *name);
std::string getX509CNString(X509_NAME *name);

std::string getX509OrgString(X509_NAME *name);
std::string getX509LocString(X509_NAME *name);
std::string getX509CountryString(X509_NAME *name);

int     LoadCheckXPGPandGetName(const char *cert_file, std::string &userName);

std::string convert_to_str(certsign &sign);
bool convert_to_certsign(std::string id, certsign &sign);

class sslroot;

class cert: public Person
{
	public:
	cert();
virtual	~cert();

virtual std::string Signature();
std::string	Hash();
void	Hash(std::string);

	XPGP *certificate;
	std::string hash;
};


// returns pointer to static variable.
// which must be inited..
sslroot *getSSLRoot();

class sslroot
{
	public:
	sslroot();
int	active();
int	setcertdir(char *path); 
int	initssl(const char *srvr_cert, const char *priv_key, 
			const char *passwd);
int	closessl();

/* Context handling  */
SSL_CTX *getCTX();

/* Certificate handling */
int	compareCerts(cert *a, cert *b);

	// network interface.

	// program interface.
int     addCertificate(cert *c);
int     addUntrustedCertificate(cert *c);
int     addCollectedCertificate(cert *c);

int	removeCertificate(cert *);

	// Creation of Certificates.... (From X509)
	// Core functions....
cert	*checkDuplicateXPGP(XPGP *x);
cert	*checkPeerXPGP(XPGP *x);
cert 	*makeCertificateXPGP(XPGP *c);
cert 	*registerCertificateXPGP(XPGP *nc, struct sockaddr_in, bool in);

int	validateCertificateXPGP(cert *c);

	/* Fns specific to XPGP */
int     checkAuthCertificate(cert *xpgp);
int	signCertificate(cert *);
int	trustCertificate(cert *, bool totrust);
int     superNodeMode(); 
int 	loadInitialTrustedPeer(std::string tp_file);

// depreciated...
cert	*findpeercert(const char *name); 
//int	loadpeercert(const char *fname);
//int	savepeercert(const char *fname);

// Configuration Handling...
int	setConfigDirs(const char *cdir, const char *ndir);

// these save both the certificates + the settings.
int 	saveCertificates(const char *fname);
int 	saveCertificates();
int 	loadCertificates(const char *fname);

	// with a hash check/recalc in there for good measure.
cert *	loadcertificate(const char* fname, std::string hash);
int 	savecertificate(cert *c, const char* fname);

	// for sending stuff as text
cert *      loadCertFromString(std::string pem);
std::string saveCertAsString(cert *c);

// digest hashing /signing or encrypting interface.
int	hashFile(std::string fname, unsigned char *hash, unsigned int hlen);
int	hashDigest(char *data, unsigned int dlen, unsigned char *hash, unsigned int hlen);
int	signDigest(EVP_PKEY *key, char *data, unsigned int dlen, unsigned char *hash, unsigned int hlen);
int	verifyDigest(EVP_PKEY *key, char *data, unsigned int dlen, unsigned char *enc, unsigned int elen);
int     generateKeyPair(EVP_PKEY *keypair, unsigned int keylen);



int	printCertificate(cert *, std::ostream &out);
/* removing the list of certificate names - ambiguity! 
 *
std::list<std::string> listCertificates();
 *
 */

std::list<cert *> &getCertList();

cert *  getOwnCert();
int	checkNetAddress();

	// extra list for certs that aren't in main list.
cert *  getCollectedCert();
bool  	collectedCerts();

bool	CertsChanged();
bool	CertsMajorChanged();
void 	IndicateCertsChanged();

std::string getSetting(std::string opt);
void setSetting(std::string opt, std::string val);


/* Fns for relating cert signatures to structures */
cert *findcertsign(certsign &sign);
int   getcertsign(cert *c, certsign &sign);
int   addtosignmap(cert *);

	private: /* data */
std::list<cert *> peercerts; 
std::list<cert *> allcerts; 
std::list<cert *> collectedcerts;

// whenever a cert is added, it should also be put in the map.
std::map<certsign, cert *> signmap;



// General Configuration System
// easy it put it here - so it can be signed easily.
std::map<std::string, std::string> settings;

std::string certdir;
std::string neighbourdir;
std::string certfile;

SSL_CTX *sslctx;
int init;

Indicator certsChanged;
Indicator certsMajorChanged;

EVP_PKEY *pkey;

cert *own_cert;

XPGP_KEYRING *pgp_keyring;

};

#endif // MRK_SSL_XPGP_CERT_HEADER

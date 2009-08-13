/*
 * libretroshare/src    gpgauthmgr.cc
 *
 * GnuPG/GPGme interface for RetroShare.
 *
 * Copyright 2008-2009 by Robert Fernie, Retroshare Team.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the termsf the GNU Library General Public
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
 */

/**** 
 * At an SSL Certificate level:
 *     Processx509() calls (virtual) ValidateCertificate() to check if it correctly
 *			signed by an known PGP certificate.
 *
 *     		this will return true - even if the pgp cert is not authed.  
 * 
 *     a connection will cause VerifyX509Callback to be called. This is a virtual fn.
 *
 *     AuthCertificate() is called to make a certificate authenticated.
 *			this does nothing but set a flag at the SSL level 
 *			no real auth at SSL only level.
 *
 * GPG Functions:
 *      ValidateCertificate() calls,
 *	     bool GPGAuthMgr::AuthX509(X509 *x509) 
 *		VerifySignature()
 *
 *	VerifyX509Callback() 
 *		calls AuthX509().
 *		calls isPGPAuthenticated(std::string id)
 * 
 *     AuthCertificate().
 *		check isPGPAuthenticated().
 *		if not - sign PGP certificate.
 *
 * access to local data is protected via pgpMtx.
 */

#include "authgpg.h"
#include <iostream>
#include <sstream>


/* Turn a set of parameters into a string */
static std::string setKeyPairParams(bool useRsa, unsigned int blen,
		std::string name, std::string comment, std::string email,
		std::string inPassphrase);

static gpgme_key_t getKey(gpgme_ctx_t, std::string, std::string, std::string);

static gpg_error_t keySignCallback(void *, gpgme_status_code_t, \
		const char *, int);

static gpg_error_t trustCallback(void *, gpgme_status_code_t, \
		const char *, int);

static void ProcessPGPmeError(gpgme_error_t ERR);

/* Function to sign X509_REQ via GPGme.
 */

// the single instance of this, but only when SSL Only
static GPGAuthMgr instance_gpgroot;

p3AuthMgr *getAuthMgr()
{
        return &instance_gpgroot;
}

gpgcert::gpgcert()
	:key(NULL)
{
	return;
}

gpgcert::~gpgcert()
{
	if (key)
	{
		gpgme_key_unref(key);
	}
}

gpg_error_t pgp_pwd_callback(void *hook, const char *uid_hint, const char *passphrase_info, int prev_was_bad, int fd)
{
	const char *passwd = (const char *) hook;

	if (prev_was_bad)
		fprintf(stderr, "pgp_pwd_callback() Prev was bad!\n");
	//fprintf(stderr, "pgp_pwd_callback() Set Password to:\"%s\"\n", passwd);
	fprintf(stderr, "pgp_pwd_callback() Set Password\n");

#ifndef WINDOWS_SYS
	write(fd, passwd, strlen(passwd));
	write(fd, "\n", 1); /* needs a new line? */
#else
	DWORD written = 0;
	HANDLE winFd = (HANDLE) fd;
	WriteFile(winFd, passwd, strlen(passwd), &written, NULL);
	WriteFile(winFd, "\n", 1, &written, NULL); 
#endif

	return 0;
}

static char *PgpPassword = NULL;

bool GPGAuthMgr::setPGPPassword_locked(std::string pwd)
{
	/* reset it while we change it */
	gpgme_set_passphrase_cb(CTX, NULL, NULL);

	if (PgpPassword)
		free(PgpPassword);
	PgpPassword = (char *) malloc(pwd.length() + 1);
	memcpy(PgpPassword, pwd.c_str(), pwd.length());
	PgpPassword[pwd.length()] = '\0';

	fprintf(stderr, "GPGAuthMgr::setPGPPassword_locked() called\n");
	gpgme_set_passphrase_cb(CTX, pgp_pwd_callback, (void *) PgpPassword);

	return true;
}


GPGAuthMgr::GPGAuthMgr()
	:gpgmeInit(false) 
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	setlocale(LC_ALL, "");
	gpgme_check_version(NULL);
	gpgme_set_locale(NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));

	#ifdef LC_MESSAGES
		gpgme_set_locale(NULL, LC_MESSAGES, setlocale (LC_MESSAGES, NULL));
	#endif
 
	if (GPG_ERR_NO_ERROR != gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP))
	{
		std::cerr << "Error check engine version";
		std::cerr << std::endl;
		return;
	}

	if (GPG_ERR_NO_ERROR != gpgme_get_engine_info(&INFO))
	{
		std::cerr << "Error getting engine info";
		std::cerr << std::endl;
		return;
	}

	/* Create New Contexts */
	if (GPG_ERR_NO_ERROR != gpgme_new(&CTX))
	{
		std::cerr << "Error creating GPGME Context";
		std::cerr << std::endl;
		return;
	}

	/* setup the protocol */
	if (GPG_ERR_NO_ERROR != gpgme_set_protocol(CTX, GPGME_PROTOCOL_OpenPGP))
	{
		std::cerr << "Error creating Setting Protocol";
		std::cerr << std::endl;
		return;
	}

	/* if we get to here -> we have inited okay */
	gpgmeInit = true;

	storeAllKeys_locked();
	printAllKeys_locked();
	updateTrustAllKeys_locked();
}

/* This function is called when retroshare is first started
 * to get the list of available GPG certificates.
 * This function should only return certs for which
 * the private(secret) keys are available.
 *
 * returns false if GnuPG is not available.
 */

bool GPGAuthMgr::availablePGPCertificates(std::list<std::string> &ids)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	int i = 0;
	gpgme_key_t KEY = NULL;
        gpg_error_t ERR;

	/* XXX should check that CTX is valid */
	if (!gpgmeInit)
	{
		return false;
	}


	/* Initiates a key listing */
	if (GPG_ERR_NO_ERROR != gpgme_op_keylist_start (CTX, "", 1))
	{
		std::cerr << "Error iterating through KeyList";
		std::cerr << std::endl;
		return false;

	}

	/* Loop until end of key */
	for(i = 0;(GPG_ERR_NO_ERROR == (ERR = gpgme_op_keylist_next (CTX, &KEY))); i++)
	{
		if (KEY->subkeys)
		{	
			ids.push_back(KEY->subkeys->keyid);
			std::cerr << "GPGAuthMgr::availablePGPCertificates() Added: " 
				<< KEY->subkeys->keyid << std::endl;
		}
		else
		{
			std::cerr << "GPGAuthMgr::availablePGPCertificates() Missing subkey" 
				<< std::endl;
		}
	}

	if (GPG_ERR_NO_ERROR != gpgme_op_keylist_end(CTX))
	{
		std::cerr << "Error ending KeyList";
		std::cerr << std::endl;
		return false;
	} 

	std::cerr << "GPGAuthMgr::availablePGPCertificates() Secret Key Count: " << i << std::endl;

	/* return false if there are no private keys */
	return (i > 0);
}

/* You can initialise Retroshare with
 * (a) load existing certificate.
 * (b) a new certificate.
 *
 * This function must be called successfully (return == 1)
 * before anything else can be done. (except above fn).
 */
int	GPGAuthMgr::GPGInit(std::string ownId)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	gpgme_key_t newKey;
        gpg_error_t ERR;
	
	if (!gpgmeInit) {
		return 0;
	}

/*****
	if (!isOwnCert(ownId))
	{
		return 0;
	}
*****/

	if(GPG_ERR_NO_ERROR != (ERR = gpgme_get_key(CTX, ownId.c_str(), &newKey, 1))) {
		std::cerr << "Error reading the key from keyring" << std::endl;
		return 0;
	}

	mOwnGpgCert.user.name = newKey->uids->name;
	mOwnGpgCert.user.email = newKey->uids->email;
	mOwnGpgCert.user.fpr = newKey->subkeys->fpr;
	mOwnGpgCert.user.id = ownId;
	mOwnGpgCert.key = newKey;

	mOwnId = ownId;
	gpgmeKeySelected = true;

	// Password set in different fn.
	//this->passphrase = passphrase;
	//setPGPPassword_locked(passphrase);

	return true;
}

int	GPGAuthMgr::GPGInit(std::string name, std::string comment, 
			std::string email, std::string inPassphrase)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	gpgme_key_t newKey;
	gpgme_genkey_result_t result;
        gpg_error_t ERR;
	
	if (!gpgmeInit) {
		return 0;
	}
	
	if(GPG_ERR_NO_ERROR != (ERR = gpgme_op_genkey(CTX, setKeyPairParams(true, 2048, name, comment, email, \
		passphrase).c_str(), NULL, NULL))) {
		std::cerr << "Error generating the key" << std::endl;
		return 0;
	}
	
	if((result = gpgme_op_genkey_result(CTX)) == NULL) 
		return 0;
	
	
	if(GPG_ERR_NO_ERROR != (ERR = gpgme_get_key(CTX, result->fpr, &newKey, 1))) {
		std::cerr << "Error reading own key" << std::endl;
		return 0;
	}
	
	mOwnGpgCert.user.name = name; 
	mOwnGpgCert.user.email = email;
	mOwnGpgCert.user.fpr = newKey->subkeys->fpr;
	mOwnGpgCert.user.id = newKey->subkeys->keyid;
	mOwnGpgCert.key = newKey;

	this->passphrase = inPassphrase;
	setPGPPassword_locked(inPassphrase);

	mOwnId = mOwnGpgCert.user.id;
	gpgmeKeySelected = true;

	return 1;
}

 GPGAuthMgr::~GPGAuthMgr()
{
}

int	GPGAuthMgr::LoadGPGPassword(std::string pwd)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	if (!gpgmeInit) {
		return 0;
	}
	
	this->passphrase = pwd;
	setPGPPassword_locked(pwd);

	return 1;
}
	


// store all keys in map mKeyList to avoid callin gpgme exe repeatedly
bool   GPGAuthMgr::storeAllKeys_locked()
{
	std::cerr << "GPGAuthMgr::storeAllKeys_locked()";
	std::cerr << std::endl;

	gpg_error_t ERR;
	if (!gpgmeInit)
	{
		std::cerr << "Error since GPG is not initialised";
		std::cerr << std::endl;
		return false;
	}
	
	std::cerr << "GPGAuthMgr::storeAllKeys_locked() clearing existing ones";
	std::cerr << std::endl;
	mKeyList.clear();

	/* enable SIG mode */
	gpgme_keylist_mode_t origmode = gpgme_get_keylist_mode(CTX);
	gpgme_keylist_mode_t mode = origmode | GPGME_KEYLIST_MODE_SIGS;

	gpgme_set_keylist_mode(CTX, mode);

         /* store keys */
	gpgme_key_t KEY = NULL;

	/* Initiates a key listing 0 = All Keys */
	if (GPG_ERR_NO_ERROR != gpgme_op_keylist_start (CTX, "", 0))
	{
		std::cerr << "Error iterating through KeyList";
		std::cerr << std::endl;
		gpgme_set_keylist_mode(CTX, origmode);
		return false;
	} 

	/* Loop until end of key */
	for(int i = 0;(GPG_ERR_NO_ERROR == (ERR = gpgme_op_keylist_next (CTX, &KEY))); i++)
	{
		/* store in pqiAuthDetails */
		gpgcert nu;

		/* NB subkeys is a linked list and can contain multiple keys.
		 * first key is primary.
		 */

		if ((!KEY->subkeys) || (!KEY->uids))
		{
			std::cerr << "Invalid Key in List... skipping";
			std::cerr << std::endl;
			continue;
		}

		/* In general MainSubKey is used to sign all others!
		 * Don't really need to worry about other ids either.
		 */
		gpgme_subkey_t mainsubkey = KEY->subkeys;
		nu.user.id  = mainsubkey->keyid;
        	nu.user.fpr = mainsubkey->fpr;

		std::cerr << "MAIN KEYID: " << nu.user.id;
		std::cerr << " FPR: " << nu.user.fpr;
		std::cerr << std::endl;


		gpgme_subkey_t subkeylist = KEY->subkeys;
		while(subkeylist != NULL)
		{
			std::cerr << "\tKEYID: " << subkeylist->keyid;
			std::cerr << " FPR: " << subkeylist->fpr;
			std::cerr << std::endl;

			subkeylist = subkeylist->next;
		}


		/* NB uids is a linked list and can contain multiple ids.
		 * first id is primary.
		 */

		gpgme_user_id_t mainuid = KEY->uids;
        	nu.user.name  = mainuid->name;
        	nu.user.email = mainuid->email;		
		gpgme_key_sig_t mainsiglist = mainuid->signatures;
		while(mainsiglist != NULL)
		{
			if (mainsiglist->status == GPG_ERR_NO_ERROR)
			{
				/* add as a signature ... even if the 
				 * we haven't go the peer yet. 
				 * (might be yet to come).
				 */
				
				std::string keyid = mainsiglist->keyid;
				if (nu.user.signers.end() == std::find(
					nu.user.signers.begin(), 
					nu.user.signers.end(),keyid))
				{
					nu.user.signers.push_back(keyid);
				}
			}
			mainsiglist = mainsiglist->next;
		}

		gpgme_user_id_t uidlist = KEY->uids;
		while(uidlist != NULL)
		{
			std::cerr << "\tUID: " << uidlist->uid;
			std::cerr << " NAME: " << uidlist->name;
			std::cerr << " EMAIL: " << uidlist->email;
			std::cerr << " VALIDITY: " << uidlist->validity;
			std::cerr << std::endl;
			gpgme_key_sig_t usiglist = uidlist->signatures;
			while(usiglist != NULL)
			{
				std::cerr << "\t\tSIG KEYID: " << usiglist->keyid;
				std::cerr << " UID: " << usiglist->uid;
				std::cerr << " NAME: " << usiglist->name;
				std::cerr << " EMAIL: " << usiglist->email;
				std::cerr << " VALIDITY: " << (usiglist->status == GPG_ERR_NO_ERROR);
				std::cerr << std::endl;

				usiglist = usiglist->next;
			}

			uidlist = uidlist->next;
		}

		/* signatures are attached to uids... but only supplied
		 * if GPGME_KEYLIST_MODE_SIGS is on.
		 * signature notation supplied is GPGME_KEYLIST_MODE_SIG_NOTATION is on
		 */

		nu.user.trustLvl = KEY->owner_trust; 
		nu.user.ownsign = KEY->can_sign;   
		nu.user.validLvl = mainuid->validity;
		nu.user.trusted = (mainuid->validity > GPGME_VALIDITY_MARGINAL); 

		/* grab a reference, so the key remains */
		gpgme_key_ref(KEY);
		nu.key = KEY;

		/* store in map */
		mKeyList[nu.user.id] = nu;
	}

	if (GPG_ERR_NO_ERROR != gpgme_op_keylist_end(CTX))
	{
		std::cerr << "Error ending KeyList";
		std::cerr << std::endl;
		gpgme_set_keylist_mode(CTX, origmode);
		return false;
	} 

	gpgme_set_keylist_mode(CTX, origmode);
	return true;

}

// update trust on all available keys.
bool   GPGAuthMgr::updateTrustAllKeys_locked()
{
	gpg_error_t ERR;
	if (!gpgmeInit)
	{
		std::cerr << "Error since GPG is not initialised";
		std::cerr << std::endl;
		return false;
	}


	/* have to do this the hard way! */
	gpgme_trust_item_t ti = NULL;
	std::map<std::string, gpgcert>::iterator it;

	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
		/* check for trust items associated with key */	
		std::string peerid = it->second.user.email;
		std::cerr << "Searching GPGme for TrustInfo on: " << peerid;
		std::cerr << std::endl;

		/* Initiates a key listing. NB: maxlevel is ignored!*/
		if (GPG_ERR_NO_ERROR != (ERR = gpgme_op_trustlist_start (CTX, peerid.c_str(), 0)))
		{
			std::cerr << "Error Starting Trust List";
			std::cerr << std::endl;
			ProcessPGPmeError(ERR);
			continue;
		} 


		/* Loop until end of key */
		for(int i = 0;(GPG_ERR_NO_ERROR == (ERR = 
					gpgme_op_trustlist_next (CTX, &ti))); i++)
		{
			std::string keyid = ti->keyid;
			int type = ti->type;
			int level = ti->level;
	
			/* identify the peers, and add trust level */
			std::cerr << "GPGme Trust Item for: " << keyid;
			std::cerr << std::endl;
	
			std::cerr << "\t Type: " << type;
			std::cerr << " Level: " << level;
			std::cerr << std::endl;
	
			std::cerr << "\t Owner Trust: " << ti->owner_trust;
			std::cerr << " Validity: " << ti->validity;
			std::cerr << " Name: " << ti->name;
			std::cerr << std::endl;
	
		}
	
		std::cerr << "End of TrustList Iteration.";
		std::cerr << std::endl;
		ProcessPGPmeError(ERR);

		if (GPG_ERR_NO_ERROR != gpgme_op_trustlist_end(CTX))
		{
			std::cerr << "Error ending TrustList";
			std::cerr << std::endl;

			ProcessPGPmeError(ERR);
		} 
	}

	return true;

}
bool   GPGAuthMgr::printAllKeys_locked()
{

	certmap::const_iterator it;
	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
		std::cerr << "PGP Key: " << it->second.user.id;
		std::cerr << std::endl;

		std::cerr << "\tName: " << it->second.user.name;
		std::cerr << std::endl;
		std::cerr << "\tEmail: " << it->second.user.email;
		std::cerr << std::endl;

		std::cerr << "\ttrustLvl: " << it->second.user.trustLvl;
		std::cerr << std::endl;
		std::cerr << "\townsign?: " << it->second.user.ownsign;
		std::cerr << std::endl;
		std::cerr << "\ttrusted/valid: " << it->second.user.trusted;
		std::cerr << std::endl;
		std::cerr << "\tEmail: " << it->second.user.email;
		std::cerr << std::endl;

		std::list<std::string>::const_iterator sit;
		for(sit = it->second.user.signers.begin();
			sit != it->second.user.signers.end(); sit++)
		{
			std::cerr << "\t\tSigner ID:" << *sit;

			/* do a naughty second search.. should be ok
			 * as we aren't modifying list.
			 */
			certmap::const_iterator kit = mKeyList.find(*sit);
			if (kit != mKeyList.end())
			{
				std::cerr << " Name:" << kit->second.user.name;
				std::cerr << std::endl;
			}
		}
	}
	return true;
}

bool   GPGAuthMgr::printOwnKeys_locked()
{

	certmap::iterator it;
	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
		if (it->second.user.ownsign)
		{
			std::cerr << "Own PGP Key: " << it->second.user.id;
			std::cerr << std::endl;

			std::cerr << "\tName: " << it->second.user.name;
			std::cerr << std::endl;
			std::cerr << "\tEmail: " << it->second.user.email;
			std::cerr << std::endl;
		}
	}
	return true;
}

bool    GPGAuthMgr::printKeys()
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/
	printAllKeys_locked();
	return printOwnKeys_locked();
}

X509 *GPGAuthMgr::SignX509Req(X509_REQ *req, long days, std::string gpg_passwd)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

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
		std::cerr << "GPGAuthMgr::SignX509Req() FAIL" << std::endl;
		return NULL;
	}

        long version = 0x00;
        unsigned long chtype = MBSTRING_ASC;
        X509_NAME *issuer_name = X509_NAME_new();
        X509_NAME_add_entry_by_txt(issuer_name, "CN", chtype,
                        (unsigned char *) mOwnId.c_str(), -1, -1, 0);
/****
        X509_NAME_add_entry_by_NID(issuer_name, 48, 0,
                        (unsigned char *) "email@email.com", -1, -1, 0);
        X509_NAME_add_entry_by_txt(issuer_name, "O", chtype,
                        (unsigned char *) "org", -1, -1, 0);
        X509_NAME_add_entry_by_txt(x509_name, "L", chtype,
                        (unsigned char *) "loc", -1, -1, 0);
****/

	std::cerr << "GPGAuthMgr::SignX509Req() Issuer name: " << mOwnId << std::endl;

        BIGNUM *btmp = BN_new();
        if (!BN_pseudo_rand(btmp, SERIAL_RAND_BITS, 0, 0))
	{
		std::cerr << "GPGAuthMgr::SignX509Req() rand FAIL" << std::endl;
		return NULL;
	}
        if (!BN_to_ASN1_INTEGER(btmp, serial))
	{
		std::cerr << "GPGAuthMgr::SignX509Req() asn1 FAIL" << std::endl;
		return NULL;
	}
        BN_free(btmp);

	if (!X509_set_serialNumber(x509, serial)) 
	{
		std::cerr << "GPGAuthMgr::SignX509Req() serial FAIL" << std::endl;
		return NULL;
	}
	ASN1_INTEGER_free(serial);

	/* Generate SUITABLE issuer name.
	 * Must reference OpenPGP key, that is used to verify it
	 */

	if (!X509_set_issuer_name(x509, issuer_name))
	{
		std::cerr << "GPGAuthMgr::SignX509Req() issue FAIL" << std::endl;
		return NULL;
	}
	X509_NAME_free(issuer_name);


        if (!X509_gmtime_adj(X509_get_notBefore(x509),0))
	{
		std::cerr << "GPGAuthMgr::SignX509Req() notbefore FAIL" << std::endl;
		return NULL;
	}

        if (!X509_gmtime_adj(X509_get_notAfter(x509), (long)60*60*24*days))
	{
		std::cerr << "GPGAuthMgr::SignX509Req() notafter FAIL" << std::endl;
		return NULL;
	}

        if (!X509_set_subject_name(x509, X509_REQ_get_subject_name(req)))
	{
		std::cerr << "GPGAuthMgr::SignX509Req() sub FAIL" << std::endl;
		return NULL;
	}

        tmppkey = X509_REQ_get_pubkey(req);
        if (!tmppkey || !X509_set_pubkey(x509,tmppkey))
	{
		std::cerr << "GPGAuthMgr::SignX509Req() pub FAIL" << std::endl;
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

	std::cerr << "Buffer Sizes: in: " << inl;
	std::cerr << "  HashOut: " << hashoutl;
	std::cerr << "  SigOut: " << sigoutl;
	std::cerr << std::endl;

        if ((buf_in == NULL) || (buf_hashout == NULL) || (buf_sigout == NULL))
                {
                hashoutl=0;
                sigoutl=0;
                fprintf(stderr, "GPGAuthMgr::SignX509Req: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_MALLOC_FAILURE)\n");
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
                fprintf(stderr, "GPGAuthMgr::SignX509Req: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_EVP_LIB)\n");
                goto err;
                }

	std::cerr << "Digest Applied: len: " << hashoutl << std::endl;

	/* NOW Sign via GPG Functions */
	if (!DoOwnSignature_locked(buf_hashout, hashoutl, buf_sigout, (unsigned int *) &sigoutl))
	{
		sigoutl = 0;	
		goto err;
	}

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



#if 0
	int ASN1_sign(int (*i2d)(), X509_ALGOR *algor1, X509_ALGOR *algor2,
             ASN1_BIT_STRING *signature, char *data, EVP_PKEY *pkey,
             const EVP_MD *type)

#define X509_sign(x,pkey,md) \
        ASN1_sign((int (*)())i2d_X509_CINF, x->cert_info->signature, \
                x->sig_alg, x->signature, (char *)x->cert_info,pkey,md)


	return NULL;
}
#endif


bool GPGAuthMgr::AuthX509(X509 *x509)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

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

        fprintf(stderr, "GPGAuthMgr::AuthX509()\n");

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

        if ((buf_in == NULL) || (buf_hashout == NULL) || (buf_sigout == NULL))
                {
                hashoutl=0;
                sigoutl=0;
                fprintf(stderr, "GPGAuthMgr::AuthX509: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_MALLOC_FAILURE)\n");
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
                fprintf(stderr, "GPGAuthMgr::AuthX509: ASN1err(ASN1_F_ASN1_SIGN,ERR_R_EVP_LIB)\n");
                goto err;
                }

	std::cerr << "Digest Applied: len: " << hashoutl << std::endl;

	/* copy data into signature */
	sigoutl = signature->length;
	memmove(buf_sigout, signature->data, sigoutl);

	/* NOW Sign via GPG Functions */
	if (!VerifySignature_locked(id, buf_hashout, hashoutl, buf_sigout, (unsigned int) sigoutl))
	{
		sigoutl = 0;	
		goto err;
	}

	return true;

  err:
	return false;
}


void ProcessPGPmeError(gpgme_error_t ERR)
{
	gpgme_err_code_t code = gpgme_err_code(ERR);
	gpgme_err_source_t src = gpgme_err_source(ERR);

	std::cerr << "GPGme ERROR: Code: " << code << " Source: " << src << std::endl;
	std::cerr << "GPGme ERROR: " << gpgme_strerror(ERR) << std::endl;

}

void print_pgpme_verify_summary(unsigned int summary)
{
	std::cerr << "\tFLAGS:";
	if (summary & GPGME_SIGSUM_VALID)
		std::cerr << " VALID ";
	if (summary & GPGME_SIGSUM_GREEN)
		std::cerr << " GREEN ";
	if (summary & GPGME_SIGSUM_RED)
		std::cerr << " RED ";
	if (summary & GPGME_SIGSUM_KEY_REVOKED)
		std::cerr << " KEY_REVOKED ";
	if (summary & GPGME_SIGSUM_KEY_EXPIRED)
		std::cerr << " KEY_EXPIRED ";
	if (summary & GPGME_SIGSUM_KEY_MISSING)
		std::cerr << " KEY_MISSING ";
	if (summary & GPGME_SIGSUM_CRL_MISSING)
		std::cerr << " CRL_MISSING ";
	if (summary & GPGME_SIGSUM_CRL_TOO_OLD)
		std::cerr << " CRL_TOO_OLD ";
	if (summary & GPGME_SIGSUM_BAD_POLICY)
		std::cerr << " BAD_POLICY ";
	if (summary & GPGME_SIGSUM_SYS_ERROR)
		std::cerr << " SYS_ERROR ";
	std::cerr << std::endl;
}


bool GPGAuthMgr::DoOwnSignature_locked(void *data, unsigned int datalen, void *buf_sigout, unsigned int *outl)
{
	/* setup signers */
	gpgme_signers_clear(CTX);
	if (GPG_ERR_NO_ERROR != gpgme_signers_add(CTX, mOwnGpgCert.key))
	{
		std::cerr << "GPGAuthMgr::DoOwnSignature() Error Adding Signer";
		std::cerr << std::endl;
	}

	gpgme_data_t gpgmeData;
	gpgme_data_t gpgmeSig;
	if (GPG_ERR_NO_ERROR != gpgme_data_new_from_mem(&gpgmeData, (const char *) data, datalen, 1))
	{
		std::cerr << "Error create Data";
		std::cerr << std::endl;
	}

	if (GPG_ERR_NO_ERROR != gpgme_data_new(&gpgmeSig))
	{
		std::cerr << "Error create Sig";
		std::cerr << std::endl;
	}

	/* move string data to gpgmeData */
	gpgme_set_armor (CTX, 0);

	gpgme_sig_mode_t mode = GPGME_SIG_MODE_DETACH;
        gpg_error_t ERR;
	if (GPG_ERR_NO_ERROR != (ERR = gpgme_op_sign(CTX,gpgmeData, gpgmeSig, mode)))
	{
		ProcessPGPmeError(ERR);
		std::cerr << "GPGAuthMgr::Sign FAILED ERR: " << ERR;
		std::cerr << std::endl;
	}

	gpgme_sign_result_t res = gpgme_op_sign_result(CTX);

	if (res)
	{
		fprintf(stderr, "Sign Got Result\n");
	}
	else
	{
		fprintf(stderr, "Sign Failed to get Result\n");
	}

	gpgme_invalid_key_t ik = res->invalid_signers;
	gpgme_new_signature_t sg = res->signatures;
	while(ik != NULL)
	{
		fprintf(stderr, "GPGAuthMgr::Sign, Invalid by: %s\n", ik->fpr);
		ik = ik->next;
	}

	while(sg != NULL)
	{
		fprintf(stderr, "GPGAuthMgr::Signed by: %s\n", sg->fpr);
		sg = sg->next;
	}

	/* now extract the data from gpgmeSig */
	size_t len = 0; 
	int len2 = len;
	char *export_sig = gpgme_data_release_and_get_mem(gpgmeSig, &len);
	fprintf(stderr, "GPGAuthMgr::Signature len: %d \n", len2);
	if (len < *outl)
	{
		*outl = len;
	}
	memmove(buf_sigout, export_sig, *outl);
	gpgme_free(export_sig);
	gpgme_data_release (gpgmeData);

	/* extract id(s)! */
	return true;
}


/* import to GnuPG and other Certificates */
bool GPGAuthMgr::VerifySignature_locked(std::string id, void *data, int datalen, 
                                                        void *sig, unsigned int siglen)
{
	gpgme_data_t gpgmeSig;
	gpgme_data_t gpgmeData;

	std::cerr << "VerifySignature: datalen: " << datalen << " siglen: " << siglen;
	std::cerr << std::endl;
	
	if (GPG_ERR_NO_ERROR != gpgme_data_new_from_mem(&gpgmeData, (const char *) data, datalen, 1))
	{
		std::cerr << "Error create Data";
		std::cerr << std::endl;
	}

	if (GPG_ERR_NO_ERROR != gpgme_data_new_from_mem(&gpgmeSig, (const char *) sig, siglen, 1))
	{
		std::cerr << "Error create Sig";
		std::cerr << std::endl;
	}

	/* move string data to gpgmeData */

	gpgme_set_armor (CTX, 0);

	gpgme_error_t ERR;
	if (GPG_ERR_NO_ERROR != (ERR = gpgme_op_verify(CTX,gpgmeSig, gpgmeData, NULL)))
	{
		ProcessPGPmeError(ERR);
		std::cerr << "GPGAuthMgr::Verify FAILED";
		std::cerr << std::endl;
	}

	gpgme_verify_result_t res = gpgme_op_verify_result(CTX);

	if (res)
	{
		fprintf(stderr, "VerifySignature Got Result\n");
	}
	else
	{
		fprintf(stderr, "VerifySignature Failed to get Result\n");
	}

	gpgme_signature_t sg = res->signatures;
	bool valid = false;

	while(sg != NULL)
	{
		fprintf(stderr, "GPGAuthMgr::Verify Sig by: %s, Result: %d\n", sg->fpr, sg->summary);
		print_pgpme_verify_summary(sg->summary);

		if (sg->summary & GPGME_SIGSUM_VALID)
		{
			fprintf(stderr, "GPGAuthMgr::VerifySignature() OK\n");
			valid = true;
		}

		sg = sg->next;
	}

	gpgme_data_release (gpgmeData);
	gpgme_data_release (gpgmeSig);

	/* extract id(s)! */
	if (!valid)
	{
		fprintf(stderr, "GPGAuthMgr::VerifySignature() FAILED\n");
	}
	

	return valid;
}



	
bool   GPGAuthMgr::active()
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	return ((gpgmeInit) && (gpgmeKeySelected) && (gpgmeX509Selected));
}

int     GPGAuthMgr::InitAuth(const char *srvr_cert, const char *priv_key, 
                                        const char *passwd)
{
	/* Initialise the SSL part */
	if (AuthSSL::InitAuth(srvr_cert, priv_key, passwd))
	{
		RsStackMutex stack(pgpMtx); /******* LOCKED ******/
		gpgmeX509Selected = true;
		return 1;
	}

	return 0;
}

bool    GPGAuthMgr::CloseAuth()
{
	return true;
}

#if 0 /**** no saving here! let AuthSSL store directories! ****/

int     GPGAuthMgr::setConfigDirectories(std::string confFile, std::string neighDir)
{
	return 1;
}

#endif

/**** The standard versions of the OwnId/get*List ... return SSL ids 
 * There are alternative functions for gpg ids.
 ****/

std::string GPGAuthMgr::OwnId()
{
	/* to the external libretroshare world, we are our SSL id */
	return AuthSSL::OwnId();
}

bool	GPGAuthMgr::getAllList(std::list<std::string> &ids)
{
	/* get all of the certificates */
	return AuthSSL::getAllList(ids);
}

bool	GPGAuthMgr::getAuthenticatedList(std::list<std::string> &ids)
{
	return AuthSSL::getAuthenticatedList(ids);
}

bool	GPGAuthMgr::getUnknownList(std::list<std::string> &ids)
{
	return AuthSSL::getUnknownList(ids);
}

/*******************************/

bool	GPGAuthMgr::isValid(std::string id)
{
	return AuthSSL::isValid(id);
}

bool	GPGAuthMgr::isAuthenticated(std::string id)
{
	/* This must be handled at PGP level */

	/* get pgpid */
	std::string pgpid = getIssuerName(id);

	return isPGPAuthenticated(pgpid);
	//return AuthSSL::isAuthenticated(id);
}

bool 	GPGAuthMgr::isTrustingMe(std::string)
{
	return false;
}

void 	GPGAuthMgr::addTrustingPeer(std::string)
{


}

/**** These Two are common */
std::string GPGAuthMgr::getName(std::string id)
{
	std::string name = AuthSSL::getName(id);
	if (name != "")
	{
		RsStackMutex stack(pgpMtx); /******* LOCKED ******/

		certmap::iterator it;
		if (mKeyList.end() != (it = mKeyList.find(id)))
		{
			return it->second.user.name;
		}
	}
	return name;
}

bool	GPGAuthMgr::getDetails(std::string id, pqiAuthDetails &details)
{
	/**** GPG Details.
	 * Ids are the SSL id cert ids, so we have to get issuer id (pgpid)
	 * before we can add any gpg details 
	 ****/
#ifdef AUTHGPG_DEBUG
        std::cerr << "GPGAuthMgr::getDetails() \"" << id << "\"";
        std::cerr << std::endl;
#endif

	if (AuthSSL::getDetails(id, details))
	{
		RsStackMutex stack(pgpMtx); /******* LOCKED ******/

		certmap::iterator it;
		if (mKeyList.end() != (it = mKeyList.find(details.issuer)))
		{
			/* what do we want from the gpg mgr */
			details.location = details.name;
			details.name = it->second.user.name;
			details.email = it->second.user.email;

			//details = it->second.user;
			return true;
		}
		return true;
	}
	else
	{
		RsStackMutex stack(pgpMtx); /******* LOCKED ******/

		/* if we cannot find a ssl cert - might be a pgp cert */
		certmap::iterator it;
		if (mKeyList.end() != (it = mKeyList.find(id)))
		{
			/* what do we want from the gpg mgr */
			details = it->second.user;
			return true;
		}
	}
	return false;
}


/**** GPG versions ***/

std::string GPGAuthMgr::PGPOwnId()
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	return mOwnId;
}

bool	GPGAuthMgr::getPGPAllList(std::list<std::string> &ids)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	/* add an id for each pgp certificate */
	certmap::iterator it;
	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
		ids.push_back(it->first);
	}
	return true;
}

bool	GPGAuthMgr::getPGPAuthenticatedList(std::list<std::string> &ids)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	certmap::iterator it;
	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
		if (it->second.user.trusted)
		{
			ids.push_back(it->first);
		}
	}
	return true;
}

bool	GPGAuthMgr::getPGPUnknownList(std::list<std::string> &ids)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	certmap::iterator it;
	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
		if (!(it->second.user.trusted))
		{
			ids.push_back(it->first);
		}
	}
	return true;
}


bool	GPGAuthMgr::isPGPValid(std::string id)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	certmap::iterator it;
	return (mKeyList.end() != mKeyList.find(id));
}


bool	GPGAuthMgr::isPGPAuthenticated(std::string id)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	certmap::iterator it;
	if (mKeyList.end() != (it = mKeyList.find(id)))
	{
		/* trustLvl... is just that ... we are interested in validity.
		 * which is the 'trusted' flag.
		 */

		return (it->second.user.trusted);
	}
	return false;
}

/****** Large Parts of the p3AuthMgr is provided by AuthSSL ******
 * As the majority of functions require SSL Certs
 *
 * We don't need to save/load openpgp certificates, as the gpgme
 * handles this.
 *
 */

#if 0

bool GPGAuthMgr::FinalSaveCertificates()
{
	return false;
}

bool GPGAuthMgr::CheckSaveCertificates()
{
	return false;
}

bool GPGAuthMgr::saveCertificates()
{
	return false;
}

bool GPGAuthMgr::loadCertificates()
{
	return false;
}

#endif

/*****************************************************************
 * Loading and Saving Certificates - this has to 
 * be able to handle both openpgp and X509 certificates.
 * 
 * X509 are passed onto AuthSSL, OpenPGP are passed to gpgme.
 *
 */


/* SKTAN : do not know how to use std::string id */
std::string GPGAuthMgr::SaveCertificateToString(std::string id)
{

	if (!isPGPValid(id))
	{
		std::cerr << "GPGAuthMgr::SaveCertificateToString() Id is Not PGP" << std::endl;
		/* check if it is a SSL Certificate */
		if (isValid(id))
		{
			std::cerr << "GPGAuthMgr::SaveCertificateToString() is SSLID!" << std::endl;
			std::string sslcert = AuthSSL::SaveCertificateToString(id);
			return sslcert;
		}
		std::cerr << "GPGAuthMgr::SaveCertificateToString() unknown ID" << std::endl;
		std::string emptystr;
		return emptystr;
	}

	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	std::string tmp;
	const char *pattern[] = { NULL, NULL };
	pattern[0] = id.c_str();
	gpgme_data_t gpgmeData;

	if (GPG_ERR_NO_ERROR != gpgme_data_new (&gpgmeData))
	{
		std::cerr << "Error create Data";
		std::cerr << std::endl;
	}
	gpgme_set_armor (CTX, 1);

	if (GPG_ERR_NO_ERROR != gpgme_op_export_ext (CTX, pattern, 0, gpgmeData))
	{
		std::cerr << "Error export Data";
		std::cerr << std::endl;
	}

	fflush (NULL);
	fputs ("Begin Result:\n", stdout);
	showData (gpgmeData);
	fputs ("End Result.\n", stdout);
  
	size_t len = 0; 
	char *export_txt = gpgme_data_release_and_get_mem(gpgmeData, &len);
	tmp = std::string(export_txt);

	std::cerr << "Exported Certificate: ";
	std::cerr << std::endl;
	std::cerr << tmp;
	std::cerr << std::endl;

	gpgme_free(export_txt);

	return tmp;
}

/* import to GnuPG and other Certificates */
bool GPGAuthMgr::LoadCertificateFromString(std::string str, std::string &id)
{

	/* catch SSL Certs and pass to AuthSSL. */
        std::string sslmarker("-----BEGIN CERTIFICATE-----");
        size_t pos = str.find(sslmarker);
        if (pos != std::string::npos)
        {
		return AuthSSL::LoadCertificateFromString(str, id);
	}

	/* otherwise assume it is a PGP cert */
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	gpgme_data_t gpgmeData;
	if (GPG_ERR_NO_ERROR != gpgme_data_new_from_mem(&gpgmeData, str.c_str(), str.length(), 1))
	{
		std::cerr << "Error create Data";
		std::cerr << std::endl;
	}

	/* move string data to gpgmeData */

	gpgme_set_armor (CTX, 1);

	if (GPG_ERR_NO_ERROR != gpgme_op_import (CTX,gpgmeData))
	{
		std::cerr << "GPGAuthMgr::Error Importing Certificate";
		std::cerr << std::endl;
	}


	gpgme_import_result_t res = gpgme_op_import_result(CTX);

	int imported = res->imported;

	fprintf(stderr, "ImportCertificate(Considered: %d Imported: %d)\n", 
					res->considered, res->imported);

	/* do we need to delete res??? */

	gpgme_data_release (gpgmeData);

	/* extract id(s)! (only if we actually imported one) */
	if (imported)
	{
		storeAllKeys_locked();
	}

	return true;
}

/*** These are passed to SSL ****/
bool GPGAuthMgr::LoadCertificateFromFile(std::string filename, std::string &id)
{
	return false;
}

bool GPGAuthMgr::SaveCertificateToFile(std::string id, std::string filename)
{
	return false;
}

bool GPGAuthMgr::LoadCertificateFromBinary(const uint8_t *ptr, uint32_t len, std::string &id)
{
	return AuthSSL::LoadCertificateFromBinary(ptr, len, id);
}

bool GPGAuthMgr::SaveCertificateToBinary(std::string id, uint8_t **ptr, uint32_t *len)
{
	return AuthSSL::SaveCertificateToBinary(id, ptr, len);
}

/*****************************************************************
 * Auth...? Signing, Revoke, Trust are all done at
 * the PGP level....
 * 
 * Only Signing of SSL is done at setup.
 * Auth should be done... ?? not sure 
 * maybe 
 *
 */


/*************************************/

/* Auth takes SSL Certificate */
bool GPGAuthMgr::AuthCertificate(std::string id)
{
	/**
	 * we are passed an SSL cert, check if the cert is signed 
	 * by an already authed peer.
	 **/
        std::cerr << "GPGAuthMgr::AuthCertificate(" << id << ")";
	std::cerr << std::endl;

	std::string pgpid = AuthSSL::getIssuerName(id);

	if (isPGPAuthenticated(pgpid))
	{
		return true;
	}

	if (!isPGPValid(pgpid))
	{
		return false;
	}

	return SignCertificate(pgpid);
}

/* These take PGP Ids */
bool GPGAuthMgr::SignCertificate(std::string id)
{

        std::cerr << "GPGAuthMgr::SignCertificate(" << id << ")";
	std::cerr << std::endl;


	if (1 != signCertificate(id))
	{
		return false;
	}

	/* reload stuff now ... */
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	storeAllKeys_locked();

	return true;
}

bool GPGAuthMgr::RevokeCertificate(std::string id)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

        std::cerr << "GPGAuthMgr::RevokeCertificate(" << id << ")";
	std::cerr << std::endl;

	return false;
}

bool GPGAuthMgr::TrustCertificate(std::string id, bool trust)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

        std::cerr << "GPGAuthMgr::TrustCertificate(" << id << "," << trust << ")";
	std::cerr << std::endl;

	return false;
}

/*****************************************************************
 * Signing data is done by the SSL certificate.
 *
 */

#if 0

bool GPGAuthMgr::SignData(std::string input, std::string &sign)
{
	return false;
}

bool GPGAuthMgr::SignData(const void *data, const uint32_t len, std::string &sign)
{
	return false;
}


bool GPGAuthMgr::SignDataBin(std::string input, unsigned char *sign, unsigned int *signlen)
{
	return false;
}

bool GPGAuthMgr::SignDataBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int *signlen)
{
	return false;
}

#endif


        /************* Virtual Functions from AuthSSL *************/

bool GPGAuthMgr::ValidateCertificate(X509 *x509, std::string &peerId)
{
        std::cerr << "GPGAuthMgr::ValidateCertificate()";
	std::cerr << std::endl;

	bool val = AuthX509(x509);
	if (val)
	{
		return getX509id(x509, peerId);
	}
	/* be sure to get the id anyway */
	getX509id(x509, peerId);

	return false;
}


int GPGAuthMgr::VerifyX509Callback(int preverify_ok, X509_STORE_CTX *ctx)
{
	char    buf[256];
	X509   *err_cert;
	int     err, depth;
	
	err_cert = X509_STORE_CTX_get_current_cert(ctx);
	err = X509_STORE_CTX_get_error(ctx);
	depth = X509_STORE_CTX_get_error_depth(ctx);

	std::cerr << "GPGAuthMgr::VerifyX509Callback(preverify_ok: " << preverify_ok
				 << " Err: " << err << " Depth: " << depth;
	std::cerr << std::endl;
	
	/*
	* Retrieve the pointer to the SSL of the connection currently treated
	* and the application specific data stored into the SSL object.
	*/
	
	X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);
	
	std::cerr << "GPGAuthMgr::VerifyX509Callback: depth: " << depth << ":" << buf;
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
			X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert), buf, 256);
			printf("issuer= %s\n", buf);
	
			fprintf(stderr, "Doing REAL PGP Certificates\n");
			/* do the REAL Authentication */
			if (!AuthX509(ctx->current_cert))
			{
				return false;
			}
			std::string pgpid = getX509CNString(ctx->current_cert->cert_info->issuer);
			if (!isPGPAuthenticated(pgpid))
			{
				return false;
			}
			preverify_ok = true;
		}
		else if ((err == X509_V_ERR_CERT_UNTRUSTED) ||
			(err == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE))
		{
			std::string pgpid = getX509CNString(ctx->current_cert->cert_info->issuer);
			if (!isPGPAuthenticated(pgpid))
			{
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

	return preverify_ok;
}


        /************* Virtual Functions from AuthSSL *************/



	/* Sign/Trust stuff */

int	GPGAuthMgr::signCertificate(std::string id)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	/* The key should be in Others list and not in Peers list ?? 
	 * Once the key is signed, it moves from Others to Peers list ??? 
	 */

	certmap::iterator it;
	if (mKeyList.end() == (it = mKeyList.find(id)))
	{
		return false;
	}

	gpgme_key_t signKey = it->second.key;
	gpgme_key_t ownKey  = mOwnGpgCert.key;
	
	class SignParams sparams("0", passphrase);
	class EditParams params(SIGN_START, &sparams);
	gpgme_data_t out;
        gpg_error_t ERR;
	
	if(GPG_ERR_NO_ERROR != (ERR = gpgme_data_new(&out))) {
		return 0;
	}

	gpgme_signers_clear(CTX);
	if(GPG_ERR_NO_ERROR != (ERR = gpgme_signers_add(CTX, ownKey))) {
		return 0;
	}
	
	
	if(GPG_ERR_NO_ERROR != (ERR = gpgme_op_edit(CTX, signKey, keySignCallback, \
		&params, out))) {
		return 0;	
	}
	
	/* Should I move the certificate from Others to Peers ???  */
	 
	return 1;
}

/* revoke the signature on Certificate */
int	GPGAuthMgr::revokeCertificate(std::string id)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	return 0;
}

int	GPGAuthMgr::trustCertificate(std::string id, int trustlvl)
{
	RsStackMutex stack(pgpMtx); /******* LOCKED ******/

	/* The certificate should be in Peers list ??? */
	
	if(!isAuthenticated(id)) {
		std::cerr << "Invalid Certificate" << std::endl;
		return 0;
	}
	
	gpgcert trustCert = mKeyList.find(id)->second;
	gpgme_key_t trustKey = trustCert.key;
	const char *lvls[] = {"1", "2", "3", "4", "5"};	
	class EditParams params(TRUST_START, (void *) *(lvls + trustlvl -1));
	gpgme_data_t out;
        gpg_error_t ERR;
	
	
	if(GPG_ERR_NO_ERROR != (ERR = gpgme_data_new(&out))) {
		return 0;
	}

	if(GPG_ERR_NO_ERROR != (ERR = gpgme_op_edit(CTX, trustKey, trustCallback, &params, out)))
		return 0;
		
	return 1;
}


/* This function to print Data */
void GPGAuthMgr::showData(gpgme_data_t dh)
{
	#define BUF_SIZE 512
	char buf[BUF_SIZE + 1];
	int ret;
  
	ret = gpgme_data_seek (dh, 0, SEEK_SET);
	if (ret)
	{
		std::cerr << "Fail data seek";
		std::cerr << std::endl;
	//	fail_if_err (gpgme_err_code_from_errno (errno));
	}

	while ((ret = gpgme_data_read (dh, buf, BUF_SIZE)) > 0)
    		fwrite (buf, ret, 1, stdout);

	if (ret < 0)
	{
		std::cerr << "Fail data seek";
		std::cerr << std::endl;
		//fail_if_err (gpgme_err_code_from_errno (errno));
	}
}

/******************************************************************************/
/*                               TEST/DEBUG                                   */
/******************************************************************************/
/*
 * Create a number of friends and add them to the Map of peers.
 * Create a number of friends and add them to the Map of "others" -- people who
 * are known but are not allowed to access retroshare
 */
void GPGAuthMgr::createDummyFriends()
{
	const unsigned int DUMMY_KEY_LEN = 2048;
	
	// create key params for a few dummies
	std::string friend1 = setKeyPairParams(true, DUMMY_KEY_LEN, "friend89", 
	"I am your first friend", "friend1@friend.com", "1234");
	std::string friend2 = setKeyPairParams(true, DUMMY_KEY_LEN, "friend2", 
	"I am your second friend", "friend2@friend.com", "2345");
	std::string friend3 = setKeyPairParams(true, DUMMY_KEY_LEN, "friend3",
	"I am your third friend", "friend3@friend.com", "3456");

	// params for others
	std::string other1 = setKeyPairParams(true, DUMMY_KEY_LEN, "other89", 
	"I am your first other", "other@other.com", "1234");
	std::string other2 = setKeyPairParams(true, DUMMY_KEY_LEN, "other2", 
	"I am your second other", "other2@other.com", "2345");
	std::string other3 = setKeyPairParams(true, DUMMY_KEY_LEN, "other3",
	"I am your third other", "other3@other.com", "3456");
	
	gpgme_error_t rc = GPG_ERR_NO_ERROR; // assume OK
	rc = gpgme_op_genkey(CTX, friend1.c_str(), NULL, NULL);
	rc = gpgme_op_genkey(CTX, friend2.c_str(), NULL, NULL);
	rc = gpgme_op_genkey(CTX, friend3.c_str(), NULL, NULL);
	
	rc = gpgme_op_genkey(CTX, other1.c_str(), NULL, NULL);
	rc = gpgme_op_genkey(CTX, other2.c_str(), NULL, NULL);
	rc = gpgme_op_genkey(CTX, other3.c_str(), NULL, NULL);

	std::cout << "createDummyFriends(): exit" << std::endl;
	return;
}

static std::string setKeyPairParams(bool useRsa, unsigned int blen,
		std::string name, std::string comment, std::string email,
		std::string inPassphrase)
{
	std::ostringstream params;
	params << "<GnupgKeyParms format=\"internal\">"<< std::endl;
	if (useRsa)
	{
		params << "Key-Type: RSA"<< std::endl;
		if (blen < 1024)
		{
			std::cerr << "Weak Key... strengthing..."<< std::endl;
			blen = 1024;
		}
		blen = ((blen / 512) * 512); /* make multiple of 512 */
		params << "Key-Length: "<< blen << std::endl;
	}
	else
	{
		params << "Key-Type: DSA"<< std::endl;
		params << "Key-Length: 1024"<< std::endl;
		params << "Subkey-Type: ELG-E"<< std::endl;
		params << "Subkey-Length: 1024"<< std::endl;
	}
	params << "Name-Real: "<< name << std::endl;
	params << "Name-Comment: "<< comment << std::endl;
	params << "Name-Email: "<< email << std::endl;
	params << "Expire-Date: 0"<< std::endl;
	params << "Passphrase: "<< inPassphrase << std::endl;
	params << "</GnupgKeyParms>"<< std::endl;

	return params.str();
}



/* Author: Shiva
 * This function returns the key macthing the user parameters 
 * from the keyring
 */

static gpgme_key_t getKey(gpgme_ctx_t CTX, std::string name, std::string comment, std::string email) {
	
	gpgme_key_t key;
	gpgme_user_id_t user;
	
		/* Initiates a key listing */
	if (GPG_ERR_NO_ERROR != gpgme_op_keylist_start (CTX, "", 0))
	{
		std::cerr << "Error iterating through KeyList";
		std::cerr << std::endl;
		return false;

	}

	/* Loop until end of key */
	for(int i = 0;(GPG_ERR_NO_ERROR == gpgme_op_keylist_next (CTX, &key)); i++)
	{
		user = key->uids; 
		
		while(user != NULL) {
			if((name.size() && name == user->name) && (comment.size() && comment == user->comment) && \
				(email.size() && email == user->email)) 
			{
					/* grab a reference to the key */
        				gpgme_op_keylist_end(CTX);
					if (GPG_ERR_NO_ERROR != gpgme_op_keylist_end(CTX))
					{
						std::cerr << "Error ending KeyList";
						std::cerr << std::endl;
					}
					gpgme_key_ref(key);
					return key;
			}
			user = user->next;
		}
	}

	if (GPG_ERR_NO_ERROR != gpgme_op_keylist_end(CTX))
	{
		std::cerr << "Error ending KeyList";
		std::cerr << std::endl;
	} 
	return NULL;	
}


/* Callback function for key signing */

static gpg_error_t keySignCallback(void *opaque, gpgme_status_code_t status, \
	const char *args, int fd) {
	
	class EditParams *params = (class EditParams *)opaque;
	class SignParams *sparams = (class SignParams *)params->oParams;
	const char *result = NULL;

	fprintf(stderr,"keySignCallback status: %d args: %s, params->state: %d\n", status, args, params->state);

	/* printf stuff out */
	if (status == GPGME_STATUS_EOF)
		fprintf(stderr,"keySignCallback GPGME_STATUS_EOF\n");
	if (status == GPGME_STATUS_GOT_IT)
		fprintf(stderr,"keySignCallback GPGME_STATUS_GOT_IT\n");
	if (status == GPGME_STATUS_USERID_HINT)
		fprintf(stderr,"keySignCallback GPGME_STATUS_USERID_HINT\n");
	if (status == GPGME_STATUS_NEED_PASSPHRASE)
		fprintf(stderr,"keySignCallback GPGME_STATUS_NEED_PASSPHRASE\n");
	if (status == GPGME_STATUS_GOOD_PASSPHRASE)
		fprintf(stderr,"keySignCallback GPGME_STATUS_GOOD_PASSPHRASE\n");
	if (status == GPGME_STATUS_BAD_PASSPHRASE)
		fprintf(stderr,"keySignCallback GPGME_STATUS_BAD_PASSPHRASE\n");

	if (status == GPGME_STATUS_GET_LINE)
		fprintf(stderr,"keySignCallback GPGME_STATUS_GET_LINE\n");
	if (status == GPGME_STATUS_GET_BOOL)
		fprintf(stderr,"keySignCallback GPGME_STATUS_GET_BOOL\n");
	if (status == GPGME_STATUS_ALREADY_SIGNED)
		fprintf(stderr,"keySignCallback GPGME_STATUS_ALREADY_SIGNED\n");


	if(status == GPGME_STATUS_EOF ||
		status == GPGME_STATUS_GOT_IT || 
		status == GPGME_STATUS_USERID_HINT ||
		status == GPGME_STATUS_NEED_PASSPHRASE || 
		// status == GPGME_STATUS_GOOD_PASSPHRASE || 
		status == GPGME_STATUS_BAD_PASSPHRASE) {


		fprintf(stderr,"keySignCallback Error status\n");
		ProcessPGPmeError(params->err);

		return params->err;
	}
	
	switch (params->state)
    	{
    		case SIGN_START:
			fprintf(stderr,"keySignCallback SIGN_START\n");

      			if (status == GPGME_STATUS_GET_LINE &&
				(!std::string("keyedit.prompt").compare(args)))
        		{
          			params->state = SIGN_COMMAND;
				result = "sign";
        		}
      			else
        		{
        	  		params->state = SIGN_ERROR;
          			params->err = gpg_error (GPG_ERR_GENERAL);
			}
      			break;
    		case SIGN_COMMAND:
			fprintf(stderr,"keySignCallback SIGN_COMMAND\n");

      			if (status == GPGME_STATUS_GET_BOOL &&
				(!std::string("keyedit.sign_all.okay").compare(args)))
			{
				params->state = SIGN_UIDS;
				result = "Y";
			}
      			else if (status == GPGME_STATUS_GET_BOOL &&
				(!std::string("sign_uid.okay").compare(args)))
        		{
          			params->state = SIGN_ENTER_PASSPHRASE;
				result = "Y";
        		}
      			else if (status == GPGME_STATUS_GET_LINE &&
				(!std::string("sign_uid.expire").compare(args)))
               		{
          			params->state = SIGN_SET_EXPIRE;
				result = "Y";
        		}
      			else if (status == GPGME_STATUS_GET_LINE &&
				(!std::string("sign_uid.class").compare(args)))
        		{
          			params->state = SIGN_SET_CHECK_LEVEL;
				result = sparams->checkLvl.c_str();
        		}
      			else if (status == GPGME_STATUS_ALREADY_SIGNED)
        		{
          			/* The key has already been signed with this key */
          			params->state = SIGN_ERROR;
          			params->err =  gpg_error (GPG_ERR_CONFLICT);
        		}
      			else if (status == GPGME_STATUS_GET_LINE &&
          			(!std::string("keyedit.prompt").compare(args)))
			{
          			/* Failed sign: expired key */
          			params->state = SIGN_ERROR;
          			params->err = gpg_error (GPG_ERR_UNUSABLE_PUBKEY);
        		}
      			else
        		{
          			params->state = SIGN_ERROR;
          			params->err = gpg_error (GPG_ERR_GENERAL);
        		}
      			break;
    		case SIGN_UIDS:
			fprintf(stderr,"keySignCallback SIGN_UIDS\n");

      			if (status == GPGME_STATUS_GET_LINE &&
				(!std::string("sign_uid.expire").compare(args)))
			{
          			params->state = SIGN_SET_EXPIRE;
				result = "Y";
			} 
      			else if (status == GPGME_STATUS_GET_LINE &&
				(!std::string("sign_uid.class").compare(args)))
        		{
          			params->state = SIGN_SET_CHECK_LEVEL;
				result = sparams->checkLvl.c_str();
        		}
      			else if (status == GPGME_STATUS_GET_BOOL &&
				(!std::string("sign_uid.okay").compare(args)))
        		{
          			params->state = SIGN_ENTER_PASSPHRASE;
				result = "Y";
        		}
      			else if (status == GPGME_STATUS_GET_LINE &&
				(!std::string("keyedit.prompt").compare(args)))
			{
          			/* Failed sign: expired key */
          			params->state = SIGN_ERROR;
          			params->err = gpg_error (GPG_ERR_UNUSABLE_PUBKEY);
        		}
      			else
        		{
          			params->state = SIGN_ERROR;
          			params->err = gpg_error (GPG_ERR_GENERAL);
        		}
      			break;
    		case SIGN_SET_EXPIRE:
			fprintf(stderr,"keySignCallback SIGN_SET_EXPIRE\n");

      			if (status == GPGME_STATUS_GET_LINE &&
				(!std::string("sign_uid.class").compare(args)))
        		{
          			params->state = SIGN_SET_CHECK_LEVEL;
				result = sparams->checkLvl.c_str();
        		}        
      			else
        		{
          			params->state = SIGN_ERROR;
          			params->err = gpg_error (GPG_ERR_GENERAL);
        		}
      			break;
    		case SIGN_SET_CHECK_LEVEL:
			fprintf(stderr,"keySignCallback SIGN_SET_CHECK_LEVEL\n");

      			if (status == GPGME_STATUS_GET_BOOL &&
				(!std::string("sign_uid.okay").compare(args)))
        		{
          			params->state = SIGN_ENTER_PASSPHRASE;
				result = "Y";
        		}
      			else
        		{
          			params->state = SIGN_ERROR;
          			params->err = gpg_error (GPG_ERR_GENERAL);
        		}
      			break;
		case SIGN_ENTER_PASSPHRASE:
			fprintf(stderr,"keySignCallback SIGN_ENTER_PASSPHRASE\n");

			if(status == GPGME_STATUS_GET_HIDDEN &&
				(!std::string("passphrase.enter").compare(args)))
			{
				params->state = SIGN_CONFIRM;
				result = sparams->passphrase.c_str();
			}
			// If using pgp_pwd_callback, then never have to enter passphrase this way.
			// must catch GOOD_PASSPHRASE to move on.
			else if (status == GPGME_STATUS_GOOD_PASSPHRASE)
			{
				params->state = SIGN_CONFIRM;
			}
			else
			{
          			params->state = SIGN_ERROR;
          			params->err = gpg_error (GPG_ERR_GENERAL);
        		}
      			break;			
    		case SIGN_CONFIRM:			
			fprintf(stderr,"keySignCallback SIGN_CONFIRM\n");

      			if (status == GPGME_STATUS_GET_LINE &&	
				(!std::string("keyedit.prompt").compare(args)))
          		{
          			params->state = SIGN_QUIT;
				result = "quit";
        		}
      			else
        		{
          			params->state = SIGN_ERROR;
          			params->err = gpg_error (GPG_ERR_GENERAL);
        		}
      			break;
    		case SIGN_QUIT:
			fprintf(stderr,"keySignCallback SIGN_QUIT\n");

      			if (status == GPGME_STATUS_GET_BOOL &&
				(!std::string("keyedit.save.okay").compare(args)))
			{
          			params->state = SIGN_SAVE;
				result = "Y";
        		}
      			else
        		{
          			params->state = SIGN_ERROR;
          			params->err = gpg_error (GPG_ERR_GENERAL);
        		}
      			break;
    		case SIGN_ERROR:
			fprintf(stderr,"keySignCallback SIGN_ERROR\n");

      			if (status == GPGME_STATUS_GET_LINE &&
				(!std::string("keyedit.prompt").compare(args)))
          		{
          			/* Go to quit operation state */
          			params->state = SIGN_QUIT;
				result = "quit";
			}
      			else
        		{
          			params->state = SIGN_ERROR;
				params->err = gpg_error (GPG_ERR_GENERAL);
        		}
      			break;
    		default:
			fprintf(stderr,"keySignCallback UNKNOWN state\n");
			break;
	}

	if (result)
        {
		fprintf(stderr,"keySignCallback result:%s\n", result);
#ifndef WINDOWS_SYS
		if (*result)
			write (fd, result, strlen (result));
          	write (fd, "\n", 1);
#else
		DWORD written = 0;
		HANDLE winFd = (HANDLE) fd;
		if (*result)
			WriteFile(winFd, result, strlen(result), &written, NULL);
		WriteFile(winFd, "\n", 1, &written, NULL); 
#endif

        }
	
	fprintf(stderr,"keySignCallback Error status\n");
	ProcessPGPmeError(params->err);

	return params->err;
}



/* Callback function for assigning trust level */

static gpgme_error_t trustCallback(void *opaque, gpgme_status_code_t status, \
	const char *args, int fd) {

	class EditParams *params = (class EditParams *)opaque;
	const char *result = NULL;
	char *trustLvl = (char *)params->oParams;

	if(status == GPGME_STATUS_EOF ||
		status == GPGME_STATUS_GOT_IT) {
		return params->err;
	}
	

  	switch (params->state)
    	{
    		case TRUST_START:
      			if (status == GPGME_STATUS_GET_LINE &&
				(!std::string("keyedit.prompt").compare(args))) {
				params->state = TRUST_COMMAND;
				result = "trust";
			} else {
				params->state = TRUST_ERROR;
				params->err = gpg_error (GPG_ERR_GENERAL);
			}
			break;
		
    		case TRUST_COMMAND:
      			if (status == GPGME_STATUS_GET_LINE &&
				(!std::string("edit_ownertrust.value").compare(args))) {
				params->state = TRUST_VALUE;
				result = trustLvl;
			} else {
				params->state = TRUST_ERROR;
				params->err = gpg_error (GPG_ERR_GENERAL);
			}
			break;
    		case TRUST_VALUE:
      			if (status == GPGME_STATUS_GET_LINE &&
				(!std::string("keyedit.prompt").compare(args))) {
          			params->state = TRUST_QUIT;
				result = "quit";
        		} 				
      			else if (status == GPGME_STATUS_GET_BOOL &&
				(!std::string("edit_ownertrust.set_ultimate.okay").compare(args))) {
          			params->state = TRUST_REALLY_ULTIMATE;
				result = "Y";
			}
      			else {
          			params->state = TRUST_ERROR;
          			params->err = gpg_error (GPG_ERR_GENERAL);
        		}
      			break;
    		case TRUST_REALLY_ULTIMATE:
      			if (status == GPGME_STATUS_GET_LINE &&
          			(!std::string("keyedit.prompt").compare(args))) {
				params->state = TRUST_QUIT;
				result = "quit";
        		} else {
          			params->state = TRUST_ERROR;
          			params->err = gpg_error (GPG_ERR_GENERAL);
        		}
      			break;
    		case TRUST_QUIT:
      			if (status == GPGME_STATUS_GET_BOOL &&
          			(!std::string("keyedit.save.okay").compare(args))) {
				params->state = TRUST_SAVE;
				result = "Y";
			} else {
				params->state = TRUST_ERROR;
          			params->err = gpg_error (GPG_ERR_GENERAL);
        		}
      			break;
    		case TRUST_ERROR:
      			if (status == GPGME_STATUS_GET_LINE &&
				(!std::string("keyedit.prompt").compare(args))) {
          			/* Go to quit operation state */
          			params->state = TRUST_QUIT;
				result = "quit";
			} else {
				params->state = TRUST_ERROR;
			}
			break;
	}

	if (result)
        {
#ifndef WINDOWS_SYS
		if (*result)
			write (fd, result, strlen (result));
          	write (fd, "\n", 1);
#else
		DWORD written = 0;
		HANDLE winFd = (HANDLE) fd;
		if (*result)
			WriteFile(winFd, result, strlen (result), &written, NULL);
		WriteFile(winFd, "\n", 1, &written, NULL); 
#endif
        }
	
	return params->err;
}

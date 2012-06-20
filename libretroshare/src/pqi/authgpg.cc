/*
 * libretroshare/src    AuthGPG.cc
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

#include "authgpg.h"
#include "retroshare/rsiface.h"		// For rsicontrol.
#include "retroshare/rspeers.h"		// For RsPeerDetails.
#ifdef WINDOWS_SYS
#include "retroshare/rsinit.h"
#endif
#include "pqi/pqinotify.h"
#include "pgp/pgphandler.h"

#include <util/rsdir.h>
#include <util/pgpkey.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include "serialiser/rsconfigitems.h"
#include "cleanupxpgp.h"

#define LIMIT_CERTIFICATE_SIZE		1
#define MAX_CERTIFICATE_SIZE		10000

const time_t STORE_KEY_TIMEOUT = 1 * 60 * 60; //store key is call around every hour

AuthGPG *AuthGPG::_instance = NULL ;

void cleanupZombies(int numkill); // function to cleanup zombies under OSX.

//#define GPG_DEBUG 1

/* Turn a set of parameters into a string */
static std::string setKeyPairParams(bool useRsa, unsigned int blen,
                std::string name, std::string comment, std::string email,
                std::string inPassphrase);

// static gpg_error_t keySignCallback(void *, gpgme_status_code_t, const char *, int);
// static gpg_error_t trustCallback(void *, gpgme_status_code_t, const char *, int);
// static std::string ProcessPGPmeError(gpgme_error_t ERR);

/* Function to sign X509_REQ via GPGme.  */

bool AuthGPG::decryptTextFromFile(std::string& text,const std::string& inputfile)
{
	return PGPHandler::decryptTextFromFile(mOwnGpgId,text,inputfile) ;
}

bool AuthGPG::encryptTextToFile(const std::string& text,const std::string& outfile)
{
	return PGPHandler::encryptTextToFile(mOwnGpgId,text,outfile) ;
}

std::string pgp_pwd_callback(void * /*hook*/, const char *uid_hint, const char * /*passphrase_info*/, int prev_was_bad)
{
#define GPG_DEBUG2
#ifdef GPG_DEBUG2
	fprintf(stderr, "pgp_pwd_callback() called.\n");
#endif
	std::string password;
	rsicontrol->getNotify().askForPassword(uid_hint, prev_was_bad, password) ;

	return password ;
}

void AuthGPG::init(const std::string& path_to_public_keyring,const std::string& path_to_secret_keyring,const std::string& path_to_trustdb,const std::string& pgp_lock_file)
{
	if(_instance != NULL)
	{
		exit();
		std::cerr << "AuthGPG::init() called twice!" << std::endl ;
	}

	PGPHandler::setPassphraseCallback(pgp_pwd_callback) ;
	_instance = new AuthGPG(path_to_public_keyring,path_to_secret_keyring,path_to_trustdb,pgp_lock_file) ;
}

void AuthGPG::exit()
{
	if(_instance != NULL)
	{
		_instance->join();
		delete _instance ;
		_instance = NULL;
	}
}

AuthGPG::AuthGPG(const std::string& path_to_public_keyring,const std::string& path_to_secret_keyring,const std::string& path_to_trustdb,const std::string& pgp_lock_file)
        :p3Config(CONFIG_TYPE_AUTHGPG), 
		   PGPHandler(path_to_public_keyring,path_to_secret_keyring,path_to_trustdb,pgp_lock_file),
		   gpgMtxEngine("AuthGPG-engine"), 
			gpgMtxData("AuthGPG-data"),
			gpgKeySelected(false), 
			gpgMtxService("AuthGPG-service")
{
	start();
}

/* This function is called when retroshare is first started
 * to get the list of available GPG certificates.
 * This function should only return certs for which
 * the private(secret) keys are available.
 *
 * returns false if GnuPG is not available.
 */
bool AuthGPG::availableGPGCertificatesWithPrivateKeys(std::list<std::string> &ids)
{
	std::list<PGPIdType> pids ;

	PGPHandler::availableGPGCertificatesWithPrivateKeys(pids) ;

	for(std::list<PGPIdType>::const_iterator it(pids.begin());it!=pids.end();++it)
		ids.push_back( (*it).toStdString() ) ;

	/* return false if there are no private keys */
	return !ids.empty();
}

/* You can initialise Retroshare with
 * (a) load existing certificate.
 * (b) a new certificate.
 *
 * This function must be called successfully (return == 1)
 * before anything else can be done. (except above fn).
 */
int AuthGPG::GPGInit(const std::string &ownId)
{
	std::cerr << "AuthGPG::GPGInit() called with own gpg id : " << ownId << std::endl;

	mOwnGpgId = PGPIdType(ownId);

	//force the validity of the private key. When set to unknown, it caused signature and text encryptions bugs
	privateTrustCertificate(ownId, 5);

	std::cerr << "AuthGPG::GPGInit finished." << std::endl;

	return 1;
}

 AuthGPG::~AuthGPG()
{
}

void AuthGPG::run()
{
    int count = 0;

    while (isRunning())
    {
#ifdef WIN32
        Sleep(100);
#else
        usleep(100000);
#endif

        /* every 100 milliseconds */
        processServices();
#ifdef SUSPENDED
        /* every minute */
        if (++count >= 600) {
            storeAllKeys_tick();
            count = 0;
        }
#endif
    }
}

void AuthGPG::processServices()
{
    AuthGPGOperation *operation = NULL;
    AuthGPGService *service = NULL;

    {
        RsStackMutex stack(gpgMtxService); /******* LOCKED ******/

        std::list<AuthGPGService*>::iterator serviceIt;
        for (serviceIt = services.begin(); serviceIt != services.end(); serviceIt++) {
            operation = (*serviceIt)->getGPGOperation();
            if (operation) {
                service = *serviceIt;
                break;
            }
        }
    } /******* UNLOCKED ******/

    if (operation == NULL) {
        /* nothing to do */
        return;
    }

    if (service == NULL) {
        /* huh ? */
        delete operation;
        return;
    }

    AuthGPGOperationLoadOrSave *loadOrSave = dynamic_cast<AuthGPGOperationLoadOrSave*>(operation);
    if (loadOrSave) 
	 {
		 if (loadOrSave->m_load) 
		 {
			 /* process load operation */


			 /* load the certificate */


			 /* don't bother loading - if we already have the certificate */
			 if (isGPGId(loadOrSave->m_certGpgId))
			 {
#ifdef GPG_DEBUG
				 std::cerr << "AuthGPGimpl::processServices() Skipping load - already have it" << std::endl;
#endif
			 }
			 else
			 {
#ifdef GPG_DEBUG
				 std::cerr << "AuthGPGimpl::processServices() Process load operation" << std::endl;
#endif
				 std::string error_string ;
				 LoadCertificateFromString(loadOrSave->m_certGpg, loadOrSave->m_certGpgId,error_string);
			 }



		 } else {
			 /* process save operation */

#ifdef GPG_DEBUG
			 std::cerr << "AuthGPGimpl::processServices() Process save operation" << std::endl;
#endif

			 /* save the certificate to string */
			 /*****
			  * #define DISABLE_CERTIFICATE_SEND	1
			  ****/

			 loadOrSave->m_certGpg = SaveCertificateToString(loadOrSave->m_certGpgId,true);

#ifdef GPG_DEBUG
			 std::cerr << "Certificate for: " << loadOrSave->m_certGpgId << " is: ";
			 std::cerr << std::endl;
			 std::cerr << loadOrSave->m_certGpg;
			 std::cerr << std::endl;
#endif

		 }

		 service->setGPGOperation(loadOrSave);
	 } 
	 else 
	 {
#ifdef GPG_DEBUG
        std::cerr << "AuthGPGimpl::processServices() Unknown operation" << std::endl;
#endif
    }

    delete operation;
}

#ifdef TO_REMOVE
// store all keys in map mKeyList to avoid callin gpgme exe repeatedly
bool   AuthGPG::storeAllKeys()
{
#ifdef GPG_DEBUG
	std::cerr << "AuthGPG::storeAllKeys()" << std::endl;
#endif

	std::string ownGpgId;

	/* store member variables locally */
	{
		RsStackMutex stack(gpgMtxData);

		if (!gpgmeInit)
		{
			std::cerr << "AuthGPG::storeAllKeys() Error since GPG is not initialised" << std::endl;
			return false;
		}

		mStoreKeyTime = time(NULL);
		ownGpgId = mOwnGpgId;
	}

	/* read keys from gpg to local list */
	std::list<gpgcert> keyList;

	{
#ifdef GPG_DEBUG
		std::cerr << "AuthGPG::storeAllKeys() clearing existing ones" << std::endl;
#endif

		gpg_error_t ERR;

		RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/

		/* enable SIG mode */
		gpgme_keylist_mode_t origmode = gpgme_get_keylist_mode(CTX);
		gpgme_keylist_mode_t mode = origmode | GPGME_KEYLIST_MODE_SIGS;

		gpgme_set_keylist_mode(CTX, mode);

		/* store keys */
		gpgme_key_t KEY = NULL;

		/* Initiates a key listing 0 = All Keys */
		if (GPG_ERR_NO_ERROR != gpgme_op_keylist_start (CTX, "", 0))
		{
			std::cerr << "AuthGPG::storeAllKeys() Error iterating through KeyList" << std::endl;
			//                if (rsicontrol != NULL) {
			//                    rsicontrol->getNotify().notifyErrorMsg(0,0,"Error reading gpg keyring, cannot acess key list.");
			//                }
			gpgme_set_keylist_mode(CTX, origmode);
			return false;
		}

		/* Loop until end of key */
		ERR = gpgme_op_keylist_next (CTX, &KEY);
		if (GPG_ERR_NO_ERROR != ERR) {
			std::cerr << "AuthGPG::storeAllKeys() didn't find any gpg key in the keyring" << std::endl;
			//                if (rsicontrol != NULL) {
			//                    rsicontrol->getNotify().notifyErrorMsg(0,0,"Error reading gpg keyring, cannot find any key in the list.");
			//                }
			gpgme_set_keylist_mode(CTX, origmode);
			return false;
		}

		for(int i = 0;GPG_ERR_NO_ERROR == ERR; i++)
		{
			/* store in pqiAuthDetails */
			gpgcert nu;

			/* NB subkeys is a linked list and can contain multiple keys.
			 * first key is primary.
			 */

			if ((!KEY->subkeys) || (!KEY->uids))
			{
				std::cerr << "AuthGPG::storeAllKeys() Invalid Key in List... skipping" << std::endl;
				continue;
			}

			/* In general MainSubKey is used to sign all others!
			 * Don't really need to worry about other ids either.
			 */
			gpgme_subkey_t mainsubkey = KEY->subkeys;
			nu.id  = mainsubkey->keyid;
			nu.fpr = mainsubkey->fpr;

#ifdef GPG_DEBUG
			std::cerr << "MAIN KEYID: " << nu.id << " FPR: " << nu.fpr << std::endl;

			gpgme_subkey_t subkeylist = KEY->subkeys;
			while(subkeylist != NULL)
			{
				std::cerr << "\tKEYID: " << subkeylist->keyid << " FPR: " << subkeylist->fpr << std::endl;

				subkeylist = subkeylist->next;
			}
#endif


			/* NB uids is a linked list and can contain multiple ids.
			 * first id is primary.
			 */
			gpgme_user_id_t mainuid = KEY->uids;
			nu.name  = mainuid->name;
			nu.email = mainuid->email;
			gpgme_key_sig_t mainsiglist = mainuid->signatures;

			nu.ownsign = false;
			while(mainsiglist != NULL)
			{
				if (mainsiglist->status == GPG_ERR_NO_ERROR)
				{
					/* add as a signature ... even if the 
					 * we haven't go the peer yet. 
					 * (might be yet to come).
					 */
					std::string keyid = mainsiglist->keyid;
					if (nu.signers.end() == std::find(
								nu.signers.begin(),
								nu.signers.end(),keyid))
					{
						nu.signers.push_back(keyid);
					}
					if (keyid == ownGpgId) {
						nu.ownsign = true;
					}
				}
				mainsiglist = mainsiglist->next;
			}

#ifdef GPG_DEBUG
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
#endif

			/* signatures are attached to uids... but only supplied
			 * if GPGME_KEYLIST_MODE_SIGS is on.
			 * signature notation supplied is GPGME_KEYLIST_MODE_SIG_NOTATION is on
			 */
			nu.trustLvl = KEY->owner_trust;
			nu.validLvl = mainuid->validity;

			/* grab a reference, so the key remains */
			gpgme_key_ref(KEY);
			nu.key = KEY;

			/* store in map */
			keyList.push_back(nu);
#ifdef GPG_DEBUG
			std::cerr << "nu.name" << nu.name << std::endl;
			std::cerr << "nu.trustLvl" << nu.trustLvl << std::endl;
			std::cerr << "nu.accept_connection" << nu.accept_connection << std::endl;
#endif

			ERR = gpgme_op_keylist_next (CTX, &KEY);
		}

		if (GPG_ERR_NO_ERROR != gpgme_op_keylist_end(CTX))
		{
			std::cerr << "Error ending KeyList" << std::endl;
			gpgme_set_keylist_mode(CTX, origmode);
			return false;
		} 

		gpgme_set_keylist_mode(CTX, origmode);
	}

	/* process read gpg keys and store it in member */
	std::list<std::string> gpg_change_trust_list;

	{
		RsStackMutex stack(gpgMtxData);

		//let's start a new list
		mKeyList.clear();

		for (std::list<gpgcert>::iterator it = keyList.begin(); it != keyList.end(); it++) {
			gpgcert &nu = *it;

			std::map<std::string, bool>::iterator itAccept;
			if (mAcceptToConnectMap.end() != (itAccept = mAcceptToConnectMap.find(nu.id))) {
				nu.accept_connection = itAccept->second;
			} else {
				nu.accept_connection = false;
				mAcceptToConnectMap[nu.id] = false;
			}

			if (nu.trustLvl < 2 && nu.accept_connection) {
				//add it to the list of key that we will force the trust to 2
				gpg_change_trust_list.push_back(nu.id);
			}

			/* grab a reference, so the key remains */
			gpgme_key_ref(nu.key);

			mKeyList[nu.id] = nu;

			//store own key
			if (nu.id == mOwnGpgId) {
				/* grab a reference, so the key remains */
				gpgme_key_ref(nu.key);

				gpgme_key_unref(mOwnGpgCert.key);
				mOwnGpgCert = nu;
			}
		}
	}

	std::list<std::string>::iterator it;
	for(it = gpg_change_trust_list.begin(); it != gpg_change_trust_list.end(); it++)
	{
		privateTrustCertificate(*it, 3);
	}

	return true;

}
std::string ProcessPGPmeError(gpgme_error_t ERR)
{
	gpgme_err_code_t code = gpgme_err_code(ERR);
	gpgme_err_source_t src = gpgme_err_source(ERR);

	std::ostringstream ss ;

	if(code > 0)
	{
		ss << "GPGme ERROR: Code: " << code << " Source: " << src << std::endl;
		ss << "GPGme ERROR: " << gpgme_strerror(ERR) << std::endl;
	}
	else
		return std::string("Unknown error") ;

	return ss.str() ;
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
        if (summary & GPGME_SIGSUM_SIG_EXPIRED)
                std::cerr << " SIG_EXPIRED ";
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
#endif

bool AuthGPG::DoOwnSignature(const void *data, unsigned int datalen, void *buf_sigout, unsigned int *outl)
{
	return PGPHandler::SignDataBin(mOwnGpgId,data,datalen,(unsigned char *)buf_sigout,outl) ;
}


/* import to GnuPG and other Certificates */
bool AuthGPG::VerifySignature(const void *data, int datalen, const void *sig, unsigned int siglen, const std::string &withfingerprint)
{
	if(withfingerprint.length() != 40)
	{
		std::cerr << "WARNING: Still need to implement signature verification from complete keyring." << std::endl;
		return false ;
	}

	return PGPHandler::VerifySignBin((unsigned char*)data,datalen,(unsigned char*)sig,siglen,PGPFingerprintType(withfingerprint)) ;
}



	
bool   AuthGPG::active()
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

        return gpgKeySelected;
}

bool    AuthGPG::GeneratePGPCertificate(const std::string& name, 
													const std::string& email, const std::string& passwd, std::string &pgpId, std::string& errString) 
{
	RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/
	
	PGPIdType id ;

	bool res = PGPHandler::GeneratePGPCertificate(name, email, passwd, id, errString) ;

	pgpId = id.toStdString() ;
	return res ;
}

/**** These Two are common */
std::string AuthGPG::getGPGName(const std::string &id,bool *success)
{
	if(id.length() != 16)
	{
		std::cerr << "Wrong string passed to getGPGDetails: \"" << id << "\"" << std::endl;
		return std::string() ;
	}
	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

	const PGPCertificateInfo *info = PGPHandler::getCertificateInfo(PGPIdType(id)) ;

	if(info != NULL)
	{
		if(success != NULL) *success = true ;
		return info->_name ;
	}
	else
	{
		if(success != NULL) *success = false ;
		return "[Unknown PGP Cert name]" ;
	}
}

/**** These Two are common */
std::string AuthGPG::getGPGEmail(const std::string &id,bool *success)
{
	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
	const PGPCertificateInfo *info = PGPHandler::getCertificateInfo(PGPIdType(id)) ;

	if(info != NULL)
	{
		if(success != NULL) *success = true ;
		return info->_email ;
	}
	else
	{
		if(success != NULL) *success = false ;
		return "[Unknown PGP Cert email]" ;
	}
}

/**** GPG versions ***/

std::string AuthGPG::getGPGOwnId()
{
	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
	return mOwnGpgId.toStdString();
}

std::string AuthGPG::getGPGOwnName()
{
	return getGPGName(mOwnGpgId.toStdString()) ;
}

bool	AuthGPG::getGPGAllList(std::list<std::string> &ids)
{
	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

	std::list<PGPIdType> list ;
	PGPHandler::getGPGFilteredList(list) ;

	for(std::list<PGPIdType>::const_iterator it(list.begin());it!=list.end();++it)
		ids.push_back( (*it).toStdString() ) ;

	return true;
}

#ifdef TO_REMOVE
bool 	AuthGPG::decryptText(gpgme_data_t CIPHER, gpgme_data_t PLAIN)
{
	RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/
	gpgme_set_armor (CTX, 1);
	gpg_error_t ERR;

	cleanupZombies(2); // cleanup zombies under OSX. (Called before gpgme operation)

	if (GPG_ERR_NO_ERROR != (ERR = gpgme_op_decrypt (CTX, CIPHER, PLAIN)))
	{
		std::cerr << "AuthGPG::decryptText() Error decrypting text" << std::endl;
		std::cerr << ProcessPGPmeError(ERR) << std::endl;
		return false;
	}

	return true;
}

bool 	AuthGPG::encryptText(gpgme_data_t PLAIN, gpgme_data_t CIPHER)
{
	RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/
	gpgme_encrypt_flags_t* flags = new gpgme_encrypt_flags_t();
	gpgme_key_t keys[2] = {mOwnGpgCert.key, NULL};
	gpgme_set_armor (CTX, 1);
	gpg_error_t ERR;

	cleanupZombies(2); // cleanup zombies under OSX. (Called before gpgme operation)

	if (GPG_ERR_NO_ERROR != (ERR = gpgme_op_encrypt(CTX, keys, *flags, PLAIN, CIPHER)))
	{
		std::cerr << "AuthGPG::encryptText() Error encrypting text" << std::endl;
		std::cerr << ProcessPGPmeError(ERR) << std::endl;
		return false;
	}

	return true;
}
#endif

bool AuthGPG::isKeySupported(const std::string& id) const
{
	const PGPCertificateInfo *pc = PGPHandler::getCertificateInfo(PGPIdType(id)) ;

	if(pc == NULL)
		return false ;

	return !(pc->_flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_UNSUPPORTED_ALGORITHM) ;
}

bool AuthGPG::getGPGDetails(const std::string& id, RsPeerDetails &d) 
{
	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

	if(id.length() != 16)
	{
		std::cerr << "Wrong string passed to getGPGDetails: \"" << id << "\"" << std::endl;
		return false ;
	}

	const PGPCertificateInfo *pc = PGPHandler::getCertificateInfo(PGPIdType(id)) ;

	if(pc == NULL)
		return false ;

	const PGPCertificateInfo& cert(*pc) ;

	d.id = id ;
	d.gpg_id = id ;
	d.name = cert._name;
	d.email = cert._email;
	d.trustLvl = cert._trustLvl;
	d.validLvl = cert._validLvl;
	d.ownsign = cert._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE;
	d.gpgSigners.clear() ;
	for(std::set<std::string>::const_iterator it(cert.signers.begin());it!=cert.signers.end();++it)
		d.gpgSigners.push_back( *it ) ;

	d.fpr = cert._fpr.toStdString();

	d.accept_connection = cert._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION;
	d.hasSignedMe = cert._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_SIGNED_ME;

	return true;
}

bool AuthGPG::getGPGFilteredList(std::list<std::string>& list,bool (*filter)(const PGPCertificateInfo&)) 
{
	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
	std::list<PGPIdType> ids ;

	PGPHandler::getGPGFilteredList(ids,filter) ;

	for(std::list<PGPIdType>::const_iterator it(ids.begin());it!=ids.end();++it)
		list.push_back( (*it).toStdString() ) ;

	return true ;
}

static bool filter_Validity(const PGPCertificateInfo& info) { return true ; } //{ return info._validLvl >= PGPCertificateInfo::GPGME_VALIDITY_MARGINAL ; }
static bool filter_Accepted(const PGPCertificateInfo& info) { return info._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION ; }
static bool filter_OwnSigned(const PGPCertificateInfo& info) { return info._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE ; }

bool	AuthGPG::getGPGValidList(std::list<std::string> &ids)
{
	return getGPGFilteredList(ids,&filter_Validity);
}

bool	AuthGPG::getGPGAcceptedList(std::list<std::string> &ids)
{
	return getGPGFilteredList(ids,&filter_Accepted);
}

bool	AuthGPG::getGPGSignedList(std::list<std::string> &ids)
{
	return getGPGFilteredList(ids,&filter_OwnSigned);
}

bool	AuthGPG::getCachedGPGCertificate(const std::string &id, std::string &certificate)
{
	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
	certificate = PGPHandler::SaveCertificateToString(PGPIdType(id),false) ;

#ifdef LIMIT_CERTIFICATE_SIZE
	std::string cleaned_key ;
	if(PGPKeyManagement::createMinimalKey(certificate,cleaned_key))
		certificate = cleaned_key ;
#endif

	return certificate.length() > 0 ;
}

/*****************************************************************
 * Loading and Saving Certificates - this has to 
 * be able to handle both openpgp and X509 certificates.
 * 
 * X509 are passed onto AuthSSL, OpenPGP are passed to gpgme.
 *
 */


/* SKTAN : do not know how to use std::string id */
std::string AuthGPG::SaveCertificateToString(const std::string &id,bool include_signatures)
{

	if (!isGPGId(id)) {
		std::cerr << "AuthGPG::SaveCertificateToString() unknown ID" << std::endl;
		return "";
	}

	RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/

	std::string tmp = PGPHandler::SaveCertificateToString(PGPIdType(id),include_signatures) ;

	// Try to remove signatures manually.
	//
	std::string cleaned_key ;

	if( (!include_signatures) && PGPKeyManagement::createMinimalKey(tmp,cleaned_key))
		return cleaned_key ;
	else
		return tmp;
}

/* import to GnuPG and other Certificates */
bool AuthGPG::LoadCertificateFromString(const std::string &str, std::string &gpg_id,std::string& error_string)
{
	RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/

	PGPIdType id ;
	bool res = PGPHandler::LoadCertificateFromString(str,id,error_string) ;

	gpg_id = id.toStdString(); 

	return res ;
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

/* These take PGP Ids */
bool AuthGPG::AllowConnection(const std::string &gpg_id, bool accept)
{
#ifdef GPG_DEBUG
        std::cerr << "AuthGPG::AllowConnection(" << gpg_id << ")" << std::endl;
#endif

	/* Was a "Reload Certificates" here -> be shouldn't be needed -> and very expensive, try without. */
	{
		RsStackMutex stack(gpgMtxData);
		PGPHandler::setAcceptConnexion(PGPIdType(gpg_id),accept) ;
	}

	IndicateConfigChanged();

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_FRIENDS, accept ? NOTIFY_TYPE_ADD : NOTIFY_TYPE_DEL);

	return true;
}

/* These take PGP Ids */
bool AuthGPG::SignCertificateLevel0(const std::string &id)
{
	/* remove unused parameter warnings */
	(void) id;

#ifdef GPG_DEBUG
	std::cerr << "AuthGPG::SignCertificat(" << id << ")" << std::endl;
#endif

	if (1 != privateSignCertificate(id))
	{
//		storeAllKeys();
		return false;
	}

	/* reload stuff now ... */
//	storeAllKeys();
	return true;
}

bool AuthGPG::RevokeCertificate(const std::string &id)
{
	//RsStackMutex stack(gpgMtx); /******* LOCKED ******/

	/* remove unused parameter warnings */
	(void) id;

#ifdef GPG_DEBUG
	std::cerr << "AuthGPG::RevokeCertificate(" << id << ") not implemented yet" << std::endl;
#endif

	return false;
}

bool AuthGPG::TrustCertificate(const std::string &id, int trustlvl)
{
#ifdef GPG_DEBUG
        std::cerr << "AuthGPG::TrustCertificate(" << id << ", " << trustlvl << ")" << std::endl;
#endif
        if (1 != privateTrustCertificate(id, trustlvl))
        {
//                storeAllKeys();
                return false;
        }

	/* Keys are reloaded by privateTrustCertificate */
        return true;
}

#if 0 
/* remove otherwise will cause bugs */
bool AuthGPG::SignData(std::string input, std::string &sign)
{
	return false;
}

bool AuthGPG::SignData(const void *data, const uint32_t len, std::string &sign)
{
	return false;
}


bool AuthGPG::SignDataBin(std::string input, unsigned char *sign, unsigned int *signlen)
{
	return false;
}
#endif

bool AuthGPG::SignDataBin(const void *data, unsigned int datalen, unsigned char *sign, unsigned int *signlen) {
        return DoOwnSignature(data, datalen,
                        sign, signlen);
}

bool AuthGPG::VerifySignBin(const void *data, uint32_t datalen, unsigned char *sign, unsigned int signlen, const std::string &withfingerprint) {
        return VerifySignature(data, datalen,
                        sign, signlen, withfingerprint);
}


	/* Sign/Trust stuff */

int	AuthGPG::privateSignCertificate(const std::string &id)
{
	std::cerr << __PRETTY_FUNCTION__ << ": To be implemented." << std::endl;

//	/* The key should be in Others list and not in Peers list ?? 
//	 * Once the key is signed, it moves from Others to Peers list ??? 
//	 */
//
//	gpgcert signKey;
//	gpgcert ownKey;
//
//	{
//		RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
//		certmap::iterator it;
//
//		if (mKeyList.end() == (it = mKeyList.find(id)))
//		{
//			return false;
//		}
//
//		/* grab a reference, so the key remains */
//		gpgme_key_ref(it->second.key);
//
//		signKey = it->second;
//
//		/* grab a reference, so the key remains */
//		gpgme_key_ref(mOwnGpgCert.key);
//
//		ownKey  = mOwnGpgCert;
//	} /******* UNLOCKED ******/
//
//	RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/
//
//	class SignParams sparams("0");
//	class EditParams params(SIGN_START, &sparams);
//	gpgme_data_t out;
//	gpg_error_t ERR;
//
//	if(GPG_ERR_NO_ERROR != (ERR = gpgme_data_new(&out))) {
//		return 0;
//	}
//
//	gpgme_signers_clear(CTX);
//	if(GPG_ERR_NO_ERROR != (ERR = gpgme_signers_add(CTX, ownKey.key))) {
//		gpgme_data_release(out);
//		return 0;
//	}
//	
//	if(GPG_ERR_NO_ERROR != (ERR = gpgme_op_edit(CTX, signKey.key, keySignCallback, &params, out))) {
//		gpgme_data_release(out);
//		gpgme_signers_clear(CTX);
//		return 0;	
//	}
//
//	gpgme_data_release(out);
//	gpgme_signers_clear(CTX);

	return 1;
}

/* revoke the signature on Certificate */
int	AuthGPG::privateRevokeCertificate(const std::string &/*id*/)
{
	//RsStackMutex stack(gpgMtx); /******* LOCKED ******/

	return 0;
}

int	AuthGPG::privateTrustCertificate(const std::string &id, int trustlvl)
{
	/* The certificate should be in Peers list ??? */	
	if(!isGPGAccepted(id)) {
		std::cerr << "Invalid Certificate" << std::endl;
		return 0;
	}

	std::cerr << __PRETTY_FUNCTION__ << ": to be implemented!" << std::endl;
	
	PGPHandler::privateTrustCertificate(PGPIdType(id),trustlvl) ;

//	{
//		gpgcert trustCert;
//		{
//			RsStackMutex stack(gpgMtxData);
//
//			trustCert = mKeyList.find(id)->second;
//		} /******* UNLOCKED ******/
//
//		RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/
//
//		gpgme_key_t trustKey = trustCert.key;
//		std::string trustString;
//		std::ostringstream trustStrOut;
//		trustStrOut << trustlvl;
//		class TrustParams sparams(trustStrOut.str());
//		class EditParams params(TRUST_START, &sparams);
//		gpgme_data_t out;
//		gpg_error_t ERR;
//
//		if(GPG_ERR_NO_ERROR != (ERR = gpgme_data_new(&out))) {
//			return 0;
//		}
//
//		if(GPG_ERR_NO_ERROR != (ERR = gpgme_op_edit(CTX, trustKey, trustCallback, &params, out))) {
//			gpgme_data_release(out);
//			return 0;
//		}
//
//		gpgme_data_release(out);
//
//		//the key ref has changed, we got to get rid of the old reference.
//		trustCert.key = NULL;
//	}
//
//	storeAllKeys();
		
	return 1;
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
#ifdef GPG_DEBUG
                        std::cerr << "Weak Key... strengthing..."<< std::endl;
#endif
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

#ifdef UNUSED_CODE
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
                std::cerr << "Error ending KeyList" << std::endl;
	} 
	return NULL;	
}
#endif

#ifdef TO_REMOVE
/* Callback function for key signing */

static gpg_error_t keySignCallback(void *opaque, gpgme_status_code_t status, \
	const char *args, int fd) {
	
	class EditParams *params = (class EditParams *)opaque;
	class SignParams *sparams = (class SignParams *)params->oParams;
	const char *result = NULL;
#ifdef GPG_DEBUG
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

                /* printf stuff out */
        if (params->state == SIGN_START)
                fprintf(stderr,"keySignCallback params->state SIGN_START\n");
        if (params->state == SIGN_COMMAND)
                fprintf(stderr,"keySignCallback params->state SIGN_COMMAND\n");
        if (params->state == SIGN_UIDS)
                fprintf(stderr,"keySignCallback params->state SIGN_UIDS\n");
        if (params->state == SIGN_SET_EXPIRE)
                fprintf(stderr,"keySignCallback params->state SIGN_SET_EXPIRE\n");
        if (params->state == SIGN_SET_CHECK_LEVEL)
                fprintf(stderr,"keySignCallback params->state SIGN_SET_CHECK_LEVEL\n");
        if (params->state == SIGN_CONFIRM)
                fprintf(stderr,"keySignCallback params->state SIGN_CONFIRM\n");
        if (params->state == SIGN_QUIT)
                fprintf(stderr,"keySignCallback params->state SIGN_QUIT\n");
        if (params->state == SIGN_ENTER_PASSPHRASE)
                fprintf(stderr,"keySignCallback params->state SIGN_ENTER_PASSPHRASE\n");
        if (params->state == SIGN_ERROR)
                fprintf(stderr,"keySignCallback params->state SIGN_ERROR");
#endif


	if(status == GPGME_STATUS_EOF ||
		status == GPGME_STATUS_GOT_IT || 
		status == GPGME_STATUS_USERID_HINT ||
		status == GPGME_STATUS_NEED_PASSPHRASE || 
		// status == GPGME_STATUS_GOOD_PASSPHRASE || 
		status == GPGME_STATUS_BAD_PASSPHRASE) {


		fprintf(stderr,"keySignCallback Error status\n");
		std::cerr << ProcessPGPmeError(params->err) << std::endl;

		return params->err;
	}
	
	switch (params->state)
    	{
    		case SIGN_START:
#ifdef GPG_DEBUG
			fprintf(stderr,"keySignCallback SIGN_START\n");
#endif

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
#ifdef GPG_DEBUG
			fprintf(stderr,"keySignCallback SIGN_COMMAND\n");
#endif

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
                                params->state = SIGN_QUIT;
                                result = "quit";
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
#ifdef GPG_DEBUG
			fprintf(stderr,"keySignCallback SIGN_UIDS\n");
#endif

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
#ifdef GPG_DEBUG
			fprintf(stderr,"keySignCallback SIGN_SET_EXPIRE\n");
#endif

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
#ifdef GPG_DEBUG
			fprintf(stderr,"keySignCallback SIGN_SET_CHECK_LEVEL\n");
#endif

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
#ifdef GPG_DEBUG
                        fprintf(stderr,"keySignCallback SIGN_ENTER_PASSPHRASE\n");
#endif

                        if (status == GPGME_STATUS_GOOD_PASSPHRASE)
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
#ifdef GPG_DEBUG
			fprintf(stderr,"keySignCallback SIGN_CONFIRM\n");
#endif

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
#ifdef GPG_DEBUG
			fprintf(stderr,"keySignCallback SIGN_QUIT\n");
#endif

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
#ifdef GPG_DEBUG
			fprintf(stderr,"keySignCallback SIGN_ERROR\n");
#endif

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
#ifdef GPG_DEBUG
		fprintf(stderr,"keySignCallback result:%s\n", result);
#endif
#ifndef WINDOWS_SYS
		if (*result)
		{
			write (fd, result, strlen (result));
			write (fd, "\n", 1);
		}
#else
		DWORD written = 0;
		HANDLE winFd = (HANDLE) fd;
		if (*result)
		{
			WriteFile(winFd, result, strlen(result), &written, NULL);
			WriteFile(winFd, "\n", 1, &written, NULL); 
		}
#endif

	}
	
	fprintf(stderr,"keySignCallback Error status\n");
	std::cerr << ProcessPGPmeError(params->err) << std::endl;

	return params->err;
}
#endif


#ifdef TO_REMOVE
/* Callback function for assigning trust level */

static gpgme_error_t trustCallback(void *opaque, gpgme_status_code_t status, \
	const char *args, int fd) {

        class EditParams *params = (class EditParams *)opaque;
        class TrustParams *tparams = (class TrustParams *)params->oParams;
        const char *result = NULL;

                /* printf stuff out */
#ifdef GPG_DEBUG
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
                fprintf(stderr,"keySignCallback GPGME_STATUS_GET_BOOL \n");
        if (status == GPGME_STATUS_ALREADY_SIGNED)
                fprintf(stderr,"keySignCallback GPGME_STATUS_ALREADY_SIGNED\n");

                /* printf stuff out */
        if (params->state == TRUST_START)
                fprintf(stderr,"keySignCallback params->state TRUST_START\n");
        if (params->state == TRUST_COMMAND)
                fprintf(stderr,"keySignCallback params->state TRUST_COMMAND\n");
        if (params->state == TRUST_VALUE)
                fprintf(stderr,"keySignCallback params->state TRUST_VALUE\n");
        if (params->state == TRUST_REALLY_ULTIMATE)
                fprintf(stderr,"keySignCallback params->state TRUST_REALLY_ULTIMATE\n");
        if (params->state == TRUST_QUIT)
                fprintf(stderr,"keySignCallback params->state TRUST_QUIT\n");
        if (params->state == TRUST_ERROR)
                fprintf(stderr,"keySignCallback params->state TRUST_ERROR\n");
#endif


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
                                result = tparams->trustLvl.c_str();;
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
#ifdef GPG_DEBUG
                std::cerr << "trustCallback() result : " << result << std::endl;
#endif
        }
	
	return params->err;
}
#endif

// -----------------------------------------------------------------------------------//
// --------------------------------  Config functions  ------------------------------ //
// -----------------------------------------------------------------------------------//
//
RsSerialiser *AuthGPG::setupSerialiser()
{
        RsSerialiser *rss = new RsSerialiser ;
        rss->addSerialType(new RsGeneralConfigSerialiser());
        return rss ;
}

bool AuthGPG::saveList(bool& cleanup, std::list<RsItem*>& lst)
{
#ifdef GPG_DEBUG
	std::cerr << "AuthGPG::saveList() called" << std::endl ;
#endif
	std::list<std::string> ids ;	
	getGPGAcceptedList(ids) ;				// needs to be done before the lock

	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

	cleanup = true ;

	// Now save config for network digging strategies
	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;

	for (std::list<std::string>::const_iterator it(ids.begin()); it != ids.end(); ++it) 
		if((*it) != mOwnGpgId.toStdString()) // skip our own id.
		{
			RsTlvKeyValue kv;
			kv.key = *it ;
#ifdef GPG_DEBUG
			std::cerr << "AuthGPG::saveList() called (it->second) : " << (it->second) << std::endl ;
#endif
			kv.value = "TRUE";
			vitem->tlvkvs.pairs.push_back(kv) ;
		}
	lst.push_back(vitem);

	return true;
}

bool AuthGPG::loadList(std::list<RsItem*>& load)
{
#ifdef GPG_DEBUG
	std::cerr << "AuthGPG::loadList() Item Count: " << load.size() << std::endl;
#endif

	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
	/* load the list of accepted gpg keys */
	std::list<RsItem *>::iterator it;
	for(it = load.begin(); it != load.end(); it++) 
	{
		RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet *>(*it);
		if(vitem) 
		{
#ifdef GPG_DEBUG
			std::cerr << "AuthGPG::loadList() General Variable Config Item:" << std::endl;
			vitem->print(std::cerr, 10);
			std::cerr << std::endl;
#endif

			std::list<RsTlvKeyValue>::iterator kit;
			for(kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); kit++) 
				if (kit->key != mOwnGpgId.toStdString()) 
					PGPHandler::setAcceptConnexion(PGPIdType(kit->key), (kit->value == "TRUE"));
		}
		delete (*it);
	}
	return true;
}

bool AuthGPG::addService(AuthGPGService *service)
{
	RsStackMutex stack(gpgMtxService); /********* LOCKED *********/

	if (std::find(services.begin(), services.end(), service) != services.end()) {
		/* it exists already! */
		return false;
	}

	services.push_back(service);
	return true;
}

#ifdef TO_REMOVE

/***************************** HACK to Cleanup OSX Zombies *****************************/


#ifdef __APPLE__
#include <sys/wait.h>
#endif

void cleanupZombies(int numkill)
{

#ifdef __APPLE__

	pid_t wpid = -1; // any child.
	int stat_loc = 0;
	int options = WNOHANG ;
	
	//std::cerr << "cleanupZombies() checking for dead children";
	//std::cerr << std::endl;

	int i;	
	for(i = 0; i < numkill; i++)
	{
		pid_t childpid = waitpid(wpid, &stat_loc, options);
	
		if (childpid > 0)
		{
			std::cerr << "cleanupZombies() Found stopped child with pid: " << childpid;
			std::cerr << std::endl;
		}
		else
		{
			//std::cerr << "cleanupZombies() No Zombies around!";
			//std::cerr << std::endl;
			break;
		}
	}

	//std::cerr << "cleanupZombies() Killed " << i << " zombies";
	//std::cerr << std::endl;
#else
	/* remove unused parameter warnings */
	(void) numkill;
#endif

	return;
}
#endif	

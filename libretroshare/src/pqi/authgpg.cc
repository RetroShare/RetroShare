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
#include "retroshare/rsiface.h"
#include "retroshare/rsinit.h"
#include <util/rsdir.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include "serialiser/rsconfigitems.h"
#include "cleanupxpgp.h"

const time_t STORE_KEY_TIMEOUT = 1 * 60 * 60; //store key is call around every hour

void cleanupZombies(int numkill); // function to cleanup zombies under OSX.

//#define GPG_DEBUG 1

static AuthGPGimpl *instance_gpg = NULL;

void AuthGPGInit()
{
	instance_gpg = new AuthGPGimpl();
}

void AuthGPGExit()
{
	if (instance_gpg) {
		instance_gpg->join();
		delete(instance_gpg);
		instance_gpg = NULL;
	}
}

AuthGPG *AuthGPG::getAuthGPG()
{
	return instance_gpg;
}

/* Turn a set of parameters into a string */
#if 0
static std::string setKeyPairParams(bool useRsa, unsigned int blen,
                std::string name, std::string comment, std::string email);
#endif
static std::string setKeyPairParams(bool useRsa, unsigned int blen,
                std::string name, std::string comment, std::string email,
                std::string inPassphrase);

#ifdef UNUSED_CODE
static gpgme_key_t getKey(gpgme_ctx_t, std::string, std::string, std::string);
#endif

static gpg_error_t keySignCallback(void *, gpgme_status_code_t, \
		const char *, int);

static gpg_error_t trustCallback(void *, gpgme_status_code_t, \
		const char *, int);

static std::string ProcessPGPmeError(gpgme_error_t ERR);

/* Function to sign X509_REQ via GPGme.
 */

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

#define GPG_DEBUG2
gpg_error_t pgp_pwd_callback(void *hook, const char *uid_hint, const char *passphrase_info, int prev_was_bad, int fd)
{
#ifdef GPG_DEBUG2
	fprintf(stderr, "pgp_pwd_callback() called.\n");
#endif
	std::string text = rsicontrol->getNotify().askForPassword(uid_hint,prev_was_bad);

#ifdef GPG_DEBUG2
	std::cerr << "pgp_pwd_callback() got GPG passwd from gui." << std::endl;
#endif

	if((void*)fd != NULL)
	{
#ifndef WINDOWS_SYS
		write(fd, text.c_str(), text.size());
		write(fd, "\n", 1); /* needs a new line? */
#else
		DWORD written = 0;
		HANDLE winFd = (HANDLE) fd;
		WriteFile(winFd, text.c_str(), text.size(), &written, NULL);
		WriteFile(winFd, "\n", 1, &written, NULL); 
#endif
	}

#ifdef GPG_DEBUG2
	fprintf(stderr, "pgp_pwd_callback() password setted\n");
#endif

	return 0;
}

AuthGPGimpl::AuthGPGimpl()
        :p3Config(CONFIG_TYPE_AUTHGPG),gpgmeInit(false),gpgmeKeySelected(false)
{
        {
            RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/

            CTX = NULL;

            setlocale(LC_ALL, "");
            gpgme_check_version(NULL);
            gpgme_set_locale(NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));

            #ifdef LC_MESSAGES
                    gpgme_set_locale(NULL, LC_MESSAGES, setlocale (LC_MESSAGES, NULL));
            #endif

            #ifndef WINDOWS_SYS
            /* setup the engine (gpg2) */
    //        if (GPG_ERR_NO_ERROR != gpgme_set_engine_info(GPGME_PROTOCOL_OpenPGP, "/usr/bin/gpg2", NULL))
    //        {
    //               std::cerr << "Error creating Setting engine" << std::endl;
    //               return;
    //        }
            #endif

            if (GPG_ERR_NO_ERROR != gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP))
            {
                    std::cerr << "Error check engine version" << std::endl;
                    return;
            }

            if (GPG_ERR_NO_ERROR != gpgme_get_engine_info(&INFO))
            {
                    std::cerr << "Error getting engine info" << std::endl;
                    while (INFO && INFO->protocol != GPGME_PROTOCOL_OpenPGP) {
                        INFO = INFO->next;
                    }
                    if (!INFO) {
                        fprintf (stderr, "GPGME compiled without support for protocol %s",
                            gpgme_get_protocol_name (INFO->protocol));
                    } else if (INFO->file_name && !INFO->version) {
                        fprintf (stderr, "Engine %s not installed properly",
                            INFO->file_name);
                    } else if (INFO->file_name && INFO->version && INFO->req_version) {
                        fprintf (stderr, "Engine %s version %s installed, "
                            "but at least version %s required", INFO->file_name,
                            INFO->version, INFO->req_version);
                    }  else {
                        fprintf (stderr, "Unknown problem with engine for protocol %s",
                            gpgme_get_protocol_name (INFO->protocol));
                    }
                    return;
            }

            /* Create New Contexts */
            if (GPG_ERR_NO_ERROR != gpgme_new(&CTX))
            {
                    std::cerr << "Error creating GPGME Context" << std::endl;
                    return;
            }

            gpgme_set_passphrase_cb(CTX, pgp_pwd_callback, (void *) NULL);
        }
}

/* Initialize */
bool AuthGPGimpl::InitAuth ()
{
    std::string HomeDir;

    if (!CTX) {
        std::cerr << "Error with gpg initialization. Is gpg missing ?" << std::endl;
        return false;
    }

#ifdef WINDOWS_SYS
    if (RsInit::isPortable ()) {
        // set home dir of gpg to configdir\gnupg
        HomeDir = RsInit::RsConfigDirectory() + RsInit::dirSeperator() + "gnupg";

        if (!RsDirUtil::checkCreateDirectory(HomeDir)) {
            std::cerr << "Error creating gnupg directory" << std::endl;
            return false;
        }
    }
#endif

    /* setup protocol and homedir */
    if (GPG_ERR_NO_ERROR != gpgme_ctx_set_engine_info(CTX, GPGME_PROTOCOL_OpenPGP, NULL, HomeDir.empty () ? NULL : HomeDir.c_str ()))
    {
        std::cerr << "Error creating Setting Protocol" << std::endl;
        return false;
    }

    gpgmeInit = true;

    storeAllKeys();
    #ifdef GPG_DEBUG
    printAllKeys_locked();
    #endif
    //updateTrustAllKeys_locked();

    /* start thread */
    start();

    return true;
}

/* This function is called when retroshare is first started
 * to get the list of available GPG certificates.
 * This function should only return certs for which
 * the private(secret) keys are available.
 *
 * returns false if GnuPG is not available.
 */
bool AuthGPGimpl::availableGPGCertificatesWithPrivateKeys(std::list<std::string> &ids)
{
	int i = 0;
	gpgme_key_t KEY = NULL;
        gpg_error_t ERR;

	{
		RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

		if (!gpgmeInit)
		{
			return false;
		}
	} /******* UNLOCKED ******/

	/* XXX should check that CTX is valid */
	RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/

	/* Initiates a key listing */
	if (GPG_ERR_NO_ERROR != gpgme_op_keylist_start (CTX, "", 1))
	{
                std::cerr << "Error iterating through KeyList" << std::endl;
		return false;

	}

	/* Loop until end of key */
	for(i = 0;(GPG_ERR_NO_ERROR == (ERR = gpgme_op_keylist_next (CTX, &KEY))); i++)
	{
                if (KEY->subkeys)
		{	
                        ids.push_back(KEY->subkeys->keyid);
#ifdef GPG_DEBUG
                        std::cerr << "AuthGPGimpl::availablePGPCertificates() Added: " << KEY->subkeys->keyid << std::endl;
#endif
		}
		else
		{
#ifdef GPG_DEBUG
                        std::cerr << "AuthGPGimpl::availablePGPCertificates() Missing subkey" << std::endl;
#endif
		}
	}

	if (GPG_ERR_NO_ERROR != gpgme_op_keylist_end(CTX))
	{
                std::cerr << "Error ending KeyList" << std::endl;
		return false;
	} 

        std::cerr << "AuthGPGimpl::availablePGPCertificates() Secret Key Count: " << i << std::endl;

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
int	AuthGPGimpl::GPGInit(const std::string &ownId)
{

        std::cerr << "AuthGPGimpl::GPGInit() called with own gpg id : " << ownId << std::endl;

        {
            RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

            if (!gpgmeInit) {
                    return 0;
            }

            mOwnGpgId = ownId;
        }

        storeAllKeys();

        int lvl = 0;

        {
            RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
            if (mOwnGpgCert.id != mOwnGpgId) {
                std::cerr << "AuthGPGimpl::GPGInit() failed to find your id." << std::endl;
                return 0;
            }

            gpgmeKeySelected = true;

            lvl = mOwnGpgCert.validLvl;
        }

        //force the validity of the private key. When set to unknown, it caused signature and text encryptions bugs
        privateTrustCertificate(mOwnGpgId, 5);

        //printAllKeys_locked();

        std::cerr << "AuthGPGimpl::GPGInit finished." << std::endl;

        return 1;
}

 AuthGPGimpl::~AuthGPGimpl()
{
}

void AuthGPGimpl::run()
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

        /* every minute */
        if (++count >= 600) {
            storeAllKeys_tick();
            count = 0;
        }
    }
}

void AuthGPGimpl::processServices()
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
    if (loadOrSave) {
        if (loadOrSave->m_load) {
            /* process load operation */

#ifdef GPG_DEBUG
            std::cerr << "AuthGPGimpl::processServices() Process load operation" << std::endl;
#endif

            /* load the certificate */
				std::string error_string ;
            LoadCertificateFromString(loadOrSave->m_certGpg, loadOrSave->m_certGpgId,error_string);
        } else {
            /* process save operation */

#ifdef GPG_DEBUG
            std::cerr << "AuthGPGimpl::processServices() Process save operation" << std::endl;
#endif

            /* save the certificate to string */
            loadOrSave->m_certGpg = SaveCertificateToString(loadOrSave->m_certGpgId);
        }

        service->setGPGOperation(loadOrSave);
    } else {
#ifdef GPG_DEBUG
        std::cerr << "AuthGPGimpl::processServices() Unknown operation" << std::endl;
#endif
    }

    delete operation;
}

bool   AuthGPGimpl::storeAllKeys_tick() {
#ifdef GPG_DEBUG
    std::cerr << "AuthGPGimpl::storeAllKeys_tick() called." << std::endl;
#endif
	time_t timeSinceStore = 0;
    {
    	RsStackMutex stack(gpgMtxData);
	timeSinceStore = time(NULL) - mStoreKeyTime;
    }

    if (timeSinceStore > STORE_KEY_TIMEOUT) 
    {
        storeAllKeys();
    }
    return true ;
}

// store all keys in map mKeyList to avoid callin gpgme exe repeatedly
bool   AuthGPGimpl::storeAllKeys()
{
#ifdef GPG_DEBUG
	std::cerr << "AuthGPGimpl::storeAllKeys()" << std::endl;
#endif

	std::string ownGpgId;

	/* store member variables locally */
	{
		RsStackMutex stack(gpgMtxData);

		if (!gpgmeInit)
		{
			std::cerr << "AuthGPGimpl::storeAllKeys() Error since GPG is not initialised" << std::endl;
			return false;
		}

		mStoreKeyTime = time(NULL);
		ownGpgId = mOwnGpgId;
	}

	/* read keys from gpg to local list */
	std::list<gpgcert> keyList;

	{
#ifdef GPG_DEBUG
		std::cerr << "AuthGPGimpl::storeAllKeys() clearing existing ones" << std::endl;
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
			std::cerr << "AuthGPGimpl::storeAllKeys() Error iterating through KeyList" << std::endl;
			//                if (rsicontrol != NULL) {
			//                    rsicontrol->getNotify().notifyErrorMsg(0,0,"Error reading gpg keyring, cannot acess key list.");
			//                }
			gpgme_set_keylist_mode(CTX, origmode);
			return false;
		}

		/* Loop until end of key */
		ERR = gpgme_op_keylist_next (CTX, &KEY);
		if (GPG_ERR_NO_ERROR != ERR) {
			std::cerr << "AuthGPGimpl::storeAllKeys() didn't find any gpg key in the keyring" << std::endl;
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
				std::cerr << "AuthGPGimpl::storeAllKeys() Invalid Key in List... skipping" << std::endl;
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

// update trust on all available keys. Not used anymore
//bool   AuthGPGimpl::updateTrustAllKeys_locked()
//{
//	gpg_error_t ERR;
//	if (!gpgmeInit)
//	{
//                std::cerr << "Error since GPG is not initialised" << std::endl;
//		return false;
//	}
//
//
//	/* have to do this the hard way! */
//	std::map<std::string, gpgcert>::iterator it;
//
//	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
//	{
//		/* check for trust items associated with key */
//                std::string peerid = it->second.email;
//#ifdef GPG_DEBUG
//                std::cerr << "Searching GPGme for TrustInfo on: " << peerid << std::endl;
//#endif
//
//		/* Initiates a key listing. NB: maxlevel is ignored!*/
//		if (GPG_ERR_NO_ERROR != (ERR = gpgme_op_trustlist_start (CTX, peerid.c_str(), 0)))
//		{
//                        std::cerr << "Error Starting Trust List" << std::endl;
//			ProcessPGPmeError(ERR);
//			continue;
//		}
//
//
//		/* Loop until end of key */
//#ifdef GPG_DEBUG
//	gpgme_trust_item_t ti = NULL;
//
//		for(int i = 0;(GPG_ERR_NO_ERROR == (ERR = gpgme_op_trustlist_next (CTX, &ti))); i++)
//		{
//			std::string keyid = ti->keyid;
//			int type = ti->type;
//			int level = ti->level;
//
//			/* identify the peers, and add trust level */
//			std::cerr << "GPGme Trust Item for: " << keyid;
//			std::cerr << std::endl;
//
//			std::cerr << "\t Type: " << type;
//			std::cerr << " Level: " << level;
//			std::cerr << std::endl;
//
//			std::cerr << "\t Owner Trust: " << ti->owner_trust;
//			std::cerr << " Validity: " << ti->validity;
//			std::cerr << " Name: " << ti->name;
//			std::cerr << std::endl;
//		}
//                std::cerr << "End of TrustList Iteration." << std::endl;
//#endif
//		ProcessPGPmeError(ERR);
//
//		if (GPG_ERR_NO_ERROR != gpgme_op_trustlist_end(CTX))
//		{
//                        std::cerr << "Error ending TrustList" << std::endl;
//
//			ProcessPGPmeError(ERR);
//		}
//	}
//
//	return true;
//
//}

bool   AuthGPGimpl::printAllKeys_locked()
{
	certmap::const_iterator it;
	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
                std::cerr << "PGP Key: " << it->second.id << std::endl;

                std::cerr << "\tName: " << it->second.name << std::endl;
                std::cerr << "\tEmail: " << it->second.email << std::endl;

                std::cerr << "\townsign: " << it->second.ownsign << std::endl;
                std::cerr << "\ttrustLvl: " << it->second.trustLvl << std::endl;
                std::cerr << "\tvalidLvl: " << it->second.validLvl << std::endl;
                std::cerr << "\tEmail: " << it->second.email << std::endl;

		std::list<std::string>::const_iterator sit;
                for(sit = it->second.signers.begin();
                        sit != it->second.signers.end(); sit++)
		{
			std::cerr << "\t\tSigner ID:" << *sit;

			/* do a naughty second search.. should be ok
			 * as we aren't modifying list.
			 */
			certmap::const_iterator kit = mKeyList.find(*sit);
			if (kit != mKeyList.end())
			{
                                std::cerr << " Name:" << kit->second.name << std::endl;
			}
		}
	}
	return true;
}

bool   AuthGPGimpl::printOwnKeys_locked()
{
	certmap::iterator it;
	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
                if (it->second.ownsign)
		{
                        std::cerr << "Own PGP Key: " << it->second.id << std::endl;

                        std::cerr << "\tName: " << it->second.name << std::endl;
                        std::cerr << "\tEmail: " << it->second.email << std::endl;
		}
	}
	return true;
}

bool    AuthGPGimpl::printKeys()
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
	printAllKeys_locked();
	return printOwnKeys_locked();
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


bool AuthGPGimpl::DoOwnSignature(const void *data, unsigned int datalen, void *buf_sigout, unsigned int *outl)
{
	gpgcert ownGpgCert;
	{
		RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

		/* grab a reference, so the key remains */
		gpgme_key_ref(mOwnGpgCert.key);

		ownGpgCert = mOwnGpgCert;
	} /******* UNLOCKED ******/

	RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/
	/* setup signers */
	gpgme_signers_clear(CTX);
	if (GPG_ERR_NO_ERROR != gpgme_signers_add(CTX, ownGpgCert.key))
	{
                std::cerr << "AuthGPGimpl::DoOwnSignature() Error Adding Signer" << std::endl;
	}

	gpgme_data_t gpgmeData;
	gpgme_data_t gpgmeSig;
	if (GPG_ERR_NO_ERROR != gpgme_data_new_from_mem(&gpgmeData, (const char *) data, datalen, 1))
	{
                std::cerr << "Error create Data" << std::endl;
	}

	if (GPG_ERR_NO_ERROR != gpgme_data_new(&gpgmeSig))
	{
                std::cerr << "Error create Sig" << std::endl;
	}

	cleanupZombies(2); // cleanup zombies under OSX. (Called before gpgme operation)

	/* move string data to gpgmeData */
	gpgme_set_armor (CTX, 0);

	gpgme_sig_mode_t mode = GPGME_SIG_MODE_DETACH;
        gpg_error_t ERR;
	if (GPG_ERR_NO_ERROR != (ERR = gpgme_op_sign(CTX,gpgmeData, gpgmeSig, mode)))
	{
		ProcessPGPmeError(ERR);
                std::cerr << "AuthGPGimpl::Sign FAILED ERR: " << ERR << std::endl;
		gpgme_data_release(gpgmeSig);
		gpgme_data_release(gpgmeData);
		gpgme_signers_clear(CTX);
		return false;
	}

	gpgme_sign_result_t res = gpgme_op_sign_result(CTX);

	if (res)
	{
#ifdef GPG_DEBUG
		fprintf(stderr, "Sign Got Result\n");
#endif
	}
	else
	{
		fprintf(stderr, "Sign Failed to get Result\n");
	}

#ifdef GPG_DEBUG
	gpgme_invalid_key_t ik = res->invalid_signers;
	gpgme_new_signature_t sg = res->signatures;

	while(ik != NULL)
	{
                fprintf(stderr, "AuthGPGimpl::Sign, Invalid by: %s\n", ik->fpr);
		ik = ik->next;
	}

	while(sg != NULL)
	{
                fprintf(stderr, "AuthGPGimpl::Signed by: %s\n", sg->fpr);
		sg = sg->next;
	}
#endif

	/* now extract the data from gpgmeSig */
	size_t len = 0; 
//	gpgme_data_write (gpgmeSig, "", 1); 	// to be able to convert it into a string
	char *export_sig = gpgme_data_release_and_get_mem(gpgmeSig, &len);
#ifdef GPG_DEBUG
	std::cerr << "AuthGPGimpl::Signature len: " << len << std::endl;
#endif

	if (len < *outl)	// -1 because we added a 0 at the end.
	{
		std::cerr << "Length is smaller. Reducing to " << len << std::endl;
		*outl = len;
	}
	memmove(buf_sigout, export_sig, *outl);
	gpgme_free(export_sig);
	gpgme_data_release (gpgmeData);
	gpgme_signers_clear(CTX);

	/* extract id(s)! */
	return true;
}


/* import to GnuPG and other Certificates */
bool AuthGPGimpl::VerifySignature(const void *data, int datalen, const void *sig, unsigned int siglen, const std::string &withfingerprint)
{
	gpgme_data_t gpgmeSig;
	gpgme_data_t gpgmeData;

#ifdef GPG_DEBUG
	std::cerr << "VerifySignature: datalen: " << datalen << " siglen: " << siglen << std::endl;
#endif

	if (!active())
	{
		return false ;
	}

	RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/

//	if(siglen==73)
//	{
//                //std::cerr << "Reducing to 72 to overcome an old bug." << std::endl ;
//		siglen=72 ;
//	}

	if (GPG_ERR_NO_ERROR != gpgme_data_new_from_mem(&gpgmeData, (const char *) data, datalen, 1))
	{
                std::cerr << "Error create Data" << std::endl;
	}

	if (siglen != 0 && GPG_ERR_NO_ERROR != gpgme_data_new_from_mem(&gpgmeSig, (const char *) sig, siglen, 1))
	{
                std::cerr << "Error create Sig" << std::endl;
	}

	cleanupZombies(2); // cleanup zombies under OSX. (Called before gpgme operation)

	gpgme_set_armor (CTX, 0);

	gpgme_error_t ERR;
        if (GPG_ERR_NO_ERROR != (ERR = gpgme_op_verify(CTX,gpgmeSig, gpgmeData, NULL)))
        {
                ProcessPGPmeError(ERR);
                std::cerr << "AuthGPGimpl::Verify FAILED" << std::endl;
        }

	gpgme_verify_result_t res = gpgme_op_verify_result(CTX);

	if (res)
	{
#ifdef GPG_DEBUG
		fprintf(stderr, "VerifySignature Got Result\n");
#endif
	}
	else
	{
#ifdef GPG_DEBUG
		fprintf(stderr, "VerifySignature Failed to get Result\n");
#endif
		gpgme_data_release(gpgmeData);
		gpgme_data_release(gpgmeSig);
		return false ;
	}

        gpgme_signature_t sg = res->signatures;
	bool valid = false;

	while(sg != NULL)
	{
#ifdef GPG_DEBUG
        fprintf(stderr, "AuthGPGimpl::Verify Sig by: %s, status: %d\n", sg->fpr, sg->status);
		print_pgpme_verify_summary(sg->summary);
#endif

		if (sg->status == GPG_ERR_NO_ERROR)
		{
#ifdef GPG_DEBUG
			fprintf(stderr, "AuthGPGimpl::VerifySignature() OK\n");
#endif
			if (withfingerprint.empty() == false && withfingerprint == sg->fpr) {
#ifdef GPG_DEBUG
				fprintf(stderr, "AuthGPGimpl::VerifySignature() for the fingerprint key : ");
				std::cerr << withfingerprint;
				fprintf(stderr, "\n");
#endif
				valid = true;
				break;
			}
		}

		sg = sg->next;
	}

	gpgme_data_release (gpgmeData);
	gpgme_data_release (gpgmeSig);

	/* extract id(s)! */
	if (!valid)
	{
                fprintf(stderr, "AuthGPGimpl::VerifySignature() FAILED\n");
	}

	return valid;
}



	
bool   AuthGPGimpl::active()
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

        return ((gpgmeInit) && (gpgmeKeySelected));
}

bool    AuthGPGimpl::GeneratePGPCertificate(std::string name, std::string email, std::string passwd, std::string &pgpId, std::string &errString) {

        RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/

        gpgme_key_t newKey;
        gpgme_genkey_result_t result;
        gpg_error_t ERR;

	cleanupZombies(2); // cleanup zombies under OSX. (Called before gpgme operation)

        if(GPG_ERR_NO_ERROR != (ERR = gpgme_op_genkey(CTX, setKeyPairParams(true, 2048, name, "generated by Retroshare", email, \
                passwd).c_str(), NULL, NULL))) {
                ProcessPGPmeError(ERR);
                std::cerr << "Error generating the key" << std::endl;
                return 0;
        }

        if((result = gpgme_op_genkey_result(CTX)) == NULL)
                return 0;


        if(GPG_ERR_NO_ERROR != (ERR = gpgme_get_key(CTX, result->fpr, &newKey, 1))) {
                std::cerr << "Error reading own key" << std::endl;
                return 0;
        }
        pgpId = newKey->subkeys->keyid;

        return true;
}

bool    AuthGPGimpl::CloseAuth()
{
	return true;
}

/**** These Two are common */
std::string AuthGPGimpl::getGPGName(const std::string &id)
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

	certmap::iterator it;
	if (mKeyList.end() != (it = mKeyList.find(id)))
                return it->second.name;

	return std::string();
}

/**** These Two are common */
std::string AuthGPGimpl::getGPGEmail(const std::string &id)
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

        certmap::iterator it;
        if (mKeyList.end() != (it = mKeyList.find(id)))
                return it->second.email;

        return std::string();
}

/**** GPG versions ***/

std::string AuthGPGimpl::getGPGOwnId()
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
        return mOwnGpgId;
}

std::string AuthGPGimpl::getGPGOwnName()
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
        return mOwnGpgCert.name;
}

bool	AuthGPGimpl::getGPGAllList(std::list<std::string> &ids)
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

	/* add an id for each pgp certificate */
	certmap::iterator it;
	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
		ids.push_back(it->first);
	}
	return true;
}

bool	AuthGPGimpl::getGPGDetails(const std::string &id, RsPeerDetails &d)
{
#ifdef GPG_DEBUG
	std::cerr << "AuthGPGimpl::getPGPDetails() called for : " << id << std::endl;
#endif

        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

	/* add an id for each pgp certificate */
	certmap::iterator it;
	if (mKeyList.end() != (it = mKeyList.find(id))) 
	{
		gpgcert &cert = it->second;

		d.id = cert.id; //keep, it but can be bug gen
		d.gpg_id = cert.id;
		d.name = cert.name;
		d.email = cert.email;
		d.trustLvl = cert.trustLvl;
		d.validLvl = cert.validLvl;
		d.ownsign = cert.ownsign;
		d.gpgSigners = cert.signers;
		d.fpr = cert.fpr;

		d.accept_connection = cert.accept_connection;

		//did the peer signed me ?
		d.hasSignedMe = false;
		std::list<std::string>::iterator signersIt;
		for(signersIt = mOwnGpgCert.signers.begin(); signersIt != mOwnGpgCert.signers.end() ; ++signersIt) 
			if (*signersIt == d.id) 
			{
				d.hasSignedMe = true;
				break;
			}

#ifdef GPG_DEBUG
		std::cerr << "AuthGPGimpl::getPGPDetails() Name : " << cert.name << std::endl;
#endif
		return true;
	} 
	return false;
}

bool 	AuthGPGimpl::decryptText(gpgme_data_t CIPHER, gpgme_data_t PLAIN) {
        RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/
        gpgme_set_armor (CTX, 1);
	gpg_error_t ERR;

	cleanupZombies(2); // cleanup zombies under OSX. (Called before gpgme operation)

	if (GPG_ERR_NO_ERROR != (ERR = gpgme_op_decrypt (CTX, CIPHER, PLAIN)))
	{
		ProcessPGPmeError(ERR);
                std::cerr << "AuthGPGimpl::decryptText() Error decrypting text." << std::endl;
		return false;
	}

	return true;
}

bool 	AuthGPGimpl::encryptText(gpgme_data_t PLAIN, gpgme_data_t CIPHER) {
        RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/
	gpgme_encrypt_flags_t* flags = new gpgme_encrypt_flags_t();
	gpgme_key_t keys[2] = {mOwnGpgCert.key, NULL};
        gpgme_set_armor (CTX, 1);
	gpg_error_t ERR;

	cleanupZombies(2); // cleanup zombies under OSX. (Called before gpgme operation)


	if (GPG_ERR_NO_ERROR != (ERR = gpgme_op_encrypt(CTX, keys, *flags, PLAIN, CIPHER)))
	{
		ProcessPGPmeError(ERR);
                std::cerr << "AuthGPGimpl::encryptText() Error encrypting text." << std::endl;
		return false;
	}

	return true;
}

bool	AuthGPGimpl::getGPGValidList(std::list<std::string> &ids)
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
        /* add an id for each pgp certificate */
        certmap::iterator it;
        for(it = mKeyList.begin(); it != mKeyList.end(); it++)
        {
            if (it->second.validLvl >= GPGME_VALIDITY_MARGINAL) {
                ids.push_back(it->first);
            }
        }
        return true;
}

bool	AuthGPGimpl::getGPGAcceptedList(std::list<std::string> &ids)
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
        certmap::iterator it;
        for(it = mKeyList.begin(); it != mKeyList.end(); it++)
        {
                if (it->second.accept_connection)
                {
                        ids.push_back(it->first);
                }
        }
        return true;
}

bool	AuthGPGimpl::getGPGSignedList(std::list<std::string> &ids)
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
        certmap::iterator it;
	for(it = mKeyList.begin(); it != mKeyList.end(); it++)
	{
                if (it->second.ownsign)
		{
			ids.push_back(it->first);
		}
	}
	return true;
}

bool	AuthGPGimpl::isGPGValid(const std::string &id)
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
        certmap::iterator it;
        if (mKeyList.end() != (it = mKeyList.find(id))) {
            return (it->second.validLvl >= GPGME_VALIDITY_MARGINAL);
        } else {
            return false;
        }

}

bool	AuthGPGimpl::isGPGId(const std::string &id)
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
        certmap::iterator it;
        if (mKeyList.end() != (it = mKeyList.find(id))) {
            return true;
        } else {
            return false;
        }
}


bool	AuthGPGimpl::isGPGSigned(const std::string &id)
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
        certmap::iterator it;
	if (mKeyList.end() != (it = mKeyList.find(id)))
	{
                return (it->second.ownsign);
	}
	return false;
}

bool	AuthGPGimpl::isGPGAccepted(const std::string &id)
{
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
        certmap::iterator it;
        if (mKeyList.end() != (it = mKeyList.find(id)))
        {
                return (it->second.accept_connection);
        }
        return false;
}

/*****************************************************************
 * Loading and Saving Certificates - this has to 
 * be able to handle both openpgp and X509 certificates.
 * 
 * X509 are passed onto AuthSSL, OpenPGP are passed to gpgme.
 *
 */


/* SKTAN : do not know how to use std::string id */
std::string AuthGPGimpl::SaveCertificateToString(const std::string &id)
{

        if (!isGPGId(id)) {
                std::cerr << "AuthGPGimpl::SaveCertificateToString() unknown ID" << std::endl;
                return "";
	}

        RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/

	std::string tmp;
	const char *pattern[] = { NULL, NULL };
	pattern[0] = id.c_str();
	gpgme_data_t gpgmeData;

	if (GPG_ERR_NO_ERROR != gpgme_data_new (&gpgmeData))
	{
                std::cerr << "Error create Data" << std::endl;
	}
	gpgme_set_armor (CTX, 1);

	if (GPG_ERR_NO_ERROR != gpgme_op_export_ext (CTX, pattern, 0, gpgmeData))
	{
                std::cerr << "Error export Data" << std::endl;
	}

	fflush (NULL);
        //showData (gpgmeData);

	size_t len = 0; 
	char *export_txt = gpgme_data_release_and_get_mem(gpgmeData, &len);

	char *str = new char[len+1] ;
	memcpy(str,export_txt,len) ;
	str[len]=0 ;

	tmp = std::string(str);

	delete[] str ;

#ifdef GPG_DEBUG
        std::cerr << "Exported Certificate: " << std::endl << tmp << std::endl;
#endif

	gpgme_free(export_txt);

	return tmp;
}

/* import to GnuPG and other Certificates */
bool AuthGPGimpl::LoadCertificateFromString(const std::string &str, std::string &gpg_id,std::string& error_string)
{
        if (str == "") {
            error_string = "Certificate is an empty string." ;
            return false;
        }
        int imported = 0;
        std::string fingerprint;

        {
            RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/
            //std::string cleancert = cleanUpCertificate(str); disable for p3disc message on windows system. Move the clean cert in p3peers
            std::string cleancert = str;

            #ifdef GPG_DEBUG
            std::cerr << "AuthGPGimpl::LoadCertificateFromString() cleancert : " << cleancert;
            #endif

            gpgme_data_t gpgmeData;
				gpg_error_t ERR ;

            if (GPG_ERR_NO_ERROR != (ERR = gpgme_data_new_from_mem(&gpgmeData, cleancert.c_str(), cleancert.length(), 1)))
            {
					error_string = ProcessPGPmeError(ERR) ;
                    std::cerr << "Error create Data" << std::endl;
                    return false;
            }

            /* move string data to gpgmeData */

            gpgme_set_armor (CTX, 1);
            if (GPG_ERR_NO_ERROR != (ERR = gpgme_op_import (CTX,gpgmeData)))
            {
                    std::cerr << "AuthGPGimpl::LoadCertificateFromString() Error Importing Certificate" << std::endl;
					error_string = ProcessPGPmeError(ERR) ;
                    gpgme_data_release (gpgmeData);
                    return false ;
            }


            gpgme_import_result_t res = gpgme_op_import_result(CTX);

            if(res == NULL || res->imports == NULL) {
                    gpgme_data_release (gpgmeData);
						  error_string = "Certificate is corrupted." ;
                    return false ;
            }

            fingerprint = std::string(res->imports->fpr);
    #ifdef GPG_DEBUG
            std::cerr << "AuthGPGimpl::LoadCertificateFromString() Importing considered folowing fpr : " << fingerprint << std::endl;
    #endif
            imported = res->imported || res->new_signatures;

    #ifdef GPG_DEBUG
            fprintf(stderr, "ImportCertificate(Considered: %d Imported: %d)\n",
                                            res->considered, res->imported);
    #endif

            /* do we need to delete res??? */
            gpgme_data_release (gpgmeData);
        }

	/* extract id(s)! (only if we actually imported one) */
        if (imported) {
		storeAllKeys();
	}
        //retrieve the id of the key
        certmap::iterator it;
        gpg_id = "" ;
        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
        for(it = mKeyList.begin(); it != mKeyList.end(); it++)
        {
                if (it->second.fpr == fingerprint)
                {
                        gpg_id = it->second.id;
                        break;
                }
        }
#ifdef GPG_DEBUG
        std::cerr << "AuthGPGimpl::LoadCertificateFromString() returning with gpg_id : " << gpg_id << std::endl;
#endif
        if (gpg_id == "") {
            return false;
        } else {
            return true;
        }
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
bool AuthGPGimpl::setAcceptToConnectGPGCertificate(const std::string &gpg_id, bool acceptance)
{

#ifdef GPG_DEBUG
        std::cerr << "AuthGPGimpl::markGPGCertificateAsFriends(" << gpg_id << ")" << std::endl;
#endif

        /* reload stuff now ... */
        storeAllKeys();
		  {
			  RsStackMutex stack(gpgMtxData);
			  certmap::iterator it;
			  if (mKeyList.end() == (it = mKeyList.find(gpg_id))) {
				  return false;
			  }
			  it->second.accept_connection = acceptance;
			  mAcceptToConnectMap[gpg_id] = acceptance;
		  }
        storeAllKeys();

        IndicateConfigChanged();

        return true;
}

/* These take PGP Ids */
bool AuthGPGimpl::SignCertificateLevel0(const std::string &id)
{

#ifdef GPG_DEBUG
        std::cerr << "AuthGPGimpl::SignCertificat(" << id << ")" << std::endl;
#endif

        if (1 != privateSignCertificate(id))
	{
                storeAllKeys();
		return false;
	}

        /* reload stuff now ... */
	storeAllKeys();
	return true;
}

bool AuthGPGimpl::RevokeCertificate(const std::string &id)
{
        //RsStackMutex stack(gpgMtx); /******* LOCKED ******/

#ifdef GPG_DEBUG
        std::cerr << "AuthGPGimpl::RevokeCertificate(" << id << ") not implemented yet" << std::endl;
#endif

	return false;
}

bool AuthGPGimpl::TrustCertificate(const std::string &id, int trustlvl)
{
#ifdef GPG_DEBUG
        std::cerr << "AuthGPGimpl::TrustCertificate(" << id << ", " << trustlvl << ")" << std::endl;
#endif
        if (1 != privateTrustCertificate(id, trustlvl))
        {
                storeAllKeys();
                return false;
        }

	/* Keys are reloaded by privateTrustCertificate */
        return true;
}

#if 0 
/* remove otherwise will cause bugs */
bool AuthGPGimpl::SignData(std::string input, std::string &sign)
{
	return false;
}

bool AuthGPGimpl::SignData(const void *data, const uint32_t len, std::string &sign)
{
	return false;
}


bool AuthGPGimpl::SignDataBin(std::string input, unsigned char *sign, unsigned int *signlen)
{
	return false;
}
#endif

bool AuthGPGimpl::SignDataBin(const void *data, unsigned int datalen, unsigned char *sign, unsigned int *signlen) {
        return DoOwnSignature(data, datalen,
                        sign, signlen);
}

bool AuthGPGimpl::VerifySignBin(const void *data, uint32_t datalen, unsigned char *sign, unsigned int signlen, const std::string &withfingerprint) {
        return VerifySignature(data, datalen,
                        sign, signlen, withfingerprint);
}


	/* Sign/Trust stuff */

int	AuthGPGimpl::privateSignCertificate(const std::string &id)
{
	/* The key should be in Others list and not in Peers list ?? 
	 * Once the key is signed, it moves from Others to Peers list ??? 
	 */

	gpgcert signKey;
	gpgcert ownKey;

	{
		RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
		certmap::iterator it;

		if (mKeyList.end() == (it = mKeyList.find(id)))
		{
			return false;
		}

		/* grab a reference, so the key remains */
		gpgme_key_ref(it->second.key);

		signKey = it->second;

		/* grab a reference, so the key remains */
		gpgme_key_ref(mOwnGpgCert.key);

		ownKey  = mOwnGpgCert;
	} /******* UNLOCKED ******/

	RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/

	class SignParams sparams("0");
	class EditParams params(SIGN_START, &sparams);
	gpgme_data_t out;
	gpg_error_t ERR;

	if(GPG_ERR_NO_ERROR != (ERR = gpgme_data_new(&out))) {
		return 0;
	}

	gpgme_signers_clear(CTX);
	if(GPG_ERR_NO_ERROR != (ERR = gpgme_signers_add(CTX, ownKey.key))) {
		gpgme_data_release(out);
		return 0;
	}
	
	if(GPG_ERR_NO_ERROR != (ERR = gpgme_op_edit(CTX, signKey.key, keySignCallback, &params, out))) {
		gpgme_data_release(out);
		gpgme_signers_clear(CTX);
		return 0;	
	}

	gpgme_data_release(out);
	gpgme_signers_clear(CTX);

	return 1;
}

/* revoke the signature on Certificate */
int	AuthGPGimpl::privateRevokeCertificate(const std::string &id)
{
        //RsStackMutex stack(gpgMtx); /******* LOCKED ******/

	return 0;
}

int	AuthGPGimpl::privateTrustCertificate(const std::string &id, int trustlvl)
{
	/* The certificate should be in Peers list ??? */	
	if(!isGPGAccepted(id)) {
		std::cerr << "Invalid Certificate" << std::endl;
		return 0;
	}
	
	{
		gpgcert trustCert;
		{
			RsStackMutex stack(gpgMtxData);

			trustCert = mKeyList.find(id)->second;
		} /******* UNLOCKED ******/

		RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/

		gpgme_key_t trustKey = trustCert.key;
		std::string trustString;
		std::ostringstream trustStrOut;
		trustStrOut << trustlvl;
		class TrustParams sparams(trustStrOut.str());
		class EditParams params(TRUST_START, &sparams);
		gpgme_data_t out;
		gpg_error_t ERR;

		if(GPG_ERR_NO_ERROR != (ERR = gpgme_data_new(&out))) {
			return 0;
		}

		if(GPG_ERR_NO_ERROR != (ERR = gpgme_op_edit(CTX, trustKey, trustCallback, &params, out))) {
			gpgme_data_release(out);
			return 0;
		}

		gpgme_data_release(out);

		//the key ref has changed, we got to get rid of the old reference.
		trustCert.key = NULL;
	}

	storeAllKeys();
		
	return 1;
}


/* This function to print Data */
#if 0
void showData(gpgme_data_t dh)
{
	#define BUF_SIZE 512
	char buf[BUF_SIZE + 1];
	int ret;
  
	ret = gpgme_data_seek (dh, 0, SEEK_SET);
	if (ret)
	{
                std::cerr << "Fail data seek" << std::endl;
	//	fail_if_err (gpgme_err_code_from_errno (errno));
	}

	while ((ret = gpgme_data_read (dh, buf, BUF_SIZE)) > 0)
    		fwrite (buf, ret, 1, stdout);

	if (ret < 0)
	{
                std::cerr << "Fail data seek" << std::endl;
		//fail_if_err (gpgme_err_code_from_errno (errno));
	}
}
#endif

#if 0
static std::string setKeyPairParams(bool useRsa, unsigned int blen,
                std::string name, std::string comment, std::string email)
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
	params << "</GnupgKeyParms>"<< std::endl;

	return params.str();
}
#endif

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
		ProcessPGPmeError(params->err);

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
	ProcessPGPmeError(params->err);

	return params->err;
}



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

// -----------------------------------------------------------------------------------//
// --------------------------------  Config functions  ------------------------------ //
// -----------------------------------------------------------------------------------//
//
RsSerialiser *AuthGPGimpl::setupSerialiser()
{
        RsSerialiser *rss = new RsSerialiser ;
        rss->addSerialType(new RsGeneralConfigSerialiser());
        return rss ;
}

bool AuthGPGimpl::saveList(bool& cleanup, std::list<RsItem*>& lst)
{
        #ifdef GPG_DEBUG
        std::cerr << "AuthGPGimpl::saveList() called" << std::endl ;
        #endif

        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

        cleanup = true ;

        // Now save config for network digging strategies
        RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
        std::map<std::string, bool>::iterator mapIt;
        for (mapIt = mAcceptToConnectMap.begin(); mapIt != mAcceptToConnectMap.end(); mapIt++) {
            if (mapIt->first == mOwnGpgId) {
                continue;
            }
            RsTlvKeyValue kv;
            kv.key = mapIt->first;
            #ifdef GPG_DEBUG
            std::cerr << "AuthGPGimpl::saveList() called (mapIt->second) : " << (mapIt->second) << std::endl ;
            #endif
            kv.value = (mapIt->second)?"TRUE":"FALSE" ;
            vitem->tlvkvs.pairs.push_back(kv) ;
        }
        lst.push_back(vitem);

        return true ;
}

bool AuthGPGimpl::loadList(std::list<RsItem*>& load)
{
        #ifdef GPG_DEBUG
        std::cerr << "AuthGPGimpl::loadList() Item Count: " << load.size() << std::endl;
        #endif

        storeAllKeys();

        RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
        /* load the list of accepted gpg keys */
        std::list<RsItem *>::iterator it;
        for(it = load.begin(); it != load.end(); it++) {
                RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet *>(*it);

                if(vitem) {
                        #ifdef GPG_DEBUG
                        std::cerr << "AuthGPGimpl::loadList() General Variable Config Item:" << std::endl;
                        vitem->print(std::cerr, 10);
                        std::cerr << std::endl;
                        #endif

                        std::list<RsTlvKeyValue>::iterator kit;
                        for(kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); kit++) {
                            if (kit->key == mOwnGpgId) {
                                continue;
                            }
                            mAcceptToConnectMap[kit->key] = (kit->value == "TRUE");
                            //set the gpg key
                            certmap::iterator it;
                            if (mKeyList.end() != (it = mKeyList.find(kit->key))) {
                                    #ifdef GPG_DEBUG
                                    std::cerr << "AuthGPGimpl::loadList() setting accept to : " << (kit->value == "TRUE");
                                    std::cerr << " for gpg key id : " << kit->key << std::endl;
                                    #endif
                                    it->second.accept_connection = (kit->value == "TRUE");
                            }
                        }
                }
                delete (*it);
        }
        return true;
}

bool AuthGPGimpl::addService(AuthGPGService *service)
{
    RsStackMutex stack(gpgMtxService); /********* LOCKED *********/

    if (std::find(services.begin(), services.end(), service) != services.end()) {
        /* it exists already! */
        return false;
    }

    services.push_back(service);
    return true;
}


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

	std::cerr << "cleanupZombies() Killed " << i << " zombies";
	std::cerr << std::endl;

#endif

	return;
}
	

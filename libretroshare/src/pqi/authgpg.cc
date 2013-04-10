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
#include <pgp/pgpkeyutil.h>
#include <unistd.h>		/* for (u)sleep() */
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include "serialiser/rsconfigitems.h"

#define LIMIT_CERTIFICATE_SIZE		1
#define MAX_CERTIFICATE_SIZE		10000

const time_t STORE_KEY_TIMEOUT = 1 * 60 * 60; //store key is call around every hour

AuthGPG *AuthGPG::_instance = NULL ;

void cleanupZombies(int numkill); // function to cleanup zombies under OSX.

//#define GPG_DEBUG 1

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
			gpgMtxService("AuthGPG-service"),
		   gpgMtxEngine("AuthGPG-engine"), 
			gpgMtxData("AuthGPG-data"),
			gpgKeySelected(false) 
{
	_force_sync_database = false ;
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
	updateOwnSignatureFlag(mOwnGpgId) ;

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

        /* every ten seconds */
        if (++count >= 100 || _force_sync_database) 
		  {
			  RsStackMutex stack(gpgMtxService); /******* LOCKED ******/
			  
			  // The call does multiple things at once:
			  // 	- checks whether the keyring has changed in memory
			  // 	- checks whether the keyring has changed on disk.
			  // 	- merges/updates according to status.
			  //
			  PGPHandler::syncDatabase() ;
			  count = 0;
			  _force_sync_database = false ;
        }
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
				 PGPIdType pgp_id ;
				 LoadCertificateFromString(loadOrSave->m_certGpg, pgp_id,error_string);
				 loadOrSave->m_certGpgId = pgp_id.toStdString() ;
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

bool AuthGPG::DoOwnSignature(const void *data, unsigned int datalen, void *buf_sigout, unsigned int *outl)
{
	return PGPHandler::SignDataBin(mOwnGpgId,data,datalen,(unsigned char *)buf_sigout,outl) ;
}


/* import to GnuPG and other Certificates */
bool AuthGPG::VerifySignature(const void *data, int datalen, const void *sig, unsigned int siglen, const std::string &withfingerprint)
{
	if(withfingerprint.length() != 40)
	{
		static bool already_reported = false ;

		if( !already_reported)
		{
			std::cerr << "AuthGPG::VerifySignature(): no (or dammaged) fingerprint. Nor verifying signature. This is likely to be an unknown peer. fingerprint=\"" << withfingerprint << "\". Not reporting other errors." << std::endl;
			already_reported = true ;
		}
		return false ;
	}

	return PGPHandler::VerifySignBin((unsigned char*)data,datalen,(unsigned char*)sig,siglen,PGPFingerprintType(withfingerprint)) ;
}

bool AuthGPG::exportProfile(const std::string& fname,const std::string& exported_id) 
{
	return PGPHandler::exportGPGKeyPair(fname,PGPIdType(exported_id)) ;
}

bool AuthGPG::importProfile(const std::string& fname,std::string& imported_id,std::string& import_error)
{
	PGPIdType id ;

	if(PGPHandler::importGPGKeyPair(fname,id,import_error))
	{
		imported_id = id.toStdString() ;
		return true ;
	}
	else
		return false ;
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
		static int already = 0 ;
		if(already < 10)
		{
			std::cerr << "Wrong string passed to getGPGDetails: \"" << id << "\"" << std::endl;
			already++ ;
		}
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
bool AuthGPG::haveSecretKey(const std::string& id) const
{
	return PGPHandler::haveSecretKey(PGPIdType(id)) ;
}
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
		static int already = 0 ;

		if(already < 10)
		{
			std::cerr << "Wrong string passed to getGPGDetails: \"" << id << "\"" << std::endl;
			++already ;
		}
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
	d.validLvl = cert._trustLvl;
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
#ifdef LIMIT_CERTIFICATE_SIZE
	certificate = PGPHandler::SaveCertificateToString(PGPIdType(id),false) ;
#else
	certificate = PGPHandler::SaveCertificateToString(PGPIdType(id),true) ;
#endif

// #ifdef LIMIT_CERTIFICATE_SIZE
// 	std::string cleaned_key ;
// 	if(PGPKeyManagement::createMinimalKey(certificate,cleaned_key))
// 		certificate = cleaned_key ;
// #endif

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

//	// Try to remove signatures manually.
//	//
//	std::string cleaned_key ;
//
//	if( (!include_signatures) && PGPKeyManagement::createMinimalKey(tmp,cleaned_key))
//		return cleaned_key ;
//	else

	return tmp;
}

/* import to GnuPG and other Certificates */
bool AuthGPG::LoadCertificateFromString(const std::string &str, PGPIdType& gpg_id,std::string& error_string)
{
	RsStackMutex stack(gpgMtxEngine); /******* LOCKED ******/

	return PGPHandler::LoadCertificateFromString(str,gpg_id,error_string) ;
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
#ifdef GPG_DEBUG
	std::cerr << "AuthGPG::SignCertificat(" << id << ")" << std::endl;
#endif

	return privateSignCertificate(id) ;
}

bool AuthGPG::RevokeCertificate(const std::string &id)
{
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
	return privateTrustCertificate(id, trustlvl) ;
}

bool AuthGPG::encryptDataBin(const std::string& pgp_id,const void *data, unsigned int datalen, unsigned char *sign, unsigned int *signlen) 
{
	return PGPHandler::encryptDataBin(PGPIdType(pgp_id),data,datalen,sign,signlen) ;
}

bool AuthGPG::decryptDataBin(const void *data, unsigned int datalen, unsigned char *sign, unsigned int *signlen) 
{
	return PGPHandler::decryptDataBin(mOwnGpgId,data,datalen,sign,signlen) ;
}
bool AuthGPG::SignDataBin(const void *data, unsigned int datalen, unsigned char *sign, unsigned int *signlen) 
{
	return DoOwnSignature(data, datalen, sign, signlen);
}

bool AuthGPG::VerifySignBin(const void *data, uint32_t datalen, unsigned char *sign, unsigned int signlen, const std::string &withfingerprint) 
{
	return VerifySignature(data, datalen, sign, signlen, withfingerprint);
}

/* Sign/Trust stuff */

int	AuthGPG::privateSignCertificate(const std::string &id)
{
	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

	int ret = PGPHandler::privateSignCertificate(mOwnGpgId,PGPIdType(id)) ;
	_force_sync_database = true ;
	return ret ;
}

/* revoke the signature on Certificate */
int	AuthGPG::privateRevokeCertificate(const std::string &/*id*/)
{
	//RsStackMutex stack(gpgMtx); /******* LOCKED ******/

	return 0;
}

int	AuthGPG::privateTrustCertificate(const std::string &id, int trustlvl)
{
	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

	/* The certificate should be in Peers list ??? */	
	if(!isGPGAccepted(id)) 
	{
		std::cerr << "Invalid Certificate" << std::endl;
		return 0;
	}

	int res = PGPHandler::privateTrustCertificate(PGPIdType(id),trustlvl) ;
	_force_sync_database = true ;
	return res ;
}

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



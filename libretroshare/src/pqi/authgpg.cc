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
#include "rsserver/p3face.h"
#include "pqi/p3notify.h"
#include "pgp/pgphandler.h"
#include "util/rssleep.h"
#include "util/rsdir.h"
#include "serialiser/rsconfigitems.h"


#include <pgp/pgpkeyutil.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>

#define LIMIT_CERTIFICATE_SIZE		1
#define MAX_CERTIFICATE_SIZE		10000

const time_t STORE_KEY_TIMEOUT = 1 * 60 * 60; //store key is call around every hour

void cleanupZombies(int numkill); // function to cleanup zombies under OSX.

//#define GPG_DEBUG 1

/* Function to sign X509_REQ via GPGme.  */

bool AuthGPG::decryptTextFromFile(std::string& text, const std::string& inputfile)
{ return PGPHandler::decryptTextFromFile(mOwnGpgId, text, inputfile); }

bool AuthGPG::encryptTextToFile(const std::string& text, const std::string& outfile)
{ return PGPHandler::encryptTextToFile(mOwnGpgId, text, outfile); }

std::string pgp_pwd_callback( void * /*hook*/, const char *uid_title,
                              const char *uid_hint,
                              const char * /*passphrase_info*/,
                              int prev_was_bad, bool *cancelled )
{
	std::cerr << "pgp_pwd_callback() called." << std::endl;

	std::string password;
	RsServer::notify()->askForPassword(uid_title, uid_hint, prev_was_bad, password,cancelled);
	return password;
}

AuthGPG &AuthGPG::instance() // static method
{
	PGPHandler::setPassphraseCallback(pgp_pwd_callback);

	std::string pgp_dir = RsAccountsDetail::instance().PathPGPDirectory();
	static AuthGPG inst( pgp_dir + "/retroshare_public_keyring.gpg",
	                     pgp_dir + "/retroshare_secret_keyring.gpg",
	                     pgp_dir + "/retroshare_trustdb.gpg",
	                     pgp_dir + "/lock" );

	return inst;
}

AuthGPG::AuthGPG( const std::string& path_to_public_keyring,
                  const std::string& path_to_secret_keyring,
                  const std::string& path_to_trustdb,
                  const std::string& pgp_lock_file ) :
    p3Config(),
    PGPHandler( path_to_public_keyring, path_to_secret_keyring,
                path_to_trustdb, pgp_lock_file),
    gpgMtxService("AuthGPG-service"), gpgMtxData("AuthGPG-data"),
    _force_sync_database(false), mCount(0)
{ start("AuthGPG"); }


/* You can initialise Retroshare with
 * (a) load existing certificate.
 * (b) a new certificate.
 *
 * This function must be called successfully (return == 1)
 * before anything else can be done. (except above fn).
 */
int AuthGPG::GPGInit(const RsPgpId &ownId)
{
	std::cerr << "AuthGPG::GPGInit() called with own gpg id : " << ownId.toStdString() << std::endl;

	mOwnGpgId = RsPgpId(ownId);

	//force the validity of the private key. When set to unknown, it caused signature and text encryptions bugs
	TrustCertificate(ownId, 5);
	updateOwnSignatureFlag(mOwnGpgId) ;

	std::cerr << "AuthGPG::GPGInit finished." << std::endl;

	return 1;
}

void AuthGPG::data_tick()
{
	rs_usleep(100 * 1000);

	processServices();

	if (++mCount >= 100 || _force_sync_database) // every ten seconds
	{
		RS_STACK_MUTEX(gpgMtxService);

		/* The call does multiple things at once:
		 * - checks whether the keyring has changed in memory.
		 * - checks whether the keyring has changed on disk.
		 * - merges/updates according to status.
		 */
		PGPHandler::syncDatabase();
		mCount = 0;
		_force_sync_database = false;
	}
}

void AuthGPG::processServices()
{
    AuthGPGOperation *operation = NULL;
    AuthGPGService *service = NULL;

	{
		RS_STACK_MUTEX(gpgMtxService);

        std::list<AuthGPGService*>::iterator serviceIt;
        for (serviceIt = services.begin(); serviceIt != services.end(); ++serviceIt) {
            operation = (*serviceIt)->getGPGOperation();
            if (operation) {
                service = *serviceIt;
                break;
            }
        }
	} // RS_STACK_MUTEX(gpgMtxService) UNLOCKED

	if (operation == NULL) return; // nothing to do

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
				 RsPgpId pgp_id ;
				 LoadCertificateFromString(loadOrSave->m_certGpg, pgp_id,error_string);
				 loadOrSave->m_certGpgId = pgp_id;
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

/**** These Two are common */
std::string AuthGPG::getGPGName(const RsPgpId& id,bool *success)
{
	RS_STACK_MUTEX(gpgMtxData);

	const PGPCertificateInfo *info = getCertificateInfo(id) ;

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
std::string AuthGPG::getGPGEmail(const RsPgpId& id,bool *success)
{
	RS_STACK_MUTEX(gpgMtxData); /******* LOCKED ******/
	const PGPCertificateInfo *info = getCertificateInfo(id) ;

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

const RsPgpId& AuthGPG::getGPGOwnId()
{
	RS_STACK_MUTEX(gpgMtxData);
	return mOwnGpgId;
}

std::string AuthGPG::getGPGOwnName()
{
	return getGPGName(mOwnGpgId);
}

bool AuthGPG::getGPGAllList(std::list<RsPgpId> &ids)
{
	PGPHandler::getGPGFilteredList(ids);
	return true;
}

const PGPCertificateInfo *AuthGPG::getCertInfoFromStdString(const std::string& pgp_id) const
{
	try
	{
		return PGPHandler::getCertificateInfo(RsPgpId(pgp_id)) ;
	}
	catch(std::exception& e)
	{
		std::cerr << "(EE) exception raised while constructing a PGP certificate from id \"" << pgp_id << "\": " << e.what() << std::endl;
		return NULL ;
	}
}

bool AuthGPG::isKeySupported(const RsPgpId& id) const
{
	const PGPCertificateInfo *pc = getCertificateInfo(id) ;

	if(pc == NULL)
		return false ;

	return !(pc->_flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_UNSUPPORTED_ALGORITHM) ;
}

bool AuthGPG::getGPGDetails(const RsPgpId& pgp_id, RsPeerDetails &d) 
{
	RS_STACK_MUTEX(gpgMtxData);

	const PGPCertificateInfo *pc = PGPHandler::getCertificateInfo(pgp_id);
	if(pc == NULL) return false;

	const PGPCertificateInfo& cert(*pc);

	d.id.clear() ;
	d.gpg_id = pgp_id ;
	d.name = cert._name;
	d.lastUsed = cert._time_stamp;
	d.email = cert._email;
	d.trustLvl = cert._trustLvl;
	d.validLvl = cert._trustLvl;
	d.ownsign = cert._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE;
	d.gpgSigners.clear() ;

	for(auto it(cert.signers.begin()); it!=cert.signers.end(); ++it)
		d.gpgSigners.push_back( *it );

	d.fpr = cert._fpr;
	d.accept_connection = cert._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION;
	d.hasSignedMe = cert._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_SIGNED_ME;

	return true;
}

static bool filter_Validity(const PGPCertificateInfo& /*info*/) { return true ; } //{ return info._validLvl >= PGPCertificateInfo::GPGME_VALIDITY_MARGINAL ; }
static bool filter_Accepted(const PGPCertificateInfo& info) { return info._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION ; }
static bool filter_OwnSigned(const PGPCertificateInfo& info) { return info._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE ; }

bool AuthGPG::getGPGValidList(std::list<RsPgpId> &ids)
{
	return getGPGFilteredList(ids,&filter_Validity);
}

bool AuthGPG::getGPGAcceptedList(std::list<RsPgpId> &ids)
{
	return getGPGFilteredList(ids,&filter_Accepted);
}

bool AuthGPG::getGPGSignedList(std::list<RsPgpId> &ids)
{
	return getGPGFilteredList(ids,&filter_OwnSigned);
}

/*****************************************************************
 * Loading and Saving Certificates - this has to 
 * be able to handle both openpgp and X509 certificates.
 * 
 * X509 are passed onto AuthSSL, OpenPGP are passed to gpgme.
 *
 */

/* import to GnuPG and other Certificates */
bool AuthGPG::LoadCertificateFromString( const std::string &str,
                                         RsPgpId& gpg_id,
                                         std::string& error_string )
{
	if(PGPHandler::LoadCertificateFromString(str, gpg_id, error_string))
	{
		updateOwnSignatureFlag(gpg_id, mOwnGpgId);
		return true;
	}

	return false;
}

/* These take PGP Ids */
bool AuthGPG::AllowConnection(const RsPgpId& gpg_id, bool accept)
{
#ifdef GPG_DEBUG
        std::cerr << "AuthGPG::AllowConnection(" << gpg_id << ")" << std::endl;
#endif

	setAcceptConnexion(gpg_id,accept) ;
	IndicateConfigChanged();
	RsServer::notify()->notifyListChange(NOTIFY_LIST_FRIENDS, accept ? NOTIFY_TYPE_ADD : NOTIFY_TYPE_DEL);
	return true;
}

/* These take PGP Ids */
bool AuthGPG::SignCertificateLevel0(const RsPgpId &id)
{
#ifdef GPG_DEBUG
	std::cerr << "AuthGPG::SignCertificat(" << id << ")" << std::endl;
#endif

	RS_STACK_MUTEX(gpgMtxData);
	_force_sync_database = true;
	return PGPHandler::privateSignCertificate(mOwnGpgId,id);
}

bool AuthGPG::RevokeCertificate(const RsPgpId &id)
{
	std::cerr << "AuthGPG::RevokeCertificate(" << id << ") not implemented yet" << std::endl;
	return false;
}

bool AuthGPG::TrustCertificate(const RsPgpId& id, int trustlvl)
{
#ifdef GPG_DEBUG
	std::cerr << "AuthGPG::TrustCertificate(" << id << ", " << trustlvl << ")" << std::endl;
#endif
	RS_STACK_MUTEX(gpgMtxData);

	/* The certificate should be in Peers list ??? */
	if(!isGPGAccepted(id))
	{
		std::cerr << "AuthGPG::privateTrustCertificate Invalid Certificate" << std::endl;
		return false;
	}

	_force_sync_database = true;
	return PGPHandler::privateTrustCertificate(id,trustlvl);
}

bool AuthGPG::decryptDataBin( const void *data, unsigned int datalen,
                              unsigned char *sign, unsigned int *signlen )
{ return PGPHandler::decryptDataBin(mOwnGpgId, data, datalen, sign, signlen); }

bool AuthGPG::SignDataBin( const void *data, unsigned int datalen,
                           unsigned char *sign, unsigned int *signlen,
                           const std::string &reason)
{ return PGPHandler::SignDataBin(mOwnGpgId, data, datalen, sign, signlen, false, reason); }

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
	std::list<RsPgpId> ids ;	
	getGPGAcceptedList(ids) ;				// needs to be done before the lock

	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

	cleanup = true ;

	// Now save config for network digging strategies
	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;

	for (std::list<RsPgpId>::const_iterator it(ids.begin()); it != ids.end(); ++it) 
		if((*it) != mOwnGpgId) // skip our own id.
		{
			RsTlvKeyValue kv;
			kv.key = (*it).toStdString() ;
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

	RS_STACK_MUTEX(gpgMtxData);
	/* load the list of accepted gpg keys */
	std::list<RsItem *>::iterator it;
	for(it = load.begin(); it != load.end(); ++it)
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
			for(kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit)
				if (kit->key != mOwnGpgId.toStdString()) 
					PGPHandler::setAcceptConnexion(RsPgpId(kit->key), (kit->value == "TRUE"));
		}
		delete (*it);
	}
    	load.clear() ;
	return true;
}

bool AuthGPG::addService(AuthGPGService *service)
{
	RS_STACK_MUTEX(gpgMtxService);

	if (std::find(services.begin(), services.end(), service) != services.end())
		return false; // it exists already!

	services.push_back(service);
	return true;
}

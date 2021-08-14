/*******************************************************************************
 * libretroshare/src/pqi: authgpg.cc                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008-2009 by Robert Fernie, Retroshare Team.                      *
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
#include "authgpg.h"
#include "retroshare/rsiface.h"		// For rsicontrol.
#include "retroshare/rspeers.h"		// For RsPeerDetails.
#ifdef WINDOWS_SYS
#include "retroshare/rsinit.h"
#endif
#include "rsserver/p3face.h"
#include "pqi/p3notify.h"
#include "pgp/pgphandler.h"

#include <util/rsdir.h>
#include <util/rstime.h>
#include <pgp/pgpkeyutil.h>
#include <unistd.h>		/* for (u)sleep() */
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include "rsitems/rsconfigitems.h"

#define LIMIT_CERTIFICATE_SIZE		1
#define MAX_CERTIFICATE_SIZE		10000

//#define DEBUG_AUTHGPG 1

//const rstime_t STORE_KEY_TIMEOUT = 1 * 60 * 60; //store key is call around every hour

AuthGPG *AuthGPG::_instance = NULL ;

void cleanupZombies(int numkill); // function to cleanup zombies under OSX.

//#define GPG_DEBUG 1

/* Function to sign X509_REQ via GPGme.  */

int AuthGPG::availableGPGCertificatesWithPrivateKeys(std::list<RsPgpId>& pgpIds)
{
    return instance()->mPgpHandler->availableGPGCertificatesWithPrivateKeys(pgpIds);
}
bool AuthGPG::getGPGDetailsFromBinaryBlock(const unsigned char *mem,size_t mem_size,RsPgpId& key_id, std::string& name, std::list<RsPgpId>& signers)
{
    return instance()->mPgpHandler->getGPGDetailsFromBinaryBlock(mem,mem_size,key_id,name,signers);
}
void AuthGPG::registerToConfigMgr(const std::string& fname,p3ConfigMgr *CfgMgr)
{
    CfgMgr->addConfiguration(fname, instance());
}
bool AuthGPG::decryptTextFromFile(std::string& text,const std::string& inputfile)
{
    return instance()->mPgpHandler->decryptTextFromFile(instance()->mOwnGpgId,text,inputfile) ;
}

bool AuthGPG::removeKeysFromPGPKeyring(const std::set<RsPgpId>& pgp_ids,std::string& backup_file,uint32_t& error_code)
{
//	std::list<RsPgpId> pids ;
//
//	for(std::list<RsPgpId>::const_iterator it(pgp_ids.begin());it!=pgp_ids.end();++it)
//		pids.push_back(RsPgpId(*it)) ;

    return instance()->mPgpHandler->removeKeysFromPGPKeyring(pgp_ids,backup_file,error_code) ;
}

// bool AuthGPG::decryptTextFromString(std::string& encrypted_text,std::string& output)
// {
// 	return instance()->mPgpHandler->decryptTextFromString(mOwnGpgId,encrypted_text,output) ;
// }

bool AuthGPG::encryptTextToFile(const std::string& text,const std::string& outfile)
{
    return instance()->mPgpHandler->encryptTextToFile(instance()->mOwnGpgId,text,outfile) ;
}

// bool AuthGPG::encryptTextToString(const std::string& pgp_id,const std::string& text,std::string& outstr)
// {
// 	return instance()->mPgpHandler->encryptTextToString(RsPgpId(pgp_id),text,outstr) ;
// }

std::string pgp_pwd_callback(void * /*hook*/, const char *uid_title, const char *uid_hint, const char * /*passphrase_info*/, int prev_was_bad,bool *cancelled)
{
#ifdef GPG_DEBUG2
	fprintf(stderr, "pgp_pwd_callback() called.\n");
#endif
	std::string password;
	RsServer::notify()->askForPassword(uid_title, uid_hint, prev_was_bad, password,cancelled) ;

	return password ;
}

void AuthGPG::init(
        const std::string& path_to_public_keyring,
        const std::string& path_to_secret_keyring,
        const std::string& path_to_trustdb,
        const std::string& pgp_lock_file)
{
	if(_instance != NULL)
	{
		exit();
		std::cerr << "AuthGPG::init() called twice!" << std::endl ;
	}

//	if(cb) instance()->mPgpHandler->setPassphraseCallback(cb);else
    instance()->mPgpHandler->setPassphraseCallback(pgp_pwd_callback);
	_instance = new AuthGPG( path_to_public_keyring,
	                         path_to_secret_keyring,
	                         path_to_trustdb, pgp_lock_file );
}

void AuthGPG::exit()
{
	if(_instance)
	{
		_instance->fullstop();
		delete _instance;
		_instance = nullptr;
	}
}

AuthGPG::AuthGPG(const std::string& path_to_public_keyring,const std::string& path_to_secret_keyring,const std::string& path_to_trustdb,const std::string& pgp_lock_file)
        :p3Config(),
	gpgMtxService("AuthGPG-service"),
	gpgMtxEngine("AuthGPG-engine"),
	gpgMtxData("AuthGPG-data"),
	mStoreKeyTime(0),
	gpgKeySelected(false),
	_force_sync_database(false),
	mCount(0)
{
    mPgpHandler = new OpenPGPSDKHandler(path_to_public_keyring,path_to_secret_keyring,path_to_trustdb,pgp_lock_file);

    start("AuthGPG");
}

/* This function is called when retroshare is first started
 * to get the list of available GPG certificates.
 * This function should only return certs for which
 * the private(secret) keys are available.
 *
 * returns false if GnuPG is not available.
 */
//bool AuthGPG::availableGPGCertificatesWithPrivateKeys(std::list<std::string> &ids)
//{
//	std::list<RsPgpId> pids ;
//
//	mPgpHandler->availableGPGCertificatesWithPrivateKeys(pids) ;
//
//	for(std::list<RsPgpId>::const_iterator it(pids.begin());it!=pids.end();++it)
//		ids.push_back( (*it).toStdString() ) ;
//
//	/* return false if there are no private keys */
//	return !ids.empty();
//}

/* You can initialise Retroshare with
 * (a) load existing certificate.
 * (b) a new certificate.
 *
 * This function must be called successfully (return == 1)
 * before anything else can be done. (except above fn).
 */
int AuthGPG::GPGInit(const RsPgpId &ownId)
{
#ifdef DEBUG_AUTHGPG
	std::cerr << "AuthGPG::GPGInit() called with own gpg id : " << ownId.toStdString() << std::endl;
#endif

    instance()->mOwnGpgId = ownId;

	//force the validity of the private key. When set to unknown, it caused signature and text encryptions bugs
    instance()->privateTrustCertificate(ownId, 5);
    instance()->mPgpHandler->updateOwnSignatureFlag(ownId) ;

#ifdef DEBUG_AUTHGPG
	std::cerr << "AuthGPG::GPGInit finished." << std::endl;
#endif

	return 1;
}

 AuthGPG::~AuthGPG()
{
}

void AuthGPG::threadTick()
{
    rstime::rs_usleep(100 * 1000); //100 msec

    /// every 100 milliseconds
    processServices();

    /// every ten seconds
    if (++mCount >= 100 || _force_sync_database) {
        RsStackMutex stack(gpgMtxService); ///******* LOCKED ******

        /// The call does multiple things at once:
        /// 	- checks whether the keyring has changed in memory
        /// 	- checks whether the keyring has changed on disk.
        /// 	- merges/updates according to status.
        ///
        mPgpHandler->syncDatabase() ;
        mCount = 0;
        _force_sync_database = false ;
    }//if (++count >= 100 || _force_sync_database)
}

void AuthGPG::processServices()
{
    AuthGPGOperation *operation = NULL;
    AuthGPGService *service = NULL;

    {
        RsStackMutex stack(gpgMtxService); /******* LOCKED ******/

        std::list<AuthGPGService*>::iterator serviceIt;
        for (serviceIt = services.begin(); serviceIt != services.end(); ++serviceIt) {
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
             if (mPgpHandler->isGPGId(loadOrSave->m_certGpgId))
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

bool AuthGPG::DoOwnSignature(const void *data, unsigned int datalen, void *buf_sigout, unsigned int *outl, std::string reason /* = "" */)
{
    return instance()->mPgpHandler->SignDataBin(mOwnGpgId,data,datalen,(unsigned char *)buf_sigout,outl,false,reason) ;
}


/* import to GnuPG and other Certificates */
bool AuthGPG::VerifySignature(const void *data, int datalen, const void *sig, unsigned int siglen, const PGPFingerprintType& withfingerprint)
{
    return instance()->mPgpHandler->VerifySignBin((unsigned char*)data,datalen,(unsigned char*)sig,siglen,withfingerprint) ;
}

bool AuthGPG::parseSignature(const void *sig, unsigned int siglen, RsPgpId& issuer_id)
{
    return instance()->mPgpHandler->parseSignature((unsigned char*)sig,siglen,issuer_id) ;
}

bool AuthGPG::exportProfile(const std::string& fname,const RsPgpId& exported_id)
{
    return instance()->mPgpHandler->exportGPGKeyPair(fname,exported_id) ;
}

bool AuthGPG::exportIdentityToString(
        std::string& data, const RsPgpId& pgpId, bool includeSignatures,
        std::string& errorMsg )
{
    return instance()->mPgpHandler->exportGPGKeyPairToString(
	            data, pgpId, includeSignatures, errorMsg);
}

bool AuthGPG::importProfile(const std::string& fname,RsPgpId& imported_id,std::string& import_error)
{
    return instance()->mPgpHandler->importGPGKeyPair(fname,imported_id,import_error) ;
}

bool AuthGPG::importProfileFromString(const std::string &data, RsPgpId &gpg_id, std::string &import_error)
{
    return instance()->mPgpHandler->importGPGKeyPairFromString(data, gpg_id, import_error);
}

bool   AuthGPG::active()
{
        RsStackMutex stack(instance()->gpgMtxData); /******* LOCKED ******/

        return instance()->gpgKeySelected;
}

bool    AuthGPG::GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passwd, RsPgpId& pgpId, const int keynumbits, std::string& errString)
{
    RsStackMutex stack(instance()->gpgMtxEngine); /******* LOCKED ******/

    return instance()->mPgpHandler->GeneratePGPCertificate(name, email, passwd, pgpId, keynumbits, errString) ;
}

/**** These Two are common */
std::string AuthGPG::getGPGName(const RsPgpId& id,bool *success)
{
    RsStackMutex stack(instance()->gpgMtxData); /******* LOCKED ******/

    const PGPCertificateInfo *info = instance()->mPgpHandler->getCertificateInfo(id) ;

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

AuthGPG *AuthGPG::instance()
{
    if(!_instance)
    {
        RsFatal() << "AuthGPG::instance() called before AuthGPG::init()! This should not happen." << std::endl;
        return nullptr;
    }

    return _instance;
}
bool AuthGPG::isPGPId(const RsPgpId& id)
{
    return instance()->mPgpHandler->isGPGId(id);
}
bool AuthGPG::isPGPAccepted(const RsPgpId& id)
{
    return instance()->mPgpHandler->isGPGAccepted(id);
}
/**** These Two are common */
std::string AuthGPG::getGPGEmail(const RsPgpId& id,bool *success)
{
    RsStackMutex stack(instance()->gpgMtxData); /******* LOCKED ******/
    const PGPCertificateInfo *info = instance()->mPgpHandler->getCertificateInfo(id) ;

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
    RsStackMutex stack(instance()->gpgMtxData); /******* LOCKED ******/
    return instance()->mOwnGpgId ;
}

std::string AuthGPG::getGPGOwnName()
{
    return getGPGName(instance()->mOwnGpgId) ;
}

bool	AuthGPG::getGPGAllList(std::list<RsPgpId> &ids)
{
    RsStackMutex stack(instance()->gpgMtxData); /******* LOCKED ******/

    instance()->mPgpHandler->getGPGFilteredList(ids) ;

	return true;
}
const PGPCertificateInfo *AuthGPG::getCertInfoFromStdString(const std::string& pgp_id) const
{
	try
	{
        return instance()->mPgpHandler->getCertificateInfo(RsPgpId(pgp_id)) ;
	}
	catch(std::exception& e)
	{
		std::cerr << "(EE) exception raised while constructing a PGP certificate from id \"" << pgp_id << "\": " << e.what() << std::endl;
		return NULL ;
	}
}
bool AuthGPG::haveSecretKey(const RsPgpId& id)
{
    return instance()->mPgpHandler->haveSecretKey(id) ;
}
bool AuthGPG::isKeySupported(const RsPgpId& id)
{
    const PGPCertificateInfo *pc = instance()->mPgpHandler->getCertificateInfo(id) ;

	if(pc == NULL)
		return false ;

	return !(pc->_flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_UNSUPPORTED_ALGORITHM) ;
}

bool AuthGPG::getGPGDetails(const RsPgpId& pgp_id, RsPeerDetails &d)
{
    RsStackMutex stack(instance()->gpgMtxData); /******* LOCKED ******/

    const PGPCertificateInfo *pc = instance()->mPgpHandler->getCertificateInfo(pgp_id) ;

	if(pc == NULL)
		return false ;

	const PGPCertificateInfo& cert(*pc) ;

	d.id.clear() ;
	d.gpg_id = pgp_id ;
	d.name = cert._name;
	d.lastUsed = cert._time_stamp;
	d.email = cert._email;
	d.trustLvl = cert._trustLvl;
	d.validLvl = cert._trustLvl;
	d.ownsign = cert._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE;
	d.gpgSigners.clear() ;

	for(std::set<RsPgpId>::const_iterator it(cert.signers.begin());it!=cert.signers.end();++it)
		d.gpgSigners.push_back( *it ) ;

	d.fpr = cert._fpr ;
	d.accept_connection = 	cert._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION;
	d.hasSignedMe = 			cert._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_SIGNED_ME;

	return true;
}

bool AuthGPG::getGPGFilteredList(std::list<RsPgpId>& list,bool (*filter)(const PGPCertificateInfo&))
{
    return instance()->mPgpHandler->getGPGFilteredList(list,filter) ;
}

static bool filter_Validity(const PGPCertificateInfo& /*info*/) { return true ; } //{ return info._validLvl >= PGPCertificateInfo::GPGME_VALIDITY_MARGINAL ; }
static bool filter_Accepted(const PGPCertificateInfo& info) { return info._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION ; }
static bool filter_OwnSigned(const PGPCertificateInfo& info) { return info._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE ; }

bool	AuthGPG::getGPGValidList(std::list<RsPgpId> &ids)
{
	return getGPGFilteredList(ids,&filter_Validity);
}

bool	AuthGPG::getGPGAcceptedList(std::list<RsPgpId> &ids)
{
	return getGPGFilteredList(ids,&filter_Accepted);
}

bool	AuthGPG::getGPGSignedList(std::list<RsPgpId> &ids)
{
	return getGPGFilteredList(ids,&filter_OwnSigned);
}

// bool	AuthGPG::getCachedGPGCertificate(const RsPgpId &id, std::string &certificate)
// {
// 	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
// #ifdef LIMIT_CERTIFICATE_SIZE
// 	certificate = instance()->mPgpHandler->SaveCertificateToString(RsPgpId(id),false) ;
// #else
// 	certificate = instance()->mPgpHandler->SaveCertificateToString(RsPgpId(id),true) ;
// #endif
//
// // #ifdef LIMIT_CERTIFICATE_SIZE
// // 	std::string cleaned_key ;
// // 	if(PGPKeyManagement::createMinimalKey(certificate,cleaned_key))
// // 		certificate = cleaned_key ;
// // #endif
//
// 	return certificate.length() > 0 ;
// }

/*****************************************************************
 * Loading and Saving Certificates - this has to
 * be able to handle both openpgp and X509 certificates.
 *
 * X509 are passed onto AuthSSL, OpenPGP are passed to gpgme.
 *
 */


/* SKTAN : do not know how to use std::string id */
 std::string AuthGPG::SaveCertificateToString(const RsPgpId &id,bool include_signatures)
 {
    RsStackMutex stack(instance()->gpgMtxEngine); /******* LOCKED ******/

    return instance()->mPgpHandler->SaveCertificateToString(id,include_signatures) ;
 }
/* import to GnuPG and other Certificates */
bool AuthGPG::LoadPGPKeyFromBinaryData(const unsigned char *data,uint32_t data_len, RsPgpId& gpg_id,std::string& error_string)
{
    RsStackMutex stack(instance()->gpgMtxEngine); /******* LOCKED ******/

    if(instance()->mPgpHandler->LoadCertificateFromBinaryData(data,data_len,gpg_id,error_string))
	{
        instance()->mPgpHandler->updateOwnSignatureFlag(gpg_id,instance()->mOwnGpgId) ;
		return true ;
	}

	return false ;
}

/* import to GnuPG and other Certificates */
bool AuthGPG::LoadCertificateFromString(const std::string &str, RsPgpId& gpg_id,std::string& error_string)
{
    RsStackMutex stack(instance()->gpgMtxEngine); /******* LOCKED ******/

    if(instance()->mPgpHandler->LoadCertificateFromString(str,gpg_id,error_string))
	{
        instance()->mPgpHandler->updateOwnSignatureFlag(gpg_id,instance()->mOwnGpgId) ;
		return true ;
	}

	return false ;
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
bool AuthGPG::AllowConnection(const RsPgpId& gpg_id, bool accept)
{
#ifdef GPG_DEBUG
        std::cerr << "AuthGPG::AllowConnection(" << gpg_id << ")" << std::endl;
#endif

	/* Was a "Reload Certificates" here -> be shouldn't be needed -> and very expensive, try without. */
	{
        RsStackMutex stack(instance()->gpgMtxData);
        instance()->mPgpHandler->setAcceptConnexion(gpg_id,accept) ;
	}

    instance()->IndicateConfigChanged();

	RsServer::notify()->notifyListChange(NOTIFY_LIST_FRIENDS, accept ? NOTIFY_TYPE_ADD : NOTIFY_TYPE_DEL);

	return true;
}

/* These take PGP Ids */
bool AuthGPG::SignCertificateLevel0(const RsPgpId &id)
{
#ifdef GPG_DEBUG
	std::cerr << "AuthGPG::SignCertificat(" << id << ")" << std::endl;
#endif

    return instance()->privateSignCertificate(id) ;
}

bool AuthGPG::RevokeCertificate(const RsPgpId &id)
{
	/* remove unused parameter warnings */
	(void) id;

#ifdef GPG_DEBUG
	std::cerr << "AuthGPG::RevokeCertificate(" << id << ") not implemented yet" << std::endl;
#endif

	return false;
}

bool AuthGPG::TrustCertificate(const RsPgpId& id, int trustlvl)
{
#ifdef GPG_DEBUG
	std::cerr << "AuthGPG::TrustCertificate(" << id << ", " << trustlvl << ")" << std::endl;
#endif
    return instance()->privateTrustCertificate(id, trustlvl) ;
}

bool AuthGPG::encryptDataBin(const RsPgpId& pgp_id,const void *data, unsigned int datalen, unsigned char *sign, unsigned int *signlen)
{
    return instance()->mPgpHandler->encryptDataBin(RsPgpId(pgp_id),data,datalen,sign,signlen) ;
}

bool AuthGPG::decryptDataBin(const void *data, unsigned int datalen, unsigned char *sign, unsigned int *signlen)
{
    return instance()->mPgpHandler->decryptDataBin(instance()->mOwnGpgId,data,datalen,sign,signlen) ;
}
bool AuthGPG::SignDataBin(const void *data, unsigned int datalen, unsigned char *sign, unsigned int *signlen, std::string reason /*= ""*/)
{
    return instance()->DoOwnSignature(data, datalen, sign, signlen, reason);
}

bool AuthGPG::exportPublicKey( const RsPgpId& id, unsigned char*& mem_block, size_t& mem_size, bool armoured, bool include_signatures )
{
    return instance()->mPgpHandler->exportPublicKey(id,mem_block,mem_size,armoured,include_signatures);
}

bool AuthGPG::isPgpPubKeyAvailable(const RsPgpId& pgp_id)
{
    return instance()->mPgpHandler->isPgpPubKeyAvailable(pgp_id);
}
bool AuthGPG::getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp)
{
    return instance()->mPgpHandler->getKeyFingerprint(id,fp);
}
bool AuthGPG::VerifySignBin(const void *data, uint32_t datalen, unsigned char *sign, unsigned int signlen, const PGPFingerprintType& withfingerprint)
{
    return instance()->VerifySignature(data, datalen, sign, signlen, withfingerprint);
}

/* Sign/Trust stuff */

int	AuthGPG::privateSignCertificate(const RsPgpId &id)
{
	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

    int ret = mPgpHandler->privateSignCertificate(mOwnGpgId,id) ;
	_force_sync_database = true ;
	return ret ;
}

/* revoke the signature on Certificate */
int	AuthGPG::privateRevokeCertificate(const RsPgpId &/*id*/)
{
	//RsStackMutex stack(gpgMtx); /******* LOCKED ******/
	std::cerr << __PRETTY_FUNCTION__ << ": not implemented!" << std::endl;

	return 0;
}

int	AuthGPG::privateTrustCertificate(const RsPgpId& id, int trustlvl)
{
	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/

	// csoler: Why are we not allowing this when the peer is not in the accepted peers list??
	//         The trust level is only a user-defined property that has nothing to
	//         do with the fact that we allow connections or not.

	if(!isGPGAccepted(id))
		return 0;

    int res = instance()->mPgpHandler->privateTrustCertificate(id,trustlvl) ;
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
bool AuthGPG::isGPGAccepted(const RsPgpId& id)
{
    return instance()->mPgpHandler->isGPGAccepted(id);
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

	RsStackMutex stack(gpgMtxData); /******* LOCKED ******/
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
                    instance()->mPgpHandler->setAcceptConnexion(RsPgpId(kit->key), (kit->value == "TRUE"));
		}
		delete (*it);
	}
    	load.clear() ;
	return true;
}

bool AuthGPG::addService(AuthGPGService *service)
{
    RsStackMutex stack(instance()->gpgMtxService); /********* LOCKED *********/

    if (std::find(instance()->services.begin(), instance()->services.end(), service) != instance()->services.end()) {
		/* it exists already! */
		return false;
	}

    instance()->services.push_back(service);
	return true;
}



/*******************************************************************************
 * libretroshare/src/pgp: pgphandler.cc                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 Cyril Soler <csoler@users.sourceforge.net>                   *
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
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef WINDOWS_SYS
#include <io.h>
#include "util/rsstring.h"
#include "util/rswin.h"
#endif

extern "C" {
#include <openpgpsdk/util.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/armour.h>
#include <openpgpsdk/keyring.h>
#include <openpgpsdk/readerwriter.h>
#include <openpgpsdk/validate.h>
#include <openpgpsdk/parse_local.h>
}
#include "pgphandler.h"
#include "retroshare/rsiface.h"		// For rsicontrol.
#include "retroshare/rspeers.h"		// For rsicontrol.
#include "util/rsdir.h"		
#include "util/rsdiscspace.h"		
#include "util/rsmemory.h"		
#include "pgp/pgpkeyutil.h"

static const uint32_t PGP_CERTIFICATE_LIMIT_MAX_NAME_SIZE   = 64 ;
static const uint32_t PGP_CERTIFICATE_LIMIT_MAX_EMAIL_SIZE  = 64 ;
static const uint32_t PGP_CERTIFICATE_LIMIT_MAX_PASSWD_SIZE = 1024 ;

//#define DEBUG_PGPHANDLER 1
//#define PGPHANDLER_DSA_SUPPORT

PassphraseCallback PGPHandler::_passphrase_callback = NULL ;

ops_keyring_t *PGPHandler::allocateOPSKeyring() 
{
	ops_keyring_t *kr = (ops_keyring_t*)rs_malloc(sizeof(ops_keyring_t)) ;
    
    	if(kr == NULL)
            return NULL ;
        
	kr->nkeys = 0 ;
	kr->nkeys_allocated = 0 ;
	kr->keys = 0 ;

	return kr ;
}

ops_parse_cb_return_t cb_get_passphrase(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)// __attribute__((unused)))
{
	const ops_parser_content_union_t *content=&content_->content;
	bool prev_was_bad = false ;
	
	switch(content_->tag)
	{
		case OPS_PARSER_CMD_GET_SK_PASSPHRASE_PREV_WAS_BAD: prev_was_bad = true ;
			/* fallthrough */
		case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
		{
			std::string passwd;
			std::string uid_hint ;

			if(cbinfo->cryptinfo.keydata->nuids > 0)
				uid_hint = std::string((const char *)cbinfo->cryptinfo.keydata->uids[0].user_id) ;
			uid_hint += "(" + RsPgpId(cbinfo->cryptinfo.keydata->key_id).toStdString()+")" ;

			bool cancelled = false ;
			passwd = PGPHandler::passphraseCallback()(NULL,"",uid_hint.c_str(),NULL,prev_was_bad,&cancelled) ;

			if(cancelled)
				*(unsigned char *)cbinfo->arg = 1;

			*(content->secret_key_passphrase.passphrase)= (char *)ops_mallocz(passwd.length()+1) ;
			memcpy(*(content->secret_key_passphrase.passphrase),passwd.c_str(),passwd.length()) ;
			return OPS_KEEP_MEMORY;
		}
		break;

		default:
			break;
	}

	return OPS_RELEASE_MEMORY;
}
void PGPHandler::setPassphraseCallback(PassphraseCallback cb)
{
	_passphrase_callback = cb ;
}

PGPHandler::PGPHandler(const std::string& pubring, const std::string& secring,const std::string& trustdb,const std::string& pgp_lock_filename)
	: pgphandlerMtx(std::string("PGPHandler")), _pubring_path(pubring),_secring_path(secring),_trustdb_path(trustdb),_pgp_lock_filename(pgp_lock_filename)
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	_pubring_changed = false ;
	_trustdb_changed = false ;

	RsStackFileLock flck(_pgp_lock_filename) ;	// lock access to PGP directory.

	if(_passphrase_callback == NULL)
		std::cerr << "WARNING: before created a PGPHandler, you need to init the passphrase callback using PGPHandler::setPassphraseCallback()" << std::endl;
		
	// Allocate public and secret keyrings.
	// 
	_pubring = allocateOPSKeyring() ;
	_secring = allocateOPSKeyring() ;

	// Check that the file exists. If not, create a void keyring.
	
	FILE *ftest ;
	ftest = RsDirUtil::rs_fopen(pubring.c_str(),"rb") ;
	bool pubring_exist = (ftest != NULL) ;
	if(ftest != NULL)
		fclose(ftest) ;
	ftest = RsDirUtil::rs_fopen(secring.c_str(),"rb") ;
	bool secring_exist = (ftest != NULL) ;
	if(ftest != NULL)
		fclose(ftest) ;

	// Read public and secret keyrings from supplied files.
	//
	if(pubring_exist)
	{
		if(ops_false == ops_keyring_read_from_file(_pubring, false, pubring.c_str()))
			throw std::runtime_error("PGPHandler::readKeyRing(): cannot read pubring. File corrupted.") ;
	}
	else
		std::cerr << "pubring file \"" << pubring << "\" not found. Creating a void keyring." << std::endl;

	const ops_keydata_t *keydata ;
	int i=0 ;
	while( (keydata = ops_keyring_get_key_by_index(_pubring,i)) != NULL )
	{
		PGPCertificateInfo& cert(_public_keyring_map[ RsPgpId(keydata->key_id) ]) ;

		// Init all certificates.
	
		initCertificateInfo(cert,keydata,i) ;

		// Validate signatures.
		
		validateAndUpdateSignatures(cert,keydata) ;

		++i ;
	}
	_pubring_last_update_time = time(NULL) ;
	std::cerr << "Pubring read successfully." << std::endl;

	if(secring_exist)
	{
		if(ops_false == ops_keyring_read_from_file(_secring, false, secring.c_str()))
			throw std::runtime_error("PGPHandler::readKeyRing(): cannot read secring. File corrupted.") ;
	}
	else
		std::cerr << "secring file \"" << secring << "\" not found. Creating a void keyring." << std::endl;

	i=0 ;
	while( (keydata = ops_keyring_get_key_by_index(_secring,i)) != NULL )
	{
		initCertificateInfo(_secret_keyring_map[ RsPgpId(keydata->key_id) ],keydata,i) ;
		++i ;
	}
	_secring_last_update_time = time(NULL) ;

	std::cerr << "Secring read successfully." << std::endl;

	locked_readPrivateTrustDatabase() ;
	_trustdb_last_update_time = time(NULL) ;
}

void PGPHandler::initCertificateInfo(PGPCertificateInfo& cert,const ops_keydata_t *keydata,uint32_t index)
{
	// Parse certificate name
	//

	if(keydata->uids != NULL)
	{
		std::string namestring( (char *)keydata->uids[0].user_id ) ;

		cert._name = "" ;
		uint32_t i=0;
		while(i < namestring.length() && namestring[i] != '(' && namestring[i] != '<') { cert._name += namestring[i] ; ++i ;}

		// trim right spaces
		std::string::size_type found = cert._name.find_last_not_of(' ');
		if (found != std::string::npos)
			cert._name.erase(found + 1);
		else
			cert._name.clear(); // all whitespace

		std::string& next = (namestring[i] == '(')?cert._comment:cert._email ;
		++i ;
		next = "" ;
		while(i < namestring.length() && namestring[i] != ')' && namestring[i] != '>') { next += namestring[i] ; ++i ;}

		while(i < namestring.length() && namestring[i] != '(' && namestring[i] != '<') { next += namestring[i] ; ++i ;}

		if(i< namestring.length())
		{
			std::string& next2 = (namestring[i] == '(')?cert._comment:cert._email ;
			++i ;
			next2 = "" ;
			while(i < namestring.length() && namestring[i] != ')' && namestring[i] != '>') { next2 += namestring[i] ; ++i ;}
		}
	}

	cert._trustLvl = 1 ;	// to be setup accordingly
	cert._validLvl = 1 ;	// to be setup accordingly
	cert._key_index = index ;
	cert._flags = 0 ;
	cert._time_stamp = 0 ;// "never" by default. Will be updated by trust database, and effective key usage.

	switch(keydata->key.pkey.algorithm)
	{
		case OPS_PKA_RSA: cert._type = PGPCertificateInfo::PGP_CERTIFICATE_TYPE_RSA ;
								break ;
		case OPS_PKA_DSA: cert._type = PGPCertificateInfo::PGP_CERTIFICATE_TYPE_DSA ;
								cert._flags |= PGPCertificateInfo::PGP_CERTIFICATE_FLAG_UNSUPPORTED_ALGORITHM ;
								break ;
		default: cert._type = PGPCertificateInfo::PGP_CERTIFICATE_TYPE_UNKNOWN ;
								cert._flags |= PGPCertificateInfo::PGP_CERTIFICATE_FLAG_UNSUPPORTED_ALGORITHM ;
								break ;
	}

	ops_fingerprint_t f ;
	ops_fingerprint(&f,&keydata->key.pkey) ; 

	cert._fpr = PGPFingerprintType(f.fingerprint) ;
}

bool PGPHandler::validateAndUpdateSignatures(PGPCertificateInfo& cert,const ops_keydata_t *keydata)
{
	ops_validate_result_t* result=(ops_validate_result_t*)ops_mallocz(sizeof *result);
	ops_boolean_t res = ops_validate_key_signatures(result,keydata,_pubring,cb_get_passphrase) ;

	if(res == ops_false)
	{
		static ops_boolean_t already = 0 ;
		if(!already)
		{
			std::cerr << "(WW) Error in PGPHandler::validateAndUpdateSignatures(). Validation failed for at least some signatures." << std::endl;
			already = 1 ;
		}
	}

	bool ret = false ;

	// Parse signers.
	//

	if(result != NULL)
		for(size_t i=0;i<result->valid_count;++i)
		{
			RsPgpId signer_id(result->valid_sigs[i].signer_id);

			if(cert.signers.find(signer_id) == cert.signers.end())
			{
				cert.signers.insert(signer_id) ;
				ret = true ;
			}
		}

	ops_validate_result_free(result) ;

	return ret ;
}

PGPHandler::~PGPHandler()
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
#ifdef DEBUG_PGPHANDLER
	std::cerr << "Freeing PGPHandler. Deleting keyrings." << std::endl;
#endif

	// no need to free the the _map_ elements. They will be freed by the following calls:
	//
	ops_keyring_free(_pubring) ;
	ops_keyring_free(_secring) ;

	free(_pubring) ;
	free(_secring) ;
}

bool PGPHandler::printKeys() const
{
#ifdef DEBUG_PGPHANDLER
	std::cerr << "Printing details of all " << std::dec << _public_keyring_map.size() << " keys: " << std::endl;
#endif

	for(std::map<RsPgpId,PGPCertificateInfo>::const_iterator it(_public_keyring_map.begin()); it != _public_keyring_map.end(); ++it)
	{
		std::cerr << "PGP Key: " << it->first.toStdString() << std::endl;

		std::cerr << "\tName          : " <<  it->second._name << std::endl;
		std::cerr << "\tEmail         : " <<  it->second._email << std::endl;
		std::cerr << "\tOwnSign       : " << (it->second._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE) << std::endl;
		std::cerr << "\tAccept Connect: " << (it->second._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION) << std::endl;
		std::cerr << "\ttrustLvl      : " <<  it->second._trustLvl << std::endl;
		std::cerr << "\tvalidLvl      : " <<  it->second._validLvl << std::endl;
		std::cerr << "\tUse time stamp: " <<  it->second._time_stamp << std::endl;
		std::cerr << "\tfingerprint   : " <<  it->second._fpr.toStdString() << std::endl;
		std::cerr << "\tSigners       : " << it->second.signers.size() <<  std::endl;

		std::set<RsPgpId>::const_iterator sit;
		for(sit = it->second.signers.begin(); sit != it->second.signers.end(); ++sit)
		{
			std::cerr << "\t\tSigner ID:" << (*sit).toStdString() << ", Name: " ;
			const PGPCertificateInfo *info = PGPHandler::getCertificateInfo(*sit) ;

			if(info != NULL)
				std::cerr << info->_name ;

			std::cerr << std::endl ;
		}
	}
	std::cerr << "Public keyring list from OPS:" << std::endl;
	ops_keyring_list(_pubring) ;

	return true ;
}

bool PGPHandler::haveSecretKey(const RsPgpId& id) const
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	return locked_getSecretKey(id) != NULL ;
}

const PGPCertificateInfo *PGPHandler::getCertificateInfo(const RsPgpId& id) const
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	std::map<RsPgpId,PGPCertificateInfo>::const_iterator it( _public_keyring_map.find(id) ) ;

	if(it != _public_keyring_map.end())
		return &it->second;
	else
		return NULL ;
}

bool PGPHandler::availableGPGCertificatesWithPrivateKeys(std::list<RsPgpId>& ids)
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
	// go through secret keyring, and check that we have the pubkey as well.
	//
	
	const ops_keydata_t *keydata = NULL ;
	int i=0 ;

	while( (keydata = ops_keyring_get_key_by_index(_secring,i++)) != NULL )
		if(ops_keyring_find_key_by_id(_pubring,keydata->key_id) != NULL) // check that the key is in the pubring as well
		{
#ifdef PGPHANDLER_DSA_SUPPORT
			if(keydata->key.pkey.algorithm == OPS_PKA_RSA || keydata->key.pkey.algorithm == OPS_PKA_DSA)
#else
			if(keydata->key.pkey.algorithm == OPS_PKA_RSA)
#endif
				ids.push_back(RsPgpId(keydata->key_id)) ;
#ifdef DEBUG_PGPHANDLER
			else
				std::cerr << "Skipping keypair " << RsPgpId(keydata->key_id).toStdString() << ", unsupported algorithm: " <<  keydata->key.pkey.algorithm << std::endl;
#endif
		}

	return true ;
}

bool PGPHandler::GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passphrase, RsPgpId& pgpId, const int keynumbits, std::string& errString)
{
	// Some basic checks
	
	if(!RsDiscSpace::checkForDiscSpace(RS_PGP_DIRECTORY))
	{
		errString = std::string("(EE) low disc space in pgp directory. Can't write safely to keyring.") ;
		return false ;
	}
	if(name.length() > PGP_CERTIFICATE_LIMIT_MAX_NAME_SIZE)
	{
		errString = std::string("(EE) name in certificate exceeds the maximum allowed name size") ;
		return false ;
	}
	if(email.length() > PGP_CERTIFICATE_LIMIT_MAX_EMAIL_SIZE)
	{
		errString = std::string("(EE) email in certificate exceeds the maximum allowed email size") ;
		return false ;
	}
	if(passphrase.length() > PGP_CERTIFICATE_LIMIT_MAX_PASSWD_SIZE)
	{
		errString = std::string("(EE) passphrase in certificate exceeds the maximum allowed passphrase size") ;
		return false ;
	}
	if(keynumbits % 1024 != 0)
	{
		errString = std::string("(EE) RSA key length is not a multiple of 1024") ;
		return false ;
	}

	// Now the real thing
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
	RsStackFileLock flck(_pgp_lock_filename) ;	// lock access to PGP directory.

	// 1 - generate keypair - RSA-2048
	//
	ops_user_id_t uid ;
	char *s = strdup((name + " (Generated by RetroShare) <" + email + ">" ).c_str()) ;
	uid.user_id = (unsigned char *)s ;
	unsigned long int e = 65537 ; // some prime number

	ops_keydata_t *key = ops_rsa_create_selfsigned_keypair(keynumbits, e, &uid) ;

	free(s) ;

	if(!key)
		return false ;

	// 2 - save the private key encrypted to a temporary memory buffer, so as to read an encrypted key to memory

	ops_create_info_t *cinfo = NULL ;
	ops_memory_t *buf = NULL ;
	ops_setup_memory_write(&cinfo, &buf, 0);

	if(!ops_write_transferable_secret_key(key,(unsigned char *)passphrase.c_str(),passphrase.length(),ops_false,cinfo))
	{
		errString = std::string("(EE) Cannot encode secret key to memory!!") ;
		return false ;
	}

	// 3 - read the memory chunk into an encrypted keyring
	
	ops_keyring_t *tmp_secring = allocateOPSKeyring() ;

	if(! ops_keyring_read_from_mem(tmp_secring, ops_false, buf))
	{
		errString = std::string("(EE) Cannot re-read key from memory!!") ;
		return false ;
	}
	ops_teardown_memory_write(cinfo,buf);	// cleanup memory

	// 4 - copy the encrypted private key to the private keyring
	
	pgpId = RsPgpId(tmp_secring->keys[0].key_id) ;
	addNewKeyToOPSKeyring(_secring,tmp_secring->keys[0]) ;
	initCertificateInfo(_secret_keyring_map[ pgpId ],&tmp_secring->keys[0],_secring->nkeys-1) ;

#ifdef DEBUG_PGPHANDLER
	std::cerr << "Added new secret key with id " << pgpId.toStdString() << " to secret keyring." << std::endl;
#endif
	ops_keyring_free(tmp_secring) ;
	free(tmp_secring) ;

	// 5 - add key to secret keyring on disk.
	
	cinfo = NULL ;
	std::string secring_path_tmp = _secring_path + ".tmp" ;

	if(RsDirUtil::fileExists(_secring_path) && !RsDirUtil::copyFile(_secring_path,secring_path_tmp))
	{
		errString= std::string("Cannot copy secret keyring !! Disk full? Out of disk quota?") ;
		return false ;
	}
	int fd=ops_setup_file_append(&cinfo, secring_path_tmp.c_str());

	if(!ops_write_transferable_secret_key(key,(unsigned char *)passphrase.c_str(),passphrase.length(),ops_false,cinfo))
	{
		errString= std::string("Cannot encode secret key to disk!! Disk full? Out of disk quota?") ;
		return false ;
	}
	ops_teardown_file_write(cinfo,fd) ;

	if(!RsDirUtil::renameFile(secring_path_tmp,_secring_path))
	{
		errString= std::string("Cannot rename tmp secret key file ") + secring_path_tmp + " into " + _secring_path +". Disk error?" ;
		return false ;
	}

	// 6 - copy the public key to the public keyring on disk
	
	cinfo = NULL ;
	std::string pubring_path_tmp = _pubring_path + ".tmp" ;

	if(RsDirUtil::fileExists(_pubring_path) && !RsDirUtil::copyFile(_pubring_path,pubring_path_tmp))
	{
		errString= std::string("Cannot encode secret key to disk!! Disk full? Out of disk quota?") ;
		return false ;
	}
	fd=ops_setup_file_append(&cinfo, pubring_path_tmp.c_str());

	if(!ops_write_transferable_public_key(key, ops_false, cinfo))
	{
		errString=std::string("Cannot encode secret key to memory!!") ;
		return false ;
	}
	ops_teardown_file_write(cinfo,fd) ;

	if(!RsDirUtil::renameFile(pubring_path_tmp,_pubring_path))
	{
		errString= std::string("Cannot rename tmp public key file ") + pubring_path_tmp + " into " + _pubring_path +". Disk error?" ;
		return false ;
	}
	// 7 - clean
	ops_keydata_free(key) ;

	// 8 - re-read the key from the public keyring, and add it to memory.

	_pubring_last_update_time = 0 ; // force update pubring from disk.
	locked_syncPublicKeyring() ;	

#ifdef DEBUG_PGPHANDLER
	std::cerr << "Added new public key with id " << pgpId.toStdString() << " to public keyring." << std::endl;
#endif

	// 9 - Update some flags.

	privateTrustCertificate(pgpId,PGPCertificateInfo::PGP_CERTIFICATE_TRUST_ULTIMATE) ;

	return true ;
}

std::string PGPHandler::makeRadixEncodedPGPKey(const ops_keydata_t *key,bool include_signatures)
{
   ops_create_info_t* cinfo;
	ops_memory_t *buf = NULL ;
   ops_setup_memory_write(&cinfo, &buf, 0);
	ops_boolean_t armoured = ops_true ;

	if(key->type == OPS_PTAG_CT_PUBLIC_KEY)
	{
		if(ops_write_transferable_public_key_from_packet_data(key,armoured,cinfo) != ops_true)
			return "ERROR: This key cannot be processed by RetroShare because\nDSA certificates are not yet handled." ;
	}
	else if(key->type == OPS_PTAG_CT_ENCRYPTED_SECRET_KEY)
	{
		if(ops_write_transferable_secret_key_from_packet_data(key,armoured,cinfo) != ops_true)
			return "ERROR: This key cannot be processed by RetroShare because\nDSA certificates are not yet handled." ;
	}
	else
	{
        ops_create_info_delete(cinfo);
		std::cerr << "Unhandled key type " << key->type << std::endl;
		return "ERROR: Cannot write key. Unhandled key type. " ;
	}

	ops_writer_close(cinfo) ;

	std::string res((char *)ops_memory_get_data(buf),ops_memory_get_length(buf)) ;
   ops_teardown_memory_write(cinfo,buf);

	if(!include_signatures)
	{
		std::string tmp ;
		if(PGPKeyManagement::createMinimalKey(res,tmp) )
			res = tmp ;
	}

	return res ;
}

const ops_keydata_t *PGPHandler::locked_getSecretKey(const RsPgpId& id) const
{
	std::map<RsPgpId,PGPCertificateInfo>::const_iterator res = _secret_keyring_map.find(id) ;

	if(res == _secret_keyring_map.end())
		return NULL ;
	else
		return ops_keyring_get_key_by_index(_secring,res->second._key_index) ;
}
const ops_keydata_t *PGPHandler::locked_getPublicKey(const RsPgpId& id,bool stamp_the_key) const
{
	std::map<RsPgpId,PGPCertificateInfo>::const_iterator res = _public_keyring_map.find(id) ;

	if(res == _public_keyring_map.end())
		return NULL ;
	else
	{
		if(stamp_the_key)		// Should we stamp the key as used?
		{
			static rstime_t last_update_db_because_of_stamp = 0 ;
			rstime_t now = time(NULL) ;

			res->second._time_stamp = now ;

			if(now > last_update_db_because_of_stamp + 3600) // only update database once every hour. No need to do it more often.
			{
				_trustdb_changed = true ;
				last_update_db_because_of_stamp = now ;
			}
		}
		return ops_keyring_get_key_by_index(_pubring,res->second._key_index) ;
	}
}

std::string PGPHandler::SaveCertificateToString(const RsPgpId& id,bool include_signatures) const
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
	const ops_keydata_t *key = locked_getPublicKey(id,false) ;

	if(key == NULL)
	{
		std::cerr << "Cannot output key " << id.toStdString() << ": not found in keyring." << std::endl;
		return "" ;
	}

	return makeRadixEncodedPGPKey(key,include_signatures) ;
}

bool PGPHandler::exportPublicKey(const RsPgpId& id,unsigned char *& mem_block,size_t& mem_size,bool armoured,bool include_signatures) const
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
	const ops_keydata_t *key = locked_getPublicKey(id,false) ;
	mem_block = NULL ;

	if(armoured)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": should not be used with armoured=true, because there's a bug in the armoured export of OPS" << std::endl;
		return false ;
	}

	if(key == NULL)
	{
		std::cerr << "Cannot output key " << id.toStdString() << ": not found in keyring." << std::endl;
		return false ;
	}

   ops_create_info_t* cinfo;
	ops_memory_t *buf = NULL ;
   ops_setup_memory_write(&cinfo, &buf, 0);

	if(ops_write_transferable_public_key_from_packet_data(key,armoured,cinfo) != ops_true)
	{
		std::cerr << "ERROR: This key cannot be processed by RetroShare because\nDSA certificates are not yet handled." << std::endl;
		return false ;
	}

	ops_writer_close(cinfo) ;

	mem_block = new unsigned char[ops_memory_get_length(buf)] ;
	mem_size = ops_memory_get_length(buf) ;
	memcpy(mem_block,ops_memory_get_data(buf),mem_size) ;

   ops_teardown_memory_write(cinfo,buf);

	if(!include_signatures)
	{
		size_t new_size ;
		PGPKeyManagement::findLengthOfMinimalKey(mem_block,mem_size,new_size) ;
		mem_size = new_size ;
	}

	return true ;
}

bool PGPHandler::exportGPGKeyPair(const std::string& filename,const RsPgpId& exported_key_id) const
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	const ops_keydata_t *pubkey = locked_getPublicKey(exported_key_id,false) ;

	if(pubkey == NULL)
	{
		std::cerr << "Cannot output key " << exported_key_id.toStdString() << ": not found in public keyring." << std::endl;
		return false ;
	}
	const ops_keydata_t *seckey = locked_getSecretKey(exported_key_id) ;

	if(seckey == NULL)
	{
		std::cerr << "Cannot output key " << exported_key_id.toStdString() << ": not found in secret keyring." << std::endl;
		return false ;
	}

	FILE *f = RsDirUtil::rs_fopen(filename.c_str(),"w") ;
	if(f == NULL)
	{
		std::cerr << "Cannot output key " << exported_key_id.toStdString() << ": file " << filename << " cannot be written. Please check for permissions, quotas, disk space." << std::endl;
		return false ;
	}

	fprintf(f,"%s\n", makeRadixEncodedPGPKey(pubkey,true).c_str()) ; 
	fprintf(f,"%s\n", makeRadixEncodedPGPKey(seckey,true).c_str()) ; 

	fclose(f) ;
	return true ;
}

bool PGPHandler::exportGPGKeyPairToString(
        std::string& data, const RsPgpId& exportedKeyId,
        bool includeSignatures, std::string& errorMsg ) const
{
	RS_STACK_MUTEX(pgphandlerMtx);

	const ops_keydata_t *pubkey = locked_getPublicKey(exportedKeyId,false);

	if(!pubkey)
	{
		errorMsg = "Cannot output key " + exportedKeyId.toStdString() +
		           ": not found in public keyring.";
		return false;
	}
	const ops_keydata_t *seckey = locked_getSecretKey(exportedKeyId);

	if(!seckey)
	{
		errorMsg = "Cannot output key " + exportedKeyId.toStdString() +
		           ": not found in secret keyring.";
		return false;
	}

	data  = makeRadixEncodedPGPKey(pubkey, includeSignatures);
	data += "\n";
	data += makeRadixEncodedPGPKey(seckey, includeSignatures);
	data += "\n";
	return true;
}

bool PGPHandler::getGPGDetailsFromBinaryBlock(const unsigned char *mem_block,size_t mem_size,RsPgpId& key_id, std::string& name, std::list<RsPgpId>& signers) const
{
	ops_keyring_t *tmp_keyring = allocateOPSKeyring();
	ops_memory_t *mem = ops_memory_new() ;
	ops_memory_add(mem,mem_block,mem_size);

	if(!ops_keyring_read_from_mem(tmp_keyring,ops_false,mem))
	{
		ops_keyring_free(tmp_keyring) ;
		free(tmp_keyring) ;
		ops_memory_release(mem) ;
		free(mem) ;

		std::cerr << "Could not read key. Format error?" << std::endl;
		//error_string = std::string("Could not read key. Format error?") ;
		return false ;
	}
	ops_memory_release(mem) ;
	free(mem) ;
	//error_string.clear() ;

	if(tmp_keyring->nkeys != 1)
	{
		std::cerr << "No or incomplete/invalid key in supplied pgp block." << std::endl;
		return false ;
	}
	if(tmp_keyring->keys[0].uids == NULL)
	{
		std::cerr << "No uid in supplied key." << std::endl;
		return false ;
	}

	key_id = RsPgpId(tmp_keyring->keys[0].key_id) ;
	name = std::string((char *)tmp_keyring->keys[0].uids[0].user_id) ;

	// now parse signatures.
	//
	ops_validate_result_t* result=(ops_validate_result_t*)ops_mallocz(sizeof *result);
	ops_boolean_t res ;

	{
		RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
		res = ops_validate_key_signatures(result,&tmp_keyring->keys[0],_pubring,cb_get_passphrase) ;
	}

	if(res == ops_false)
		std::cerr << "(WW) Error in PGPHandler::validateAndUpdateSignatures(). Validation failed for at least some signatures." << std::endl;

	// also add self-signature if any (there should be!).
	//
	res = ops_validate_key_signatures(result,&tmp_keyring->keys[0],tmp_keyring,cb_get_passphrase) ;

	if(res == ops_false)
		std::cerr << "(WW) Error in PGPHandler::validateAndUpdateSignatures(). Validation failed for at least some signatures." << std::endl;

	// Parse signers.
	//

	std::set<RsPgpId> signers_set ;	// Use a set to remove duplicates.

	if(result != NULL)
		for(size_t i=0;i<result->valid_count;++i)
			signers_set.insert(RsPgpId(result->valid_sigs[i].signer_id)) ;

	ops_validate_result_free(result) ;

	ops_keyring_free(tmp_keyring) ;
	free(tmp_keyring) ;

	// write to the output variable
	
	signers.clear() ;

	for(std::set<RsPgpId>::const_iterator it(signers_set.begin());it!=signers_set.end();++it)
		signers.push_back(*it) ;

	return true ;
}

bool PGPHandler::importGPGKeyPair(const std::string& filename,RsPgpId& imported_key_id,std::string& import_error)
{
	import_error = "" ;

	// 1 - Test for file existance
	//
	FILE *ftest = RsDirUtil::rs_fopen(filename.c_str(),"r") ;

	if(ftest == NULL)
	{
		import_error = "Cannot open file " + filename + " for read. Please check access permissions." ;
		return false ;
	}

	fclose(ftest) ;

	// 2 - Read keyring from supplied file.
	//
	ops_keyring_t *tmp_keyring = allocateOPSKeyring();

	if(ops_false == ops_keyring_read_from_file(tmp_keyring, ops_true, filename.c_str()))
	{
		import_error = "PGPHandler::readKeyRing(): cannot read key file. File corrupted?" ;
        free(tmp_keyring);
		return false ;
	}

    return checkAndImportKeyPair(tmp_keyring, imported_key_id, import_error);
}

bool PGPHandler::importGPGKeyPairFromString(const std::string &data, RsPgpId &imported_key_id, std::string &import_error)
{
    import_error = "" ;

    ops_memory_t* mem = ops_memory_new();
    ops_memory_add(mem, (unsigned char*)data.data(), data.length());

    ops_keyring_t *tmp_keyring = allocateOPSKeyring();

    if(ops_false == ops_keyring_read_from_mem(tmp_keyring, ops_true, mem))
    {
        import_error = "PGPHandler::importGPGKeyPairFromString(): cannot parse key data" ;
        free(tmp_keyring);
        return false ;
    }
    return checkAndImportKeyPair(tmp_keyring, imported_key_id, import_error);
}

bool PGPHandler::checkAndImportKeyPair(ops_keyring_t *tmp_keyring, RsPgpId &imported_key_id, std::string &import_error)
{
    if(tmp_keyring == 0)
    {
        import_error = "PGPHandler::checkAndImportKey(): keyring is null" ;
        return false;
    }

	if(tmp_keyring->nkeys != 2)
	{
		import_error = "PGPHandler::importKeyPair(): file does not contain a valid keypair." ;
		if(tmp_keyring->nkeys > 2)
			import_error += "\nMake sure that your key is a RSA key (DSA is not yet supported) and does not contain subkeys (not supported yet).";
		return false ;
	}

	// 3 - Test that keyring contains a valid keypair.
	//
	const ops_keydata_t *pubkey = NULL ;
	const ops_keydata_t *seckey = NULL ;

	if(tmp_keyring->keys[0].type == OPS_PTAG_CT_PUBLIC_KEY) 
		pubkey = &tmp_keyring->keys[0] ;
	else if(tmp_keyring->keys[0].type == OPS_PTAG_CT_ENCRYPTED_SECRET_KEY) 
		seckey = &tmp_keyring->keys[0] ;
	else
	{
		import_error = "Unrecognised key type in key file for key #0. Giving up." ;
		std::cerr << "Unrecognised key type " << tmp_keyring->keys[0].type << " in key file for key #0. Giving up." << std::endl;
		return false ;
	}
	if(tmp_keyring->keys[1].type == OPS_PTAG_CT_PUBLIC_KEY) 
		pubkey = &tmp_keyring->keys[1] ;
	else if(tmp_keyring->keys[1].type == OPS_PTAG_CT_ENCRYPTED_SECRET_KEY) 
		seckey = &tmp_keyring->keys[1] ;
	else
	{
		import_error = "Unrecognised key type in key file for key #1. Giving up." ;
		std::cerr << "Unrecognised key type " << tmp_keyring->keys[1].type << " in key file for key #1. Giving up." << std::endl;
		return false ;
	}

	if(pubkey == nullptr || seckey == nullptr || pubkey == seckey)
	{
		import_error = "File does not contain a public and a private key. Sorry." ;
		return false ;
	}
	if(memcmp( pubkey->fingerprint.fingerprint,
	           seckey->fingerprint.fingerprint,
	           RsPgpFingerprint::SIZE_IN_BYTES ) != 0)
	{
		import_error = "Public and private keys do nt have the same fingerprint. Sorry!" ;
		return false ;
	}
	if(pubkey->key.pkey.version != 4)
	{
		import_error = "Public key is not version 4. Rejected!" ;
		return false ;
	}

	// 4 - now check self-signature for this keypair. For this we build a dummy keyring containing only the key.
	//
	ops_validate_result_t *result=(ops_validate_result_t*)ops_mallocz(sizeof *result);

	ops_keyring_t dummy_keyring ;
	dummy_keyring.nkeys=1 ;
	dummy_keyring.nkeys_allocated=1 ;
	dummy_keyring.keys=const_cast<ops_keydata_t*>(pubkey) ;

	ops_validate_key_signatures(result, const_cast<ops_keydata_t*>(pubkey), &dummy_keyring, cb_get_passphrase) ;
	
	// Check that signatures contain at least one certification from the user id.
	//
	bool found = false ;

	for(uint32_t i=0;i<result->valid_count;++i)
		if(!memcmp(
		            static_cast<uint8_t*>(result->valid_sigs[i].signer_id),
		            pubkey->key_id,
		            RsPgpFingerprint::SIZE_IN_BYTES ))
		{
			found = true ;
			break ;
		}

	if(!found)
	{
		import_error = "Cannot validate self signature for the imported key. Sorry." ;
		return false ;
	}
	ops_validate_result_free(result);

	if(!RsDiscSpace::checkForDiscSpace(RS_PGP_DIRECTORY))
	{
		import_error = std::string("(EE) low disc space in pgp directory. Can't write safely to keyring.") ;
		return false ;
	}
	// 5 - All test passed. Adding key to keyring.
	//
	{
		RsStackMutex mtx(pgphandlerMtx) ;					// lock access to PGP memory structures.

		imported_key_id = RsPgpId(pubkey->key_id) ;

		if(locked_getSecretKey(imported_key_id) == NULL)
		{
			RsStackFileLock flck(_pgp_lock_filename) ;	// lock access to PGP directory.

			ops_create_info_t *cinfo = NULL ;

			// Make a copy of the secret keyring
			//
			std::string secring_path_tmp = _secring_path + ".tmp" ;
			if(RsDirUtil::fileExists(_secring_path) && !RsDirUtil::copyFile(_secring_path,secring_path_tmp)) 
			{
				import_error = "(EE) Cannot write secret key to disk!! Disk full? Out of disk quota. Keyring will be left untouched." ;
				return false ;
			}

			// Append the new key

			int fd=ops_setup_file_append(&cinfo, secring_path_tmp.c_str());

			if(!ops_write_transferable_secret_key_from_packet_data(seckey,ops_false,cinfo))
			{
				import_error = "(EE) Cannot encode secret key to disk!! Disk full? Out of disk quota?" ;
				return false ;
			}
			ops_teardown_file_write(cinfo,fd) ;

			// Rename the new keyring to overwrite the old one.
			//
			if(!RsDirUtil::renameFile(secring_path_tmp,_secring_path))
			{
				import_error = "  (EE) Cannot move temp file " + secring_path_tmp + ". Bad write permissions?" ;
				return false ;
			}

			addNewKeyToOPSKeyring(_secring,*seckey) ;
			initCertificateInfo(_secret_keyring_map[ imported_key_id ],seckey,_secring->nkeys-1) ;
		}
		else
			import_error = "Private key already exists! Not importing it again." ;

		if(locked_addOrMergeKey(_pubring,_public_keyring_map,pubkey))
			_pubring_changed = true ;
	}

	// 6 - clean
	//
	ops_keyring_free(tmp_keyring) ;
    free(tmp_keyring);

    // write public key to disk
    syncDatabase();

	return true ;
}

void PGPHandler::addNewKeyToOPSKeyring(ops_keyring_t *kr,const ops_keydata_t& key)
{
	if(kr->nkeys >= kr->nkeys_allocated)
	{
		kr->keys = (ops_keydata_t *)realloc(kr->keys,(kr->nkeys+1)*sizeof(ops_keydata_t)) ; 
		kr->nkeys_allocated = kr->nkeys+1;
	}
	memset(&kr->keys[kr->nkeys],0,sizeof(ops_keydata_t)) ;
	ops_keydata_copy(&kr->keys[kr->nkeys],&key) ;
	kr->nkeys++ ;
}

bool PGPHandler::LoadCertificateFromString(const std::string& pgp_cert,RsPgpId& id,std::string& error_string)
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
#ifdef DEBUG_PGPHANDLER
	std::cerr << "Reading new key from string: " << std::endl;
#endif

	ops_keyring_t *tmp_keyring = allocateOPSKeyring();
	ops_memory_t *mem = ops_memory_new() ;
	ops_memory_add(mem,(unsigned char *)pgp_cert.c_str(),pgp_cert.length()) ;

	if(!ops_keyring_read_from_mem(tmp_keyring,ops_true,mem))
	{
		ops_keyring_free(tmp_keyring) ;
		free(tmp_keyring) ;
		ops_memory_release(mem) ;
		free(mem) ;

		std::cerr << "Could not read key. Format error?" << std::endl;
		error_string = std::string("Could not read key. Format error?") ;
		return false ;
	}
	ops_memory_release(mem) ;
	free(mem) ;
	error_string.clear() ;

	// Check that there is exactly one key in this data packet.
	//
	if(tmp_keyring->nkeys != 1)
	{
		std::cerr << "Loaded certificate contains more than one PGP key. This is not allowed." << std::endl;
		error_string = "Loaded certificate contains more than one PGP key. This is not allowed." ;
		return false ;
	}

	const ops_keydata_t *keydata = ops_keyring_get_key_by_index(tmp_keyring,0);

	// Check that the key is a version 4 key
	//
	if(keydata->key.pkey.version != 4)
	{
		error_string = "Public key is not version 4. Rejected!" ;
		std::cerr << "Received a key with unhandled version number (" << keydata->key.pkey.version << ")" << std::endl;
		return false ;
	}

	// Check that the key is correctly self-signed.
	//
	ops_validate_result_t* result=(ops_validate_result_t*)ops_mallocz(sizeof *result);

    ops_validate_key_signatures(result,keydata,tmp_keyring,cb_get_passphrase) ;

	bool found = false ;

	for(uint32_t i=0;i<result->valid_count;++i)
		if(!memcmp(
		            static_cast<uint8_t*>(result->valid_sigs[i].signer_id),
		            keydata->key_id,
		            RsPgpFingerprint::SIZE_IN_BYTES ))
		{
			found = true ;
			break ;
		}

	if(!found)
	{
		error_string = "This key is not self-signed. This is required by Retroshare." ;
		std::cerr <<   "This key is not self-signed. This is required by Retroshare." << std::endl;
		ops_validate_result_free(result);
		return false ;
	}
	ops_validate_result_free(result);

#ifdef DEBUG_PGPHANDLER
	std::cerr << "  Key read correctly: " << std::endl;
	ops_keyring_list(tmp_keyring) ;
#endif

	int i=0 ;

	while( (keydata = ops_keyring_get_key_by_index(tmp_keyring,i++)) != NULL )
		if(locked_addOrMergeKey(_pubring,_public_keyring_map,keydata)) 
		{
			_pubring_changed = true ;
#ifdef DEBUG_PGPHANDLER
			std::cerr << "  Added the key in the main public keyring." << std::endl;
#endif
		}
		else
			std::cerr << "Key already in public keyring." << std::endl;
	
	if(tmp_keyring->nkeys > 0)
		id = RsPgpId(tmp_keyring->keys[0].key_id) ;
	else
		return false ;

	ops_keyring_free(tmp_keyring) ;
	free(tmp_keyring) ;

	_pubring_changed = true ;

	return true ;
}

bool PGPHandler::locked_addOrMergeKey(ops_keyring_t *keyring,std::map<RsPgpId,PGPCertificateInfo>& kmap,const ops_keydata_t *keydata)
{
	bool ret = false ;
	RsPgpId id(keydata->key_id) ;

#ifdef DEBUG_PGPHANDLER
	std::cerr << "AddOrMergeKey():" << std::endl;
	std::cerr << "  id: " << id.toStdString() << std::endl;
#endif

	// See if the key is already in the keyring
	const ops_keydata_t *existing_key = NULL;
	std::map<RsPgpId,PGPCertificateInfo>::const_iterator res = kmap.find(id) ;

	// Checks that
	// 	- the key is referenced by keyid
	// 	- the map is initialized
	// 	- the fingerprint matches!
	//
	if(res == kmap.end() || (existing_key = ops_keyring_get_key_by_index(keyring,res->second._key_index)) == NULL)
	{
#ifdef DEBUG_PGPHANDLER
		std::cerr << "  Key is new. Adding it to keyring" << std::endl;
#endif
		addNewKeyToOPSKeyring(keyring,*keydata) ; // the key is new.
		initCertificateInfo(kmap[id],keydata,keyring->nkeys-1) ;
		existing_key = &(keyring->keys[keyring->nkeys-1]) ;
		ret = true ;
	}
	else
	{
		if(memcmp( existing_key->fingerprint.fingerprint,
		           keydata->fingerprint.fingerprint,
		           RsPgpFingerprint::SIZE_IN_BYTES ))
		{
			std::cerr << "(EE) attempt to merge key with identical id, but different fingerprint!" << std::endl;
			return false ;
		}

#ifdef DEBUG_PGPHANDLER
		std::cerr << "  Key exists. Merging signatures." << std::endl;
#endif
		ret = mergeKeySignatures(const_cast<ops_keydata_t*>(existing_key),keydata) ;

		if(ret)
			initCertificateInfo(kmap[id],existing_key,res->second._key_index) ;
	}

	if(ret)
	{
		validateAndUpdateSignatures(kmap[id],existing_key) ;
		kmap[id]._time_stamp = time(NULL) ;
	}

	return ret ;
}
//   bool PGPHandler::encryptTextToString(const RsPgpId& key_id,const std::string& text,std::string& outstring) 
//   {
//   	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
//   
//   	const ops_keydata_t *public_key = getPublicKey(key_id) ;
//   
//   	if(public_key == NULL)
//   	{
//   		std::cerr << "Cannot get public key of id " << key_id.toStdString() << std::endl;
//   		return false ;
//   	}
//   
//   	if(public_key->type != OPS_PTAG_CT_PUBLIC_KEY)
//   	{
//   		std::cerr << "PGPHandler::encryptTextToFile(): ERROR: supplied id did not return a public key!" << std::endl;
//   		return false ;
//   	}
//   
//   	ops_create_info_t *info;
//   	ops_memory_t *buf = NULL ;
//      ops_setup_memory_write(&info, &buf, 0);
//   
//   	ops_encrypt_stream(info, public_key, NULL, ops_false, ops_true);
//   	ops_write(text.c_str(), text.length(), info);
//   	ops_writer_close(info);
//   
//   	outstring = std::string((char *)ops_memory_get_data(buf),ops_memory_get_length(buf)) ;
//   	ops_create_info_delete(info);
//   
//   	return true ;
//   }
bool PGPHandler::encryptTextToFile(const RsPgpId& key_id,const std::string& text,const std::string& outfile) 
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	const ops_keydata_t *public_key = locked_getPublicKey(key_id,true) ;

	if(public_key == NULL)
	{
		std::cerr << "Cannot get public key of id " << key_id.toStdString() << std::endl;
		return false ;
	}

	if(public_key->type != OPS_PTAG_CT_PUBLIC_KEY)
	{
		std::cerr << "PGPHandler::encryptTextToFile(): ERROR: supplied id did not return a public key!" << std::endl;
		return false ;
	}

	std::string outfile_tmp = outfile + ".tmp" ;

	ops_create_info_t *info;
	int fd = ops_setup_file_write(&info, outfile_tmp.c_str(), ops_true);

	if (fd < 0)
	{
		std::cerr << "PGPHandler::encryptTextToFile(): ERROR: Cannot write to " << outfile_tmp << std::endl;
		return false ;
	}

	if(!ops_encrypt_stream(info, public_key, NULL, ops_false, ops_true))
	{
		std::cerr << "PGPHandler::encryptTextToFile(): ERROR: encryption failed." << std::endl;
		return false ;
	}

	ops_write(text.c_str(), text.length(), info);
	ops_teardown_file_write(info, fd);

	if(!RsDirUtil::renameFile(outfile_tmp,outfile))
	{
		std::cerr << "PGPHandler::encryptTextToFile(): ERROR: Cannot rename " + outfile_tmp + " to " + outfile + ". Disk error?" << std::endl;
		return false ;
	}

	return true ;
}

bool PGPHandler::encryptDataBin(const RsPgpId& key_id,const void *data, const uint32_t len, unsigned char *encrypted_data, unsigned int *encrypted_data_len) 
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	const ops_keydata_t *public_key = locked_getPublicKey(key_id,true) ;

	if(public_key == NULL)
	{
		std::cerr << "Cannot get public key of id " << key_id.toStdString() << std::endl;
		return false ;
	}

	if(public_key->type != OPS_PTAG_CT_PUBLIC_KEY)
	{
		std::cerr << "PGPHandler::encryptTextToFile(): ERROR: supplied id did not return a public key!" << std::endl;
		return false ;
	}
	if(public_key->key.pkey.algorithm != OPS_PKA_RSA)
	{
		std::cerr << "PGPHandler::encryptTextToFile(): ERROR: supplied key id " << key_id.toStdString() << " is not an RSA key (DSA for instance, is not supported)!" << std::endl;
		return false ;
	}
	ops_create_info_t *info;
	ops_memory_t *buf = NULL ;
   ops_setup_memory_write(&info, &buf, 0);
	bool res = true;

	if(!ops_encrypt_stream(info, public_key, NULL, ops_false, ops_false))
	{
		std::cerr << "Encryption failed." << std::endl;
		res = false ;
	}

	ops_write(data,len,info);
	ops_writer_close(info);
	ops_create_info_delete(info);

	int tlen = ops_memory_get_length(buf) ;

	if( (int)*encrypted_data_len >= tlen)
	{
		if(res)
		{
			memcpy(encrypted_data,ops_memory_get_data(buf),tlen) ;
			*encrypted_data_len = tlen ;
			res = true ;
		}
	}
	else
	{
		std::cerr << "Not enough room to fit encrypted data. Size given=" << *encrypted_data_len << ", required=" << tlen << std::endl;
		res = false ;
	}

	ops_memory_release(buf) ;
	free(buf) ;

	return res ;
}

bool PGPHandler::decryptDataBin(const RsPgpId& /*key_id*/,const void *encrypted_data, const uint32_t encrypted_len, unsigned char *data, unsigned int *data_len)
{
	int out_length ;
	unsigned char *out ;
	ops_boolean_t res = ops_decrypt_memory((const unsigned char *)encrypted_data,encrypted_len,&out,&out_length,_secring,ops_false,cb_get_passphrase) ;

	if(*data_len < (unsigned int)out_length)
	{
		std::cerr << "Not enough room to store decrypted data! Please give more."<< std::endl;
		return false ;
	}

	*data_len = (unsigned int)out_length ;
	memcpy(data,out,out_length) ;
	free(out) ;

	return (bool)res ;
}

bool PGPHandler::decryptTextFromFile(const RsPgpId&,std::string& text,const std::string& inputfile) 
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	unsigned char *out_buf = NULL ;
	std::string buf ;

	FILE *f = RsDirUtil::rs_fopen(inputfile.c_str(),"rb") ;

	if (f == NULL)
	{
        std::cerr << "Cannot open file " << inputfile << " for read." << std::endl;
		return false;
	}

	int c ;
	while( (c = fgetc(f))!= EOF)
		buf += (unsigned char)c;

	fclose(f) ;

#ifdef DEBUG_PGPHANDLER
	std::cerr << "PGPHandler::decryptTextFromFile: read a file of length " << std::dec << buf.length() << std::endl;
	std::cerr << "buf=\"" << buf << "\"" << std::endl;
#endif

	int out_length ;
	ops_boolean_t res = ops_decrypt_memory((const unsigned char *)buf.c_str(),buf.length(),&out_buf,&out_length,_secring,ops_true,cb_get_passphrase) ;

	text = std::string((char *)out_buf,out_length) ;
	free (out_buf);
	return (bool)res ;
}

bool PGPHandler::SignDataBin(const RsPgpId& id,const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen,bool use_raw_signature, std::string reason /* = "" */)
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
	// need to find the key and to decrypt it.
	
	const ops_keydata_t *key = locked_getSecretKey(id) ;

	if(!key)
	{
		std::cerr << "Cannot sign: no secret key with id " << id.toStdString() << std::endl;
		return false ;
	}

	std::string uid_hint ;
	if(key->nuids > 0)
		uid_hint = std::string((const char *)key->uids[0].user_id) ;
	uid_hint += "(" + RsPgpId(key->key_id).toStdString()+")" ;

#ifdef DEBUG_PGPHANDLER
	ops_fingerprint_t f ;
	ops_fingerprint(&f,&key->key.pkey) ; 

	PGPFingerprintType fp(f.fingerprint) ;
#endif

    bool last_passwd_was_wrong = false ;
ops_secret_key_t *secret_key = NULL ;

    for(int i=0;i<3;++i)
    {
        bool cancelled =false;
        std::string passphrase = _passphrase_callback(NULL,reason.c_str(),uid_hint.c_str(),"Please enter passwd for encrypting your key : ",last_passwd_was_wrong,&cancelled) ;//TODO reason

        secret_key = ops_decrypt_secret_key_from_data(key,passphrase.c_str()) ;

        if(cancelled)
        {
            std::cerr << "Key entering cancelled" << std::endl;
            return false ;
        }
        if(secret_key)
            break ;

        std::cerr << "Key decryption went wrong. Wrong passwd?" << std::endl;
        last_passwd_was_wrong = true ;
    }
    if(!secret_key)
    {
        std::cerr << "Could not obtain secret key. Signature cancelled." << std::endl;
        return false ;
    }

	// then do the signature.

	ops_boolean_t not_raw = !use_raw_signature ;
#ifdef V07_NON_BACKWARD_COMPATIBLE_CHANGE_002
	ops_memory_t *memres = ops_sign_buf(data,len,OPS_SIG_BINARY,OPS_HASH_SHA256,secret_key,ops_false,ops_false,not_raw,not_raw) ;
#else
	ops_memory_t *memres = ops_sign_buf(data,len,OPS_SIG_BINARY,OPS_HASH_SHA1,secret_key,ops_false,ops_false,not_raw,not_raw) ;
#endif

	if(!memres)
		return false ;

	bool res ;
	uint32_t slen = (uint32_t)ops_memory_get_length(memres);

	if(*signlen >= slen)
	{
		*signlen = slen ;

		memcpy(sign,ops_memory_get_data(memres),*signlen) ;
		res = true ;
	}
	else
	{
		std::cerr << "(EE) memory chunk is not large enough for signature packet. Requred size: " << slen << " bytes." << std::endl;
		res = false ;
	}

	ops_memory_release(memres) ;
	free(memres) ;
	ops_secret_key_free(secret_key) ;
	free(secret_key) ;

#ifdef DEBUG_PGPHANDLER
	std::cerr << "Signed with fingerprint " << fp.toStdString() << ", length " << std::dec << *signlen << ", literal data length = " << len << std::endl;
	std::cerr << "Signature body: " << std::endl;
	hexdump( (unsigned char *)data,     len) ;
	std::cerr << std::endl;
	std::cerr << "Data: " << std::endl;
	hexdump( (unsigned char *)sign,*signlen) ;
	std::cerr << std::endl;
#endif
	return res ;
}

bool PGPHandler::privateSignCertificate(const RsPgpId& ownId,const RsPgpId& id_of_key_to_sign) 
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	ops_keydata_t *key_to_sign = const_cast<ops_keydata_t*>(locked_getPublicKey(id_of_key_to_sign,true)) ;

	if(key_to_sign == NULL)
	{
		std::cerr << "Cannot sign: no public key with id " << id_of_key_to_sign.toStdString() << std::endl;
		return false ;
	}

	// 1 - get decrypted secret key
	//
	const ops_keydata_t *skey = locked_getSecretKey(ownId) ;

	if(!skey)
	{
		std::cerr << "Cannot sign: no secret key with id " << ownId.toStdString() << std::endl;
		return false ;
	}
	const ops_keydata_t *pkey = locked_getPublicKey(ownId,true) ;

	if(!pkey)
	{
		std::cerr << "Cannot sign: no public key with id " << ownId.toStdString() << std::endl;
		return false ;
	}

	bool cancelled = false;
	std::string passphrase = _passphrase_callback(NULL,"",RsPgpId(skey->key_id).toStdString().c_str(),"Please enter passwd for encrypting your key : ",false,&cancelled) ;

	ops_secret_key_t *secret_key = ops_decrypt_secret_key_from_data(skey,passphrase.c_str()) ;

    if(cancelled)
    {
        std::cerr << "Key cancelled by used." << std::endl;
        return false ;
    }
    if(!secret_key)
	{
		std::cerr << "Key decryption went wrong. Wrong passwd?" << std::endl;
		return false ;
	}

	// 2 - then do the signature.

	if(!ops_sign_key(key_to_sign,pkey->key_id,secret_key)) 
	{
		std::cerr << "Key signature went wrong. Wrong passwd?" << std::endl;
		return false ;
	}

	// 3 - free memory
	//
	ops_secret_key_free(secret_key) ;
	free(secret_key) ;

	_pubring_changed = true ;

	// 4 - update signatures.
	//
	PGPCertificateInfo& cert(_public_keyring_map[ id_of_key_to_sign ]) ;
	validateAndUpdateSignatures(cert,key_to_sign) ;
	cert._flags |= PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE ;

	return true ;
}

void PGPHandler::updateOwnSignatureFlag(const RsPgpId& own_id) 
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

    if(_public_keyring_map.find(own_id)==_public_keyring_map.end())
    {
        std::cerr << __func__ << ": key with id=" << own_id.toStdString() << " not in keyring." << std::endl;
        // return now, because the following operation would add an entry to _public_keyring_map
        return;
    }

	PGPCertificateInfo& own_cert(_public_keyring_map[ own_id ]) ;

	for(std::map<RsPgpId,PGPCertificateInfo>::iterator it=_public_keyring_map.begin();it!=_public_keyring_map.end();++it)
		locked_updateOwnSignatureFlag(it->second,it->first,own_cert,own_id) ;
}
void PGPHandler::updateOwnSignatureFlag(const RsPgpId& cert_id,const RsPgpId& own_id)
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	std::map<RsPgpId,PGPCertificateInfo>::iterator it( _public_keyring_map.find(cert_id) ) ;

	if(it == _public_keyring_map.end())
	{
		std::cerr << "updateOwnSignatureFlag: Cannot get certificate for string " << cert_id.toStdString() << ". This is probably a bug." << std::endl; 
		return ;
	}

	PGPCertificateInfo& cert( it->second );

	PGPCertificateInfo& own_cert(_public_keyring_map[ own_id ]) ;

	locked_updateOwnSignatureFlag(cert,cert_id,own_cert,own_id) ;
}
void PGPHandler::locked_updateOwnSignatureFlag(PGPCertificateInfo& cert,const RsPgpId& cert_id,PGPCertificateInfo& own_cert,const RsPgpId& own_id_str)
{
	if(cert.signers.find(own_id_str) != cert.signers.end())
		cert._flags |= PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE ;
	else
		cert._flags &= ~PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE ;

	if(own_cert.signers.find( cert_id ) != own_cert.signers.end())
		cert._flags |= PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_SIGNED_ME ;
	else
		cert._flags &= ~PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_SIGNED_ME ;
}

bool PGPHandler::getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp) const
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	const ops_keydata_t *key = locked_getPublicKey(id,false) ;

	if(key == NULL)
		return false ;

	ops_fingerprint_t f ;
	ops_fingerprint(&f,&key->key.pkey) ; 

	fp = PGPFingerprintType(f.fingerprint) ;

	return true ;
}

bool PGPHandler::VerifySignBin(const void *literal_data, uint32_t literal_data_length, unsigned char *sign, unsigned int sign_len, const PGPFingerprintType& key_fingerprint)
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	RsPgpId id = RsPgpId(key_fingerprint.toByteArray() + PGPFingerprintType::SIZE_IN_BYTES - RsPgpId::SIZE_IN_BYTES) ;
	const ops_keydata_t *key = locked_getPublicKey(id,true) ;

	if(key == NULL)
	{
		std::cerr << "No key returned by fingerprint " << key_fingerprint.toStdString() << ", and ID " << id.toStdString() << ", signature verification failed!" << std::endl;
		return false ;
	}

	// Check that fingerprint is the same.
	const ops_public_key_t *pkey = &key->key.pkey ;
	ops_fingerprint_t fp ;
	ops_fingerprint(&fp,pkey) ; 

	if(key_fingerprint != PGPFingerprintType(fp.fingerprint))
	{
		std::cerr << "Key fingerprint does not match " << key_fingerprint.toStdString() << ", for ID " << id.toStdString() << ", signature verification failed!" << std::endl;
		return false ;
	}

#ifdef DEBUG_PGPHANDLER
	std::cerr << "Verifying signature from fingerprint " << key_fingerprint.toStdString() << ", length " << std::dec << sign_len << ", literal data length = " << literal_data_length << std::endl;
	std::cerr << "Signature body: " << std::endl;
	hexdump( (unsigned char *)sign,sign_len) ;
	std::cerr << std::endl;
	std::cerr << "Signed data: " << std::endl;
	hexdump( (unsigned char *)literal_data,     literal_data_length) ;
	std::cerr << std::endl;
#endif

	return ops_validate_detached_signature(literal_data,literal_data_length,sign,sign_len,key) ;
}

void PGPHandler::setAcceptConnexion(const RsPgpId& id,bool b)
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	std::map<RsPgpId,PGPCertificateInfo>::iterator res = _public_keyring_map.find(id) ;

	if(res != _public_keyring_map.end())
	{
		if(b)
			res->second._flags |= PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION ;
		else
			res->second._flags &= ~PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION ;
	}
}

bool PGPHandler::getGPGFilteredList(std::list<RsPgpId>& list,bool (*filter)(const PGPCertificateInfo&)) const
{
	RsStackMutex mtx(pgphandlerMtx) ;	// lock access to PGP directory.
	list.clear() ;

	for(std::map<RsPgpId,PGPCertificateInfo>::const_iterator it(_public_keyring_map.begin());it!=_public_keyring_map.end();++it)
		if( filter == NULL || (*filter)(it->second) )
			list.push_back(RsPgpId(it->first)) ;

	return true ;
}

bool PGPHandler::isGPGId(const RsPgpId &id)
{
	return _public_keyring_map.find(id) != _public_keyring_map.end() ;
}

bool PGPHandler::isGPGSigned(const RsPgpId &id)
{
	std::map<RsPgpId,PGPCertificateInfo>::const_iterator res = _public_keyring_map.find(id) ;
	return res != _public_keyring_map.end() && (res->second._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE) ;
}

bool PGPHandler::isGPGAccepted(const RsPgpId &id)
{
	std::map<RsPgpId,PGPCertificateInfo>::const_iterator res = _public_keyring_map.find(id) ;
	return (res != _public_keyring_map.end()) && (res->second._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION) ;
}

// Lexicographic order on signature packets
//
bool operator<(const ops_packet_t& p1,const ops_packet_t& p2)
{
	if(p1.length < p2.length)
		return true ;
	if(p1.length > p2.length)
		return false ;

	for(uint32_t i=0;i<p1.length;++i)
	{
		if(p1.raw[i] < p2.raw[i])
			return true ;
		if(p1.raw[i] > p2.raw[i])
			return false ;
	}
	return false ;
}

bool PGPHandler::mergeKeySignatures(ops_keydata_t *dst,const ops_keydata_t *src)
{
	// First sort all signatures into lists to see which is new, which is not new

#ifdef DEBUG_PGPHANDLER
	std::cerr << "Merging signatures for key " << RsPgpId(dst->key_id).toStdString() << std::endl;
#endif
	std::set<ops_packet_t> dst_packets ;

	for(uint32_t i=0;i<dst->npackets;++i) dst_packets.insert(dst->packets[i]) ;

	std::set<ops_packet_t> to_add ;

	for(uint32_t i=0;i<src->npackets;++i) 
		if(dst_packets.find(src->packets[i]) == dst_packets.end())
		{
			uint8_t tag ;
			uint32_t length ;
			unsigned char *tmp_data = src->packets[i].raw ; // put it in a tmp variable because read_packetHeader() will modify it!!

			PGPKeyParser::read_packetHeader(tmp_data,tag,length) ;

			if(tag == PGPKeyParser::PGP_PACKET_TAG_SIGNATURE)
				to_add.insert(src->packets[i]) ;
#ifdef DEBUG_PGPHANDLER
			else
				std::cerr << "  Packet with tag 0x" << std::hex << (int)(src->packets[i].raw[0]) << std::dec << " not merged, because it is not a signature." << std::endl;
#endif
		}

	for(std::set<ops_packet_t>::const_iterator it(to_add.begin());it!=to_add.end();++it)
	{
#ifdef DEBUG_PGPHANDLER
		std::cerr << "  Adding packet with tag 0x" << std::hex << (int)(*it).raw[0] << std::dec << std::endl;
#endif
		ops_add_packet_to_keydata(dst,&*it) ;
	}
	return to_add.size() > 0 ;
}

bool PGPHandler::parseSignature(unsigned char *sign, unsigned int signlen,RsPgpId& issuer_id) 
{
	PGPSignatureInfo info ;
    
    if(!PGPKeyManagement::parseSignature(sign,signlen,info))
        return false ;
    
    unsigned char bytes[8] ;
    for(int i=0;i<8;++i)
    {
        bytes[7-i] = info.issuer & 0xff ;
        info.issuer >>= 8 ;
    }
    issuer_id = RsPgpId(bytes) ;
    
    return true ;     
}

bool PGPHandler::privateTrustCertificate(const RsPgpId& id,int trustlvl)
{
	if(trustlvl < 0 || trustlvl >= 6 || trustlvl == 1)
	{
		std::cerr << "Invalid trust level " << trustlvl << " passed to privateTrustCertificate." << std::endl;
		return false ;
	}

	std::map<RsPgpId,PGPCertificateInfo>::iterator it = _public_keyring_map.find(id);

	if(it == _public_keyring_map.end())
	{
		std::cerr << "(EE) Key id " << id.toStdString() << " not in the keyring. Can't setup trust level." << std::endl;
		return false ;
	}

	if( (int)it->second._trustLvl != trustlvl )
		_trustdb_changed = true ;

	it->second._trustLvl = trustlvl ;

	return true ;
}

struct PrivateTrustPacket
{
	/// pgp id in unsigned char format.
	unsigned char user_id[RsPgpId::SIZE_IN_BYTES];
	uint8_t trust_level ;						// trust level. From 0 to 6.
	uint32_t time_stamp ;						// last time the cert was ever used, in seconds since the epoch. 0 means not initialized.
};

void PGPHandler::locked_readPrivateTrustDatabase()
{
	FILE *fdb = RsDirUtil::rs_fopen(_trustdb_path.c_str(),"rb") ;
#ifdef DEBUG_PGPHANDLER
	std::cerr << "PGPHandler:  Reading private trust database." << std::endl;
#endif

	if(fdb == NULL)
	{
		std::cerr << "  private trust database not found. No trust info loaded." << std::endl ;
		return ;
	}
	std::map<RsPgpId,PGPCertificateInfo>::iterator it ;
	PrivateTrustPacket trustpacket;
	int n_packets = 0 ;

	while(fread((void*)&trustpacket,sizeof(PrivateTrustPacket),1,fdb) == 1)
	{
		it = _public_keyring_map.find(RsPgpId(trustpacket.user_id)) ;

		if(it == _public_keyring_map.end())
		{
			std::cerr << "  (WW) Trust packet found for unknown key id " << RsPgpId(trustpacket.user_id).toStdString() << std::endl;
			continue ;
		}
		if(trustpacket.trust_level > 6)
		{
			std::cerr << "  (WW) Trust packet found with unexpected trust level " << trustpacket.trust_level << std::endl;
			continue ;
		}
		
		++n_packets ;
		it->second._trustLvl = trustpacket.trust_level ;

		if(trustpacket.time_stamp > it->second._time_stamp)	// only update time stamp if the loaded time stamp is newer
		   it->second._time_stamp = trustpacket.time_stamp ;
	}

	fclose(fdb) ;

	std::cerr << "PGPHandler: Successfully read " << std::hex << n_packets << std::dec << " trust packets." << std::endl;
}

bool PGPHandler::locked_writePrivateTrustDatabase()
{
	FILE *fdb = RsDirUtil::rs_fopen((_trustdb_path+".tmp").c_str(),"wb") ;
#ifdef DEBUG_PGPHANDLER
	std::cerr << "PGPHandler:  Reading private trust database." << std::endl;
#endif

	if(fdb == NULL)
	{
		std::cerr << "  (EE) Can't open private trust database file " << _trustdb_path << " for write. Giving up!" << std::endl ;
		return false;
	}
	PrivateTrustPacket trustpacket ;

	for( std::map<RsPgpId,PGPCertificateInfo>::iterator it =
	     _public_keyring_map.begin(); it!=_public_keyring_map.end(); ++it )
	{
		memcpy( trustpacket.user_id,
		        it->first.toByteArray(),
		        RsPgpId::SIZE_IN_BYTES );
		trustpacket.trust_level = it->second._trustLvl ;
		trustpacket.time_stamp = it->second._time_stamp ;

		if(fwrite((void*)&trustpacket,sizeof(PrivateTrustPacket),1,fdb) != 1)
		{
			std::cerr << "  (EE) Cannot write to trust database " << _trustdb_path << ". Disc full, or quota exceeded ? Leaving database untouched." << std::endl;
			fclose(fdb) ;
			return false;
		}
	}

	fclose(fdb) ;

	if(!RsDirUtil::renameFile(_trustdb_path+".tmp",_trustdb_path))
	{
		std::cerr << "  (EE) Cannot move temp file " << _trustdb_path+".tmp" << ". Bad write permissions?" << std::endl;
		return false ;
	}
	else
		return true ;
}

bool PGPHandler::syncDatabase()
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
	RsStackFileLock flck(_pgp_lock_filename) ;	// lock access to PGP directory.

#ifdef DEBUG_PGPHANDLER
	std::cerr << "Sync-ing keyrings." << std::endl;
#endif
	locked_syncPublicKeyring() ;
	//locked_syncSecretKeyring() ;
	
	// Now sync the trust database as well.
	//
	locked_syncTrustDatabase() ;

#ifdef DEBUG_PGPHANDLER
	std::cerr << "Done. " << std::endl;
#endif
	return true ;
}

bool PGPHandler::locked_syncPublicKeyring()
{
	struct stat64 buf ;
#ifdef WINDOWS_SYS
	std::wstring wfullname;
	librs::util::ConvertUtf8ToUtf16(_pubring_path, wfullname);
	if(-1 == _wstati64(wfullname.c_str(), &buf))
#else
	if(-1 == stat64(_pubring_path.c_str(), &buf))
#endif
		std::cerr << "PGPHandler::syncDatabase(): can't stat file " << _pubring_path << ". Can't sync public keyring." << std::endl;

	if(_pubring_last_update_time < buf.st_mtime)
	{
		std::cerr << "Detected change on disk of public keyring. Merging!" << std::endl ;

		locked_mergeKeyringFromDisk(_pubring,_public_keyring_map,_pubring_path) ;
		_pubring_last_update_time = buf.st_mtime ;
	}

	// Now check if the pubring was locally modified, which needs saving it again
	if(_pubring_changed && RsDiscSpace::checkForDiscSpace(RS_PGP_DIRECTORY))
	{
		std::string tmp_keyring_file = _pubring_path + ".tmp" ;

		std::cerr << "Local changes in public keyring. Writing to disk..." << std::endl;
		if(!ops_write_keyring_to_file(_pubring,ops_false,tmp_keyring_file.c_str(),ops_true)) 
		{
			std::cerr << "Cannot write public keyring tmp file. Disk full? Disk quota exceeded?" << std::endl;
			return false ;
		}
		if(!RsDirUtil::renameFile(tmp_keyring_file,_pubring_path))
		{
			std::cerr << "Cannot rename tmp pubring file " << tmp_keyring_file << " into actual pubring file " << _pubring_path << ". Check writing permissions?!?" << std::endl;
			return false ;
		}

		std::cerr << "Done." << std::endl;
		_pubring_last_update_time = time(NULL) ;	// should we get this value from the disk instead??
		_pubring_changed = false ;
	}
	return true ;
}

bool PGPHandler::locked_syncTrustDatabase()
{
	struct stat64 buf ;
#ifdef WINDOWS_SYS
	std::wstring wfullname;
	librs::util::ConvertUtf8ToUtf16(_trustdb_path, wfullname);
	if(-1 == _wstati64(wfullname.c_str(), &buf))
#else
		if(-1 == stat64(_trustdb_path.c_str(), &buf))
#endif
		{
			std::cerr << "PGPHandler::syncDatabase(): can't stat file " << _trustdb_path << ". Will force write it." << std::endl;
			_trustdb_changed = true ;	// we force write of trust database if it does not exist.
		}

	if(_trustdb_last_update_time < buf.st_mtime)
	{
		std::cerr << "Detected change on disk of trust database. " << std::endl ;

		locked_readPrivateTrustDatabase();
		_trustdb_last_update_time = time(NULL) ;
	}

	if(_trustdb_changed)
	{
		std::cerr << "Local changes in trust database. Writing to disk..." << std::endl;
		if(!locked_writePrivateTrustDatabase())
			std::cerr << "Cannot write trust database. Disk full? Disk quota exceeded?" << std::endl;
		else
		{
			std::cerr << "Done." << std::endl;
			_trustdb_last_update_time = time(NULL) ;
			_trustdb_changed = false ;
		}
	}
	return true ;
}
void PGPHandler::locked_mergeKeyringFromDisk(	ops_keyring_t *keyring,
													std::map<RsPgpId,PGPCertificateInfo>& kmap,
													const std::string& keyring_file)
{
#ifdef DEBUG_PGPHANDLER
	std::cerr << "Merging keyring " << keyring_file << " from disk to memory." << std::endl;
#endif

	// 1 - load keyring into a temporary keyring list.
	ops_keyring_t *tmp_keyring = PGPHandler::allocateOPSKeyring() ;

	if(ops_false == ops_keyring_read_from_file(tmp_keyring, false, keyring_file.c_str()))
	{
		std::cerr << "PGPHandler::locked_mergeKeyringFromDisk(): cannot read keyring. File corrupted?" ;
		ops_keyring_free(tmp_keyring) ;
		return ;
	}

	// 2 - load new keys and merge existing key signatures

	for(int i=0;i<tmp_keyring->nkeys;++i)
		locked_addOrMergeKey(keyring,kmap,&tmp_keyring->keys[i]) ;// we dont' account for the return value. This is disk merging, not local changes.	

	// 4 - clean
	ops_keyring_free(tmp_keyring) ;
}

bool PGPHandler::removeKeysFromPGPKeyring(const std::set<RsPgpId>& keys_to_remove,std::string& backup_file,uint32_t& error_code)
{
	// 1 - lock everything.
	//
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
	RsStackFileLock flck(_pgp_lock_filename) ;	// lock access to PGP directory.

	error_code = PGP_KEYRING_REMOVAL_ERROR_NO_ERROR ;

    for(std::set<RsPgpId>::const_iterator it(keys_to_remove.begin());it!=keys_to_remove.end();++it)
		if(locked_getSecretKey(*it) != NULL)
		{
			std::cerr << "(EE) PGPHandler:: can't remove key " << (*it).toStdString() << " since its shared by a secret key! Operation cancelled." << std::endl;
			error_code = PGP_KEYRING_REMOVAL_ERROR_CANT_REMOVE_SECRET_KEYS ;
			return false ;
		}

	// 2 - sync everything.
	//
	locked_syncPublicKeyring() ;

	// 3 - make a backup of the public keyring
	//
	char template_name[_pubring_path.length()+8] ;
	sprintf(template_name,"%s.XXXXXX",_pubring_path.c_str()) ;
	
#if defined __USE_XOPEN_EXTENDED || defined __USE_XOPEN2K8
	int fd_keyring_backup(mkstemp(template_name));
	if (fd_keyring_backup == -1)
#else
	if(mktemp(template_name) == NULL)
#endif
	{
		std::cerr << "PGPHandler::removeKeysFromPGPKeyring(): cannot create keyring backup file. Giving up." << std::endl;
		error_code = PGP_KEYRING_REMOVAL_ERROR_CANNOT_CREATE_BACKUP ;
		return false ;
	}
#if defined __USE_XOPEN_EXTENDED || defined __USE_XOPEN2K8
	close(fd_keyring_backup);	// TODO: keep the file open and use the fd
#endif

	if(!ops_write_keyring_to_file(_pubring,ops_false,template_name,ops_true)) 
	{
		std::cerr << "PGPHandler::removeKeysFromPGPKeyring(): cannot write keyring backup file. Giving up." << std::endl;
		error_code = PGP_KEYRING_REMOVAL_ERROR_CANNOT_WRITE_BACKUP ;
		return false ;
	}
	backup_file = std::string(template_name,_pubring_path.length()+7) ;

	std::cerr << "Keyring was backed up to file " << backup_file << std::endl;

	// Remove keys from the keyring, and update the keyring map.
	//
    for(std::set<RsPgpId>::const_iterator it(keys_to_remove.begin());it!=keys_to_remove.end();++it)
	{
		if(locked_getSecretKey(*it) != NULL)
		{
			std::cerr << "(EE) PGPHandler:: can't remove key " << (*it).toStdString() << " since its shared by a secret key!" << std::endl;
			continue ;
		}

		std::map<RsPgpId,PGPCertificateInfo>::iterator res = _public_keyring_map.find(*it) ;

		if(res == _public_keyring_map.end())
		{
			std::cerr << "(EE) PGPHandler:: can't remove key " << (*it).toStdString() << " from keyring: key not found." << std::endl;
			continue ;
		}

		if(res->second._key_index >= (unsigned int)_pubring->nkeys || RsPgpId(_pubring->keys[res->second._key_index].key_id) != *it)
		{
			std::cerr << "(EE) PGPHandler:: can't remove key " << (*it).toStdString() << ". Inconsistency found." << std::endl;
			error_code = PGP_KEYRING_REMOVAL_ERROR_DATA_INCONSISTENCY ;
			return false ;
		}

		// Move the last key to the freed place. This deletes the key in place.
		//
		ops_keyring_remove_key(_pubring,res->second._key_index) ;

		// Erase the info from the keyring map.
		//
		_public_keyring_map.erase(res) ;

		// now update all indices back. This internal look is very costly, but it avoids deleting the wrong keys, since the keyring structure is
		// changed by ops_keyring_remove_key and therefore indices don't point to the correct location anymore.

		int i=0 ;
		const ops_keydata_t *keydata ;
		while( (keydata = ops_keyring_get_key_by_index(_pubring,i)) != NULL )
		{
			PGPCertificateInfo& cert(_public_keyring_map[ RsPgpId(keydata->key_id) ]) ;
			cert._key_index = i ;
			++i ;
		}
	}

	// Everything went well, sync back the keyring on disk
	
	_pubring_changed = true ;
	_trustdb_changed = true ;

	locked_syncPublicKeyring() ;
	locked_syncTrustDatabase() ;

	return true ;
}


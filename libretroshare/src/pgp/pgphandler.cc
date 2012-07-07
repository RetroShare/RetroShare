#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef WINDOWS_SYS
#include "util/rsstring.h"
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
#include "util/rsdir.h"		
#include "util/pgpkey.h"

#define DEBUG_PGPHANDLER

PassphraseCallback PGPHandler::_passphrase_callback = NULL ;

ops_keyring_t *PGPHandler::allocateOPSKeyring() 
{
	ops_keyring_t *kr = (ops_keyring_t*)malloc(sizeof(ops_keyring_t)) ;
	kr->nkeys = 0 ;
	kr->nkeys_allocated = 0 ;
	kr->keys = 0 ;

	return kr ;
}

ops_parse_cb_return_t cb_get_passphrase(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)// __attribute__((unused)))
{
	const ops_parser_content_union_t *content=&content_->content;
	//    validate_key_cb_arg_t *arg=ops_parse_cb_get_arg(cbinfo);
	//    ops_error_t **errors=ops_parse_cb_get_errors(cbinfo);

	bool prev_was_bad = false ;
	
	switch(content_->tag)
	{
		case OPS_PARSER_CMD_GET_SK_PASSPHRASE_PREV_WAS_BAD: prev_was_bad = true ;
		case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
																				{
																					std::string passwd;
																					std::string uid_hint = std::string((const char *)cbinfo->cryptinfo.keydata->uids[0].user_id) ;
																					uid_hint += "(" + PGPIdType(cbinfo->cryptinfo.keydata->key_id).toStdString()+")" ;

																					passwd = PGPHandler::passphraseCallback()(NULL,uid_hint.c_str(),NULL,prev_was_bad) ;
//																					if (rsicontrol->getNotify().askForPassword(uid_hint, prev_was_bad, passwd) == false) 
//																						return OPS_RELEASE_MEMORY;

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
	{
		std::cerr << "WARNING: before created a PGPHandler, you need to init the passphrase callback using PGPHandler::setPassphraseCallback()" << std::endl;
		exit(-1) ;
	}
		
	// Allocate public and secret keyrings.
	// 
	_pubring = allocateOPSKeyring() ;
	_secring = allocateOPSKeyring() ;

	// Check that the file exists. If not, create a void keyring.
	
	FILE *ftest ;
	ftest = fopen(pubring.c_str(),"rb") ;
	bool pubring_exist = (ftest != NULL) ;
	if(ftest != NULL)
		fclose(ftest) ;
	ftest = fopen(secring.c_str(),"rb") ;
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
		PGPCertificateInfo& cert(_public_keyring_map[ PGPIdType(keydata->key_id).toStdString() ]) ;

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
		initCertificateInfo(_secret_keyring_map[ PGPIdType(keydata->key_id).toStdString() ],keydata,i) ;
		++i ;
	}
	_secring_last_update_time = time(NULL) ;

	std::cerr << "Secring read successfully." << std::endl;

	locked_readPrivateTrustDatabase() ;
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

	ops_fingerprint_t f ;
	ops_fingerprint(&f,&keydata->key.pkey) ; 

	cert._fpr = PGPFingerprintType(f.fingerprint) ;

	if(keydata->key.pkey.algorithm != OPS_PKA_RSA)
		cert._flags |= PGPCertificateInfo::PGP_CERTIFICATE_FLAG_UNSUPPORTED_ALGORITHM ;
}

bool PGPHandler::validateAndUpdateSignatures(PGPCertificateInfo& cert,const ops_keydata_t *keydata)
{
	ops_validate_result_t* result=(ops_validate_result_t*)ops_mallocz(sizeof *result);
	ops_boolean_t res = ops_validate_key_signatures(result,keydata,_pubring,cb_get_passphrase) ;

	if(res == ops_false)
		std::cerr << "(EE) Error in PGPHandler::validateAndUpdateSignatures(). Validation failed for at least some signatures." << std::endl;

	bool ret = false ;

	// Parse signers.
	//

	if(result != NULL)
		for(size_t i=0;i<result->valid_count;++i)
		{
			std::string signer_str = PGPIdType(result->valid_sigs[i].signer_id).toStdString() ;

			if(cert.signers.find(signer_str) == cert.signers.end())
			{
				cert.signers.insert(signer_str) ;
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

	for(std::map<std::string,PGPCertificateInfo>::const_iterator it(_public_keyring_map.begin()); it != _public_keyring_map.end(); it++)
	{
		std::cerr << "PGP Key: " << it->first << std::endl;

		std::cerr << "\tName          : " <<  it->second._name << std::endl;
		std::cerr << "\tEmail         : " <<  it->second._email << std::endl;
		std::cerr << "\tOwnSign       : " << (it->second._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE) << std::endl;
		std::cerr << "\tAccept Connect: " << (it->second._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION) << std::endl;
		std::cerr << "\ttrustLvl      : " <<  it->second._trustLvl << std::endl;
		std::cerr << "\tvalidLvl      : " <<  it->second._validLvl << std::endl;
		std::cerr << "\tfingerprint   : " <<  it->second._fpr.toStdString() << std::endl;
		std::cerr << "\tSigners       : " << it->second.signers.size() <<  std::endl;

		std::set<std::string>::const_iterator sit;
		for(sit = it->second.signers.begin(); sit != it->second.signers.end(); sit++)
		{
			std::cerr << "\t\tSigner ID:" << *sit << ", Name: " ;
			const PGPCertificateInfo *info = PGPHandler::getCertificateInfo(PGPIdType(*sit)) ;

			if(info != NULL)
				std::cerr << info->_name ;

			std::cerr << std::endl ;
		}
	}
	std::cerr << "Public keyring list from OPS:" << std::endl;
	ops_keyring_list(_pubring) ;

	return true ;
}

const PGPCertificateInfo *PGPHandler::getCertificateInfo(const PGPIdType& id) const
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	std::map<std::string,PGPCertificateInfo>::const_iterator it( _public_keyring_map.find(id.toStdString()) ) ;

	if(it != _public_keyring_map.end())
		return &it->second;
	else
		return NULL ;
}

bool PGPHandler::availableGPGCertificatesWithPrivateKeys(std::list<PGPIdType>& ids)
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
	// go through secret keyring, and check that we have the pubkey as well.
	//
	
	const ops_keydata_t *keydata = NULL ;
	int i=0 ;

	while( (keydata = ops_keyring_get_key_by_index(_secring,i++)) != NULL )
		if(ops_keyring_find_key_by_id(_pubring,keydata->key_id) != NULL) // check that the key is in the pubring as well
		{
			if(keydata->key.pkey.algorithm == OPS_PKA_RSA)
				ids.push_back(PGPIdType(keydata->key_id)) ;
#ifdef DEBUG_PGPHANDLER
			else
				std::cerr << "Skipping keypair " << PGPIdType(keydata->key_id).toStdString() << ", unsupported algorithm: " <<  keydata->key.pkey.algorithm << std::endl;
#endif
		}

	return true ;
}

bool PGPHandler::GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passphrase, PGPIdType& pgpId, std::string& errString) 
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
	RsStackFileLock flck(_pgp_lock_filename) ;	// lock access to PGP directory.

	static const int KEY_NUMBITS = 2048 ;

	// 1 - generate keypair - RSA-2048
	//
	ops_user_id_t uid ;
	char *s = strdup((name + " " + email + " (Generated by RetroShare)").c_str()) ;
	uid.user_id = (unsigned char *)s ;
	unsigned long int e = 65537 ; // some prime number

	ops_keydata_t *key = ops_rsa_create_selfsigned_keypair(KEY_NUMBITS,e,&uid) ;

	free(s) ;

	if(!key)
		return false ;

	// 2 - save the private key encrypted to a temporary memory buffer, so as to read an encrypted key to memory

	ops_create_info_t *cinfo = NULL ;
	ops_memory_t *buf = NULL ;
   ops_setup_memory_write(&cinfo, &buf, 0);

	if(!ops_write_transferable_secret_key(key,(unsigned char *)passphrase.c_str(),passphrase.length(),ops_false,cinfo))
	{
		std::cerr << "(EE) Cannot encode secret key to memory!!" << std::endl;
		return false ;
	}

	// 3 - read the memory chunk into an encrypted keyring
	
	ops_keyring_t *tmp_secring = allocateOPSKeyring() ;

	if(! ops_keyring_read_from_mem(tmp_secring, ops_false, buf))
	{
		std::cerr << "(EE) Cannot re-read key from memory!!" << std::endl;
		return false ;
	}
	ops_teardown_memory_write(cinfo,buf);	// cleanup memory

	// 4 - copy the encrypted private key to the private keyring
	
	pgpId = PGPIdType(tmp_secring->keys[0].key_id) ;
	addNewKeyToOPSKeyring(_secring,tmp_secring->keys[0]) ;
	initCertificateInfo(_secret_keyring_map[ pgpId.toStdString() ],&tmp_secring->keys[0],_secring->nkeys-1) ;

#ifdef DEBUG_PGPHANDLER
	std::cerr << "Added new secret key with id " << pgpId.toStdString() << " to secret keyring." << std::endl;
#endif
	ops_keyring_free(tmp_secring) ;
	free(tmp_secring) ;

	// 5 - add key to secret keyring on disk.
	
	cinfo = NULL ;
	int fd=ops_setup_file_append(&cinfo, _secring_path.c_str());

	if(!ops_write_transferable_secret_key(key,(unsigned char *)passphrase.c_str(),passphrase.length(),ops_false,cinfo))
	{
		std::cerr << "(EE) Cannot encode secret key to disk!! Disk full? Out of disk quota?" << std::endl;
		return false ;
	}
	ops_teardown_file_write(cinfo,fd) ;

	// 6 - copy the public key to the public keyring on disk
	
	cinfo = NULL ;
	fd=ops_setup_file_append(&cinfo, _pubring_path.c_str());

	if(!ops_write_transferable_public_key(key, ops_false, cinfo))
	{
		std::cerr << "(EE) Cannot encode secret key to memory!!" << std::endl;
		return false ;
	}
	ops_teardown_file_write(cinfo,fd) ;

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

std::string PGPHandler::makeRadixEncodedPGPKey(const ops_keydata_t *key)
{
	ops_boolean_t armoured=ops_true;
   ops_create_info_t* cinfo;

	ops_memory_t *buf = NULL ;
   ops_setup_memory_write(&cinfo, &buf, 0);

   if(ops_write_transferable_public_key_from_packet_data(key,armoured,cinfo) != ops_true)
		return "ERROR: This key cannot be processed by RetroShare because\nDSA certificates are not yet handled." ;

	ops_writer_close(cinfo) ;

	std::string akey((char *)ops_memory_get_data(buf),ops_memory_get_length(buf)) ;

   ops_teardown_memory_write(cinfo,buf);

	return akey ;
}

const ops_keydata_t *PGPHandler::getSecretKey(const PGPIdType& id) const
{
	std::map<std::string,PGPCertificateInfo>::const_iterator res = _secret_keyring_map.find(id.toStdString()) ;

	if(res == _secret_keyring_map.end())
		return NULL ;
	else
		return ops_keyring_get_key_by_index(_secring,res->second._key_index) ;
}
const ops_keydata_t *PGPHandler::getPublicKey(const PGPIdType& id) const
{
	std::map<std::string,PGPCertificateInfo>::const_iterator res = _public_keyring_map.find(id.toStdString()) ;

	if(res == _public_keyring_map.end())
		return NULL ;
	else
		return ops_keyring_get_key_by_index(_pubring,res->second._key_index) ;
}

std::string PGPHandler::SaveCertificateToString(const PGPIdType& id,bool include_signatures)
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
	const ops_keydata_t *key = getPublicKey(id) ;

	if(key == NULL)
	{
		std::cerr << "Cannot output key " << id.toStdString() << ": not found in keyring." << std::endl;
		return "" ;
	}

	return makeRadixEncodedPGPKey(key) ;
}

void PGPHandler::addNewKeyToOPSKeyring(ops_keyring_t *kr,const ops_keydata_t& key)
{
	kr->keys = (ops_keydata_t*)realloc(kr->keys,(kr->nkeys+1)*sizeof(ops_keydata_t)) ;
	memset(&kr->keys[kr->nkeys],0,sizeof(ops_keydata_t)) ;
	ops_keydata_copy(&kr->keys[kr->nkeys],&key) ;
	kr->nkeys++ ;
}

bool PGPHandler::LoadCertificateFromString(const std::string& pgp_cert,PGPIdType& id,std::string& error_string)
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

#ifdef DEBUG_PGPHANDLER
	std::cerr << "  Key read correctly: " << std::endl;
#endif
	ops_keyring_list(tmp_keyring) ;

	const ops_keydata_t *keydata = NULL ;
	int i=0 ;

	while( (keydata = ops_keyring_get_key_by_index(tmp_keyring,i++)) != NULL )
		if(addOrMergeKey(_pubring,_public_keyring_map,keydata)) 
		{
			_pubring_changed = true ;
#ifdef DEBUG_PGPHANDLER
			std::cerr << "  Added the key in the main public keyring." << std::endl;
#endif
		}
		else
			std::cerr << "Key already in public keyring." << std::endl;
	
	id = PGPIdType(tmp_keyring->keys[0].key_id) ;

	ops_keyring_free(tmp_keyring) ;
	free(tmp_keyring) ;

	_pubring_changed = true ;

	return true ;
}

bool PGPHandler::addOrMergeKey(ops_keyring_t *keyring,std::map<std::string,PGPCertificateInfo>& kmap,const ops_keydata_t *keydata)
{
	bool ret = false ;
	PGPIdType id(keydata->key_id) ;

#ifdef DEBUG_PGPHANDLER
	std::cerr << "AddOrMergeKey():" << std::endl;
	std::cerr << "  id: " << id.toStdString() << std::endl;
#endif

	// See if the key is already in the keyring
	const ops_keydata_t *existing_key = NULL;
	std::map<std::string,PGPCertificateInfo>::const_iterator res = kmap.find(id.toStdString()) ;

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
		initCertificateInfo(kmap[id.toStdString()],keydata,keyring->nkeys-1) ;
		existing_key = &(keyring->keys[keyring->nkeys-1]) ;
		ret = true ;
	}
	else
	{
		if(memcmp(existing_key->fingerprint.fingerprint, keydata->fingerprint.fingerprint,KEY_FINGERPRINT_SIZE))
		{
			std::cerr << "(EE) attempt to merge key with identical id, but different fingerprint!" << std::endl;
			return false ;
		}

#ifdef DEBUG_PGPHANDLER
		std::cerr << "  Key exists. Merging signatures." << std::endl;
#endif
		ret = mergeKeySignatures(const_cast<ops_keydata_t*>(existing_key),keydata) ;

		if(ret)
			initCertificateInfo(kmap[id.toStdString()],existing_key,res->second._key_index) ;
	}

	if(ret)
		validateAndUpdateSignatures(kmap[id.toStdString()],existing_key) ;

	return ret ;
}

bool PGPHandler::encryptTextToFile(const PGPIdType& key_id,const std::string& text,const std::string& outfile) 
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	ops_create_info_t *info;
	int fd = ops_setup_file_write(&info, outfile.c_str(), ops_true);

	const ops_keydata_t *public_key = getPublicKey(key_id) ;

	if(public_key == NULL)
	{
		std::cerr << "Cannot get public key of id " << key_id.toStdString() << std::endl;
		return false ;
	}

	if(public_key->type != OPS_PTAG_CT_PUBLIC_KEY)
	{
		std::cerr << "PGPHandler::encryptTextToFile(): ERROR: supplied id did not return a public key!" << outfile << std::endl;
		return false ;
	}

	if (fd < 0) 
	{
		std::cerr << "PGPHandler::encryptTextToFile(): ERROR: Cannot write to " << outfile << std::endl;
		return false ;
	}
	ops_encrypt_stream(info, public_key, NULL, ops_false, ops_true);
	ops_write(text.c_str(), text.length(), info);
	ops_writer_close(info);
	ops_create_info_delete(info);

	return true ;
}

bool PGPHandler::decryptTextFromFile(const PGPIdType& key_id,std::string& text,const std::string& inputfile) 
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	unsigned char *out_buf = NULL ;
	std::string buf ;

	FILE *f = fopen(inputfile.c_str(),"rb") ;

	if (f == NULL)
	{
		return false;
	}

	char c ;
	while( (c = getc(f))!= EOF)
		buf += c;

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

bool PGPHandler::SignDataBin(const PGPIdType& id,const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen)
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.
	// need to find the key and to decrypt it.
	
	const ops_keydata_t *key = getSecretKey(id) ;

	if(!key)
	{
		std::cerr << "Cannot sign: no secret key with id " << id.toStdString() << std::endl;
		return false ;
	}

	std::string passphrase = _passphrase_callback(NULL,PGPIdType(key->key_id).toStdString().c_str(),"Please enter passwd for encrypting your key : ",false) ;

	ops_secret_key_t *secret_key = ops_decrypt_secret_key_from_data(key,passphrase.c_str()) ;

	if(!secret_key)
	{
		std::cerr << "Key decryption went wrong. Wrong passwd?" << std::endl;
		return false ;
	}

	// then do the signature.

	ops_memory_t *memres = ops_sign_buf(data,len,(ops_sig_type_t)0x00,secret_key,ops_false,ops_false) ;

	if(!memres)
		return false ;

	uint32_t tlen = std::min(*signlen,(uint32_t)ops_memory_get_length(memres)) ;

	memcpy(sign,ops_memory_get_data(memres),tlen) ;
	*signlen = tlen ;

	ops_memory_release(memres) ;
	free(memres) ;
	ops_secret_key_free(secret_key) ;
	free(secret_key) ;

	return true ;
}

bool PGPHandler::privateSignCertificate(const PGPIdType& ownId,const PGPIdType& id_of_key_to_sign) 
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	ops_keydata_t *key_to_sign = const_cast<ops_keydata_t*>(getPublicKey(id_of_key_to_sign)) ;

	if(key_to_sign == NULL)
	{
		std::cerr << "Cannot sign: no public key with id " << id_of_key_to_sign.toStdString() << std::endl;
		return false ;
	}

	// 1 - get decrypted secret key
	//
	const ops_keydata_t *skey = getSecretKey(ownId) ;

	if(!skey)
	{
		std::cerr << "Cannot sign: no secret key with id " << ownId.toStdString() << std::endl;
		return false ;
	}
	const ops_keydata_t *pkey = getPublicKey(ownId) ;

	if(!pkey)
	{
		std::cerr << "Cannot sign: no public key with id " << ownId.toStdString() << std::endl;
		return false ;
	}

	std::string passphrase = _passphrase_callback(NULL,PGPIdType(skey->key_id).toStdString().c_str(),"Please enter passwd for encrypting your key : ",false) ;

	ops_secret_key_t *secret_key = ops_decrypt_secret_key_from_data(skey,passphrase.c_str()) ;

	if(!secret_key)
	{
		std::cerr << "Key decryption went wrong. Wrong passwd?" << std::endl;
		return false ;
	}

	// 2 - then do the signature.

	bool ret = ops_sign_key(key_to_sign,pkey->key_id,secret_key) ;

	// 3 - free memory
	//
	ops_secret_key_free(secret_key) ;
	free(secret_key) ;

	_pubring_changed = true ;

	// 4 - update signatures.
	//
	PGPCertificateInfo& cert(_public_keyring_map[ id_of_key_to_sign.toStdString() ]) ;
	validateAndUpdateSignatures(cert,key_to_sign) ;

	return true ;
}

bool PGPHandler::getKeyFingerprint(const PGPIdType& id,PGPFingerprintType& fp) const
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	const ops_keydata_t *key = getPublicKey(id) ;

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

	PGPIdType id = PGPIdType(key_fingerprint.toByteArray() + PGPFingerprintType::SIZE_IN_BYTES - PGPIdType::SIZE_IN_BYTES) ;
	const ops_keydata_t *key = getPublicKey(id) ;

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
#endif

	return ops_validate_detached_signature(literal_data,literal_data_length,sign,sign_len,key) ;
}

void PGPHandler::setAcceptConnexion(const PGPIdType& id,bool b)
{
	RsStackMutex mtx(pgphandlerMtx) ;				// lock access to PGP memory structures.

	std::map<std::string,PGPCertificateInfo>::iterator res = _public_keyring_map.find(id.toStdString()) ;

	if(res != _public_keyring_map.end())
	{
		if(b)
			res->second._flags |= PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION ;
		else
			res->second._flags &= ~PGPCertificateInfo::PGP_CERTIFICATE_FLAG_ACCEPT_CONNEXION ;
	}
}

bool PGPHandler::getGPGFilteredList(std::list<PGPIdType>& list,bool (*filter)(const PGPCertificateInfo&)) const
{
	RsStackMutex mtx(pgphandlerMtx) ;	// lock access to PGP directory.
	list.clear() ;

	for(std::map<std::string,PGPCertificateInfo>::const_iterator it(_public_keyring_map.begin());it!=_public_keyring_map.end();++it)
		if( filter == NULL || (*filter)(it->second) )
			list.push_back(PGPIdType(it->first)) ;

	return true ;
}

bool PGPHandler::isGPGId(const std::string &id)
{
	return _public_keyring_map.find(id) != _public_keyring_map.end() ;
}

bool PGPHandler::isGPGSigned(const std::string &id)
{
	std::map<std::string,PGPCertificateInfo>::const_iterator res = _public_keyring_map.find(id) ;
	return res != _public_keyring_map.end() && (res->second._flags & PGPCertificateInfo::PGP_CERTIFICATE_FLAG_HAS_OWN_SIGNATURE) ;
}

bool PGPHandler::isGPGAccepted(const std::string &id)
{
	std::map<std::string,PGPCertificateInfo>::const_iterator res = _public_keyring_map.find(id) ;
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
	std::cerr << "Merging signatures for key " << PGPIdType(dst->key_id).toStdString() << std::endl;
#endif
	std::set<ops_packet_t> dst_packets ;

	for(uint32_t i=0;i<dst->npackets;++i) dst_packets.insert(dst->packets[i]) ;

	std::set<ops_packet_t> to_add ;

	for(uint32_t i=0;i<src->npackets;++i) 
		if(dst_packets.find(src->packets[i]) == dst_packets.end())
		{
			uint8_t tag ;
			uint32_t length ;

			PGPKeyParser::read_packetHeader(src->packets[i].raw,tag,length) ;

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

bool PGPHandler::privateTrustCertificate(const PGPIdType& id,int trustlvl)
{
	if(trustlvl < 0 || trustlvl >= 6 || trustlvl == 1)
	{
		std::cerr << "Invalid trust level " << trustlvl << " passed to privateTrustCertificate." << std::endl;
		return false ;
	}

	std::map<std::string,PGPCertificateInfo>::iterator it = _public_keyring_map.find(id.toStdString());

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
	unsigned char user_id[KEY_ID_SIZE] ;  	// pgp id in unsigned char format.
	uint8_t trust_level ;						// trust level. From 0 to 6.
	uint32_t flags ;								// not used yet, but who knows?
};

void PGPHandler::locked_readPrivateTrustDatabase()
{
	FILE *fdb = fopen(_trustdb_path.c_str(),"rb") ;
#ifdef DEBUG_PGPHANDLER
	std::cerr << "PGPHandler:  Reading private trust database." << std::endl;
#endif

	if(fdb == NULL)
	{
		std::cerr << "  private trust database not found. No trust info loaded." << std::endl ;
		return ;
	}
	std::map<std::string,PGPCertificateInfo>::iterator it ;
	PrivateTrustPacket trustpacket;
	int n_packets = 0 ;

	while(fread((void*)&trustpacket,sizeof(PrivateTrustPacket),1,fdb) == 1)
	{
		it = _public_keyring_map.find(PGPIdType(trustpacket.user_id).toStdString()) ;

		if(it == _public_keyring_map.end())
		{
			std::cerr << "  (WW) Trust packet found for unknown key id " << PGPIdType(trustpacket.user_id).toStdString() << std::endl;
			continue ;
		}
		if(trustpacket.trust_level > 6)
		{
			std::cerr << "  (WW) Trust packet found with unexpected trust level " << trustpacket.trust_level << std::endl;
			continue ;
		}
		
		++n_packets ;
		it->second._trustLvl = trustpacket.trust_level ;
	}

	fclose(fdb) ;

	std::cerr << "PGPHandler: Successfully read " << n_packets << " trust packets." << std::endl;
}

bool PGPHandler::locked_writePrivateTrustDatabase()
{
	FILE *fdb = fopen((_trustdb_path+".tmp").c_str(),"wb") ;
#ifdef DEBUG_PGPHANDLER
	std::cerr << "PGPHandler:  Reading private trust database." << std::endl;
#endif

	if(fdb == NULL)
	{
		std::cerr << "  (EE) Can't open private trust database file " << _trustdb_path << " for write. Giving up!" << std::endl ;
		return false;
	}
	PrivateTrustPacket trustpacket ;

	for(std::map<std::string,PGPCertificateInfo>::iterator it = _public_keyring_map.begin();it!=_public_keyring_map.end() ;++it)
	{
		memcpy(&trustpacket.user_id,PGPIdType(it->first).toByteArray(),KEY_ID_SIZE) ;
		trustpacket.trust_level = it->second._trustLvl ;

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

		mergeKeyringFromDisk(_pubring,_public_keyring_map,_pubring_path) ;
		_pubring_last_update_time = buf.st_mtime ;
	}

	// Now check if the pubring was locally modified, which needs saving it again
	if(_pubring_changed)
	{
		std::cerr << "Local changes in public keyring. Writing to disk..." << std::endl;
		if(!ops_write_keyring_to_file(_pubring,ops_false,_pubring_path.c_str(),ops_true)) 
			std::cerr << "Cannot write public keyring. Disk full? Disk quota exceeded?" << std::endl;
		else
		{
			std::cerr << "Done." << std::endl;
			_pubring_last_update_time = time(NULL) ;	// should we get this value from the disk instead??
			_pubring_changed = false ;
		}
	}
	return true ;
}

bool PGPHandler::locked_syncTrustDatabase()
{
	struct stat64 buf ;
	std::wstring wfullname;
#ifdef WINDOWS_SYS
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
void PGPHandler::mergeKeyringFromDisk(	ops_keyring_t *keyring,
													std::map<std::string,PGPCertificateInfo>& kmap,
													const std::string& keyring_file)
{
#ifdef DEBUG_PGPHANDLER
	std::cerr << "Merging keyring " << keyring_file << " from disk to memory." << std::endl;
#endif

	// 1 - load keyring into a temporary keyring list.
	ops_keyring_t *tmp_keyring = PGPHandler::allocateOPSKeyring() ;

	if(ops_false == ops_keyring_read_from_file(tmp_keyring, false, keyring_file.c_str()))
	{
		std::cerr << "PGPHandler::mergeKeyringFromDisk(): cannot read keyring. File corrupted?" ;
		ops_keyring_free(tmp_keyring) ;
		return ;
	}

	// 2 - load new keys and merge existing key signatures

	for(int i=0;i<tmp_keyring->nkeys;++i)
		addOrMergeKey(keyring,kmap,&tmp_keyring->keys[i]) ;// we dont' account for the return value. This is disk merging, not local changes.	

	// 4 - clean
	ops_keyring_free(tmp_keyring) ;
}



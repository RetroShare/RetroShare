#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include <openpgpsdk/util.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/armour.h>
#include <openpgpsdk/keyring.h>
#include <openpgpsdk/readerwriter.h>
#include <openpgpsdk/validate.h>
#include <openpgpsdk/../../src/parse_local.h>
}
#include "pgphandler.h"
#include "retroshare/rsiface.h"		// For rsicontrol.
#include "util/rsdir.h"		// For rsicontrol.
#include "util/pgpkey.h"		// For rsicontrol.

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

PGPHandler::PGPHandler(const std::string& pubring, const std::string& secring,const std::string& pgp_lock_filename)
	: pgphandlerMtx(std::string("PGPHandler")), _pubring_path(pubring),_secring_path(secring),_pgp_lock_filename(pgp_lock_filename)
{
	_pubring_changed = false ;
	_secring_changed = false ;

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

	std::cerr << "Secring read successfully." << std::endl;
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

void PGPHandler::validateAndUpdateSignatures(PGPCertificateInfo& cert,const ops_keydata_t *keydata)
{
	ops_validate_result_t* result=(ops_validate_result_t*)ops_mallocz(sizeof *result);
	ops_boolean_t res = ops_validate_key_signatures(result,keydata,_pubring,cb_get_passphrase) ;

	// Parse signers.
	//

	if(result != NULL)
		for(size_t i=0;i<result->valid_count;++i)
			cert.signers.insert(PGPIdType(result->valid_sigs[i].signer_id).toStdString()) ;

	ops_validate_result_free(result) ;
}

PGPHandler::~PGPHandler()
{
	std::cerr << "Freeing PGPHandler. Deleting keyrings." << std::endl;

	// no need to free the the _map_ elements. They will be freed by the following calls:
	//
	ops_keyring_free(_pubring) ;
	ops_keyring_free(_secring) ;

	free(_pubring) ;
	free(_secring) ;
}

bool PGPHandler::printKeys() const
{
	std::cerr << "Printing details of all " << std::dec << _public_keyring_map.size() << " keys: " << std::endl;

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
	std::map<std::string,PGPCertificateInfo>::const_iterator it( _public_keyring_map.find(id.toStdString()) ) ;

	if(it != _public_keyring_map.end())
		return &it->second;
	else
		return NULL ;
}

bool PGPHandler::availableGPGCertificatesWithPrivateKeys(std::list<PGPIdType>& ids)
{
	// go through secret keyring, and check that we have the pubkey as well.
	//
	
	const ops_keydata_t *keydata = NULL ;
	int i=0 ;

	while( (keydata = ops_keyring_get_key_by_index(_secring,i++)) != NULL )
	{
		// check that the key is in the pubring as well

		if(ops_keyring_find_key_by_id(_pubring,keydata->key_id) != NULL)
			if(keydata->key.pkey.algorithm == OPS_PKA_RSA)
				ids.push_back(PGPIdType(keydata->key_id)) ;
			else
				std::cerr << "Skipping keypair " << PGPIdType(keydata->key_id).toStdString() << ", unsupported algorithm: " <<  keydata->key.pkey.algorithm << std::endl;
	}

	return true ;
}



bool PGPHandler::GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passwd, PGPIdType& pgpId, std::string& errString) 
{
	static const int KEY_NUMBITS = 2048 ;

	ops_user_id_t uid ;
	char *s = strdup((name + " " + email + " (Generated by RetroShare)").c_str()) ;
	uid.user_id = (unsigned char *)s ;
	unsigned long int e = 17 ; // some prime number

	ops_keydata_t *key = ops_rsa_create_selfsigned_keypair(KEY_NUMBITS,e,&uid) ;

	free(s) ;

	if(!key)
		return false ;

	// 1 - get a passphrase for encrypting.
	
	std::string passphrase = _passphrase_callback(NULL,PGPIdType(key->key_id).toStdString().c_str(),"Please enter passwd for encrypting your key : ",false) ;

	// 2 - save the private key encrypted to a temporary memory buffer

	ops_create_info_t *cinfo = NULL ;
	ops_memory_t *buf = NULL ;
   ops_setup_memory_write(&cinfo, &buf, 0);

	ops_write_transferable_secret_key(key,(unsigned char *)passphrase.c_str(),passphrase.length(),ops_false,cinfo);

	ops_keydata_free(key) ;

	// 3 - read the file into a keyring
	
	ops_keyring_t *tmp_keyring = allocateOPSKeyring() ;
	if(! ops_keyring_read_from_mem(tmp_keyring, ops_false, buf))
	{
		std::cerr << "Cannot re-read key from memory!!" << std::endl;
		return false ;
	}
	ops_teardown_memory_write(cinfo,buf);	// cleanup memory

	// 4 - copy the private key to the private keyring
	
	pgpId = PGPIdType(tmp_keyring->keys[0].key_id) ;
	addNewKeyToOPSKeyring(_secring,tmp_keyring->keys[0]) ;
	initCertificateInfo(_secret_keyring_map[ pgpId.toStdString() ],&tmp_keyring->keys[0],_secring->nkeys-1) ;

	std::cerr << "Added new secret key with id " << pgpId.toStdString() << " to secret keyring." << std::endl;

	// 5 - copy the private key to the public keyring
	
	addNewKeyToOPSKeyring(_pubring,tmp_keyring->keys[0]) ;
	initCertificateInfo(_public_keyring_map[ pgpId.toStdString() ],&tmp_keyring->keys[0],_pubring->nkeys-1) ;

	std::cerr << "Added new public key with id " << pgpId.toStdString() << " to public keyring." << std::endl;

	// 6 - clean

	ops_keyring_free(tmp_keyring) ;
	free(tmp_keyring) ;
	
	// 7 - validate own signature and update certificate.

//	validateAndUpdateSignatures(_public_keyring_map[ pgpId.toStdString() ],getPublicKey(pgpId)) ;

	_pubring_changed = true ;
	_secring_changed = true ;

	return true ;
}

std::string PGPHandler::makeRadixEncodedPGPKey(const ops_keydata_t *key)
{
	ops_boolean_t armoured=ops_true;
   ops_create_info_t* cinfo;

	ops_memory_t *buf = NULL ;
   ops_setup_memory_write(&cinfo, &buf, 0);

   if(ops_write_transferable_public_key(key,armoured,cinfo) != ops_true)
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
	std::cerr << "Reading new key from string: " << std::endl;

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

	std::cerr << "  Key read correctly: " << std::endl;
	ops_keyring_list(tmp_keyring) ;

	const ops_keydata_t *keydata = NULL ;
	int i=0 ;

	while( (keydata = ops_keyring_get_key_by_index(tmp_keyring,i++)) != NULL )
	{
		id = PGPIdType(keydata->key_id) ;

		std::cerr << "  id: " << id.toStdString() << std::endl;

		// See if the key is already in the keyring
		const ops_keydata_t *existing_key ;
		std::map<std::string,PGPCertificateInfo>::const_iterator res = _public_keyring_map.find(id.toStdString()) ;

		if(res == _public_keyring_map.end() || (existing_key = ops_keyring_get_key_by_index(_pubring,res->second._key_index)) == NULL)
		{
			std::cerr << "  Key is new. Adding it to keyring" << std::endl;
			addNewKeyToOPSKeyring(_pubring,*keydata) ; // the key is new.
		}
		else
		{
			std::cerr << "  Key exists. Merging signatures." << std::endl;
			if(mergeKeySignatures(const_cast<ops_keydata_t*>(existing_key),keydata) )
				_pubring_changed = true ;
		}

		initCertificateInfo(_public_keyring_map[id.toStdString()],keydata,_pubring->nkeys-1) ;
		validateAndUpdateSignatures(_public_keyring_map[id.toStdString()],keydata) ;
	}

	std::cerr << "  Added the key in the main public keyring." << std::endl;
	
	ops_keyring_free(tmp_keyring) ;
	free(tmp_keyring) ;

	_pubring_changed = true ;

	return true ;
}

bool PGPHandler::writePublicKeyring() 
{
	RsStackFileLock flck(_pgp_lock_filename) ; // locks access to pgp directory

	_pubring_changed = false ;
	return ops_write_keyring_to_file(_pubring,ops_false,_pubring_path.c_str()) ;
}

bool PGPHandler::writeSecretKeyring() 
{
	RsStackFileLock flck(_pgp_lock_filename) ; // locks access to pgp directory

	_secring_changed = false ;
	return ops_write_keyring_to_file(_secring,ops_false,_secring_path.c_str()) ;
}

bool PGPHandler::encryptTextToFile(const PGPIdType& key_id,const std::string& text,const std::string& outfile) 
{
	ops_create_info_t *info;
	int fd = ops_setup_file_write(&info, outfile.c_str(), ops_true);

	const ops_keydata_t *public_key = getPublicKey(key_id) ;

	if(public_key == NULL)
	{
		std::cerr << "Cannot get public key of id " << key_id.toStdString() << std::endl;
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

	std::cerr << "PGPHandler::decryptTextFromFile: read a file of length " << std::dec << buf.length() << std::endl;
	std::cerr << "buf=\"" << buf << "\"" << std::endl;

	int out_length ;
	ops_boolean_t res = ops_decrypt_memory((const unsigned char *)buf.c_str(),buf.length(),&out_buf,&out_length,_secring,ops_true,cb_get_passphrase) ;

	text = std::string((char *)out_buf,out_length) ;
	free (out_buf);
	return (bool)res ;
}

bool PGPHandler::SignDataBin(const PGPIdType& id,const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen)
{
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

bool PGPHandler::getKeyFingerprint(const PGPIdType& id,PGPFingerprintType& fp) const
{
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

	std::cerr << "Verifying signature from fingerprint " << key_fingerprint.toStdString() << ", length " << std::dec << sign_len << ", literal data length = " << literal_data_length << std::endl;

	return ops_validate_detached_signature(literal_data,literal_data_length,sign,sign_len,key) ;
}

void PGPHandler::setAcceptConnexion(const PGPIdType& id,bool b)
{
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

	for(int i=0;i<p1.length;++i)
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

	std::cerr << "Merging signatures for key " << PGPIdType(dst->key_id).toStdString() << std::endl;
	std::set<ops_packet_t> dst_packets ;

	for(int i=0;i<dst->npackets;++i) dst_packets.insert(dst->packets[i]) ;

	std::set<ops_packet_t> to_add ;

	for(int i=0;i<src->npackets;++i) 
		if(dst_packets.find(src->packets[i]) == dst_packets.end())
		{
			uint8_t tag ;
			uint32_t length ;

			PGPKeyParser::read_packetHeader(src->packets[i].raw,tag,length) ;

			if(tag == PGPKeyParser::PGP_PACKET_TAG_SIGNATURE)
				to_add.insert(src->packets[i]) ;
			else
				std::cerr << "  Packet with tag 0x" << std::hex << (int)(src->packets[i].raw[0]) << std::dec << " not merged, because it is not a signature." << std::endl;
		}

	for(std::set<ops_packet_t>::const_iterator it(to_add.begin());it!=to_add.end();++it)
	{
		std::cerr << "  Adding packet with tag 0x" << std::hex << (int)(*it).raw[0] << std::dec << std::endl;
		ops_add_packet_to_keydata(dst,&*it) ;
	}
	return to_add.size() > 0 ;
}



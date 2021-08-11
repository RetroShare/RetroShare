/*******************************************************************************
 * libretroshare/src/pgp: pgphandler.h                                         *
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
#pragma once

#include <stdint.h>
#include <string>
#include <list>
#include <map>
#include <set>

#include "util/rsthreads.h"
#include "pgp/pgphandler.h"
#include "retroshare/rstypes.h"

extern "C" {
    // we should make sure later on to get rid of these structures in the .h
    #include "openpgpsdk/keyring.h"
}

/// This class offer an abstract pgp handler to be used in RetroShare.
class OpenPGPSDKHandler: public PGPHandler
{
public:
        OpenPGPSDKHandler(	const std::string& path_to_public_keyring,
						const std::string& path_to_secret_keyring, 
						const std::string& path_to_trust_database, 
						const std::string& pgp_lock_file) ;

        virtual ~OpenPGPSDKHandler() ;

        //================================================================================================//
        //                                Implemented API from PGPHandler                                 //
        //================================================================================================//

        virtual std::string makeRadixEncodedPGPKey(uint32_t key_index,bool include_signatures) override;
        virtual bool removeKeysFromPGPKeyring(const std::set<RsPgpId>& key_ids,std::string& backup_file,uint32_t& error_code) override;
        virtual bool availableGPGCertificatesWithPrivateKeys(std::list<RsPgpId>& ids) override;
        virtual bool GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passphrase, RsPgpId& pgpId, const int keynumbits, std::string& errString) override;

        virtual std::string SaveCertificateToString(const RsPgpId& id,bool include_signatures) const override;
        virtual bool exportPublicKey( const RsPgpId& id, unsigned char*& mem_block, size_t& mem_size, bool armoured, bool include_signatures ) const override;

        virtual bool exportGPGKeyPair(const std::string& filename,const RsPgpId& exported_key_id) const override;
        virtual bool exportGPGKeyPairToString( std::string& data, const RsPgpId& exportedKeyId, bool includeSignatures, std::string& errorMsg ) const override;
        virtual bool getGPGDetailsFromBinaryBlock(const unsigned char *mem_block,size_t mem_size,RsPgpId& key_id, std::string& name, std::list<RsPgpId>& signers) const override;
        virtual bool importGPGKeyPair(const std::string& filename,RsPgpId& imported_key_id,std::string& import_error) override;
        virtual bool importGPGKeyPairFromString(const std::string &data, RsPgpId &imported_key_id, std::string &import_error) override;
        virtual bool LoadCertificateFromBinaryData(const unsigned char *data,uint32_t data_len,RsPgpId& id,std::string& error_string) override;
        virtual bool LoadCertificateFromString(const std::string& pgp_cert,RsPgpId& id,std::string& error_string) override;
        virtual bool encryptTextToFile(const RsPgpId& key_id,const std::string& text,const std::string& outfile) override;
        virtual bool encryptDataBin(const RsPgpId& key_id,const void *data, const uint32_t len, unsigned char *encrypted_data, unsigned int *encrypted_data_len) override;
        virtual bool decryptDataBin(const RsPgpId& /*key_id*/,const void *encrypted_data, const uint32_t encrypted_len, unsigned char *data, unsigned int *data_len) override;
        virtual bool decryptTextFromFile(const RsPgpId&,std::string& text,const std::string& inputfile) override;
        virtual bool SignDataBin(const RsPgpId& id,const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen,bool use_raw_signature, std::string reason /* = "" */) override;
        virtual bool privateSignCertificate(const RsPgpId& ownId,const RsPgpId& id_of_key_to_sign) override;
        virtual bool VerifySignBin(const void *literal_data, uint32_t literal_data_length, unsigned char *sign, unsigned int sign_len, const PGPFingerprintType& key_fingerprint) override;
        virtual bool getKeyFingerprint(const RsPgpId& id, RsPgpFingerprint& fp) const override;
        virtual bool haveSecretKey(const RsPgpId& id) const override;
        virtual bool syncDatabase() override;
    private:
        bool locked_syncPublicKeyring() ;

        void initCertificateInfo(PGPCertificateInfo& cert,const ops_keydata_t *keydata,uint32_t i) ;
        bool LoadCertificate(const unsigned char *data,uint32_t data_len,bool armoured,RsPgpId& id,std::string& error_string) ;

		// Returns true if the signatures have been updated
		//
		bool validateAndUpdateSignatures(PGPCertificateInfo& cert,const ops_keydata_t *keydata) ;

        /** Check public/private key and import them into the keyring
         * @param keyring keyring with the new public/private key pair. Will be freed by the function.
         * @param imported_key_id PGP id of the imported key
         * @param import_error human readbale error message
         * @returns true on success
         * */
        bool checkAndImportKeyPair(ops_keyring_t *keyring, RsPgpId& imported_key_id,std::string& import_error);

		const ops_keydata_t *locked_getPublicKey(const RsPgpId&,bool stamp_the_key) const;
		const ops_keydata_t *locked_getSecretKey(const RsPgpId&) const ;

		void locked_mergeKeyringFromDisk(ops_keyring_t *keyring, std::map<RsPgpId,PGPCertificateInfo>& kmap, const std::string& keyring_file) ;
		bool locked_addOrMergeKey(ops_keyring_t *keyring,std::map<RsPgpId,PGPCertificateInfo>& kmap,const ops_keydata_t *keydata) ;

		// Members.
		//
		ops_keyring_t *_pubring ;
		ops_keyring_t *_secring ;

        void printOPSKeys() const;

        // Helper functions.
		//
		static std::string makeRadixEncodedPGPKey(const ops_keydata_t *key,bool include_signatures) ;
		static ops_keyring_t *allocateOPSKeyring() ;
		static void addNewKeyToOPSKeyring(ops_keyring_t*, const ops_keydata_t&) ;
		static bool mergeKeySignatures(ops_keydata_t *dst,const ops_keydata_t *src) ;	// returns true if signature lists are different
};

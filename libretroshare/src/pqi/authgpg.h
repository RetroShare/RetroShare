/*
 * libretroshare/src/   : gpgauthmgr.h
 *
 * GPG  interface for RetroShare.
 *
 * Copyright 2008-2009 by Raghu Dev R.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
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
 * This is *THE* auth manager. It provides the web-of-trust via
 * gpgme, and authenticates the certificates that are managed
 * by the sublayer AuthSSL.
 *
 */

/****
 * Here's GPG policy :
 * By default, all pgpg keys imported via a RS user (make friend and accept friend action) are signed at level 0.
 * All signed keys by RS are set to be trusted marginally. You can change it to full or no trust in the friend profile
 * For a key to be marginaly valid, it has to be signed by one fully trusted key, or at least by 3 marginally trusted keys.
 * All keys that have at least marginal validity are designed as valid in RS. They are shown in the RS gui in order to be signed.
 * If there is no validity then the key is not shown.
 */

#ifndef RS_GPG_AUTH_HEADER
#define RS_GPG_AUTH_HEADER

#include "util/rsthreads.h"
#include "pqi/p3cfgmgr.h"
#include "pgp/pgphandler.h"
#include "rsserver/rsaccounts.h"

#define MAX_GPG_SIGNATURE_SIZE  4096

class RsPeerDetails;

/*!
 * gpgcert is the identifier for a person.
 * It is a wrapper class for a OpenPGP certificate.
 */

class AuthGPGOperation
{
public:
    AuthGPGOperation(void *userdata)
    {
        m_userdata = userdata;
    }
    virtual ~AuthGPGOperation() {}

public:
    void *m_userdata;
};

class AuthGPGOperationLoadOrSave : public AuthGPGOperation
{
public:
	AuthGPGOperationLoadOrSave( bool load, const RsPgpId &gpgId,
	                            const std::string &gpgCert, void *userdata )
    : AuthGPGOperation(userdata)
    {
        m_load = load;
		if (m_load)
		{
			m_certGpg = gpgCert;
			m_certGpgId = gpgId;
		}
		else m_certGpgId = gpgId;
    }

	bool m_load;
	RsPgpId m_certGpgId; // set for save & load.
	std::string m_certGpg; // set for load
};

class AuthGPGService
{
public:
	AuthGPGService() {}
	~AuthGPGService() {}

    virtual AuthGPGOperation *getGPGOperation() = 0;
    virtual void setGPGOperation(AuthGPGOperation *operation) = 0;
};

class AuthGPG: public p3Config, public RsTickingThread, public PGPHandler
{
public:
	/**
	 * Return the only existing instance of AuthGPG.
	 * The instance is created at first invocation of this function.
	 * @return Reference to the only AuthGPG instance
	 */
	static AuthGPG& instance();

	/// @deprecated @see AuthGPG::instance()
	static AuthGPG *getAuthGPG() { return &(instance()); }


	/// Init by generating new Own PGP Cert, or selecting existing PGP Cert
	virtual int GPGInit(const RsPgpId &ownId);

		/*********************************************************************************/
		/************************* STAGE 3 ***********************************************/
		/*********************************************************************************/
		/*****
		 * STAGE 3: These are some of the most commonly used functions in Retroshare.
		 *
		 * More commonly used functions.
		 *
		 * provide access to details in cache list.
		 *
		 ****/
		virtual std::string getGPGName(const RsPgpId &pgp_id,bool *success = NULL);
		virtual std::string getGPGEmail(const RsPgpId &pgp_id,bool *success = NULL);

		/* PGP web of trust management */
        virtual const RsPgpId& getGPGOwnId();
		virtual std::string getGPGOwnName();

		virtual bool	isKeySupported(const RsPgpId &id) const ;
		virtual bool	getGPGDetails(const RsPgpId& id, RsPeerDetails &d);
		virtual bool	getGPGAllList(std::list<RsPgpId> &ids);
		virtual bool	getGPGValidList(std::list<RsPgpId> &ids);
		virtual bool	getGPGAcceptedList(std::list<RsPgpId> &ids);
		virtual bool	getGPGSignedList(std::list<RsPgpId> &ids);

	    virtual bool LoadCertificateFromString( const std::string &pem,
	                                            RsPgpId& gpg_id,
	                                            std::string& error_string );

		/*********************************************************************************/
		/************************* STAGE 6 ***********************************************/
		/*********************************************************************************/
		/*****
		 * STAGE 6: Authentication, Trust and Signing.
		 *
		 * This is some of the harder functions, but they should have been 
		 * done in gpgroot already.
		 *
		 ****/
		virtual bool AllowConnection(const RsPgpId &gpg_id, bool accept);

		virtual bool SignCertificateLevel0(const RsPgpId &id);
		virtual bool RevokeCertificate(const RsPgpId &id);  /* Particularly hard - leave for later */

		virtual bool TrustCertificate(const RsPgpId& id,  int trustlvl); //trustlvl is 2 for none, 3 for marginal and 4 for full trust

	    // Signing and [de]encrypting data.

	    virtual bool SignDataBin( const void *data, uint32_t len,
	                              unsigned char *sign, unsigned int *signlen,
	                              const std::string &reason = "" );

	    // bool encryptDataBin(...); @see PGPHandler::encryptDataBin(...)
		virtual bool decryptDataBin(const void *data, const uint32_t len, unsigned char *decr, unsigned int *decrlen);

		virtual bool decryptTextFromFile(      std::string& text,const std::string& filename);
		virtual bool encryptTextToFile  (const std::string& text,const std::string& filename);

		//END of PGP public functions

		/* GPG service */
		virtual bool addService(AuthGPGService *service) ;

	protected:
		AuthGPG(const std::string& path_to_pubring, const std::string& path_to_secring,const std::string& path_to_trustdb,const std::string& pgp_lock_file);
		~AuthGPG() { fullstop(); }

		/*****************************************************************/
		/***********************  p3config  ******************************/
		/* Key Functions to be overloaded for Full Configuration */
		virtual RsSerialiser *setupSerialiser();
		virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
		virtual bool    loadList(std::list<RsItem *>& load);
		/*****************************************************************/

private:
	/**
	 * Gets the certificate pointer and returns NULL if the string is
	 * invalid, or the cert was not found.
	 */
	const PGPCertificateInfo *getCertInfoFromStdString(const std::string& ) const;

		/* GPG service */
		void    processServices();

		bool    printAllKeys_locked();
		bool    printOwnKeys_locked();

		/* own thread */
        virtual void data_tick();

private:
	RsMutex gpgMtxService;

	RsMutex gpgMtxData; // Below is protected via the mutex

	time_t mStoreKeyTime;

	RsPgpId mOwnGpgId;
	bool _force_sync_database;
	uint32_t mCount;

	std::list<AuthGPGService*> services;
};

#endif

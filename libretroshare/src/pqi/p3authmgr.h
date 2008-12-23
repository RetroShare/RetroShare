/*
 * libretroshare/src/pqi: p3authmgr.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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
 */

#ifndef RS_GENERIC_AUTH_HEADER
#define RS_GENERIC_AUTH_HEADER

#include <list>
#include <map>
#include <string>

/************** GENERIC AUTHENTICATION MANAGER ***********
 * Provides a common interface for certificates.
 *
 * Initialisation must be done in derived classes
 *
 * Key features:
 *  everything indexed by std::string id;
 *  has auth perspective: authed / not authed - different to friends.
 *  load/save certificates as strings or files.
 *
 */

class p3AuthMgr;
extern p3AuthMgr *authMgr;

p3AuthMgr *getAuthMgr();

class pqiAuthDetails
{
	public:
	pqiAuthDetails();

	std::string id;
	std::string name;
	std::string email;
	std::string location;
	std::string org;

	std::string fpr; /* fingerprint */
	std::list<std::string> signers;

	uint32_t trustLvl;

	bool ownsign;
	bool trusted;
};


class p3AuthMgr
{
	public:

virtual ~p3AuthMgr() { return; }

		/* initialisation -> done by derived classes */
virtual bool    active() = 0;
virtual int     InitAuth(const char *srvr_cert, const char *priv_key, 
                                        const char *passwd) = 0;
virtual bool    CloseAuth() = 0;
virtual int     setConfigDirectories(std::string confFile, std::string neighDir) = 0;

		/* get Certificate Ids */

virtual std::string OwnId() = 0;
virtual bool	getAllList(std::list<std::string> &ids) = 0;
virtual bool	getAuthenticatedList(std::list<std::string> &ids) = 0;
virtual bool	getUnknownList(std::list<std::string> &ids) = 0;

		/* get Details from the Certificates */

virtual	bool	isValid(std::string id) = 0;
virtual	bool	isAuthenticated(std::string id) = 0;
virtual	std::string getName(std::string id) = 0;
virtual	bool	getDetails(std::string id, pqiAuthDetails &details) = 0;

		/* High Level Load/Save Configuration */
virtual bool FinalSaveCertificates() = 0;
virtual bool CheckSaveCertificates() = 0;
virtual bool saveCertificates() = 0;
virtual bool loadCertificates() = 0;

		/* first party trust info */
virtual bool isTrustingMe(std::string id) = 0;
virtual void addTrustingPeer(std::string id) = 0;

		/* Load/Save certificates */

virtual bool LoadCertificateFromString(std::string pem, std::string &id)  = 0;
virtual std::string SaveCertificateToString(std::string id)  = 0;
virtual bool LoadCertificateFromFile(std::string filename, std::string &id)  = 0;
virtual bool SaveCertificateToFile(std::string id, std::string filename)  = 0;

		/* specific OpenSSL ones -> careful with pointers.... 
		 * save will allocate space, 
		 */
virtual bool LoadCertificateFromBinary(const uint8_t *ptr, uint32_t len, std::string &id) = 0;
virtual bool SaveCertificateToBinary(std::string id, uint8_t **ptr, uint32_t *len) 	= 0;

		/* Signatures */
virtual bool AuthCertificate(std::string uid) = 0;
virtual bool SignCertificate(std::string id) = 0;
virtual	bool RevokeCertificate(std::string id) = 0;
virtual bool TrustCertificate(std::string id, bool trust) = 0;

		/* Sign / Encrypt / Verify Data (TODO) */
virtual bool    SignData(std::string input, std::string &sign) = 0;
virtual bool    SignData(const void *data, const uint32_t len, std::string &sign) = 0;
virtual bool    SignDataBin(std::string input, unsigned char *sign, unsigned int *signlen) = 0;
virtual bool    SignDataBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int *signlen) = 0;

virtual bool    VerifySignBin(std::string pid, 
				const void *data, const uint32_t len,
                        	unsigned char *sign, unsigned int signlen) = 0;

//virtual	bool encryptData(std::string recipientId, std::string plaindata, std::string &result);

};


class p3DummyAuthMgr: public p3AuthMgr
{
	public:

	p3DummyAuthMgr();
	p3DummyAuthMgr(std::string ownId, std::list<pqiAuthDetails> peers);

		/* initialisation -> done by derived classes */
virtual bool    active();
virtual int     InitAuth(const char *srvr_cert, const char *priv_key, 
                                        const char *passwd);
virtual bool    CloseAuth();
virtual int     setConfigDirectories(std::string confFile, std::string neighDir);

		/* get Certificate Ids */

virtual std::string OwnId();
virtual bool	getAllList(std::list<std::string> &ids);
virtual bool	getAuthenticatedList(std::list<std::string> &ids);
virtual bool	getUnknownList(std::list<std::string> &ids);

		/* get Details from the Certificates */

virtual	bool	isValid(std::string id);
virtual	bool	isAuthenticated(std::string id);
virtual	std::string getName(std::string id);
virtual	bool	getDetails(std::string id, pqiAuthDetails &details);

		/* High Level Load/Save Configuration */
virtual bool FinalSaveCertificates();
virtual bool CheckSaveCertificates();
virtual bool saveCertificates();
virtual bool loadCertificates();

		/* first party trust info */
virtual bool isTrustingMe(std::string id) ;
virtual void addTrustingPeer(std::string id) ;

		/* Load/Save certificates */
virtual bool LoadCertificateFromString(std::string pem, std::string &id);
virtual std::string SaveCertificateToString(std::string id);
virtual bool LoadCertificateFromFile(std::string filename, std::string &id);
virtual bool SaveCertificateToFile(std::string id, std::string filename);

virtual bool LoadCertificateFromBinary(const uint8_t *ptr, uint32_t len, std::string &id);
virtual bool SaveCertificateToBinary(std::string id, uint8_t **ptr, uint32_t *len);
		/* Signatures */

virtual bool AuthCertificate(std::string uid);
virtual bool SignCertificate(std::string id);
virtual	bool RevokeCertificate(std::string id);
virtual bool TrustCertificate(std::string id, bool trust);

virtual bool SignData(std::string input, std::string &sign);
virtual bool SignData(const void *data, const uint32_t len, std::string &sign);
virtual bool    SignDataBin(std::string input, unsigned char *sign, unsigned int *signlen);
virtual bool    SignDataBin(const void *data, const uint32_t len,
                        unsigned char *sign, unsigned int *signlen);

virtual bool    VerifySignBin(std::string pid, 
				const void *data, const uint32_t len,
                        	unsigned char *sign, unsigned int signlen);

	std::string mOwnId;
	std::map<std::string, pqiAuthDetails> mPeerList;
};

#endif





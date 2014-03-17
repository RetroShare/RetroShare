/*
 * libretroshare/src/rsserver/rsaccounts.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2013-2014 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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


/*********************************************************************
 * Header providing interface for libretroshare access to RsAccounts stuff.
 * External access must be through rsinit.g where a RsAccounts namespace + fns 
 * are available.
 *
 */

#pragma once

#include <string>
#include <list>
#include <map>
#include "retroshare/rstypes.h"

class AccountDetails
{
	public:
		AccountDetails();

		RsPeerId mSslId;
		std::string mAccountDir;

		RsPgpId mPgpId;
		std::string mPgpName;
		std::string mPgpEmail;

		std::string mLocation;
		bool mIsHiddenLoc;
		bool mFirstRun;

};

class RsAccountsDetail
{
	public:
		RsAccountsDetail();

	// These functions are externally accessible via RsAccounts namespace.



	// These functions are accessible from inside libretroshare.

		bool	setupBaseDirectory(std::string alt_basedir);
		bool 	loadAccounts();
		bool	lockPreferredAccount();

		// Paths.
		std::string 	PathDataDirectory();
		std::string 	PathBaseDirectory();

		// PGP Path is only dependent on BaseDirectory.
		std::string 	PathPGPDirectory();

		// Below are dependent on mPreferredId.
		std::string 	PathAccountDirectory();
		std::string 	PathAccountKeysDirectory();
		std::string 	PathKeyFile();
		std::string 	PathCertFile();

		// PGP Accounts.

		int  	GetPGPLogins(std::list<RsPgpId> &pgpIds);
		int	GetPGPLoginDetails(const RsPgpId& id, std::string &name, std::string &email);
		bool	GeneratePGPCertificate(const std::string&, const std::string& email, const std::string& passwd, RsPgpId &pgpId, std::string &errString);

                bool    SelectPGPAccount(const RsPgpId& pgpId);

		// PGP Support Functions.
		bool    exportIdentity(const std::string& fname,const RsPgpId& pgp_id) ;
		bool    importIdentity(const std::string& fname,RsPgpId& imported_pgp_id,std::string& import_error) ;
        	void    getUnsupportedKeys(std::map<std::string,std::vector<std::string> > &unsupported_keys);
		bool    copyGnuPGKeyrings() ;


		// Selecting Rs Account.
		bool selectAccountByString(const std::string &prefUserString);
		bool selectId(const RsPeerId& preferredId);

		// Details of Rs Account.
		bool getPreferredAccountId(RsPeerId &id);
		bool getAccountDetails(const RsPeerId &id, RsPgpId& gpgId, std::string &gpgName, std::string &gpgEmail, std::string &location);

		bool getAccountOptions(bool &ishidden, bool isFirstTimeRun);


		bool getAccountIds(std::list<RsPeerId> &ids);

		bool GenerateSSLCertificate(const RsPgpId& gpg_id, 
			const std::string& org, const std::string& loc, 
			const std::string& country, const bool ishiddenloc, 
			const std::string& passwd, RsPeerId &sslId, 
			std::string &errString);

		// From init file.
		bool storePreferredAccount();
		bool loadPreferredAccount();

	private:
		bool checkPreferredId();

		bool defaultBaseDirectory();

		std::string getHomePath() ;

		bool getAvailableAccounts(std::map<RsPeerId, AccountDetails> &accounts, 
			int& failing_accounts,
			std::map<std::string,std::vector<std::string> >& unsupported_keys);

		bool setupAccount(const std::string& accountdir);

	private:

		bool mAccountsLocked;

		std::map<RsPeerId, AccountDetails> mAccounts;
		RsPeerId mPreferredId;
		std::string mBaseDirectory;

		std::map<std::string,std::vector<std::string> > mUnsupportedKeys ;
};

// Global singleton declaration of data.
extern RsAccountsDetail rsAccounts;


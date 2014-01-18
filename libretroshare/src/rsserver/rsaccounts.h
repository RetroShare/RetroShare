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

class AccountDetails
{
	public:
		AccountDetails();

		std::string mSslId;
		std::string mAccountDir;

		std::string mPgpId;
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

		int  	GetPGPLogins(std::list<std::string> &pgpIds);
		int	GetPGPLoginDetails(const std::string& id, std::string &name, std::string &email);
		bool	GeneratePGPCertificate(const std::string&, const std::string& email, const std::string& passwd, std::string &pgpId, std::string &errString);

                bool    SelectPGPAccount(const std::string& pgpId);

		// PGP Support Functions.
		bool    exportIdentity(const std::string& fname,const std::string& pgp_id) ;
		bool    importIdentity(const std::string& fname,std::string& imported_pgp_id,std::string& import_error) ;
        	void    getUnsupportedKeys(std::map<std::string,std::vector<std::string> > &unsupported_keys);
		bool    copyGnuPGKeyrings() ;


		// Selecting Rs Account.
		bool selectAccountByString(const std::string &prefUserString);
		bool selectId(const std::string preferredId);

		// Details of Rs Account.
		bool getPreferredAccountId(std::string &id);
		bool getAccountDetails(const std::string &id,
			std::string &gpgId, std::string &gpgName, 
			std::string &gpgEmail, std::string &location);

		bool getAccountOptions(bool &ishidden, bool isFirstTimeRun);


		bool getAccountIds(std::list<std::string> &ids);

		bool GenerateSSLCertificate(const std::string& gpg_id, 
			const std::string& org, const std::string& loc, 
			const std::string& country, const bool ishiddenloc, 
			const std::string& passwd, std::string &sslId, 
			std::string &errString);

		// From init file.
		bool storePreferredAccount();
		bool loadPreferredAccount();

	private:
		bool checkPreferredId();

		bool defaultBaseDirectory();

		std::string getHomePath() ;

		bool getAvailableAccounts(std::map<std::string, AccountDetails> &accounts, 
			int& failing_accounts,
			std::map<std::string,std::vector<std::string> >& unsupported_keys);

		bool setupAccount(const std::string& accountdir);

	private:

		bool mAccountsLocked;

		std::map<std::string, AccountDetails> mAccounts;
		std::string mPreferredId;
		std::string mBaseDirectory;

		std::map<std::string,std::vector<std::string> > mUnsupportedKeys ;
};

// Global singleton declaration of data.
extern RsAccountsDetail rsAccounts;


/*******************************************************************************
 * libretroshare/src/rsserver: rsaccounts.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013-2014 by Robert Fernie <retroshare@lunamutt.com>              *
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
        bool mIsAutoTor;

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
		void	unlockPreferredAccount();
		bool	checkAccountDirectory();

		// Paths.
        /**
         * @brief PathDataDirectory
         * @param check if set to true and directory does not exist, return empty string
         * @return path where global platform independent files are stored, like bdboot.txt or webinterface files
         */
        static std::string 	PathDataDirectory(bool check = true);

		/**
		 * @brief PathBaseDirectory
		 * @return path where user data is stored ( on Linux and similar
		 * systems it is usually something like /home/USERNAME/.retroshare ).
		 */
		static std::string PathBaseDirectory();

		// PGP Path is only dependent on BaseDirectory.
		std::string 	PathPGPDirectory();

        // Generate a new account based on a given PGP key returns its SSL id and sets it to be the preferred account.

		bool GenerateSSLCertificate(const RsPgpId& gpg_id,  const std::string& org, const std::string& loc,  const std::string& country, bool ishiddenloc,  bool is_auto_tor,const std::string& passwd, RsPeerId &sslId,  std::string &errString);

		// PGP Accounts.

		int  	GetPGPLogins(std::list<RsPgpId> &pgpIds);
		int	    GetPGPLoginDetails(const RsPgpId& id, std::string &name, std::string &email);
		bool	GeneratePGPCertificate(const std::string&, const std::string& email, const std::string& passwd, RsPgpId &pgpId, const int keynumbits, std::string &errString);
		bool    SelectPGPAccount(const RsPgpId& pgpId);

		// PGP Support Functions.
		bool    exportIdentity(const std::string& fname,const RsPgpId& pgp_id) ;
		bool    importIdentity(const std::string& fname,RsPgpId& imported_pgp_id,std::string& import_error) ;
	bool exportIdentityToString(
	        std::string& data, const RsPgpId& pgpId, bool includeSignatures,
	        std::string& errorMsg );
        bool    importIdentityFromString(const std::string& data,RsPgpId& imported_pgp_id,std::string& import_error) ;
		void    getUnsupportedKeys(std::map<std::string,std::vector<std::string> > &unsupported_keys);
		bool    copyGnuPGKeyrings() ;

		// Selecting Rs Account.
		bool getAccountIds(std::list<RsPeerId> &ids);
		bool selectAccountByString(const std::string &prefUserString);
		bool selectId(const RsPeerId& preferredId);
		bool storePreferredAccount();
		bool loadPreferredAccount();

		// Details of current Rs Account.
		bool getCurrentAccountId(RsPeerId &id);
		bool getCurrentAccountDetails(const RsPeerId &id, RsPgpId& gpgId, std::string &gpgName, std::string &gpgEmail, std::string &location);
		bool getCurrentAccountOptions(bool &ishidden, bool &isautotor, bool &isFirstTimeRun);

		std::string 	getCurrentAccountPathAccountDirectory();
		std::string 	getCurrentAccountPathAccountKeysDirectory();
		std::string 	getCurrentAccountPathKeyFile();
		std::string 	getCurrentAccountPathCertFile();
        std::string     getCurrentAccountLocationName();


	private:
		bool checkPreferredId();

		static bool defaultBaseDirectory();

		bool getAvailableAccounts(std::map<RsPeerId, AccountDetails> &accounts,
			int& failing_accounts,
			std::map<std::string,std::vector<std::string> >& unsupported_keys, bool hidden_only=false);

		bool setupAccount(const std::string& accountdir);

	private:

		bool mAccountsLocked;

		std::map<RsPeerId, AccountDetails> mAccounts;
		RsPeerId mPreferredId;
		static std::string mBaseDirectory;

		std::map<std::string,std::vector<std::string> > mUnsupportedKeys ;
};



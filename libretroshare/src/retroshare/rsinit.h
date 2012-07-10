#ifndef RETROSHARE_INIT_INTERFACE_H
#define RETROSHARE_INIT_INTERFACE_H

/*
 * "$Id: rsiface.h,v 1.9 2007-04-21 19:08:51 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2006 by Robert Fernie.
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

// Initialize ok, result >= 0
#define RS_INIT_OK              0 // Initialize ok
#define RS_INIT_HAVE_ACCOUNT    1 // Initialize ok, have account
// Initialize failed, result < 0
#define RS_INIT_AUTH_FAILED    -1 // AuthGPG::InitAuth failed
#define RS_INIT_BASE_DIR_ERROR -2 // AuthGPG::InitAuth failed
#define RS_INIT_NO_KEYRING     -3 // Keyring is empty. Need to import it.


/****
 * #define RS_USE_PGPSSL 1
 ***/

#define RS_USE_PGPSSL 1

#include <list>

/*!
 * Initialisation Class (not publicly disclosed to RsIFace)
 */
class RsInit
{
	public:
		/* reorganised RsInit system */

		/*!
		 * PreLogin
		 * Call before init retroshare, initialises rsinitconfig's public attributes
		 */
		static void	InitRsConfig() ;

		/*!
		 * Should be called to load up ssl cert and private key, and intialises gpg
		 * this must be called before accessing rsserver (e.g. ::startupretroshare)
		 * @param argc passed from executable
		 * @param argv commandline arguments passed to executable
		 * @param strictCheck set to true if you want rs to continue executing if invalid argument passed and vice versa
		 * @return RS_INIT_...
		 */
		static int 	InitRetroShare(int argc, char **argv, bool strictCheck=true);

		static bool isPortable();
		static bool isWindowsXP();

		/*!
		 *  Account Details (Combined GPG+SSL Setup)
		 */
		static bool 	getPreferedAccountId(std::string &id);
		static bool     getPGPEngineFileName(std::string &fileName);
		static bool 	getAccountIds(std::list<std::string> &ids);
		static bool 	getAccountDetails(std::string id, std::string &gpgId, std::string &gpgName, std::string &gpgEmail, std::string &sslName);

		static bool	ValidateCertificate(std::string &userName) ;

		static bool exportIdentity(const std::string& fname,const std::string& pgp_id) ;
		static bool importIdentity(const std::string& fname,std::string& imported_pgp_id) ;

		/*!
		 *  Generating GPGme Account
		 */
		static int 	GetPGPLogins(std::list<std::string> &pgpIds);
		static int 	GetPGPLoginDetails(const std::string& id, std::string &name, std::string &email);
		static bool	GeneratePGPCertificate(const std::string&, const std::string& email, const std::string& passwd, std::string &pgpId, std::string &errString);

		// copies existing gnupg keyrings to the new place of the OpenPGP-SDK version. Returns true on success.
		static bool copyGnuPGKeyrings() ;

		/*!
		 * Login GGP
		 */
		static bool 	SelectGPGAccount(const std::string& gpgId);
		static bool 	LoadGPGPassword(const std::string& passwd);

		/*!
		 * Create SSL Certificates
		 */
		static bool	GenerateSSLCertificate(const std::string& name, const std::string& org, const std::string& loc, const std::string& country, const std::string& passwd, std::string &sslId, std::string &errString);

		/*!
		 * intialises directories for passwords and ssl keys
		 */
		static bool	LoadPassword(const std::string& id, const std::string& passwd) ;

		/*!
		 * Final Certificate load. This can be called if:
		 * a) InitRetroshare() returns RS_INIT_HAVE_ACCOUNT -> autoLoad/password Set.
		 * b) SelectGPGAccount() && LoadPassword()
		 *
		 * This wrapper is used to lock the profile first before
		 * finalising the login
		 */
		static int 	LockAndLoadCertificates(bool autoLoginNT, std::string& lockFilePath);

		/*!
		 * Post Login Options
		 */
		static std::string 	RsConfigDirectory();
		static std::string 	RsConfigKeysDirectory();

		static std::string  RsProfileConfigDirectory();
		static bool         getStartMinimised() ;
		static std::string  getRetroShareLink();

		static int getSslPwdLen();
		static bool getAutoLogin();
		static void setAutoLogin(bool autoLogin);
		static bool RsClearAutoLogin() ;

		/* used for static install data */
		static std::string getRetroshareDataDirectory();

	private:

		/* PreLogin */
		static std::string getHomePath() ;
		static bool setupBaseDir();

		/* Account Details */
		static bool    get_configinit(const std::string& dir, std::string &id);
		static bool    create_configinit(const std::string& dir, const std::string& id);

		static bool setupAccount(const std::string& accountdir);

		/* Auto Login */
		static bool RsStoreAutoLogin() ;
		static bool RsTryAutoLogin() ;

		/* Lock/unlock profile directory */
                static int	LockConfigDirectory(const std::string& accountDir, std::string& lockFilePath);
		static void	UnlockConfigDirectory();

		/* The true LoadCertificates() method */
		static int 	LoadCertificates(bool autoLoginNT) ;

};

#endif

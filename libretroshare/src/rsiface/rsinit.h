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


/* Initialisation Class (not publicly disclosed to RsIFace) */

/****
 * #define RS_USE_PGPSSL 1
 ***/

#define RS_USE_PGPSSL 1

class RsInit
{
	public:
		/* reorganised RsInit system */

		/* PreLogin */
		static void	InitRsConfig() ;
		static int 	InitRetroShare(int argc, char **argv,
					bool strictCheck=true);

		static char dirSeperator();
		static bool isPortable();


		/* Account Details (Combined GPG+SSL Setup) */
		static bool 	getPreferedAccountId(std::string &id);
                static bool     getPGPEngineFileName(std::string &fileName);
		static bool 	getAccountIds(std::list<std::string> &ids);
		static bool 	getAccountDetails(std::string id, 
					std::string &gpgId, std::string &gpgName, 
					std::string &gpgEmail, std::string &sslName);

		static bool	ValidateCertificate(std::string &userName) ;


		/* Generating GPGme Account */
		static int 	GetPGPLogins(std::list<std::string> &pgpIds);
		static int 	GetPGPLoginDetails(std::string id, std::string &name, std::string &email);
                static bool	GeneratePGPCertificate(std::string name, std::string email, std::string passwd, std::string &pgpId, std::string &errString);

		/* Login PGP */
		static bool 	SelectGPGAccount(const std::string& gpgId);
		static bool 	LoadGPGPassword(std::string passwd);

		/* Create SSL Certificates */
		static bool	GenerateSSLCertificate(std::string name, std::string org, std::string loc, std::string country, std::string passwd, std::string &sslId, std::string &errString);

		/* Login SSL */
		static bool	LoadPassword(std::string id, std::string passwd) ;

		/** Final Certificate load. This can be called if:
		 * a) InitRetroshare() returns true -> autoLoad/password Set.
		 * b) SelectGPGAccount() && LoadPassword()
		 *
		 * This wrapper is used to lock the profile first before
		 * finalising the login
		 */
		static int 	LockAndLoadCertificates(bool autoLoginNT);


		/* Post Login Options */
		static std::string 	RsConfigDirectory();
                static std::string      RsProfileConfigDirectory();
		static bool	setStartMinimised() ;

		static int getSslPwdLen();
		static bool getAutoLogin();
		static void setAutoLogin(bool autoLogin);
		static bool RsClearAutoLogin() ;


	private:
		/* PreLogin */
		static std::string getHomePath() ;
		static void setupBaseDir();

		/* Account Details */
		static bool    get_configinit(std::string dir, std::string &id);
		static bool    create_configinit(std::string dir, std::string id);

		static bool setupAccount(std::string accountdir);

		/* Auto Login */
		static bool RsStoreAutoLogin() ;
		static bool RsTryAutoLogin() ;

		/* Lock/unlock profile directory */
		static int	LockConfigDirectory(const std::string& accountDir);
		static void	UnlockConfigDirectory();

		/* The true LoadCertificates() method */
		static int 	LoadCertificates(bool autoLoginNT) ;

};



#endif

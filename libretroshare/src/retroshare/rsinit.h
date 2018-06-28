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

#include <stdint.h>
#include <list>
#include <map>
#include <vector>
#include <retroshare/rstypes.h>

struct RsLoginHelper;

/**
 * Pointer to global instance of RsLoginHelper
 * @jsonapi{development}
 */
extern RsLoginHelper* rsLoginHelper;

/*!
 * Initialisation Class (not publicly disclosed to RsIFace)
 */
class RsInit
{
public:
	enum LoadCertificateStatus : uint8_t
	{
		OK,                     /// Everything go as expected, no error occurred
		ERR_ALREADY_RUNNING,    /// Another istance is running already
		ERR_CANT_ACQUIRE_LOCK,  /// Another istance is already running?
		ERR_UNKOWN              /// Unkown error, maybe password is wrong?
	};

	/* reorganised RsInit system */

	/*!
	 * PreLogin
	 * Call before init retroshare, initialises rsinitconfig's public attributes
	 */
	static void	InitRsConfig();

	/*!
	 * Should be called to load up ssl cert and private key, and intialises gpg
	 * this must be called before accessing rsserver (e.g. ::startupretroshare)
	 * @param argc passed from executable
	 * @param argv commandline arguments passed to executable
	 * @param strictCheck set to true if you want rs to continue executing if
	 *	invalid argument passed and vice versa
	 * @return RS_INIT_...
	 */
	static int InitRetroShare(int argc, char **argv, bool strictCheck=true);

	static bool isPortable();
	static bool isWindowsXP();
	static bool collectEntropy(uint32_t bytes) ;

	/*
	 * Setup Hidden Location;
	 */
	static void SetHiddenLocation(const std::string& hiddenaddress, uint16_t port, bool useBob);

	static bool LoadPassword(const std::string& passwd) ;

	/*
	 * Final Certificate load. This can be called if:
	 * a) InitRetroshare() returns RS_INIT_HAVE_ACCOUNT -> autoLoad/password Set.
	 * b) or LoadPassword()
	 *
	 * This uses the preferredId from RsAccounts.
	 * This wrapper also locks the profile before finalising the login
	 */
	static LoadCertificateStatus LockAndLoadCertificates(
	        bool autoLoginNT, std::string& lockFilePath );

	// Post Login Options
	static bool getStartMinimised();

	static int getSslPwdLen();
	static bool getAutoLogin();
	static void setAutoLogin(bool autoLogin);
	static bool RsClearAutoLogin() ;

private:
	/** @brief Lock profile directory
	 * param[in] accountDir account directory to lock
	 * param[out] lockFilePath path of the created lock-file
	 */
	static LoadCertificateStatus LockConfigDirectory(
	        const std::string& accountDir, std::string& lockFilePath);

	/// @brief Unlock profile directory
	static void UnlockConfigDirectory();

	static int LoadCertificates(bool autoLoginNT);
};



/* Seperate Class for dealing with Accounts */

namespace RsAccounts
{
    /**
	 * @brief ConfigDirectory (normally ~/.retroshare) you can call this method
	 * even before initialisation (you can't with some other methods)
	 * @see RsAccountsDetail::PathBaseDirectory()
	 */
    std::string ConfigDirectory();

	/**
     * @brief DataDirectory
	 * you can call this method even before initialisation (you can't with some other methods)
     * @param check if set to true and directory does not exist, return empty string
     * @return path where global platform independent files are stored, like bdboot.txt or webinterface files
     */
    std::string DataDirectory(bool check = true);

	std::string PGPDirectory();
	std::string AccountDirectory();

	// PGP Accounts.
	int     GetPGPLogins(std::list<RsPgpId> &pgpIds);
	int     GetPGPLoginDetails(const RsPgpId& id, std::string &name, std::string &email);
	bool    GeneratePGPCertificate(const std::string&, const std::string& email, const std::string& passwd, RsPgpId &pgpId, const int keynumbits, std::string &errString);

	// PGP Support Functions.
	bool    ExportIdentity(const std::string& fname,const RsPgpId& pgp_id) ;
	bool    ImportIdentity(const std::string& fname,RsPgpId& imported_pgp_id,std::string& import_error) ;
    bool    ImportIdentityFromString(const std::string& data,RsPgpId& imported_pgp_id,std::string& import_error) ;
        void    GetUnsupportedKeys(std::map<std::string,std::vector<std::string> > &unsupported_keys);
	bool    CopyGnuPGKeyrings() ;

	// Rs Accounts
        bool    SelectAccount(const RsPeerId& id);

	bool	GetPreferredAccountId(RsPeerId &id);
	bool    GetAccountIds(std::list<RsPeerId> &ids);
	bool	GetAccountDetails(const RsPeerId &id,
                        RsPgpId &gpgId, std::string &gpgName,
                        std::string &gpgEmail, std::string &location);

	bool	GenerateSSLCertificate(const RsPgpId& pgp_id, const std::string& org, const std::string& loc, const std::string& country, const bool ishiddenloc, const std::string& passwd, RsPeerId &sslId, std::string &errString);

}


/**
 * This helper class have been implemented because there was not reasonable way
 * to login in the API that could be exposed via JSON API
 */
struct RsLoginHelper
{
	/**
	 * @brief Normal way to attempt login
	 * @jsonapi{development}
	 * @param[in] account Id of the account to which attempt login
	 * @param[in] password Password for the given account
	 * @return RsInit::OK if login attempt success, error code otherwhise
	 */
	RsInit::LoadCertificateStatus attemptLogin(
	        const RsPeerId& account, const std::string& password );

	struct Location : RsSerializable
	{
		RsPeerId mLocationId;
		RsPgpId mPgpId;
		std::string mLocationName;
		std::string mPpgName;

		/// @see RsSerializable::serial_process
		void serial_process( RsGenericSerializer::SerializeJob j,
		                     RsGenericSerializer::SerializeContext& ctx );
	};

	/**
	 * @brief Get locations and associated information
	 * @jsonapi{development}
	 * @param[out] locations storage for the retrived locations
	 */
	void getLocations(std::vector<RsLoginHelper::Location>& locations);

	/**
	 * @brief Creates a new RetroShare location
	 * @jsonapi{development}
	 * @param[inout] location provide input information to generate the location
	 *	and storage to output the data of the generated location
	 * @param[in] password to protect and unlock the associated PGP key
	 * @param[out] errorMessage
	 * @return true if success, false otherwise
	 */
	bool createLocation( RsLoginHelper::Location& location,
	                     const std::string& password,
	                     std::string& errorMessage );
};

#endif

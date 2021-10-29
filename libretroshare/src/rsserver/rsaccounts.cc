/*******************************************************************************
 * libretroshare/src/rsserver: rsaccounts.cc                                   *
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
 * Libretroshare interface declared in rsaccounts.h.
 * external interface in rsinit.h RsAccounts namespace.
 *
 */

#ifdef WINDOWS_SYS
#include "util/rswin.h"
#endif // WINDOWS_SYS

#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>

#include "retroshare/rsinit.h"
#include "rsaccounts.h"

#include "util/rsdir.h"
#include "util/rsstring.h"
#include "util/folderiterator.h"

#include "pqi/authssl.h"
#include "pqi/sslfns.h"
#include "pqi/authgpg.h"

#include <openssl/ssl.h>

// Global singleton declaration of data.
RsAccountsDetail* RsAccounts::rsAccountsDetails = nullptr;

/* Uses private class - so must be hidden */
static bool checkAccount(const std::string &accountdir, AccountDetails &account,std::map<std::string,std::vector<std::string> >& unsupported_keys);

AccountDetails::AccountDetails()
  :mSslId(""), mAccountDir(""), mPgpId(""), mPgpName(""), mPgpEmail(""),
    mLocation(""), mIsHiddenLoc(false), mFirstRun(false)
{
	return;
}

RsAccountsDetail::RsAccountsDetail() : mAccountsLocked(false), mPreferredId("")
{}

bool RsAccountsDetail::loadAccounts()
{
	int failing_accounts ;
#warning we might need some switch here for hidden nodes only
	getAvailableAccounts(mAccounts,failing_accounts,mUnsupportedKeys,false);

	loadPreferredAccount();
	checkPreferredId();

        if(failing_accounts > 0 && mAccounts.empty())
                return false;

	return true;
}

bool RsAccountsDetail::lockPreferredAccount()
{
	if (checkPreferredId())
	{
		mAccountsLocked = true;
		return true;
	}
	
	return false;
}

void RsAccountsDetail::unlockPreferredAccount()
{
	mAccountsLocked = false;
}

bool RsAccountsDetail::checkAccountDirectory()
{
	if (!checkPreferredId())
	{
		return false;
	}

	return setupAccount(getCurrentAccountPathAccountDirectory());
}

#warning we need to clean that up. Login should only ask for a SSL id, instead of a std::string.

bool RsAccountsDetail::selectAccountByString(const std::string &prefUserString)
{
	if (mAccountsLocked)
	{
		std::cerr << "RsAccountsDetail::selectAccountByString() ERROR Accounts Locked";
		std::cerr << std::endl;
		return false;
	}
		
	// try both.
	//
	RsPeerId ssl_id(prefUserString) ;
	RsPgpId pgp_id(prefUserString) ;

	std::cerr << "RsAccountsDetail::selectAccountByString(" << prefUserString << ")" << std::endl;
	
	//bool pgpNameFound = false;
	std::map<RsPeerId, AccountDetails>::const_iterator it;
	for(it = mAccounts.begin() ; it!= mAccounts.end() ; ++it)
	{
		std::cerr << "\tChecking account (pgpid = " << it->second.mPgpId;
		std::cerr << ", name=" << it->second.mPgpName << ", sslId="; 
		std::cerr << it->second.mSslId << ")" << std::endl;

		if(prefUserString == it->second.mPgpName || pgp_id == it->second.mPgpId || ssl_id == it->second.mSslId)
		{
			mPreferredId = it->second.mSslId;
			//pgpNameFound = true;

			std::cerr << "Account selected: " << ssl_id << std::endl;

			return true;
		}
	}
	std::cerr << "No suitable candidate found." << std::endl;
	return false;
}


bool RsAccountsDetail::selectId(const RsPeerId& preferredId)
{
	
	if (mAccountsLocked)
	{
		std::cerr << "RsAccountsDetail::selectId() ERROR Accounts Locked";
		std::cerr << std::endl;
		return false;
	}
	
	std::map<RsPeerId, AccountDetails>::const_iterator it;
	it = mAccounts.find(preferredId);

	if (it != mAccounts.end())
	{
		mPreferredId = preferredId;
		return true;
	}
	else
	{
		return false;
	}
}


bool RsAccountsDetail::checkPreferredId()
{
	std::map<RsPeerId, AccountDetails>::const_iterator it;
	it = mAccounts.find(mPreferredId);

	if (it != mAccounts.end())
	{
		return true;
	}
	else
	{
		mPreferredId.clear();
		return false;
	}
}

// initial configuration bootstrapping...

const std::string kPathPGPDirectory = "pgp";
const std::string kPathKeyDirectory = "keys";
const std::string kPathConfigDirectory = "config";

const std::string kFilenamePreferredAccount = "default_cert_06.txt";
const std::string kFilenameKey = "user_pk.pem";
const std::string kFilenameCert = "user_cert.pem";
const std::string kFilenameLocation = "location_name.txt";


/*********************************************************************
 * Directories...  based on current PreferredId.
 */

std::string RsAccountsDetail::PathPGPDirectory()
{
	return mBaseDirectory + "/" + kPathPGPDirectory;
}


std::string RsAccountsDetail::PathBaseDirectory()
{
	if(mBaseDirectory.empty()) defaultBaseDirectory();
	return mBaseDirectory;
}


std::string RsAccountsDetail::getCurrentAccountPathAccountDirectory()
{
	std::string path;

	std::map<RsPeerId, AccountDetails>::const_iterator it;
	it = mAccounts.find(mPreferredId);
	if (it == mAccounts.end())
	{
		return path;
	}

	path = mBaseDirectory + "/";
	path += it->second.mAccountDir;
	return path;
}

std::string RsAccountsDetail::getCurrentAccountPathAccountKeysDirectory()
{
	std::string path = getCurrentAccountPathAccountDirectory();
	if (path.empty())
	{
		return path;	
	}

	path += "/" + kPathKeyDirectory;
	return path;
}

std::string RsAccountsDetail::getCurrentAccountPathKeyFile()
{
	std::string path = getCurrentAccountPathAccountKeysDirectory();
	if (path.empty())
	{
		return path;	
	}

	path += "/" + kFilenameKey;
	return path;
}

std::string RsAccountsDetail::getCurrentAccountPathCertFile()
{
	std::string path = getCurrentAccountPathAccountKeysDirectory();
	if (path.empty())
	{
        return path;
	}
	path += "/" + kFilenameCert;
	return path;
}

std::string RsAccountsDetail::getCurrentAccountLocationName()
{
    std::map<RsPeerId, AccountDetails>::const_iterator it;
    it = mAccounts.find(mPreferredId);
    if (it == mAccounts.end())
    {
        return "";
    }
    return it->second.mLocation;
}


/*********************************************************************
 * Setup Base Directories.
 *
 */

bool RsAccountsDetail::setupBaseDirectory(std::string alt_basedir)
{
	if (alt_basedir.empty())
	{
		if (!defaultBaseDirectory())
		{
			std::cerr << "RsAccounts::setupBaseDirectory() Cannot find defaultBaseDirectory";
			std::cerr << std::endl;
			return false;
		}
	}
	else
	{
		mBaseDirectory = alt_basedir;
	}

	/* Check for trailing '/' */
	if (!mBaseDirectory.empty())
	{
		char lastChar = *mBaseDirectory.rbegin();
		if (lastChar == '/'
#ifdef WINDOWS_SYS
		    || lastChar == '\\'
#endif
		    )
		{
			mBaseDirectory.erase(mBaseDirectory.end() - 1);
		}
	}

	if (!RsDirUtil::checkCreateDirectory(mBaseDirectory))
	{
		std::cerr << "RsAccounts::setupBaseDirectory() Cannot Create BaseConfig Dir:" << mBaseDirectory;
		std::cerr << std::endl;
		return false ;
	}
	return true ;
}


bool RsAccountsDetail::defaultBaseDirectory()
{
	std::string basedir;

/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS

	// unix: homedir + /.retroshare
	char *h = getenv("HOME");
	if (h == NULL)
	{
		std::cerr << "defaultBaseDirectory() Error: cannot determine $HOME dir"
		          << std::endl;
		return false ;
	}

	basedir = h;
	basedir += "/.retroshare";

#else
	if (RsInit::isPortable())
	{
		// use directory "Data" in portable version
		basedir = "Data";
	} 
	else 
	{
		wchar_t *wh = _wgetenv(L"APPDATA");
		std::string h;
		librs::util::ConvertUtf16ToUtf8(std::wstring(wh), h);
		if (h.empty())
		{
			// generating default
			std::cerr << "defaultBaseDirectory() Error: ";
			std::cerr << " getEnv Error --Win95/98?";
			std::cerr << std::endl;
			basedir="C:\\Retro";
		}
		else
		{
			basedir = h;
		}

		if (!RsDirUtil::checkCreateDirectory(basedir))
		{
			std::cerr << "defaultBaseDirectory() Error: ";
			std::cerr << "Cannot Create BaseConfig Dir : " << basedir << std::endl;
			return false ;
		}
		basedir += "\\RetroShare";
	}
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

	/* store to class variable */
	mBaseDirectory = basedir;
	std::cerr << "defaultBaseDirectory() = " << mBaseDirectory;
	std::cerr << std::endl;
	return true;
}


bool	RsAccountsDetail::loadPreferredAccount()
{
	std::string initfile = mBaseDirectory + "/";
	initfile += kFilenamePreferredAccount;

	// open and read in the lines.
	FILE *ifd = RsDirUtil::rs_fopen(initfile.c_str(), "r");
	char path[1024];
	int i;

	if (ifd != NULL)
	{
		if (NULL != fgets(path, 1024, ifd))
		{
			for(i = 0; (path[i] != '\0') && (path[i] != '\n'); i++) ;
			path[i] = '\0';

			// Store PreferredId.
			mPreferredId = RsPeerId(std::string(path));

			if(mPreferredId.isNull())
			{
				fclose(ifd);
				return false ;
			}
		}
		fclose(ifd);
		return true;
	}
	return false;
}

bool	RsAccountsDetail::storePreferredAccount()
{
	// Check for config file.
	std::string initfile = mBaseDirectory + "/";
	initfile += kFilenamePreferredAccount;

	// open and read in the lines.
	FILE *ifd = RsDirUtil::rs_fopen(initfile.c_str(), "w");

	if (ifd != NULL)
	{
		fprintf(ifd, "%s\n", mPreferredId.toStdString().c_str());
		fclose(ifd);

		std::cerr << "Creating Init File: " << initfile << std::endl;
		std::cerr << "\tId: " << mPreferredId << std::endl;

		return true;
	}
	std::cerr << "Failed To Create Init File: " << initfile << std::endl;
	return false;
}


/*********************************************************************
 * Accounts
 *
 */

bool     RsAccountsDetail::getCurrentAccountId(RsPeerId &id)
{
	id = mPreferredId;
	return (!mPreferredId.isNull());
}

bool     RsAccountsDetail::getAccountIds(std::list<RsPeerId> &ids)
{
	std::map<RsPeerId, AccountDetails>::iterator it;
#ifdef DEBUG_ACCOUNTS
	std::cerr << "getAccountIds:" << std::endl;
#endif

	for(it = mAccounts.begin(); it != mAccounts.end(); ++it)
	{
#ifdef DEBUG_ACCOUNTS
		std::cerr << "SSL Id: " << it->second.mSslId << " PGP Id " << it->second.mPgpId;
		std::cerr << " PGP Name: " << it->second.mPgpName;
		std::cerr << " PGP Email: " << it->second.mPgpEmail;
		std::cerr << " Location: " << it->second.mLocation;
		std::cerr << std::endl;
#endif

		ids.push_back(it->first);
	}
	return true;
}


bool     RsAccountsDetail::getCurrentAccountDetails(const RsPeerId &id,
                                RsPgpId &gpgId, std::string &gpgName, 
                                std::string &gpgEmail, std::string &location)
{
	std::map<RsPeerId, AccountDetails>::iterator it;
	it = mAccounts.find(id);
	if (it != mAccounts.end())
	{
		gpgId = it->second.mPgpId;
		gpgName = it->second.mPgpName;
		gpgEmail = it->second.mPgpEmail;
		location = it->second.mLocation;
		return true;
	}
	return false;
}

bool RsAccountsDetail::getCurrentAccountOptions(bool &ishidden,bool& isautotor, bool &isFirstTimeRun)
{
	std::map<RsPeerId, AccountDetails>::iterator it;
	it = mAccounts.find(mPreferredId);
	if (it != mAccounts.end())
	{
		ishidden       = it->second.mIsHiddenLoc;
		isFirstTimeRun = it->second.mFirstRun;
        isautotor      = it->second.mIsAutoTor;

		return true;
	}
	return false;
}


/* directories with valid certificates in the expected location */
bool RsAccountsDetail::getAvailableAccounts(std::map<RsPeerId, AccountDetails> &accounts,int& failing_accounts,std::map<std::string,std::vector<std::string> >& unsupported_keys,bool hidden_only)
{
	failing_accounts = 0 ;
	/* get the directories */
	std::list<std::string> directories;
	std::list<std::string>::iterator it;

	std::cerr << "RsAccounts::getAvailableAccounts()";
	std::cerr << std::endl;

	/* now iterate through the directory...
	 * directories - flags as old,
	 * files checked to see if they have changed. (rehashed)
	 */

	/* check for the dir existance */
	librs::util::FolderIterator dirIt(mBaseDirectory,false);

	if (!dirIt.isValid())
	{
		std::cerr << "Cannot Open Base Dir - No Available Accounts" << std::endl;
		return false ;
	}

	struct stat64 buf;

    for(;dirIt.isValid();dirIt.next())
	{
		/* check entry type */
        std::string fname = dirIt.file_name();
		std::string fullname = mBaseDirectory + "/" + fname;
#ifdef FIM_DEBUG
		std::cerr << "calling stats on " << fullname <<std::endl;
#endif

#ifdef WINDOWS_SYS
		std::wstring wfullname;
		librs::util::ConvertUtf8ToUtf16(fullname, wfullname);
		if (-1 != _wstati64(wfullname.c_str(), &buf))
#else
		if (-1 != stat64(fullname.c_str(), &buf))
#endif

		{
#ifdef FIM_DEBUG
			std::cerr << "buf.st_mode: " << buf.st_mode <<std::endl;
#endif
			if (S_ISDIR(buf.st_mode))
			{
				if ((fname == ".") || (fname == ".."))
				{
#ifdef FIM_DEBUG
					std::cerr << "Skipping:" << fname << std::endl;
#endif
					continue; /* skipping links */
				}

#ifdef FIM_DEBUG
				std::cerr << "Is Directory: " << fullname << std::endl;
#endif

				/* */
				directories.push_back(fname);

			}
		}	
	}
	/* close directory */
	dirIt.closedir();

	for(it = directories.begin(); it != directories.end(); ++it)
	{
		// For V0.6 Accounts we expect format:
		// LOC06_xxxhexaxxx or
		// HID06_xxxhexaxxx
		// split into prefix and hex string.

		if (it->length() != 32 + 6)
		{
			std::cerr << "getAvailableAccounts() Skipping Invalid sized dir: " << *it << std::endl;
			continue;
		}

		std::string prefix = (*it).substr(0, 6);
		std::string lochex = (*it).substr(6);  // rest of string.

		bool hidden_location = false;
		bool auto_tor = false;
		bool valid_prefix = false;

		if (prefix == "LOC06_")
		{
			valid_prefix = true;
		}
		else if (prefix == "HID06_")
		{
			valid_prefix = true;
			hidden_location = true;

            auto_tor = RsDirUtil::checkDirectory(mBaseDirectory+"/"+*it+"/hidden_service");
		}
		else
		{
			std::cerr << "getAvailableAccounts() Skipping Invalid Prefix dir: " << *it << std::endl;
			continue;
		}

		if(hidden_only && !hidden_location)
			continue ;

		if(valid_prefix && isHexaString(lochex) && (lochex).length() == 32)
		{
			std::string accountdir = mBaseDirectory + "/" + *it;
#ifdef GPG_DEBUG
			std::cerr << "getAvailableAccounts() Checking: " << *it << std::endl;
#endif

			AccountDetails tmpId;
			tmpId.mIsHiddenLoc = hidden_location;
			tmpId.mIsAutoTor = auto_tor;
			tmpId.mAccountDir = *it;

			if (checkAccount(accountdir, tmpId,unsupported_keys))
			{
#ifdef GPG_DEBUG
				std::cerr << "getAvailableAccounts() Accepted: " << *it << std::endl;
#endif

				std::map<RsPeerId, AccountDetails>::iterator ait;
				ait = accounts.find(tmpId.mSslId);
				if (ait != accounts.end())
				{
					std::cerr << "getAvailableAccounts() ERROR Duplicate SSLIDs";
					std::cerr << " - only one will be available";
					std::cerr << std::endl;
					std::cerr << " ID1 (overridden) : " << ait->first << " Directory: " << ait->second.mAccountDir;
					std::cerr << std::endl;
					std::cerr << " ID2 (available)  : " << tmpId.mSslId << " Directory: " << tmpId.mAccountDir;
					std::cerr << std::endl;

				}

				accounts[tmpId.mSslId] = tmpId;
			}
			else
				++failing_accounts ;
		}
#ifdef GPG_DEBUG
		else
			std::cerr << "Skipped non SSLid directory " << *it << std::endl;
#endif
	}
	return true;
}



static bool checkAccount(const std::string &accountdir, AccountDetails &account,std::map<std::string,std::vector<std::string> >& unsupported_keys)
{
	/* check if the cert/key file exists */

	// Create the filename.
    // TODO: use kFilenameKey
	std::string basename = accountdir + "/";
	basename += kPathKeyDirectory + "/";
    basename += "user";

	std::string cert_name = basename + "_cert.pem";
	//std::string userName;

#ifdef AUTHSSL_DEBUG
	std::cerr << "checkAccount() dir: " << accountdir << std::endl;
#endif
	bool ret = false;

	/* check against authmanagers private keys */
	if(AuthSSL::instance().parseX509DetailsFromFile(
	            cert_name, account.mSslId, account.mPgpId, account.mLocation ))
	{
        // new locations store the name in an extra file
        if(account.mLocation == "")
            RsDirUtil::loadStringFromFile(accountdir + "/" + kPathKeyDirectory + "/" + kFilenameLocation,
                                          account.mLocation);
#ifdef AUTHSSL_DEBUG
		std::cerr << "location: " << account.mLocation << " id: " << account.mSslId << std::endl;
		std::cerr << "issuerName: " << account.mPgpId << " id: " << account.mSslId << std::endl;
#endif

		if(! RsAccounts::GetPGPLoginDetails(account.mPgpId, account.mPgpName, account.mPgpEmail))
			return false ;

        if(!AuthPGP::haveSecretKey(account.mPgpId))
			return false ;

        if(!AuthPGP::isKeySupported(account.mPgpId))
		{
			std::string keystring = account.mPgpId.toStdString() + " " + account.mPgpName + "&#60;" + account.mPgpEmail ;
			unsupported_keys[keystring].push_back("Location: " + account.mLocation + "&nbsp;&nbsp;(" + account.mSslId.toStdString() + ")") ;
			return false ;
		}

#ifdef GPG_DEBUG
		std::cerr << "PGPLoginDetails: " << account.mPgpId << " name: " << account.mPgpName;
		std::cerr << " email: " << account.mPgpEmail << std::endl;
#endif
		ret = true;
	}
	else
	{
		std::cerr << "GetIssuerName FAILED!" << std::endl;
		ret = false;
	}

	return ret;
}





/**************************** Access Functions for Init Data **************************/
/**************************** Private Functions for InitRetroshare ********************/
/**************************** Private Functions for InitRetroshare ********************/


/***********************************************************
 * This Directory is used to store data and "template" file that Retroshare requires.
 * These files will either be copied into Retroshare's configuration directory, 
 * if they are to be modified. Or used directly, if read-only.
 *
 * This will initially be used for the DHT bootstrap file.
 *
 * Please modify the code below to suit your platform!
 * 
 * WINDOWS: 
 * WINDOWS PORTABLE:
 * Linux:
 * OSX:

 ***********/

#ifdef __APPLE__
	/* needs CoreFoundation Framework */
	#include <CoreFoundation/CoreFoundation.h>
	//#include <CFURL.h>
	//#include <CFBundle.h>
#endif

/*static*/ std::string RsAccountsDetail::PathDataDirectory(bool check)
{
	std::string dataDirectory;

#ifdef __APPLE__
	/* NOTE: OSX also qualifies as BSD... so this #ifdef must be before the BSD check. */

	/* For OSX, applications are Bundled in a directory...
	 * need to get the path to the executable Bundle.
	 * 
	 * Code nicely supplied by Qt!
	 */

	CFURLRef pluginRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFStringRef macPath = CFURLCopyFileSystemPath(pluginRef,
       	                                    kCFURLPOSIXPathStyle);
	const char *pathPtr = CFStringGetCStringPtr(macPath,
                                           CFStringGetSystemEncoding());
	dataDirectory = pathPtr;
	CFRelease(pluginRef);
	CFRelease(macPath);

    	dataDirectory += "/Contents/Resources";
	std::cerr << "getRetroshareDataDirectory() OSX: " << dataDirectory;

#elif (defined(BSD) && (BSD >= 199103))
	/* For BSD, the default is LOCALBASE which will be set
	 * before compilation via the ports/pkg-src mechanisms.
	 * For compilation without ports/pkg-src it is set to
	 * /usr/local (default on Open and Free; Net has /usr/pkg)
	 */
	dataDirectory = "/usr/local/share/retroshare";
	std::cerr << "getRetroshareDataDirectory() BSD: " << dataDirectory;
#elif defined(WINDOWS_SYS)
//	if (RsInitConfig::portable)
//	{
//		/* For Windows Portable, files must be in the data directory */
//		dataDirectory = "Data";
//		std::cerr << "getRetroshareDataDirectory() WINDOWS PORTABLE: " << dataDirectory;
//		std::cerr << std::endl;
//	}
//	else
//	{
//		/* For Windows: environment variable APPDATA should be suitable */
//		dataDirectory = getenv("APPDATA");
//		dataDirectory += "\\RetroShare";
//
//		std::cerr << "getRetroshareDataDirectory() WINDOWS: " << dataDirectory;
//		std::cerr << std::endl;
//	}

	/* Use RetroShare's exe dir */
	dataDirectory = ".";
#elif defined(ANDROID)
	dataDirectory = PathBaseDirectory()+"/usr/share/retroshare";
#elif defined(DATA_DIR)
	// cppcheck-suppress ConfigurationNotChecked
	dataDirectory = DATA_DIR;
	// For all other OS the data directory must be set in libretroshare.pro
#else
#	error "For your target OS automatic data dir discovery is not supported, cannot compile if DATA_DIR variable not set."
#endif

	if (!check)
	{
		std::cerr << "getRetroshareDataDirectory() unckecked: " << dataDirectory << std::endl;
		return dataDirectory;
	}

	/* Make sure the directory exists, else return emptyString */
	if (!RsDirUtil::checkDirectory(dataDirectory))
	{
		std::cerr << "getRetroshareDataDirectory() not found: " << dataDirectory << std::endl;
		dataDirectory = "";
	}
	else
	{
		std::cerr << "getRetroshareDataDirectory() found: " << dataDirectory << std::endl;
	}

	return dataDirectory;
}



/*****************************************************************************/
/*****************************************************************************/
/************************* Generating Certificates ***************************/
/*****************************************************************************/
/*****************************************************************************/


                /* Generating GPGme Account */
int      RsAccountsDetail::GetPGPLogins(std::list<RsPgpId>& pgpIds)
{
    AuthPGP::availableGPGCertificatesWithPrivateKeys(pgpIds);
    return 1;
}

int      RsAccountsDetail::GetPGPLoginDetails(const RsPgpId& id, std::string &name, std::string &email)
{
        #ifdef GPG_DEBUG
        std::cerr << "RsInit::GetPGPLoginDetails for \"" << id << "\"" << std::endl;
        #endif

		  bool ok = true ;
        name = AuthPGP::getPgpName(id,&ok);
		  if(!ok)
			  return 0 ;
        email = AuthPGP::getPgpEmail(id,&ok);
		  if(!ok)
			  return 0 ;

        if (name != "") {
            return 1;
        } else {
            return 0;
        }
}



/* Before any SSL stuff can be loaded, the correct PGP must be selected / generated:
 **/

bool RsAccountsDetail::SelectPGPAccount(const RsPgpId& pgpId)
{
	bool retVal = false;

    if (0 < AuthPGP::GPGInit(pgpId))
	{
		retVal = true;
#ifdef DEBUG_ACCOUNTS
		std::cerr << "PGP Auth Success!";
#endif
	}
	else
		std::cerr << "PGP Auth Failed!";

#ifdef DEBUG_ACCOUNTS
	std::cerr << " ID: " << pgpId << std::endl;
#endif

	return retVal;
}


bool     RsAccountsDetail::GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passwd, RsPgpId &pgpId, const int keynumbits, std::string &errString)
{
    return AuthPGP::GeneratePGPCertificate(name, email, passwd, pgpId, keynumbits, errString);
}

		// PGP Support Functions.
void RsAccountsDetail::getUnsupportedKeys(std::map<std::string,std::vector<std::string> > &unsupported_keys)
{
	unsupported_keys = mUnsupportedKeys;
	return;
}

bool RsAccountsDetail::exportIdentity(const std::string& fname,const RsPgpId& id)
{
    return AuthPGP::exportProfile(fname,id);
}

bool RsAccountsDetail::importIdentity(const std::string& fname,RsPgpId& id,std::string& import_error)
{
    return AuthPGP::importProfile(fname,id,import_error);
}

bool RsAccountsDetail::importIdentityFromString(const std::string &data, RsPgpId &imported_pgp_id, std::string &import_error)
{
    return AuthPGP::importProfileFromString(data, imported_pgp_id, import_error);
}

bool RsAccountsDetail::exportIdentityToString(
        std::string& data, const RsPgpId& pgpId, bool includeSignatures,
        std::string& errorMsg )
{
    return AuthPGP::exportIdentityToString(
	            data, pgpId, includeSignatures, errorMsg );
}

bool RsAccountsDetail::copyGnuPGKeyrings()
{
	std::string pgp_dir = PathPGPDirectory() ;

	if(!RsDirUtil::checkCreateDirectory(pgp_dir))
		throw std::runtime_error("Cannot create pgp directory " + pgp_dir) ;

	std::string source_public_keyring;
	std::string source_secret_keyring;

#ifdef WINDOWS_SYS
	source_public_keyring = mBaseDirectory + "/gnupg/pubring.gpg";
	source_secret_keyring = mBaseDirectory + "/gnupg/secring.gpg" ;
#else
	char *env_gnupghome = getenv("GNUPGHOME") ;

	if(env_gnupghome != NULL)
	{
		std::cerr << "looking into $GNUPGHOME/" << std::endl;

		source_public_keyring = std::string(env_gnupghome) + "/pubring.gpg" ;
		source_secret_keyring = std::string(env_gnupghome) + "/secring.gpg" ;
	}
	else
	{
		char *env_homedir = getenv("HOME") ;

		if(env_homedir != NULL)
		{
			std::cerr << "looking into $HOME/.gnupg/" << std::endl;
			std::string home_dir(env_homedir) ;

			// We need a specific part for MacOS and Linux as well
			source_public_keyring = home_dir + "/.gnupg/pubring.gpg" ;
			source_secret_keyring = home_dir + "/.gnupg/secring.gpg" ;
		}
		else
			return false ;
	}
#endif

	if(!RsDirUtil::copyFile(source_public_keyring,pgp_dir + "/retroshare_public_keyring.gpg"))
	{
		std::cerr << "Cannot copy pub keyring " << source_public_keyring << " to destination file " << pgp_dir + "/retroshare_public_keyring.gpg. If you believe your keyring is in a different place, please make the copy yourself." << std::endl;
		return false ;
	}
	if(!RsDirUtil::copyFile(source_secret_keyring,pgp_dir + "/retroshare_secret_keyring.gpg"))
	{
		std::cerr << "Cannot copy sec keyring " << source_secret_keyring << " to destination file " << pgp_dir + "/retroshare_secret_keyring.gpg. your keyring is in a different place, please make the copy yourself." << std::endl;
		return false ;
	}

	return true ;
}



                /* Create SSL Certificates */
bool     RsAccountsDetail::GenerateSSLCertificate(const RsPgpId& pgp_id, const std::string& org, const std::string& loc, const std::string& country, bool ishiddenloc,bool isautotor, const std::string& passwd, RsPeerId &sslId, std::string &errString)
{
	/* select the PGP Identity first */
	if (!SelectPGPAccount(pgp_id))
	{
		errString = "Invalid PGP Identity";
		return false;
	}

	// generate the private_key / certificate.
	// save to file.
	//
	// then load as if they had entered a passwd.

	// check password.
	if (passwd.length() < 4)
	{
		errString = "Password is Unsatisfactory (must be 4+ chars)";
		return false;
	}

	int nbits = 4096;

    //std::string pgp_name = AuthGPG::getGPGName(pgp_id);

	// Create the filename .....
	// Temporary Directory for creating files....
	std::string tmpdir = "TMPCFG";

	std::string tmpbase = mBaseDirectory + "/" + tmpdir + "/";
	
	if(!setupAccount(tmpbase))
		return false ;

	/* create directory structure */
	std::string keypath = tmpbase + kPathKeyDirectory + "/";
	std::string key_name = keypath + kFilenameKey;
	std::string cert_name = keypath + kFilenameCert;

	/* Extra step required for SSL + PGP, user must have selected
	 * or generated a suitable key so the signing can happen.
	 */

	X509_REQ *req = GenerateX509Req(
			key_name.c_str(),
			passwd.c_str(),
            "-", //pgp_name.c_str(), // does not allow empty name, set to constant instead
			"", //ui -> gen_email -> value(),
			org.c_str(),
            "", //loc.c_str(),
			"", //ui -> gen_state -> value(),
			country.c_str(),
			nbits, errString);

	if (req == NULL)
	{
		fprintf(stderr,"RsGenerateCert() Couldn't create Request. Reason: %s\n", errString.c_str());
		return false;
	}

	long days = 3000;
	X509 *x509 = AuthSSL::getAuthSSL()->SignX509ReqWithGPG(req, days);

	X509_REQ_free(req);
	if (x509 == NULL) {
		fprintf(stderr,"RsGenerateCert() Couldn't sign ssl certificate. Probably PGP password is wrong.\n");
		return false;
	}

	/* save to file */

        bool gen_ok = true;

		/* Print the signed Certificate! */
		BIO *bio_out = BIO_new(BIO_s_file());
		BIO_set_fp(bio_out,stdout,BIO_NOCLOSE);

		/* Print it out */
		int nmflag = 0;
		int reqflag = 0;

		X509_print_ex(bio_out, x509, nmflag, reqflag);

		(void) BIO_flush(bio_out);
		BIO_free(bio_out);

		/* Save cert to file */
		// open the file.
		FILE *out = NULL;
		if (NULL == (out = RsDirUtil::rs_fopen(cert_name.c_str(), "w")))
		{
			fprintf(stderr,"RsGenerateCert() Couldn't create Cert File");
			fprintf(stderr," : %s\n", cert_name.c_str());
			gen_ok = false;
		}

		if (!PEM_write_X509(out,x509))
		{
			fprintf(stderr,"RsGenerateCert() Couldn't Save Cert");
			fprintf(stderr," : %s\n", cert_name.c_str());
			gen_ok = false;
		}
	
		fclose(out);
		X509_free(x509);

        // store location name in a file
        if(!RsDirUtil::saveStringToFile(keypath + kFilenameLocation, loc))
            std::cerr << "RsInit::GenerateSSLCertificate() failed to save location name to into file." << std::endl;

	if (!gen_ok)
	{
		errString = "Generation of Certificate Failed";
		return false;
	}

	/* try to load it, and get Id */

	std::string location;
	RsPgpId pgpid_retrieved;

	if(!AuthSSL::instance().parseX509DetailsFromFile(
	            cert_name, sslId, pgpid_retrieved, location ))
	{
		RsErr() << __PRETTY_FUNCTION__ << " Cannot check own signature, maybe "
		        << "the files are corrupted." << std::endl;
		return false;
	}

	/* Move directory to correct id */
	std::string accountdir;
	if (ishiddenloc)
		accountdir = "HID06_" + sslId.toStdString();
	else
		accountdir = "LOC06_" + sslId.toStdString();

	std::string fullAccountDir = mBaseDirectory + "/" + accountdir;
	std::string finalbase = fullAccountDir + "/";

	/* Rename Directory */
	std::cerr << "Mv Config Dir from: " << tmpbase << " to: " << finalbase;
	std::cerr << std::endl;

	if (!RsDirUtil::renameFile(tmpbase, finalbase))
	{
		std::cerr << "rename FAILED" << std::endl;
	}

	AccountDetails newAccount;

	newAccount.mSslId = sslId;
	newAccount.mAccountDir = accountdir;
	newAccount.mPgpId = pgp_id;

	newAccount.mLocation = loc;
	newAccount.mIsHiddenLoc = ishiddenloc;
	newAccount.mIsAutoTor = isautotor;

	newAccount.mFirstRun = true;

	// rest of newAccount pgp filled in checkAccount.
	if (!checkAccount(fullAccountDir, newAccount, mUnsupportedKeys))
	{
		std::cerr << "RsInit::GenerateSSLCertificate() Cannot check own signature, maybe the files are corrupted." << std::endl;
		return false;
	}

	mAccounts[newAccount.mSslId] = newAccount;
	mPreferredId = newAccount.mSslId;

	std::cerr << "RetroShare has Successfully generated a Certficate/Key" << std::endl;
	std::cerr << "\tCert Located: " << cert_name << std::endl;
	std::cerr << "\tLocated: " << key_name << std::endl;

	return true;
}


/******************* PRIVATE FNS TO HELP with GEN **************/
bool RsAccountsDetail::setupAccount(const std::string& accountdir)
{
	/* actual config directory isd */

	std::string subdir1 = accountdir + "/";
	subdir1 += kPathKeyDirectory;

	std::string subdir2 = accountdir + "/";
	subdir2 += kPathConfigDirectory;

	std::string subdir3 = accountdir + "/";
	subdir3 += "cache";

	std::string subdir4 = subdir3 + "/";
	std::string subdir5 = subdir3 + "/";
	subdir4 += "local";
	subdir5 += "remote";

	// fatal if cannot find/create.
	std::cerr << "Checking For Directories" << std::endl;
	if (!RsDirUtil::checkCreateDirectory(accountdir))
	{
		std::cerr << "Cannot Create BaseConfig Dir" << std::endl;
		return false ;
	}
	if (!RsDirUtil::checkCreateDirectory(subdir1))
	{
		std::cerr << "Cannot Create Key Directory" << std::endl;
		return false ;
	}
	if (!RsDirUtil::checkCreateDirectory(subdir2))
	{
		std::cerr << "Cannot Create Config Directory" << std::endl;
		return false ;
	}
	if (!RsDirUtil::checkCreateDirectory(subdir3))
	{
		std::cerr << "Cannot Create Config/Cache Dir" << std::endl;
		return false ;
	}
	if (!RsDirUtil::checkCreateDirectory(subdir4))
	{
		std::cerr << "Cannot Create Config/Cache/local Dir" << std::endl;
		return false ;
	}
	if (!RsDirUtil::checkCreateDirectory(subdir5))
	{
		std::cerr << "Cannot Create Config/Cache/remote Dir" << std::endl;
		return false ;
	}

	return true;
}






/***************************** FINAL LOADING OF SETUP *************************/

#if 0
                /* Login SSL */
bool     RsInit::LoadPassword(const std::string& id, const std::string& inPwd)
{
	/* select configDir */

	RsInitConfig::preferredId = id;

	std::map<std::string, accountId>::iterator it = RsInitConfig::accountIds.find(id);
	if (it == RsInitConfig::accountIds.end())
	{
		std::cerr << "RsInit::LoadPassword() Cannot Locate Identity: " << id;
		std::cerr << std::endl;
		exit(1);
	}

	std::string accountdir = it->second.accountDir;

	RsInitConfig::configDir = RsInitConfig::basedir + "/" + accountdir;
	RsInitConfig::passwd = inPwd;

	//	if(inPwd != "")
	//		RsInitConfig::havePasswd = true;

	// Create the filename.
	std::string basename = RsInitConfig::configDir + "/";
	basename += configKeyDir + "/";
	basename += "user";

	RsInitConfig::load_key  = basename + "_pk.pem";
	RsInitConfig::load_cert = basename + "_cert.pem";

	return true;
}
#endif

/*********************************************************************************
 * PUBLIC INTERFACE FUNCTIONS 
 ********************************************************************************/

bool RsAccounts::init(const std::string& opt_base_dir,int& error_code)
{
	rsAccountsDetails = new RsAccountsDetail;
	rsAccounts = new RsAccounts;

	// first check config directories, and set bootstrap values.
	if(!rsAccountsDetails->setupBaseDirectory(opt_base_dir))
    {
		error_code = RS_INIT_BASE_DIR_ERROR ;
        return false ;
    }

	// Setup PGP stuff.
	std::string pgp_dir = rsAccountsDetails->PathPGPDirectory();

	if(!RsDirUtil::checkCreateDirectory(pgp_dir))
		throw std::runtime_error("Cannot create pgp directory " + pgp_dir) ;

	AuthPGP::init(	pgp_dir + "/retroshare_public_keyring.gpg",
	                pgp_dir + "/retroshare_secret_keyring.gpg",
	                pgp_dir + "/retroshare_trustdb.gpg",
	                pgp_dir + "/lock");

	// load Accounts.
	if (!rsAccountsDetails->loadAccounts())
    {
		error_code = RS_INIT_NO_KEYRING ;
        return false ;
    }
    return true;
}

        // Directories.
std::string RsAccounts::ConfigDirectory() { return RsAccountsDetail::PathBaseDirectory(); }
std::string RsAccounts::systemDataDirectory(bool check) { return RsAccountsDetail::PathDataDirectory(check); }
std::string RsAccounts::PGPDirectory() { return rsAccountsDetails->PathPGPDirectory(); }
std::string RsAccounts::AccountDirectory() { return rsAccountsDetails->getCurrentAccountPathAccountDirectory(); }
std::string RsAccounts::AccountKeysDirectory() { return rsAccountsDetails->getCurrentAccountPathAccountKeysDirectory(); }
std::string RsAccounts::AccountPathCertFile() { return rsAccountsDetails->getCurrentAccountPathCertFile(); }
std::string RsAccounts::AccountPathKeyFile() { return rsAccountsDetails->getCurrentAccountPathKeyFile(); }
std::string RsAccounts::AccountLocationName() { return rsAccountsDetails->getCurrentAccountLocationName(); }

bool RsAccounts::lockPreferredAccount()  { return rsAccountsDetails->lockPreferredAccount();}	// are these methods any useful??
void RsAccounts::unlockPreferredAccount() { rsAccountsDetails->unlockPreferredAccount(); }

bool RsAccounts::checkCreateAccountDirectory() { return rsAccountsDetails->checkAccountDirectory(); }

// PGP Accounts.
int     RsAccounts::GetPGPLogins(std::list<RsPgpId> &pgpIds)
{
	return rsAccountsDetails->GetPGPLogins(pgpIds);
}

int     RsAccounts::GetPGPLoginDetails(const RsPgpId& id, std::string &name, std::string &email)
{
	return rsAccountsDetails->GetPGPLoginDetails(id, name, email);
}

bool    RsAccounts::GeneratePGPCertificate(const std::string &name, const std::string& email, const std::string& passwd, RsPgpId &pgpId, const int keynumbits, std::string &errString)
{
	return rsAccountsDetails->GeneratePGPCertificate(name, email, passwd, pgpId, keynumbits, errString);
}

// PGP Support Functions.
bool    RsAccounts::ExportIdentity(const std::string& fname,const RsPgpId& pgp_id)
{
	return rsAccountsDetails->exportIdentity(fname,pgp_id);
}

bool    RsAccounts::ImportIdentity(const std::string& fname,RsPgpId& imported_pgp_id,std::string& import_error)
{
	return rsAccountsDetails->importIdentity(fname,imported_pgp_id,import_error);
}

bool RsAccounts::importIdentityFromString(
        const std::string& data, RsPgpId& imported_pgp_id,
        std::string& import_error )
{
	return rsAccountsDetails->
	        importIdentityFromString(data, imported_pgp_id, import_error);
}

/*static*/ bool RsAccounts::exportIdentityToString(
        std::string& data, const RsPgpId& pgpId, std::string& errorMsg,
        bool includeSignatures )
{
	return rsAccountsDetails->exportIdentityToString(
	            data, pgpId, includeSignatures, errorMsg);
}

void    RsAccounts::GetUnsupportedKeys(std::map<std::string,std::vector<std::string> > &unsupported_keys)
{
	return rsAccountsDetails->getUnsupportedKeys(unsupported_keys);
}

bool    RsAccounts::CopyGnuPGKeyrings() 
{
	return rsAccountsDetails->copyGnuPGKeyrings();
}

void RsAccounts::storeSelectedAccount() { rsAccountsDetails->storePreferredAccount() ;}
// Rs Accounts
bool    RsAccounts::SelectAccount(const RsPeerId &id)
{
	return rsAccountsDetails->selectId(id);
}

bool    RsAccounts::GetPreferredAccountId(RsPeerId &id)
{
	return rsAccountsDetails->getCurrentAccountId(id);
}

bool RsAccounts::getCurrentAccountOptions(bool& is_hidden,bool& is_tor_auto,bool& is_first_time)
{
    return rsAccountsDetails->getCurrentAccountOptions(is_hidden,is_tor_auto,is_first_time);
}
bool RsAccounts::isHiddenNode()
{
    bool hidden = false ;
    bool is_tor_only = false ;
    bool is_first_time = false ;

    if(!getCurrentAccountOptions(hidden,is_tor_only,is_first_time))
    {
        std::cerr << "(EE) Critical problem: RsAccounts::getCurrentAccountOptions() called but no account chosen!" << std::endl;
        throw std::runtime_error("inconsistent configuration") ;
    }

    return hidden ;
}
bool RsAccounts::isTorAuto()
{
    bool hidden = false ;
    bool is_tor_only = false ;
    bool is_first_time = false ;

    if(!getCurrentAccountOptions(hidden,is_tor_only,is_first_time))
    {
        std::cerr << "(EE) Critical problem: RsAccounts::getCurrentAccountOptions() called but no account chosen!" << std::endl;
        throw std::runtime_error("inconsistent configuration") ;
    }

    return is_tor_only ;
}

bool    RsAccounts::GetAccountIds(std::list<RsPeerId> &ids)
{
	return rsAccountsDetails->getAccountIds(ids);
}

bool    RsAccounts::GetAccountDetails(const RsPeerId &id,
		RsPgpId &pgpId, std::string &pgpName,
		std::string &pgpEmail, std::string &location)
{
	return rsAccountsDetails->getCurrentAccountDetails(id, pgpId, pgpName, pgpEmail, location);
}

bool RsAccounts::createNewAccount(
        const RsPgpId& pgp_id, const std::string& org, const std::string& loc,
        const std::string& country, bool ishiddenloc, bool isautotor,
        const std::string& passwd, RsPeerId &sslId, std::string &errString )
{
	return rsAccountsDetails->GenerateSSLCertificate(pgp_id, org, loc, country, ishiddenloc, isautotor, passwd, sslId, errString);
}

/*********************************************************************************
 * END OF: PUBLIC INTERFACE FUNCTIONS 
 ********************************************************************************/

std::string RsAccountsDetail::mBaseDirectory;

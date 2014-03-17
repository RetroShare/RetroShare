#include <string>
#include <iostream>
#include <pqi/authgpg.h>
#include "rsloginhandler.h"
#include "util/rsdir.h"
#include "rsaccounts.h"

#if defined(UBUNTU) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <gnome-keyring-1/gnome-keyring.h>

	GnomeKeyringPasswordSchema my_schema = {
      GNOME_KEYRING_ITEM_ENCRYPTION_KEY_PASSWORD,
      {
           { "RetroShare SSL Id", GNOME_KEYRING_ATTRIBUTE_TYPE_STRING },
           { NULL, (GnomeKeyringAttributeType)0 }
      }
  };
#endif


#ifdef __APPLE__
	/* OSX Headers */

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

#endif

/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
/* WINDOWS STRUCTURES FOR DPAPI */

#ifndef WINDOWS_SYS /* UNIX */

#include <openssl/rc4.h>

#else
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/


#include <windows.h>
#include <wincrypt.h>
#include <iomanip>

/*
class CRYPTPROTECT_PROMPTSTRUCT;
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WINDOWS_SYS
#if defined(__CYGWIN__)

typedef struct _CRYPTPROTECT_PROMPTSTRUCT {
  DWORD cbSize;
  DWORD dwPromptFlags;
  HWND hwndApp;
  LPCWSTR szPrompt;
} CRYPTPROTECT_PROMPTSTRUCT,
 *PCRYPTPROTECT_PROMPTSTRUCT;

#endif
#endif

/* definitions for the two functions */
__declspec (dllimport)
extern BOOL WINAPI CryptProtectData(
  DATA_BLOB* pDataIn,
  LPCWSTR szDataDescr,
  DATA_BLOB* pOptionalEntropy,
  PVOID pvReserved,
  /* PVOID prompt, */
  /* CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct, */
  CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct,
  DWORD dwFlags,
  DATA_BLOB* pDataOut
);

__declspec (dllimport)
extern BOOL WINAPI CryptUnprotectData(
  DATA_BLOB* pDataIn,
  LPWSTR* ppszDataDescr,
  DATA_BLOB* pOptionalEntropy,
  PVOID pvReserved,
  /* PVOID prompt, */
  /* CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct, */
  CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct,
  DWORD dwFlags,
  DATA_BLOB* pDataOut
);

#ifdef __cplusplus
}
#endif

#endif


bool RsLoginHandler::getSSLPassword(const RsPeerId& ssl_id,bool enable_gpg_ask_passwd,std::string& ssl_passwd)
{
	// First, see if autologin is available
	//
	if(tryAutoLogin(ssl_id,ssl_passwd))
		return true ;

	// If we're not expecting to enter a passwd (e.g. test for autologin before
	// display of the login window), safely respond false.
	//
	if(!enable_gpg_ask_passwd)
		return false ;

	return getSSLPasswdFromGPGFile(ssl_id,ssl_passwd) ;
}

bool RsLoginHandler::tryAutoLogin(const RsPeerId& ssl_id,std::string& ssl_passwd)
{
	std::cerr << "RsTryAutoLogin()" << std::endl;

	/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS /* UNIX */
#if defined(UBUNTU) || defined(__FreeBSD__) || defined(__OpenBSD__)

	gchar *passwd = NULL;

	std::cerr << "Using attribute: " << ssl_id << std::endl;
	if( gnome_keyring_find_password_sync(&my_schema, &passwd,"RetroShare SSL Id",ssl_id.toStdString().c_str(),NULL) == GNOME_KEYRING_RESULT_OK )
	{
		std::cerr << "Got SSL passwd ********************" /*<< passwd*/ << " from gnome keyring" << std::endl;
		ssl_passwd = std::string(passwd);
		return true ;
	}
	else
	{
		std::cerr << "Could not get passwd from gnome keyring" << std::endl;
		return false ;
	}

#else
	/******************** OSX KeyChain stuff *****************************/
#ifdef __APPLE__

	std::cerr << "RsTryAutoLogin() OSX Version" << std::endl;
	//Call SecKeychainFindGenericPassword to get a password from the keychain:

	void *passwordData = NULL;
	UInt32 passwordLength = 0;
	const char *userId = ssl_id.c_str();
	UInt32 uidLength = strlen(ssl_id.c_str());
	SecKeychainItemRef itemRef = NULL;

	OSStatus status = SecKeychainFindGenericPassword (
			NULL,           // default keychain
			10,             // length of service name
			"Retroshare",   // service name
			uidLength,             // length of account name
			userId,   // account name
			&passwordLength,  // length of password
			&passwordData,   // pointer to password data
			&itemRef         // the item reference
			);

	std::cerr << "RsTryAutoLogin() SecKeychainFindGenericPassword returned: " << status << std::endl;

	if (status != 0)
	{
		std::cerr << "RsTryAutoLogin() Error " << std::endl;

		/* error */
		if (status == errSecItemNotFound) 
		{ 
			//Is password on keychain?
			std::cerr << "RsTryAutoLogin() Error - Looks like password is not in KeyChain " << std::endl;
		}
	}
	else
	{
		std::cerr << "RsTryAutoLogin() Password found on KeyChain! " << std::endl;

		/* load up password to correct location */
		ssl_passwd.clear();
		ssl_passwd.insert(0, (char*)passwordData, passwordLength);
	}

	//Free the data allocated by SecKeychainFindGenericPassword:

	SecKeychainItemFreeContent (
			NULL,           //No attribute data to release
			passwordData    //Release data buffer allocated by SecKeychainFindGenericPassword
			);

	if (itemRef) CFRelease(itemRef);

	return (status == 0);

	/******************** OSX KeyChain stuff *****************************/
#else /* UNIX, but not UBUNTU or APPLE */

	FILE* helpFile = RsDirUtil::rs_fopen(getAutologinFileName(ssl_id).c_str(), "r");

	if(helpFile == NULL){
		std::cerr << "\nFailed to open help file\n" << std::endl;
		return false;
	}

	/* decrypt help */

	int c ;
	std::string passwd ;
	while( (c = getc(helpFile)) != EOF )
		passwd += (char)c ;

	const int DAT_LEN = passwd.length();
	const int KEY_DAT_LEN = RsInitConfig::load_cert.length();
	unsigned char* key_data  = (unsigned char*)RsInitConfig::load_cert.c_str();
	unsigned char* indata = new unsigned char[DAT_LEN];
	unsigned char* outdata = new unsigned char[DAT_LEN];

	for(int i=0;i<DAT_LEN;++i)
		indata[i] = passwd[i] ;

	//	if(fscanf(helpFile, "%s", indata) != 1)
	//	{
	//		std::cerr << "Can't read RSA key in help file " << helpFileName << ". Sorry." << std::endl ;
	//		return false ;
	//	}

	RC4_KEY* key = new RC4_KEY;
	RC4_set_key(key, KEY_DAT_LEN, key_data);

	RC4(key, DAT_LEN, indata, outdata);

	ssl_passwd.clear();
	ssl_passwd.insert(0, (char*)outdata, DAT_LEN);


	fclose(helpFile);


	delete[] indata;
	delete[] outdata;

	if(key != NULL)
		delete key;

	return true;
#endif // APPLE
#endif	// UBUNTU
	/******* WINDOWS BELOW *****/
#else

	/* try to load from file */
	std::string entropy = getSSLPasswdFileName(ssl_id);
	/* get the data out */

	DATA_BLOB DataIn;
	DATA_BLOB DataEnt;
	DATA_BLOB DataOut;

	BYTE *pbDataEnt   =(BYTE *)  strdup(entropy.c_str());
	DWORD cbDataEnt   = strlen((char *)pbDataEnt)+1;
	DataEnt.pbData = pbDataEnt;
	DataEnt.cbData = cbDataEnt;

	char *dataptr = NULL;
	int   datalen = 0;

	/* open the data to the file */
	FILE *fp = RsDirUtil::rs_fopen(getAutologinFileName(ssl_id).c_str(), "rb");
	if (fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		datalen = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		dataptr = (char *) malloc(datalen);
		fread(dataptr, 1, datalen, fp);
		fclose(fp);

		/*****
		  std::cerr << "Data loaded from: " << passwdfile;
		  std::cerr << std::endl;

		  std::cerr << "Size :";
		  std::cerr << datalen << std::endl;

		  for(unsigned int i = 0; i < datalen; i++)
		  {
		  std::cerr << std::setw(2) << (int) dataptr[i];
		  std::cerr << " ";
		  }
		  std::cerr << std::endl;
		 *****/
	}
	else
	{
		return false;
	}

	BYTE *pbDataInput =(BYTE *) dataptr;
	DWORD cbDataInput = datalen;
	DataIn.pbData = pbDataInput;
	DataIn.cbData = cbDataInput;


	CRYPTPROTECT_PROMPTSTRUCT prom;

	prom.cbSize = sizeof(prom);
	prom.dwPromptFlags = 0;


	bool isDecrypt = CryptUnprotectData(
			&DataIn,
			NULL,
			&DataEnt,  /* entropy.c_str(), */
			NULL,                 // Reserved
			&prom,                 // Opt. Prompt
			0,
			&DataOut);

	if (isDecrypt)
	{
		//std::cerr << "Decrypted size: " << DataOut.cbData;
		//std::cerr << std::endl;
		if (DataOut.pbData[DataOut.cbData - 1] != '\0')
		{
			std::cerr << "Error: Decrypted Data not a string...";
			std::cerr << std::endl;
			isDecrypt = false;
		}
		else
		{
			//std::cerr << "The decrypted data is: " << DataOut.pbData;
			//std::cerr << std::endl;
			ssl_passwd = (char *) DataOut.pbData;
		}
	}
	else
	{
		std::cerr << "Decryption error!";
		std::cerr << std::endl;
	}

	/* strings to be freed */
	free(pbDataInput);
	free(pbDataEnt);

	/* generated data space */
	LocalFree(DataOut.pbData);

	return isDecrypt;
#endif
	/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

	return false;
}


bool RsLoginHandler::enableAutoLogin(const RsPeerId& ssl_id,const std::string& ssl_passwd)
{
	std::cerr << "RsStoreAutoLogin()" << std::endl;

	/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS /* UNIX */
#if defined(UBUNTU) || defined(__FreeBSD__) || defined(__OpenBSD__)
	if(GNOME_KEYRING_RESULT_OK == gnome_keyring_store_password_sync(&my_schema, NULL, (gchar*)("RetroShare password for SSL Id "+ssl_id.toStdString()).c_str(),(gchar*)ssl_passwd.c_str(),"RetroShare SSL Id",ssl_id.toStdString().c_str(),NULL)) 
	{
		std::cerr << "Stored passwd " << "************************" << " into gnome keyring" << std::endl;
		return true ;
	}
	else
	{
		std::cerr << "Could not store passwd into gnome keyring" << std::endl;
		return false ;
	}
#else
#ifdef __APPLE__
	/***************** OSX KEYCHAIN ****************/
	//Call SecKeychainAddGenericPassword to add a new password to the keychain:

	std::cerr << "RsStoreAutoLogin() OSX Version!" << std::endl;

	const void *password = ssl_passwd.c_str();
	UInt32 passwordLength = strlen(ssl_passwd.c_str());
	const char *userid = ssl_id.c_str();
	UInt32 uidLength = strlen(ssl_id.c_str());

	OSStatus status = SecKeychainAddGenericPassword (
			NULL,            // default keychain
			10,              // length of service name
			"Retroshare",    // service name
			uidLength,              // length of account name
			userid,    // account name
			passwordLength,  // length of password
			password,        // pointer to password data
			NULL             // the item reference
			);

	std::cerr << "RsStoreAutoLogin() Call to SecKeychainAddGenericPassword returned: " << status << std::endl;

	if (status != 0)
	{
		std::cerr << "RsStoreAutoLogin() SecKeychainAddGenericPassword Failed" << std::endl;
		return false;
	}
	return true;

	/***************** OSX KEYCHAIN ****************/
#else

	/* WARNING: Autologin is inherently unsafe */
	FILE* helpFile = RsDirUtil::rs_fopen(getAutologinFileName.c_str(), "w");

	if(helpFile == NULL){
		std::cerr << "\nRsStoreAutoLogin(): Failed to open help file\n" << std::endl;
		return false;
	}

	/* encrypt help */

	const int DAT_LEN = ssl_passwd.length();
	const int KEY_DAT_LEN = RsInitConfig::load_cert.length();
	unsigned char* key_data = (unsigned char*)RsInitConfig::load_cert.c_str();
	unsigned char* indata = (unsigned char*)ssl_passwd.c_str();
	unsigned char* outdata = new unsigned char[DAT_LEN];

	RC4_KEY* key = new RC4_KEY;
	RC4_set_key(key, KEY_DAT_LEN, key_data);

	RC4(key, DAT_LEN, indata, outdata);


	fprintf(helpFile, "%s", outdata);
	fclose(helpFile);

	delete key;
	delete[] outdata;


	return true;
#endif // __APPLE__
#endif // UBUNTU.
#else  /* windows */

	/* store password encrypted in a file */
	std::string entropy = getSSLPasswdFileName(ssl_id);

	DATA_BLOB DataIn;
	DATA_BLOB DataEnt;
	DATA_BLOB DataOut;
	BYTE *pbDataInput = (BYTE *) strdup(ssl_passwd.c_str());
	DWORD cbDataInput = strlen((char *)pbDataInput)+1;
	BYTE *pbDataEnt   =(BYTE *)  strdup(entropy.c_str());
	DWORD cbDataEnt   = strlen((char *)pbDataEnt)+1;
	DataIn.pbData = pbDataInput;
	DataIn.cbData = cbDataInput;
	DataEnt.pbData = pbDataEnt;
	DataEnt.cbData = cbDataEnt;

	CRYPTPROTECT_PROMPTSTRUCT prom;

	prom.cbSize = sizeof(prom);
	prom.dwPromptFlags = 0;

	/*********
	  std::cerr << "Password (" << cbDataInput << "):";
	  std::cerr << pbDataInput << std::endl;
	  std::cerr << "Entropy (" << cbDataEnt << "):";
	  std::cerr << pbDataEnt   << std::endl;
	 *********/

	if(CryptProtectData(
				&DataIn,
				NULL,
				&DataEnt, /* entropy.c_str(), */
				NULL,                               // Reserved.
				&prom,
				0,
				&DataOut))
	{

		/**********
		  std::cerr << "The encryption phase worked. (";
		  std::cerr << DataOut.cbData << ")" << std::endl;

		  for(unsigned int i = 0; i < DataOut.cbData; i++)
		  {
		  std::cerr << std::setw(2) << (int) DataOut.pbData[i];
		  std::cerr << " ";
		  }
		  std::cerr << std::endl;
		 **********/

		//std::cerr << "Save to: " << passwdfile;
		//std::cerr << std::endl;

		/* save the data to the file */
		FILE *fp = RsDirUtil::rs_fopen(getAutologinFileName(ssl_id).c_str(), "wb");
		if (fp != NULL)
		{
			fwrite(DataOut.pbData, 1, DataOut.cbData, fp);
			fclose(fp);

			std::cerr << "AutoLogin Data saved: ";
			std::cerr << std::endl;
		}
	}
	else
	{
		std::cerr << "Encryption Failed";
		std::cerr << std::endl;
	}

	free(pbDataInput);
	free(pbDataEnt);
	LocalFree(DataOut.pbData);
#endif
	/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

	return false;
}

bool RsLoginHandler::clearAutoLogin(const RsPeerId& ssl_id)
{
#ifdef UBUNTU
	if(GNOME_KEYRING_RESULT_OK == gnome_keyring_delete_password_sync(&my_schema,"RetroShare SSL Id", ssl_id.toStdString().c_str(),NULL))
	{
		std::cerr << "Successfully Cleared gnome keyring passwd for SSLID " << ssl_id << std::endl;
		return true ;
	}
	else
	{
		std::cerr << "Could not clear gnome keyring passwd for SSLID " << ssl_id << std::endl;
		return false ;
	}
#else 
  #ifdef __APPLE__

	std::cerr << "clearAutoLogin() OSX Version" << std::endl;
	//Call SecKeychainFindGenericPassword to get a password from the keychain:

	void *passwordData = NULL;
	UInt32 passwordLength = 0;
	const char *userId = ssl_id.c_str();
	UInt32 uidLength = strlen(ssl_id.c_str());
	SecKeychainItemRef itemRef = NULL;

	OSStatus status = SecKeychainFindGenericPassword (
			NULL,           // default keychain
			10,             // length of service name
			"Retroshare",   // service name
			uidLength,             // length of account name
			userId,   // account name
			&passwordLength,  // length of password
			&passwordData,   // pointer to password data
			&itemRef         // the item reference
			);

	std::cerr << "RsTryAutoLogin() SecKeychainFindGenericPassword returned: " << status << std::endl;

	if (status != 0)
	{
		std::cerr << "clearAutoLogin() Error Finding password " << std::endl;

		/* error */
		if (status == errSecItemNotFound) 
		{ 
			//Is password on keychain?
			std::cerr << "RsTryAutoLogin() Error - Looks like password is not in KeyChain " << std::endl;
		}
	}
	else
	{
		std::cerr << "clearAutoLogin() Password found on KeyChain! " << std::endl;

		OSStatus deleteStatus = SecKeychainItemDelete (itemRef);
		if (status != 0)
		{
			std::cerr << "clearAutoLogin() Failed to Delete Password status: " << deleteStatus << std::endl;
		}
		else
		{
			std::cerr << "clearAutoLogin() Deleted Password" << std::endl;
		}
	}

	//Free the data allocated by SecKeychainFindGenericPassword:

	SecKeychainItemFreeContent (
			NULL,           //No attribute data to release
			passwordData    //Release data buffer allocated by SecKeychainFindGenericPassword
			);

	if (itemRef) CFRelease(itemRef);

	return (status == 0);

	/******************** OSX KeyChain stuff *****************************/

  #else // WINDOWS / Generic Linux.
	
	std::string passwdfile = getAutologinFileName(ssl_id) ;

	FILE *fp = RsDirUtil::rs_fopen(passwdfile.c_str(), "wb");

	if (fp != NULL)
	{
		fwrite(" ", 1, 1, fp);
		fclose(fp);
		bool removed = remove(passwdfile.c_str());

		if(removed != 0)
			std::cerr << "RsLoginHandler::clearAutoLogin(): Failed to Removed help file" << std::endl;

		std::cerr << "AutoLogin Data cleared ";
		std::cerr << std::endl;
		return true;
	}

	return false;
  #endif
#endif
}

bool RsLoginHandler::checkAndStoreSSLPasswdIntoGPGFile(const RsPeerId& ssl_id,const std::string& ssl_passwd)
{
	// We want to pursue login with gpg passwd. Let's do it:
	//
	std::cerr << "let's store the ssl Password into a pgp ecrypted file" << std::endl;

	FILE *sslPassphraseFile = RsDirUtil::rs_fopen(getSSLPasswdFileName(ssl_id).c_str(), "r");

	if(sslPassphraseFile != NULL)	// already have it.
	{
		fclose(sslPassphraseFile) ;
		return true ;
	}

	bool ok ;
	std::string cipher ;

	if(AuthGPG::getAuthGPG()->encryptTextToFile(ssl_passwd, getSSLPasswdFileName(ssl_id))) 
	{
		std::cerr << "Encrypting went ok !" << std::endl;
		ok= true ;
	}
	else 
	{
		std::cerr << "Encrypting went wrong !" << std::endl;
		ok= false ;
	}
	
	return ok ;
}

bool RsLoginHandler::getSSLPasswdFromGPGFile(const RsPeerId& ssl_id,std::string& sslPassword)
{
	// Let's read the password from an encrypted file
	// Let's check if there's a ssl_passpharese_file that we can decrypt with PGP
	//
	FILE *sslPassphraseFile = RsDirUtil::rs_fopen(getSSLPasswdFileName(ssl_id).c_str(), "r");

	if (sslPassphraseFile == NULL)
	{
		std::cerr << "No password provided, and no sslPassphraseFile : " << getSSLPasswdFileName(ssl_id).c_str() << std::endl;
		return 0;
	} 
	fclose(sslPassphraseFile);

	std::cerr << "opening sslPassphraseFile : " << getSSLPasswdFileName(ssl_id).c_str() << std::endl;
	std::string plain ;

	if (AuthGPG::getAuthGPG()->decryptTextFromFile(plain,getSSLPasswdFileName(ssl_id))) 
	{
		std::cerr << "Decrypting went ok !" << std::endl;
		sslPassword = plain ;
		std::cerr << "sslpassword: " << "******************** (length = " << sslPassword.length() << ")" << std::endl;

		return sslPassword.length() > 0 ;
	} 
	else 
	{
		sslPassword = "" ;
		std::cerr << "Error : decrypting went wrong !" << std::endl;

		return false;
	}
}


std::string RsLoginHandler::getSSLPasswdFileName(const RsPeerId& /*ssl_id*/)
{
	return rsAccounts.PathAccountKeysDirectory() + "/" + "ssl_passphrase.pgp";
}

std::string RsLoginHandler::getAutologinFileName(const RsPeerId& /*ssl_id*/)
{
	return rsAccounts.PathAccountKeysDirectory() + "/" + "help.dta" ;
}


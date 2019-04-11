/*******************************************************************************
 * libretroshare/src/rsserver: rsloginhandler.h                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018         retroshare team <retroshare.team@gmail.com>          *
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

#pragma once

#include <string>

/**
 * This class handles login, meaning that it retrieves the SSL password from
 * either the keyring or help.dta file, if autologin is enabled, or from the
 * ssl_passphrase.pgp file, asking for the GPG password to decrypt it.
 *
 * This class should handle the following scenario:
 *
 * Normal login:
 *  - SSL key is stored  ->  do autologin
 *  - SSL key is not stored
 *     - if we're actually in the login process, ask for the gpg passwd, and
 *       decrypt the key file
 *     - if we're just trying for autologin, don't ask for the gpg passwd and
 *       return null
 *
 * Key creation:
 *  - the key should be stored in the gpg file.
 */
class RsLoginHandler
{
public:
	/**
	 * Gets the SSL passwd by any means: try autologin, and look into gpg file
	 * if enable_gpg_key_callback==true
	 */
	static bool getSSLPassword( const RsPeerId& ssl_id,
	                            bool enable_gpg_key_callback,
	                            std::string& ssl_password);

	/**
	 * Checks whether the ssl passwd is already in the gpg file. If the file's
	 * not here, the passwd is stored there, encrypted with the current GPG key.
	 */
	static bool checkAndStoreSSLPasswdIntoGPGFile(
	        const RsPeerId& ssl_id, const std::string& ssl_passwd );

#ifdef RS_AUTOLOGIN
	/**
	 * Stores the given ssl_id/passwd pair into the keyring, or by default into
	 * a file in /[ssl_id]/keys/help.dta
	 */
	static bool enableAutoLogin(const RsPeerId& ssl_id,const std::string& passwd) ;

	/// Clears autologin entry.
	static bool clearAutoLogin(const RsPeerId& ssl_id) ;
#endif // RS_AUTOLOGIN

private:
	static bool getSSLPasswdFromGPGFile(const RsPeerId& ssl_id,std::string& sslPassword);
	static std::string getSSLPasswdFileName(const RsPeerId& ssl_id);

#ifdef RS_AUTOLOGIN
	static bool tryAutoLogin(const RsPeerId& ssl_id,std::string& ssl_passwd);
	static std::string getAutologinFileName(const RsPeerId& ssl_id);
#endif // RS_AUTOLOGIN
};


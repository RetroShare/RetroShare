/*******************************************************************************
 * libretroshare/src/retroshare: rsjsonapi.h                                   *
 *                                                                             *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio.eigenlab.org>             *
 * Copyright (C) 2019-2019  Cyril Soler <csoler@users.sourceforge.net>         *
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
          *
 *******************************************************************************/
#pragma once

class RsJsonAPI
{
public:
    enum {
        JSONAPI_STATUS_UNKNOWN     = 0x00,
        JSONAPI_STATUS_RUNNING     = 0x01,
        JSONAPI_STATUS_NOT_RUNNING = 0x02
    };

	static const uint16_t    DEFAULT_PORT = 9092 ;
	static const std::string DEFAULT_BINDING_ADDRESS ;	// 127.0.0.1

	virtual bool restart() =0;
	virtual bool stop()  =0;

	virtual void setBindingAddress(const std::string& address) =0;
	virtual void setListeningPort(uint16_t port) =0;

	/**
	 * @brief Get status of the json api server
	 * @jsonapi{development}
	 * @return the status picked in the enum JSONAPI_STATUS_UNKNOWN/RUNNING/NOT_RUNNING
	 */
	virtual int status() const=0;

    //=============================================================================================//
    //                        API methods that are also accessible through http                    //
    //=============================================================================================//
	/**
	 * @brief This function should be used by JSON API clients that aren't
	 * authenticated yet, to ask their token to be authorized, the success or
	 * failure will depend on mNewAccessRequestCallback return value, and it
	 * will likely need human user interaction in the process.
	 * @jsonapi{development,unauthenticated}
	 * @param[in] token token to autorize
	 * @return true if authorization succeded, false otherwise.
	 */
	virtual bool requestNewTokenAutorization(const std::string& token)=0;

    //=============================================================================================//
    //                      API methods that SHOULD NOT be accessible through http                 //
    //=============================================================================================//

    //////////////// @Gio: The methods below should not be accessible from the API server !
    ///
	/**
	 * @brief Add new auth (user,passwd) token to the authorized set, creating the token user:passwd internally.
	 * @param[in] alphanumeric_user username to autorize decoded
	 * @param[in] alphanumeric_passwd passwd to autorize decoded
	 * @return true if the token has been added to authorized, false if error occurred
	 */
	virtual bool authorizeUser(const std::string& alphanumeric_user,const std::string& alphanumeric_passwd)=0;

	/**
	 * @brief Revoke given auth token
	 * @param[in] user par of the decoded token
	 * @return true if the token has been revoked, false otherwise
	 */
	virtual bool revokeAuthToken(const std::string& user)=0;

	/**
	 * @brief Get authorized tokens
	 * @return the set of authorized encoded tokens
	 */
    virtual std::map<std::string,std::string> getAuthorizedTokens() =0;

	/**
	 * @brief Check if given JSON API auth token is authorized
	 * @param[in] token decoded
	 * @return tru if authorized, false otherwise
	 */
	virtual bool isAuthTokenValid(const std::string& token);


};

extern RsJsonAPI *rsJsonAPI;


/*******************************************************************************
 * libretroshare/src/retroshare: rsjsonapi.h                                   *
 *                                                                             *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio.eigenlab.org>             *
 * Copyright (C) 2019-2019  Cyril Soler <csoler@users.sourceforge.net>         *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
          *
 *******************************************************************************/
#pragma once

#include <map>
#include <string>

class p3ConfigMgr;
class JsonApiResourceProvider;

class RsJsonAPI
{
public:
	static const uint16_t    DEFAULT_PORT = 9092 ;
	static const std::string DEFAULT_BINDING_ADDRESS ;	// 127.0.0.1

	virtual bool restart() =0;
	virtual bool stop()  =0;
	/**
	 * @brief Get status of the json api server
	 * @jsonapi{development}
	 * @return Returns true if the server is running
	 */
    virtual bool isRunning() =0;

    /*!
     * \brief setBindingAddress
     * 				Sets the binding address of the jsonapi server. Will only take effect after the server is restarted
     * \param address
     * 				Address in IPv4 or IPv6 format.
     */

	virtual void setBindingAddress(const std::string& address) =0;

    /*!
     * \brief setListeningPort
     * 				Port to use when talking to the jsonAPI server.
     * \param port
     * 				Should be a non system-reserved 16 bits port number (1024 to 65535)
     */
	virtual void setListeningPort(uint16_t port) =0;
	virtual uint16_t listeningPort() const =0;

    /*!
     * \brief connectToConfigManager
     * 				Should be called after creating the JsonAPI object so that it publishes itself with the proper config file.
     * 				Since JsonAPI is created *before* login, the config manager does not exist at this time.
     * \param cfgmgr
     */
    virtual void connectToConfigManager(p3ConfigMgr *cfgmgr)=0;

    /*!
     * \brief registerResourceProvider
     * 				This is used to add/remove new web services to JsonAPI. The client should take care of not using a path range already
     * 				used by the jsonAPI server.
     */
    virtual void registerResourceProvider(const JsonApiResourceProvider *)=0;
    virtual void unregisterResourceProvider(const JsonApiResourceProvider *)=0;
    virtual bool hasResourceProvider(const JsonApiResourceProvider *)=0;

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
    //                                    Utility methods                                          //
    //=============================================================================================//

    static bool parseToken(const std::string& clear_token,std::string& user,std::string& passwd);

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
	virtual bool isAuthTokenValid(const std::string& token)=0;


};

extern RsJsonAPI *rsJsonAPI;


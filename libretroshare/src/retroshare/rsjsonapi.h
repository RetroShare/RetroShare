/*******************************************************************************
 * libretroshare/src/retroshare: rsjsonapi.h                                   *
 *                                                                             *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio.eigenlab.org>             *
 * Copyright (C) 2019-2019  Cyril Soler <csoler@users.sourceforge.net>         *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License version 3 as    *
 * published by the Free Software Foundation.                                  *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <map>
#include <string>

class p3ConfigMgr;
class JsonApiResourceProvider;
class RsJsonApi;

/**
 * Pointer to global instance of RsJsonApi service implementation
 * @jsonapi{development}
 */
extern RsJsonApi* rsJsonApi;

class RsJsonApi
{
public:
	static const uint16_t    DEFAULT_PORT = 9092;
	static const std::string DEFAULT_BINDING_ADDRESS; // 127.0.0.1

	/**
	 * @brief Restart RsJsonApi server
	 * @jsonapi{development}
	 */
	virtual bool restart() = 0;

	/**
	 * @brief Request RsJsonApi to stop and wait until ti has stopped.
	 * Be expecially carefull to call this from JSON API because you will loose
	 * access to the API.
	 * @jsonapi{development}
	 */
	virtual bool fullstop() = 0;

	/**
	 * @brief Get status of the json api server
	 * @return Returns true if the server is running
	 */
	virtual bool isRunning() = 0;

	/**
	 * Sets the binding address of the JSON API server. Will only take effect
	 * after the server is restarted.
	 * @jsonapi{development}
	 * @param[in] address Binding address in IPv4 or IPv6 format.
	 */
	virtual void setBindingAddress(const std::string& address) = 0;

	/**
	 * Get the binding address of the JSON API server.
	 * @jsonapi{development}
	 * @return string representing binding address
	 */
	virtual std::string getBindingAddress() const = 0;

	/*!
	 * Set port on which JSON API server will listen. Will only take effect
	 * after the server is restarted.
	 * @jsonapi{development}
	 * @param[in] port Must be available otherwise the binding will fail
	 */
	virtual void setListeningPort(uint16_t port) = 0;

	/*!
	 * Get port on which JSON API server will listen.
	 * @jsonapi{development}
	 */
	virtual uint16_t listeningPort() const = 0;

	/*!
	 * Should be called after creating the JsonAPI object so that it publishes
	 * itself with the proper config file.
	 * Since JsonAPI is created *before* login, the config manager does not
	 * exist at that time.
	 * @param cfgmgr
	 */
	virtual void connectToConfigManager(p3ConfigMgr& cfgmgr) = 0;

	/**
	 * This is used to add/remove new web services to JsonAPI. The client
	 * should take care of not using a path range already used by the jsonAPI
	 * server
	 */
	virtual void registerResourceProvider(const JsonApiResourceProvider&) = 0;
	virtual void unregisterResourceProvider(const JsonApiResourceProvider&) = 0;
	virtual bool hasResourceProvider(const JsonApiResourceProvider&) = 0;

	/**
	 * @brief This function should be used by JSON API clients that aren't
	 * authenticated yet, to ask their token to be authorized, the success or
	 * failure will depend on mNewAccessRequestCallback return value, and it
	 * will likely need human user interaction in the process.
	 * @jsonapi{development,unauthenticated}
	 * @param[in] user user name to authorize
	 * @param[in] password password for the new user
	 * @return true if authorization succeded, false otherwise.
	 */
	virtual bool requestNewTokenAutorization(
	        const std::string& user, const std::string& password) = 0;

	/** Split a token in USER:PASSWORD format into user and password */
	static bool parseToken(
	        const std::string& clear_token,
	        std::string& user, std::string& passwd );

	/**
	 * Add new auth (user,passwd) token to the authorized set, creating the
	 * token user:passwd internally.
	 * @jsonapi{development}
	 * @param[in] user user name to autorize, must be alphanumerinc
	 * @param[in] password password for the user, must be alphanumerinc
	 * @return true if the token has been added to authorized, false if error
	 * occurred
	 */
	virtual bool authorizeUser(
	        const std::string& user, const std::string& password ) = 0;

	/**
	 * @brief Revoke given auth token
	 * @jsonapi{development}
	 * @param[in] user user name of which to revoke authorization
	 * @return true if the token has been revoked, false otherwise
	 */
	virtual bool revokeAuthToken(const std::string& user)=0;

	/**
	 * @brief Get authorized tokens
	 * @jsonapi{development}
	 * @return the set of authorized encoded tokens
	 */
	virtual std::map<std::string,std::string> getAuthorizedTokens() = 0;

	/**
	 * @brief Check if given JSON API auth token is authorized
	 * @jsonapi{development,unauthenticated}
	 * @param[in] token decoded
	 * @return tru if authorized, false otherwise
	 */
	virtual bool isAuthTokenValid(const std::string& token) = 0;

	virtual ~RsJsonApi() = default;
};

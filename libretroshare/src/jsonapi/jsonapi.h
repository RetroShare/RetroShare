/*******************************************************************************
 * libretroshare/src/gxs: jsonapi.h                                            *
 *                                                                             *
 * RetroShare JSON API                                                         *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#include <memory>
#include <restbed>
#include <cstdint>
#include <map>

#include "util/rsthreads.h"
#include "pqi/p3cfgmgr.h"
#include "rsitems/rsitem.h"
#include "jsonapi/jsonapiitems.h"
#include "util/rsthreads.h"

namespace rb = restbed;

struct JsonApiServer;

/**
 * Pointer to global instance of JsonApiServer
 * @jsonapi{development}
 */
extern JsonApiServer* jsonApiServer;

/**
 * Simple usage
 *  \code{.cpp}
 *    JsonApiServer jas(9092);
 *    jsonApiServer = &jas;
 *    jas.start("JsonApiServer");
 *  \endcode
 * Uses p3Config to securely store persistent JSON API authorization tokens
 */
struct JsonApiServer : RsSingleJobThread, p3Config
{
	/**
	 * @brief construct a JsonApiServer instance with given parameters
	 * @param[in] port listening port fpt the JSON API socket
	 * @param[in] bindAddress binding address for the JSON API socket
	 * @param newAccessRequestCallback called when a new auth token is asked to
	 *	be authorized via JSON API, the auth token is passed as parameter, and
	 *	the callback should return true if the new token get access granted and
	 *	false otherwise, this usually requires user interacion to confirm access
	 */
	JsonApiServer(
	        uint16_t port = 9092,
	        const std::string& bindAddress = "127.0.0.1",
	        const std::function<bool(const std::string&)> newAccessRequestCallback = [](const std::string&){return false;} );

	/**
	 * @param[in] path Path itno which publish the API call
	 * @param[in] handler function which will be called to handle the requested
	 * @param[in] requiresAutentication specify if the API call must be
	 *	autenticated or not
	 *   path, the function must be declared like:
	 *   \code{.cpp}
	 *     void functionName(const shared_ptr<restbed::Session> session)
	 *   \endcode
	 */
	void registerHandler(
	        const std::string& path,
	        const std::function<void(const std::shared_ptr<rb::Session>)>& handler,
	        bool requiresAutentication = true );

	/**
	 * @brief Set new access request callback
	 * @param callback function to call when a new JSON API access is requested
	 */
	void setNewAccessRequestCallback(
	        const std::function<bool(const std::string&)>& callback );

	/**
	 * @brief Shutdown the JSON API server
	 * Beware that this method shout down only the JSON API server instance not
	 */
	void shutdown();

	/**
	 * @brief This function should be used by JSON API clients that aren't
	 * authenticated yet, to ask their token to be authorized, the success or
	 * failure will depend on mNewAccessRequestCallback return value, and it
	 * will likely need human user interaction in the process.
	 * @jsonapi{development,unauthenticated}
	 * @param[in] token token to autorize
	 * @return true if authorization succeded, false otherwise.
	 */
	bool requestNewTokenAutorization(const std::string& token);

	/**
	 * @brief Check if given JSON API auth token is authorized
	 * @jsonapi{development}
	 * @param[in] token decoded
	 * @return tru if authorized, false otherwise
	 */
	bool isAuthTokenValid(const std::string& token);

	/**
	 * @brief Get uthorized tokens
	 * @jsonapi{development}
	 * @return the set of authorized encoded tokens
	 */
	std::set<std::string> getAuthorizedTokens();

	/**
	 * @brief Revoke given auth token
	 * @jsonapi{development}
	 * @param[in] token decoded
	 * @return true if the token has been revoked, false otherwise
	 */
	bool revokeAuthToken(const std::string& token);

	/**
	 * @brief Add new auth token to the authorized set
	 * @jsonapi{development}
	 * @param[in] token toke to autorize decoded
	 * @return true if the token has been added to authorized, false if error
	 *	occurred or if the token was already authorized
	 */
	bool authorizeToken(const std::string& token);

	/**
	 * @brief Get decoded version of the given encoded token
	 * @jsonapi{development,unauthenticated}
	 * @param[in] token encoded
	 * @return token decoded
	 */
	static std::string decodeToken(const std::string& token);

	/**
	 * @brief Get encoded version of the given decoded token
	 * @jsonapi{development,unauthenticated}
	 * @param[in] token decoded
	 * @return token encoded
	 */
	static std::string encondeToken(const std::string& token);

	/**
	 * @brief Write version information to given paramethers
	 * @jsonapi{development,unauthenticated}
	 * @param[out] major storage
	 * @param[out] minor storage
	 * @param[out] mini storage
	 * @param[out] extra storage
	 * @param[out] human storage
	 */
	static void version( uint32_t& major, uint32_t& minor, uint32_t& mini,
	                     std::string& extra, std::string&human );

	/// @see RsSingleJobThread
	virtual void run();

private:
	/// @see p3Config::setupSerialiser
	virtual RsSerialiser* setupSerialiser();

	/// @see p3Config::saveList
	virtual bool saveList(bool &cleanup, std::list<RsItem *>& saveItems);

	/// @see p3Config::loadList
	virtual bool loadList(std::list<RsItem *>& loadList);

	/// @see p3Config::saveDone
	virtual void saveDone();

	const uint16_t mPort;
	const std::string mBindAddress;
	rb::Service mService;

	/// Called when new JSON API auth token is requested to be authorized
	std::function<bool(const std::string&)> mNewAccessRequestCallback;

	/// Encrypted persistent storage for authorized JSON API tokens
	JsonApiServerAuthTokenStorage mAuthTokenStorage;
	RsMutex configMutex;

	static const std::multimap<std::string, std::string> corsHeaders;
	static const std::multimap<std::string, std::string> corsOptionsHeaders;
	static void handleCorsOptions(const std::shared_ptr<rb::Session> session);

	static bool checkRsServicePtrReady(
	        void* serviceInstance, const std::string& serviceName,
	        RsGenericSerializer::SerializeContext& ctx,
	        const std::shared_ptr<restbed::Session> session );
};


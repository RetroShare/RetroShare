/*******************************************************************************
 * RetroShare JSON API                                                         *
 *                                                                             *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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

#include <string>
#include <memory>
#include <restbed>
#include <cstdint>
#include <map>
#include <set>
#include <functional>
#include <vector>

#include "util/rsthreads.h"
#include "pqi/p3cfgmgr.h"
#include "rsitems/rsitem.h"
#include "jsonapi/jsonapiitems.h"
#include "retroshare/rsjsonapi.h"
#include "util/rsthreads.h"

namespace rb = restbed;

class JsonApiResourceProvider
{
public:
	virtual ~JsonApiResourceProvider() = default;

	virtual std::vector<std::shared_ptr<rb::Resource>> getResources() const = 0;

	inline bool operator< (const JsonApiResourceProvider& rp) const
	{ return this < &rp; }
};

/**
 * Uses p3Config to securely store persistent JSON API authorization tokens
 */
class JsonApiServer : public p3Config, public RsThread, public RsJsonApi
{
public:
	JsonApiServer();
	~JsonApiServer() override = default;

	std::vector<std::shared_ptr<rb::Resource>> getResources() const;

	/// @see RsJsonApi
	bool restart() override;

	/// @see RsJsonApi
	bool fullstop() override;

	/// @see RsJsonApi
	inline bool isRunning() override { return RsThread::isRunning(); }

	/// @see RsJsonApi
	void setListeningPort(uint16_t port) override;

	/// @see RsJsonApi
	void setBindingAddress(const std::string& bindAddress) override;

	/// @see RsJsonApi
	std::string getBindingAddress() const override;

	/// @see RsJsonApi
	uint16_t listeningPort() const override;

	/// @see RsJsonApi
	void connectToConfigManager(p3ConfigMgr& cfgmgr) override;

	/// @see RsJsonApi
	virtual bool authorizeUser(
	        const std::string& alphanumeric_user,
	        const std::string& alphanumeric_passwd ) override;

	/// @see RsJsonApi
	std::map<std::string,std::string> getAuthorizedTokens() override;

	/// @see RsJsonApi
	bool revokeAuthToken(const std::string& user) override;

	/// @see RsJsonApi
	bool isAuthTokenValid(const std::string& token) override;

	/// @see RsJsonAPI
	bool requestNewTokenAutorization(
	        const std::string& user, const std::string& password ) override;

	/// @see RsJsonApi
	void registerResourceProvider(const JsonApiResourceProvider&) override;

	/// @see RsJsonApi
	void unregisterResourceProvider(const JsonApiResourceProvider&) override;

	/// @see RsJsonApi
	bool hasResourceProvider(const JsonApiResourceProvider&) override;

	/**
	 * @brief Get decoded version of the given encoded token
	 * @param[in] radix64_token encoded
	 * @return token decoded
	 */
	static std::string decodeToken(const std::string& radix64_token);

	/**
	 * Register an unique handler for a resource path
	 * @param[in] path Path into which publish the API call
	 * @param[in] handler function which will be called to handle the requested
	 * @param[in] requiresAutentication specify if the API call must be
	 *	autenticated or not.
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
	        const std::function<bool(const std::string&, const std::string&)>&
	        callback );

private:
	/// @see p3Config::setupSerialiser
	RsSerialiser* setupSerialiser() override;

	/// @see p3Config::saveList
	bool saveList(bool &cleanup, std::list<RsItem *>& saveItems) override;

	/// @see p3Config::loadList
	bool loadList(std::list<RsItem *>& loadList) override;

	/// @see p3Config::saveDone
	void saveDone() override;

	uint16_t mPort;
	std::string mBindAddress;

	/// Called when new JSON API auth token is requested to be authorized
	std::function<bool(const std::string&, const std::string& passwd)>
	    mNewAccessRequestCallback;

	/// Encrypted persistent storage for authorized JSON API tokens
	JsonApiServerAuthTokenStorage mAuthTokenStorage;
	RsMutex configMutex;

	static const std::multimap<std::string, std::string> corsHeaders;
	static const std::multimap<std::string, std::string> corsOptionsHeaders;
	static void handleCorsOptions(const std::shared_ptr<rb::Session> session);

	static bool checkRsServicePtrReady(
	        const void* serviceInstance, const std::string& serviceName,
	        RsGenericSerializer::SerializeContext& ctx,
	        const std::shared_ptr<rb::Session> session );

	static inline bool checkRsServicePtrReady(
	        const std::shared_ptr<const void> serviceInstance,
	        const std::string& serviceName,
	        RsGenericSerializer::SerializeContext& ctx,
	        const std::shared_ptr<rb::Session> session )
	{
		return checkRsServicePtrReady(
		            serviceInstance.get(), serviceName, ctx, session );
	}

	std::vector<std::shared_ptr<rb::Resource>> mResources;
	std::set<
	    std::reference_wrapper<const JsonApiResourceProvider>,
	    std::less<const JsonApiResourceProvider> > mResourceProviders;

	/// @see RsThread
	void runloop() override;

	std::shared_ptr<restbed::Service> mService;

	uint16_t mListeningPort;
	std::string mBindingAddress;
};


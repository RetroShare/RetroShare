/*******************************************************************************
 * RetroShare JSON API                                                         *
 *                                                                             *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU Affero General Public License for more details.                         *
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

#include "util/rsthreads.h"
#include "pqi/p3cfgmgr.h"
#include "rsitems/rsitem.h"
#include "jsonapi/jsonapiitems.h"
#include "jsonapi/restbedservice.h"
#include "retroshare/rsjsonapi.h"
#include "util/rsthreads.h"

namespace rb = restbed;

class JsonApiResourceProvider
{
public:
    JsonApiResourceProvider() {}
    virtual ~JsonApiResourceProvider() = default;

    virtual std::string getName() const =0;
    virtual std::vector<std::shared_ptr<rb::Resource> > getResources() const =0;
};

/**
 * Simple usage
 *  \code{.cpp}
 *    JsonApiServer jas(9092);
 *    jsonApiServer = &jas;
 *    jas.start("JsonApiServer");
 *  \endcode
 * Uses p3Config to securely store persistent JSON API authorization tokens
 */
class JsonApiServer : public p3Config, public RestbedService, public RsJsonAPI
{
public:
    JsonApiServer() ;
    virtual ~JsonApiServer() = default;

    // implements RestbedService

    virtual std::vector<std::shared_ptr<rb::Resource> > getResources() const override ;

     // RsJsonAPI public API

    bool restart()   override { return RestbedService::restart(); }
    bool stop()      override { return RestbedService::fullstop();}
    bool isRunning() override { return RestbedService::isRunning(); }

    void setListeningPort(uint16_t port) override { return RestbedService::setListeningPort(port); }
    void setBindingAddress(const std::string& bind_address) override { return RestbedService::setBindAddress(bind_address); }
    uint16_t listeningPort() const override { return RestbedService::listeningPort() ; }

    virtual void connectToConfigManager(p3ConfigMgr *cfgmgr);

	virtual bool authorizeUser(const std::string& alphanumeric_user,const std::string& alphanumeric_passwd) override;
	virtual std::map<std::string,std::string> getAuthorizedTokens() override;
	bool revokeAuthToken(const std::string& user) override;
	bool isAuthTokenValid(const std::string& token) override;
	bool requestNewTokenAutorization(const std::string& user) override;

    void registerResourceProvider(const JsonApiResourceProvider *);
    void unregisterResourceProvider(const JsonApiResourceProvider *);
    bool hasResourceProvider(const JsonApiResourceProvider *);

    // private API. These methods may be moved to RsJsonAPI so as to be visible in http mode, if needed.

	/**
	 * @brief Get decoded version of the given encoded token
	 * @jsonapi{development,unauthenticated}
	 * @param[in] radix64_token encoded
	 * @return token decoded
	 */
	static std::string decodeToken(const std::string& radix64_token);

	/**
	 * @brief Get encoded version of the given decoded token
	 * @jsonapi{development,unauthenticated}
	 * @param[in] clear_token decoded
	 * @return token encoded
	 */
	static std::string encodeToken(const std::string& clear_token);

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


    //   /!\ These methods shouldn't be accessible through http!

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
	void setNewAccessRequestCallback(const std::function<bool(const std::string&,std::string&)>& callback );

private:

	/// @see p3Config::setupSerialiser
	virtual RsSerialiser* setupSerialiser();

	/// @see p3Config::saveList
	virtual bool saveList(bool &cleanup, std::list<RsItem *>& saveItems);

	/// @see p3Config::loadList
	virtual bool loadList(std::list<RsItem *>& loadList);

	/// @see p3Config::saveDone
	virtual void saveDone();

	uint16_t mPort;
	std::string mBindAddress;

	/// Called when new JSON API auth token is requested to be authorized
	/// The callback supplies the password to be used to make the token
	///
	std::function<bool(const std::string&,std::string& passwd)> mNewAccessRequestCallback;

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
		return checkRsServicePtrReady( serviceInstance.get(), serviceName, ctx, session );
	}

	std::vector<std::shared_ptr<rb::Resource> > mResources;
	std::set<const JsonApiResourceProvider *> mResourceProviders;
};


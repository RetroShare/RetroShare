/*******************************************************************************
 * libretroshare/src/gxs: jsonapi.cpp                                          *
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

#include "jsonapi.h"

#include <string>
#include <sstream>
#include <memory>
#include <restbed>
#include <vector>
#include <openssl/crypto.h>

#include "util/rsjson.h"
#include "retroshare/rsfiles.h"
#include "util/radix64.h"
#include "retroshare/rsversion.h"
#include "retroshare/rsinit.h"
#include "util/rsnet.h"
#include "retroshare/rsiface.h"
#include "retroshare/rsinit.h"
#include "util/rsurl.h"
#include "util/rstime.h"

// Generated at compile time
#include "jsonapi-includes.inl"

/*extern*/ JsonApiServer* jsonApiServer = nullptr;

/*static*/ const std::multimap<std::string, std::string>
JsonApiServer::corsHeaders =
{
    { "Access-Control-Allow-Origin", "*" },
    { "Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
    { "Access-Control-Allow-Headers", "DNT,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Range" },
    { "Access-Control-Expose-Headers", "Content-Length,Content-Range" }
};

/*static*/ const std::multimap<std::string, std::string>
JsonApiServer::corsOptionsHeaders =
{
    { "Access-Control-Allow-Origin", "*" },
    { "Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
    { "Access-Control-Allow-Headers", "DNT,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Range" },
    { "Access-Control-Max-Age", "1728000" }, // 20 days
    { "Content-Type", "text/plain; charset=utf-8" },
    { "Content-Length", "0" }
};

#define INITIALIZE_API_CALL_JSON_CONTEXT \
	RsGenericSerializer::SerializeContext cReq( \
	            nullptr, 0, \
	            RsGenericSerializer::SERIALIZATION_FLAG_YIELDING ); \
	RsJson& jReq(cReq.mJson); \
	if(session->get_request()->get_method() == "GET") \
    { \
	    const std::string jrqp(session->get_request()->get_query_parameter("jsonData")); \
	    jReq.Parse(jrqp.c_str(), jrqp.size()); \
	} \
	else \
	    jReq.Parse(reinterpret_cast<const char*>(body.data()), body.size()); \
\
	RsGenericSerializer::SerializeContext cAns; \
	RsJson& jAns(cAns.mJson); \
\
	/* if caller specified caller_data put it back in the answhere */ \
	const char kcd[] = "caller_data"; \
	if(jReq.HasMember(kcd)) \
	    jAns.AddMember(kcd, jReq[kcd], jAns.GetAllocator())

#define DEFAULT_API_CALL_JSON_RETURN(RET_CODE) \
	std::stringstream ss; \
	ss << jAns; \
	std::string&& ans(ss.str()); \
	auto headers = corsHeaders; \
	headers.insert({ "Content-Type", "text/json" }); \
	headers.insert({ "Content-Length", std::to_string(ans.length()) }); \
	session->close(RET_CODE, ans, headers)


/*static*/ bool JsonApiServer::checkRsServicePtrReady(
        void* serviceInstance, const std::string& serviceName,
        RsGenericSerializer::SerializeContext& ctx,
        const std::shared_ptr<restbed::Session> session)
{
	if(serviceInstance) return true;

	std::string jsonApiError = __PRETTY_FUNCTION__;
	jsonApiError += "Service: ";
	jsonApiError += serviceName;
	jsonApiError += " not initialized!";

	RsGenericSerializer::SerializeJob j(RsGenericSerializer::TO_JSON);
	RS_SERIAL_PROCESS(jsonApiError);

	RsJson& jAns(ctx.mJson);
	DEFAULT_API_CALL_JSON_RETURN(rb::CONFLICT);
	return false;
}

JsonApiServer::JsonApiServer(uint16_t port, const std::string& bindAddress,
        const std::function<bool(const std::string&)> newAccessRequestCallback ) :
    mPort(port), mBindAddress(bindAddress),
    mNewAccessRequestCallback(newAccessRequestCallback),
    configMutex("JsonApiServer config")
{
	registerHandler("/rsLoginHelper/createLocation",
	                [this](const std::shared_ptr<rb::Session> session)
	{
		size_t reqSize = session->get_request()->get_header("Content-Length", 0);
		session->fetch( reqSize, [this](
		                const std::shared_ptr<rb::Session> session,
		                const rb::Bytes& body )
		{
			INITIALIZE_API_CALL_JSON_CONTEXT;

			RsLoginHelper::Location location;
			std::string password;
			std::string errorMessage;
			bool makeHidden = false;
			bool makeAutoTor = false;

			// deserialize input parameters from JSON
			{
				RsGenericSerializer::SerializeContext& ctx(cReq);
				RsGenericSerializer::SerializeJob j(RsGenericSerializer::FROM_JSON);
				RS_SERIAL_PROCESS(location);
				RS_SERIAL_PROCESS(password);
				RS_SERIAL_PROCESS(makeHidden);
				RS_SERIAL_PROCESS(makeAutoTor);
			}

			// call retroshare C++ API
			bool retval = rsLoginHelper->createLocation(
			            location, password, errorMessage, makeHidden,
			            makeAutoTor );

			if(retval)
				authorizeToken(location.mLocationId.toStdString()+":"+password);

			// serialize out parameters and return value to JSON
			{
				RsGenericSerializer::SerializeContext& ctx(cAns);
				RsGenericSerializer::SerializeJob j(RsGenericSerializer::TO_JSON);
				RS_SERIAL_PROCESS(location);
				RS_SERIAL_PROCESS(errorMessage);
				RS_SERIAL_PROCESS(retval);
			}

			// return them to the API caller
			DEFAULT_API_CALL_JSON_RETURN(rb::OK);
		} );
	}, false);

	registerHandler("/rsLoginHelper/attemptLogin",
	                [this](const std::shared_ptr<rb::Session> session)
	{
		size_t reqSize = session->get_request()->get_header("Content-Length", 0);
		session->fetch( reqSize, [this](
		                const std::shared_ptr<rb::Session> session,
		                const rb::Bytes& body )
		{
			INITIALIZE_API_CALL_JSON_CONTEXT;

			RsPeerId account;
			std::string password;

			// deserialize input parameters from JSON
			{
				RsGenericSerializer::SerializeContext& ctx(cReq);
				RsGenericSerializer::SerializeJob j(RsGenericSerializer::FROM_JSON);
				RS_SERIAL_PROCESS(account);
				RS_SERIAL_PROCESS(password);
			}

			// call retroshare C++ API
			RsInit::LoadCertificateStatus retval =
			        rsLoginHelper->attemptLogin(account, password);

			if( retval == RsInit::OK )
				authorizeToken(account.toStdString()+":"+password);

			// serialize out parameters and return value to JSON
			{
				RsGenericSerializer::SerializeContext& ctx(cAns);
				RsGenericSerializer::SerializeJob j(RsGenericSerializer::TO_JSON);
				RS_SERIAL_PROCESS(retval);
			}

			// return them to the API caller
			DEFAULT_API_CALL_JSON_RETURN(rb::OK);
		} );
	}, false);

	registerHandler("/rsControl/rsGlobalShutDown",
	                [](const std::shared_ptr<rb::Session> session)
	{
		size_t reqSize = session->get_request()->get_header("Content-Length", 0);
		session->fetch( reqSize, [](
		                const std::shared_ptr<rb::Session> session,
		                const rb::Bytes& body )
		{
			INITIALIZE_API_CALL_JSON_CONTEXT;
			DEFAULT_API_CALL_JSON_RETURN(rb::OK);
			rsControl->rsGlobalShutDown();
		} );
	}, true);

	registerHandler("/rsFiles/getFileData",
	                [](const std::shared_ptr<rb::Session> session)
	{
		size_t reqSize = session->get_request()->get_header("Content-Length", 0);
		session->fetch( reqSize, [](
		                const std::shared_ptr<rb::Session> session,
		                const rb::Bytes& body )
		{
			INITIALIZE_API_CALL_JSON_CONTEXT;

			if(!checkRsServicePtrReady(rsFiles, "rsFiles", cAns, session))
				return;

			RsFileHash hash;
			uint64_t offset;
			uint32_t requested_size;
			bool retval = false;
			std::string errorMessage;
			std::string base64data;

			// deserialize input parameters from JSON
			{
				RsGenericSerializer::SerializeContext& ctx(cReq);
				RsGenericSerializer::SerializeJob j(RsGenericSerializer::FROM_JSON);
				RS_SERIAL_PROCESS(hash);
				RS_SERIAL_PROCESS(offset);
				RS_SERIAL_PROCESS(requested_size);
			}

			if(requested_size > 10485760)
				errorMessage = "requested_size is too big! Better less then 1M";
			else
			{
				std::vector<uint8_t> buffer(requested_size);

				// call retroshare C++ API
				retval = rsFiles->getFileData(
				            hash, offset, requested_size, buffer.data());

				Radix64::encode(buffer.data(), requested_size, base64data);
			}

			// serialize out parameters and return value to JSON
			{
				RsGenericSerializer::SerializeContext& ctx(cAns);
				RsGenericSerializer::SerializeJob j(RsGenericSerializer::TO_JSON);
				RS_SERIAL_PROCESS(retval);
				RS_SERIAL_PROCESS(requested_size);
				RS_SERIAL_PROCESS(base64data);
				if(!errorMessage.empty()) RS_SERIAL_PROCESS(errorMessage);
			}

			DEFAULT_API_CALL_JSON_RETURN(rb::OK);
		} );
	}, true);

// Generated at compile time
#include "jsonapi-wrappers.inl"
}

void JsonApiServer::run()
{
	std::shared_ptr<rb::Settings> settings(new rb::Settings);
	settings->set_port(mPort);
	settings->set_bind_address(mBindAddress);
	settings->set_default_header("Cache-Control", "no-cache");

	{
		sockaddr_storage tmp;
		sockaddr_storage_inet_pton(tmp, mBindAddress);
		sockaddr_storage_setport(tmp, mPort);
		sockaddr_storage_ipv6_to_ipv4(tmp);
		RsUrl tmpUrl(sockaddr_storage_tostring(tmp));
		tmpUrl.setScheme("http");

		std::cerr << "JSON API listening on " << tmpUrl.toString()
		          << std::endl;
	}

	mService.start(settings);
}

void JsonApiServer::registerHandler(
        const std::string& path,
        const std::function<void (const std::shared_ptr<restbed::Session>)>& handler,
        bool requiresAutentication )
{
	std::shared_ptr<restbed::Resource> resource(new rb::Resource);
	resource->set_path(path);
	resource->set_method_handler("GET", handler);
	resource->set_method_handler("POST", handler);
	resource->set_method_handler("OPTIONS", handleCorsOptions);

	if(requiresAutentication)
		resource->set_authentication_handler(
		            [this](
		                const std::shared_ptr<rb::Session> session,
		                const std::function<void (const std::shared_ptr<rb::Session>)>& callback )
		{
			if(!rsLoginHelper->isLoggedIn())
			{
				session->close(rb::CONFLICT);
				return;
			}

			std::istringstream authHeader;
			authHeader.str(session->get_request()->get_header("Authorization"));

			std::string authToken;
			std::getline(authHeader, authToken, ' ');

			if(authToken != "Basic")
			{
				session->close(rb::UNAUTHORIZED);
				return;
			}

			std::getline(authHeader, authToken, ' ');
			authToken = decodeToken(authToken);

			if(isAuthTokenValid(authToken)) callback(session);
			else session->close(rb::UNAUTHORIZED);
		} );

	mService.publish(resource);
}

void JsonApiServer::setNewAccessRequestCallback(
        const std::function<bool (const std::string&)>& callback )
{ mNewAccessRequestCallback = callback; }

void JsonApiServer::shutdown() { mService.stop(); }

bool JsonApiServer::requestNewTokenAutorization(const std::string& token)
{
	if(rsLoginHelper->isLoggedIn() && mNewAccessRequestCallback(token))
		return authorizeToken(token);
	return false;
}

bool JsonApiServer::isAuthTokenValid(const std::string& token)
{
	RS_STACK_MUTEX(configMutex);

	// attempt avoiding +else CRYPTO_memcmp+ being optimized away
	int noOptimiz = 1;

	/* Do not use mAuthTokenStorage.mAuthorizedTokens.count(token), because
	 * std::string comparison is usually not constant time on content to be
	 * faster, so an attacker may use timings to guess authorized tokens */
	for(const std::string& vTok : mAuthTokenStorage.mAuthorizedTokens)
	{
		if( token.size() == vTok.size() &&
		        ( noOptimiz = CRYPTO_memcmp( token.data(), vTok.data(),
		                                     vTok.size() ) ) == 0 )
			return true;
		// Make token size guessing harder
		else noOptimiz = CRYPTO_memcmp(token.data(), token.data(), token.size());
	}

	// attempt avoiding +else CRYPTO_memcmp+ being optimized away
	return static_cast<uint32_t>(noOptimiz) + 1 == 0;
}

std::set<std::string> JsonApiServer::getAuthorizedTokens()
{
	RS_STACK_MUTEX(configMutex);
	return mAuthTokenStorage.mAuthorizedTokens;
}

bool JsonApiServer::revokeAuthToken(const std::string& token)
{
	RS_STACK_MUTEX(configMutex);
	if(mAuthTokenStorage.mAuthorizedTokens.erase(token))
	{
		IndicateConfigChanged();
		return true;
	}
	return false;
}

bool JsonApiServer::authorizeToken(const std::string& token)
{
	if(token.empty()) return false;

	RS_STACK_MUTEX(configMutex);
	if(mAuthTokenStorage.mAuthorizedTokens.insert(token).second)
	{
		IndicateConfigChanged();
		return true;
	}
	return false;
}

/*static*/ std::string JsonApiServer::decodeToken(const std::string& token)
{
	std::vector<uint8_t> decodedVect(Radix64::decode(token));
	std::string decodedToken(
	            reinterpret_cast<const char*>(&decodedVect[0]),
	            decodedVect.size() );
	return decodedToken;
}

/*static*/ std::string JsonApiServer::encondeToken(const std::string& token)
{
	std::string encoded;
	Radix64::encode( reinterpret_cast<const uint8_t*>(token.c_str()),
	                 token.length(), encoded );
	return encoded;
}

/*static*/ void JsonApiServer::version(
        uint32_t& major, uint32_t& minor, uint32_t& mini, std::string& extra,
        std::string& human )
{
	major = RS_MAJOR_VERSION;
	minor = RS_MINOR_VERSION;
	mini = RS_MINI_VERSION;
	extra = RS_EXTRA_VERSION;
	human = RS_HUMAN_READABLE_VERSION;
}

RsSerialiser* JsonApiServer::setupSerialiser()
{
	RsSerialiser* rss = new RsSerialiser;
	rss->addSerialType(new JsonApiConfigSerializer);
	return rss;
}

bool JsonApiServer::saveList(bool& cleanup, std::list<RsItem*>& saveItems)
{
	cleanup = false;
	configMutex.lock();
	saveItems.push_back(&mAuthTokenStorage);
	return true;
}

bool JsonApiServer::loadList(std::list<RsItem*>& loadList)
{
	for(RsItem* it : loadList)
		switch (static_cast<JsonApiItemsType>(it->PacketSubType()))
		{
		case JsonApiItemsType::AuthTokenItem:
			mAuthTokenStorage = *static_cast<JsonApiServerAuthTokenStorage*>(it);
			delete it;
			break;
		default:
			delete it;
			break;
		}
	return true;
}

void JsonApiServer::saveDone() { configMutex.unlock(); }

void JsonApiServer::handleCorsOptions(
        const std::shared_ptr<restbed::Session> session )
{ session->close(rb::NO_CONTENT, corsOptionsHeaders); }


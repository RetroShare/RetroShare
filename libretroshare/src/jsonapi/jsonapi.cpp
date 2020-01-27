/*
 * RetroShare JSON API
 *
 * Copyright (C) 2018-2020  Gioacchino Mazzurco <gio@eigenlab.org>
 * Copyright (C) 2019-2020  Asociación Civil Altermundi <info@altermundi.net>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 * SPDX-FileCopyrightText: 2004-2019 RetroShare Team <contact@retroshare.cc>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include <string>
#include <sstream>
#include <memory>
#include <restbed>
#include <vector>
#include <openssl/crypto.h>


#include "jsonapi.h"

#include "util/rsjson.h"
#include "retroshare/rsfiles.h"
#include "util/radix64.h"
#include "retroshare/rsinit.h"
#include "util/rsnet.h"
#include "retroshare/rsiface.h"
#include "retroshare/rsinit.h"
#include "util/rsurl.h"
#include "util/rstime.h"
#include "retroshare/rsevents.h"
#include "retroshare/rsversion.h"

// Generated at compile time
#include "jsonapi-includes.inl"

/*extern*/ RsJsonApi* rsJsonApi = nullptr;

const std::string RsJsonApi::DEFAULT_BINDING_ADDRESS = "127.0.0.1";

/*static*/ const std::multimap<std::string, std::string>
JsonApiServer::corsHeaders =
{
    { "Access-Control-Allow-Origin", "*" },
    { "Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
    { "Access-Control-Allow-Headers", "Authorization,DNT,User-Agent,"
                                      "X-Requested-With,If-Modified-Since,"
                                      "Cache-Control,Content-Type,Range" },
    { "Access-Control-Expose-Headers", "Content-Length,Content-Range" }
};

/*static*/ const std::multimap<std::string, std::string>
JsonApiServer::corsOptionsHeaders =
{
    { "Access-Control-Allow-Origin", "*" },
    { "Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
    { "Access-Control-Allow-Headers", "Authorization,DNT,User-Agent,"
                                      "X-Requested-With,If-Modified-Since,"
                                      "Cache-Control,Content-Type,Range" },
    { "Access-Control-Max-Age", "1728000" }, // 20 days
    { "Content-Type", "text/plain; charset=utf-8" },
    { "Content-Length", "0" }
};

/* static */const RsJsonApiErrorCategory JsonApiServer::sErrorCategory;

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
        const void* serviceInstance, const std::string& serviceName,
        RsGenericSerializer::SerializeContext& ctx,
        const std::shared_ptr<rb::Session> session )
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

bool RsJsonApi::parseToken(
        const std::string& clear_token, std::string& user,std::string& passwd )
{
	auto colonIndex = std::string::npos;
	const auto tkLen = clear_token.length();

	for(uint32_t i=0; i < tkLen; ++i)
		if(clear_token[i] == ':') colonIndex = i;

	user = clear_token.substr(0, colonIndex);

	if(colonIndex < tkLen)
		passwd = clear_token.substr(colonIndex + 1);

	return true;
}


JsonApiServer::JsonApiServer(): configMutex("JsonApiServer config"),
    mService(std::make_shared<restbed::Service>()),
    mServiceMutex("JsonApiServer restbed ptr"),
    mListeningPort(RsJsonApi::DEFAULT_PORT),
    mBindingAddress(RsJsonApi::DEFAULT_BINDING_ADDRESS)
{
	registerHandler("/rsLoginHelper/createLocation",
	                [this](const std::shared_ptr<rb::Session> session)
	{
		auto reqSize = session->get_request()->get_header("Content-Length", 0);
		session->fetch( static_cast<size_t>(reqSize), [this](
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
				authorizeUser(location.mLocationId.toStdString(),password);

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
		auto reqSize = session->get_request()->get_header("Content-Length", 0);
		session->fetch( static_cast<size_t>(reqSize), [this](
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
				authorizeUser(account.toStdString(), password);

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
		auto reqSize = session->get_request()->get_header("Content-Length", 0);
		session->fetch( static_cast<size_t>(reqSize), [](
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
		auto reqSize = session->get_request()->get_header("Content-Length", 0);
		session->fetch( static_cast<size_t>(reqSize), [](
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

	registerHandler("/rsEvents/registerEventsHandler",
	        [this](const std::shared_ptr<rb::Session> session)
	{
		const std::weak_ptr<rb::Service> weakService(mService);
		const std::multimap<std::string, std::string> headers
		{
			{ "Connection", "keep-alive" },
			{ "Content-Type", "text/event-stream" }
		};
		session->yield(rb::OK, headers);

		size_t reqSize = static_cast<size_t>(
		            session->get_request()->get_header("Content-Length", 0) );
		session->fetch( reqSize, [weakService](
		                const std::shared_ptr<rb::Session> session,
		                const rb::Bytes& body )
		{
			INITIALIZE_API_CALL_JSON_CONTEXT;

			if( !checkRsServicePtrReady(
						rsEvents, "rsEvents", cAns, session ) )
				return;

			RsEventType eventType = RsEventType::NONE;

			// deserialize input parameters from JSON
			{
				RsGenericSerializer::SerializeContext& ctx(cReq);
				RsGenericSerializer::SerializeJob j(RsGenericSerializer::FROM_JSON);
				RS_SERIAL_PROCESS(eventType);
			}

			const std::weak_ptr<rb::Session> weakSession(session);
			RsEventsHandlerId_t hId = rsEvents->generateUniqueHandlerId();
			std::function<void(std::shared_ptr<const RsEvent>)> multiCallback =
			        [weakSession, weakService, hId](
			        std::shared_ptr<const RsEvent> event )
			{
				auto lService = weakService.lock();
				if(!lService || lService->is_down())
				{
					if(rsEvents) rsEvents->unregisterEventsHandler(hId);
					return;
				}

				lService->schedule( [weakSession, hId, event]()
				{
					auto session = weakSession.lock();
					if(!session || session->is_closed())
					{
						if(rsEvents) rsEvents->unregisterEventsHandler(hId);
						return;
					}

					RsGenericSerializer::SerializeContext ctx;
					RsTypeSerializer::serial_process(
					            RsGenericSerializer::TO_JSON, ctx,
					            *const_cast<RsEvent*>(event.get()), "event" );

					std::stringstream message;
					message << "data: " << compactJSON << ctx.mJson << "\n\n";

					session->yield(message.str());
				} );
			};

			bool retval = rsEvents->registerEventsHandler(eventType,multiCallback, hId);

			{
				RsGenericSerializer::SerializeContext& ctx(cAns);
				RsGenericSerializer::SerializeJob j(RsGenericSerializer::TO_JSON);
				RS_SERIAL_PROCESS(retval);
			}

			// return them to the API caller
			std::stringstream message;
			message << "data: " << compactJSON << cAns.mJson << "\n\n";
			session->yield(message.str());
		} );
	}, true);
// Generated at compile time
#include "jsonapi-wrappers.inl"
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
		            [this, path](
		                const std::shared_ptr<rb::Session> session,
		                const std::function<void (const std::shared_ptr<rb::Session>)>& callback )
		{
			const auto authFail =
			        [&path, &session](int status) -> RsWarn::stream_type&
			{
				/* Capture session by reference as it is cheaper then copying
				 * shared_ptr by value which is not needed in this case */

				session->close(status, corsOptionsHeaders);
				return RsWarn() << "JsonApiServer authentication handler "
				                   "blocked an attempt to call JSON API "
				                   "authenticated method: " << path;
			};

			if(session->get_request()->get_method() == "OPTIONS")
			{
				callback(session);
				return;
			}

			if(!rsLoginHelper->isLoggedIn())
			{
				authFail(rb::CONFLICT) << " before RetroShare login"
				                       << std::endl;
				return;
			}

			std::istringstream authHeader;
			authHeader.str(session->get_request()->get_header("Authorization"));

			std::string authToken;
			std::getline(authHeader, authToken, ' ');

			if(authToken != "Basic")
			{
				authFail(rb::UNAUTHORIZED)
				        << " with wrong Authorization header: "
				        << authHeader.str() << std::endl;
				return;
			}

			std::getline(authHeader, authToken, ' ');
			authToken = decodeToken(authToken);

			std::error_condition ec;
			if(isAuthTokenValid(authToken, ec)) callback(session);
			else
			{
				std::string tUser;
				parseToken(authToken, tUser, RS_DEFAULT_STORAGE_PARAM(std::string));
				authFail(rb::UNAUTHORIZED)
				        << " user: " << tUser
				        << " error: " << ec.value() << " " << ec.message()
				        << std::endl;
			}
		} );

	mResources.push_back(resource);
}

void JsonApiServer::setNewAccessRequestCallback(
        const std::function<bool (const std::string&, const std::string&)>& callback )
{ mNewAccessRequestCallback = callback; }

/*static*/ std::error_condition JsonApiServer::badApiCredientalsFormat(
        const std::string& user, const std::string& passwd )
{
	if(user.find(':') < std::string::npos)
		return RsJsonApiErrorNum::API_USER_CONTAIN_COLON;

	if(user.empty())
		RsWarn() << __PRETTY_FUNCTION__ << " User is empty, are you sure "
		         << "this what you wanted?" << std::endl;

	if(passwd.empty())
		RsWarn() << __PRETTY_FUNCTION__ << " Password is empty, are you sure "
		         << "this what you wanted?" << std::endl;

	return std::error_condition();
}

std::error_condition JsonApiServer::requestNewTokenAutorization(
        const std::string& user, const std::string& passwd )
{
	auto ec = badApiCredientalsFormat(user, passwd);
	if(ec) return ec;

	if(!rsLoginHelper->isLoggedIn())
		return RsJsonApiErrorNum::CANNOT_EXECUTE_BEFORE_RS_LOGIN;

	if(mNewAccessRequestCallback(user, passwd))
		return authorizeUser(user, passwd);

	return RsJsonApiErrorNum::AUTHORIZATION_REQUEST_DENIED;
}

bool JsonApiServer::isAuthTokenValid(
        const std::string& token, std::error_condition& error )
{
	RS_STACK_MUTEX(configMutex);

	const auto failure = [&error](RsJsonApiErrorNum e) -> bool
	{
		error = e;
		return false;
	};

	const auto success = [&error]()
	{
		error.clear();
		return true;
	};

	std::string user,passwd;
	if(!parseToken(token, user, passwd))
		return failure(RsJsonApiErrorNum::TOKEN_FORMAT_INVALID);

	auto it = mAuthTokenStorage.mAuthorizedTokens.find(user);
	if(it == mAuthTokenStorage.mAuthorizedTokens.end())
		return failure(RsJsonApiErrorNum::UNKNOWN_API_USER);

	// attempt avoiding +else CRYPTO_memcmp+ being optimized away
	int noOptimiz = 1;

	/* Do not use mAuthTokenStorage.mAuthorizedTokens.count(token), because
	 * std::string comparison is usually not constant time on content to be
	 * faster, so an attacker may use timings to guess authorized tokens */

	if( passwd.size() == it->second.size() &&
	        ( noOptimiz = CRYPTO_memcmp(
	              passwd.data(), it->second.data(), it->second.size() ) ) == 0 )
		return success();
	// Make token size guessing harder
	else noOptimiz = CRYPTO_memcmp(passwd.data(), passwd.data(), passwd.size());

	/* At this point we are sure password is wrong, and one could think to
	 * plainly `return false` still this ugly and apparently unuseful extra
	 * calculation is here to avoid `else CRYPTO_memcmp` being optimized away,
	 * so a pontential attacker cannot guess password size based  on timing */
	return static_cast<uint32_t>(noOptimiz) + 1 == 0 ?
	                success() : failure(RsJsonApiErrorNum::WRONG_API_PASSWORD);
}

std::map<std::string, std::string> JsonApiServer::getAuthorizedTokens()
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

void JsonApiServer::connectToConfigManager(p3ConfigMgr& cfgmgr)
{
	cfgmgr.addConfiguration("jsonapi.cfg",this);

	RsFileHash hash;
	loadConfiguration(hash);
}

std::error_condition JsonApiServer::authorizeUser(
        const std::string& user, const std::string& passwd )
{
	auto ec = badApiCredientalsFormat(user, passwd);
	if(ec) return ec;

	RS_STACK_MUTEX(configMutex);

	std::string& p(mAuthTokenStorage.mAuthorizedTokens[user]);
	if(p != passwd)
	{
		p = passwd;
		IndicateConfigChanged();
	}
	return ec;
}

/*static*/ std::string JsonApiServer::decodeToken(const std::string& radix64_token)
{
	std::vector<uint8_t> decodedVect(Radix64::decode(radix64_token));
	std::string decodedToken(
	            reinterpret_cast<const char*>(&decodedVect[0]),
	            decodedVect.size() );
	return decodedToken;
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

void JsonApiServer::registerResourceProvider(const JsonApiResourceProvider& rp)
{ mResourceProviders.insert(rp); }
void JsonApiServer::unregisterResourceProvider(const JsonApiResourceProvider& rp)
{ mResourceProviders.erase(rp); }
bool JsonApiServer::hasResourceProvider(const JsonApiResourceProvider& rp)
{ return mResourceProviders.find(rp) != mResourceProviders.end(); }

std::vector<std::shared_ptr<rb::Resource> > JsonApiServer::getResources() const
{
	auto tab = mResources;

	for(auto& rp: mResourceProviders)
		for(auto r: rp.get().getResources()) tab.push_back(r);

	return tab;
}

void JsonApiServer::restart()
{
	/* It is important to wrap into async(...) because fullstop() method can't
	 * be called from same thread of execution hence from JSON API thread! */
	RsThread::async([this]()
	{
		fullstop();
		RsThread::start("JSON API Server");
	});
}

void JsonApiServer::onStopRequested()
{
	RS_STACK_MUTEX(mServiceMutex);
	mService->stop();
}

uint16_t JsonApiServer::listeningPort() const { return mListeningPort; }
void JsonApiServer::setListeningPort(uint16_t p) { mListeningPort = p; }
void JsonApiServer::setBindingAddress(const std::string& bindAddress)
{ mBindingAddress = bindAddress; }
std::string JsonApiServer::getBindingAddress() const { return mBindingAddress; }

void JsonApiServer::run()
{
	auto settings = std::make_shared<restbed::Settings>();
	settings->set_port(mListeningPort);
	settings->set_bind_address(mBindingAddress);
	settings->set_default_header("Connection", "close");

	/* re-allocating mService is important because it deletes the existing
	 * service and therefore leaves the listening port open */
	{
		RS_STACK_MUTEX(mServiceMutex);
		mService = std::make_shared<restbed::Service>();
	}

	for(auto& r: getResources()) mService->publish(r);

	try
	{
		RsUrl apiUrl; apiUrl.setScheme("http").setHost(mBindingAddress)
		        .setPort(mListeningPort);
		RsInfo() << __PRETTY_FUNCTION__ << " JSON API server listening on "
		         << apiUrl.toString() << std::endl;
		mService->start(settings);
	}
	catch(std::exception& e)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Failure starting JSON API server: "
		        << e.what() << std::endl;
		print_stacktrace();
		return;
	}

	RsDbg() << __PRETTY_FUNCTION__ << " finished!" << std::endl;
}

/*static*/ void RsJsonApi::version(
        uint32_t& major, uint32_t& minor, uint32_t& mini, std::string& extra,
        std::string& human )
{
	major = RS_MAJOR_VERSION;
	minor = RS_MINOR_VERSION;
	mini = RS_MINI_VERSION;
	extra = RS_EXTRA_VERSION;
	human = RS_HUMAN_READABLE_VERSION;
}


//std::error_code make_error_code(RsJsonApiErrorCode e)
//{ return std::error_code(static_cast<int>(e), rsJsonApi->errorCategory()); }

std::error_condition make_error_condition(RsJsonApiErrorNum e)
{
return std::error_condition(static_cast<int>(e), rsJsonApi->errorCategory());
}


std::error_condition RsJsonApiErrorCategory::default_error_condition(int ev) const noexcept
{
	switch(static_cast<RsJsonApiErrorNum>(ev))
	{
	case RsJsonApiErrorNum::TOKEN_FORMAT_INVALID: // fallthrough
	case RsJsonApiErrorNum::UNKNOWN_API_USER: // fallthrough
	case RsJsonApiErrorNum::WRONG_API_PASSWORD: // fallthrough
	case RsJsonApiErrorNum::AUTHORIZATION_REQUEST_DENIED:
		return std::errc::permission_denied;
	case RsJsonApiErrorNum::API_USER_CONTAIN_COLON:
		return std::errc::invalid_argument;
	default:
		return std::error_condition(ev, *this);
	}
}

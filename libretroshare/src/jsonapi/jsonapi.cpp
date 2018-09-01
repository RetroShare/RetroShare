/*
 * RetroShare JSON API
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "jsonapi.h"

#include <sstream>
#include <memory>
#include <restbed>
#include <vector>

#include "util/rsjson.h"
#include "retroshare/rsfiles.h"
#include "util/radix64.h"

// Generated at compile time
#include "jsonapi-includes.inl"

static bool checkRsServicePtrReady(
        void* serviceInstance, const std::string& serviceName,
        RsGenericSerializer::SerializeContext& ctx,
        const std::shared_ptr<restbed::Session> session)
{
	if(serviceInstance) return true;

	std::string jsonApiError;
	jsonApiError += "Service: ";
	jsonApiError += serviceName;
	jsonApiError += " not initialized! Are you sure you logged in already?";

	RsGenericSerializer::SerializeJob j(RsGenericSerializer::TO_JSON);
	RS_SERIAL_PROCESS(jsonApiError);

	std::stringstream ss;
	ss << ctx.mJson;
	std::string&& ans(ss.str());
	const std::multimap<std::string, std::string> headers
	{
		{ "Content-Type", "text/json" },
		{ "Content-Length", std::to_string(ans.length()) }
	};
	session->close(rb::CONFLICT, ans, headers);
	return false;
}

JsonApiServer::JsonApiServer(
        uint16_t port, const std::function<void(int)> shutdownCallback) :
    mPort(port), mShutdownCallback(shutdownCallback)
{
	registerHandler("/jsonApiServer/shutdown",
	                [this](const std::shared_ptr<rb::Session>)
	{
		shutdown();
	});

	registerHandler("/rsFiles/getFileData",
	                [](const std::shared_ptr<rb::Session> session)
	{
		size_t reqSize = session->get_request()->get_header("Content-Length", 0);
		session->fetch( reqSize, [](
		                const std::shared_ptr<rb::Session> session,
		                const rb::Bytes& body )
		{
			RsGenericSerializer::SerializeContext cReq(
			            nullptr, 0,
			            RsGenericSerializer::SERIALIZATION_FLAG_YIELDING );
			RsJson& jReq(cReq.mJson);
			jReq.Parse(reinterpret_cast<const char*>(body.data()), body.size());

			RsGenericSerializer::SerializeContext cAns;
			RsJson& jAns(cAns.mJson);

			// if caller specified caller_data put it back in the answhere
			const char kcd[] = "caller_data";
			if(jReq.HasMember(kcd))
				jAns.AddMember(kcd, jReq[kcd], jAns.GetAllocator());

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

			// return them to the API caller
			std::stringstream ss;
			ss << jAns;
			std::string&& ans(ss.str());
			const std::multimap<std::string, std::string> headers
			{
				{ "Content-Type", "text/json" },
				{ "Content-Length", std::to_string(ans.length()) }
			};
			session->close(rb::OK, ans, headers);
		} );
	});

// Generated at compile time
#include "jsonapi-wrappers.inl"
}

void JsonApiServer::run()
{
	std::shared_ptr<rb::Settings> settings(new rb::Settings);
	settings->set_port(mPort);
//	settings->set_default_header("Connection", "close");
	settings->set_default_header("Cache-Control", "no-cache");
	mService.start(settings);
}

void JsonApiServer::registerHandler(
        const std::string& path,
        const std::function<void (const std::shared_ptr<restbed::Session>)>& handler)
{
	std::shared_ptr<restbed::Resource> resource(new rb::Resource);
	resource->set_path(path);
	resource->set_method_handler("GET", handler);
	resource->set_method_handler("POST", handler);
	mService.publish(resource);
}

void JsonApiServer::shutdown(int exitCode)
{
	mService.stop();
	mShutdownCallback(exitCode);
}

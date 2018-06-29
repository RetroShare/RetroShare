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

#include <rapid_json/document.h>
#include <rapid_json/writer.h>
#include <rapid_json/stringbuffer.h>

// Generated at compile time
#include "jsonapi-wrappers.h"

JsonApiServer::JsonApiServer(
        uint16_t port, const std::function<void(int)> shutdownCallback) :
    mPort(port), mShutdownCallback(shutdownCallback), notifyClientWrapper(*this)
{
	registerHandler("/jsonApiServer/shutdown",
	                [this](const std::shared_ptr<rb::Session>)
	{
		shutdown();
	});

	registerHandler("/jsonApiServer/notifications",
	        [this](const std::shared_ptr<rb::Session> session)
	{
		const auto headers = std::multimap<std::string, std::string>
		{
			{ "Connection", "keep-alive" },
			{ "Cache-Control", "no-cache" },
			{ "Content-Type", "text/event-stream" },
		};

		session->yield(rb::OK, headers,
		               [this](const std::shared_ptr<rb::Session> session)
		{
			notifySessions.push_back(session);
		} );
	} );

// Generated at compile time
#include "jsonapi-register.inl"
}

void JsonApiServer::run()
{
	std::shared_ptr<rb::Settings> settings(new rb::Settings);
	settings->set_port(mPort);
	settings->set_default_header("Connection", "close");
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

void JsonApiServer::cleanClosedNotifySessions()
{
	notifySessions.erase(
	            std::remove_if(
	                notifySessions.begin(), notifySessions.end(),
	                [](const std::shared_ptr<rb::Session> &s)
	{ return s->is_closed(); } ), notifySessions.end());
}

JsonApiServer::NotifyClientWrapper::NotifyClientWrapper(JsonApiServer& parent) :
    NotifyClient(), mJsonApiServer(parent)
{
	rsNotify->registerNotifyClient(static_cast<NotifyClient*>(this));
}

void JsonApiServer::NotifyClientWrapper::notifyTurtleSearchResult(
        uint32_t searchId, const std::list<TurtleFileInfo>& files )
{
	mJsonApiServer.cleanClosedNotifySessions();

	RsGenericSerializer::SerializeContext cAns;
	RsJson& jAns(cAns.mJson);

	// serialize parameters and method name to JSON
	{
		std::string methodName("NotifyClient/notifyTurtleSearchResult");
		std::list<TurtleFileInfo> filesCopy(files);
		RsGenericSerializer::SerializeContext& ctx(cAns);
		RsGenericSerializer::SerializeJob j(RsGenericSerializer::TO_JSON);
		RS_SERIAL_PROCESS(methodName);
		RS_SERIAL_PROCESS(searchId);
//		RS_SERIAL_PROCESS(filesCopy);
	}

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	jAns.Accept(writer);
	std::string message(buffer.GetString(), buffer.GetSize());
	message.insert(0, "data: "); message.append("\n\n");

	for(auto session : mJsonApiServer.notifySessions)
		session->yield(message);
}

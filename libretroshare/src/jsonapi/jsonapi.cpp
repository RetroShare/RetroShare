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

#include "util/rsjson.h"
#include "retroshare/rsgxschannels.h"

// Generated at compile time
#include "jsonapi-includes.inl"

JsonApiServer::JsonApiServer(
        uint16_t port, const std::function<void(int)> shutdownCallback) :
    mPort(port), mShutdownCallback(shutdownCallback)
{
	registerHandler("/jsonApiServer/shutdown",
	                [this](const std::shared_ptr<rb::Session>)
	{
		shutdown();
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

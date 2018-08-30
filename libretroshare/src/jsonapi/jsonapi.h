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

#include <string>
#include <memory>
#include <restbed>

#include "util/rsthreads.h"

namespace rb = restbed;


/**
 * Simple usage
 *  \code{.cpp}
 *    JsonApiServer jas(9092);
 *    jas.start("JsonApiServer");
 *  \endcode
 */
struct JsonApiServer : RsSingleJobThread
{
	JsonApiServer(
	        uint16_t port,
	        const std::function<void(int)> shutdownCallback = [](int){} );

	/// @see RsSingleJobThread
	virtual void run();

	/**
	 * @param[in] path Path itno which publish the API call
	 * @param[in] handler function which will be called to handle the requested
	 *   path, the function must be declared like:
	 *   \code{.cpp}
	 *     void functionName(const shared_ptr<restbed::Session> session)
	 *   \endcode
	 */
	void registerHandler(
	        const std::string& path,
	        const std::function<void(const std::shared_ptr<rb::Session>)>& handler );

	/**
	 * @brief Shutdown the JSON API server and call shutdownCallback
	 * @jsonapi{development}
	 * Beware that this method shout down only the JSON API server instance not
	 * the whole RetroShare instance, this behaviour can be altered via
	 * shutdownCallback paramether of @see JsonApiServer::JsonApiServer
	 * This method is made available also via JSON API with path
	 * /jsonApiServer/shutdown
	 * @param exitCode just passed down to the shutdownCallback
	 */
	void shutdown(int exitCode = 0);

private:
	uint16_t mPort;
	rb::Service mService;
	const std::function<void(int)> mShutdownCallback;
};


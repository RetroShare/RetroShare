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
#include <rapid_json/document.h>

#include "retroshare/rsgxschannels.h"
#include "serialiser/rstypeserializer.h"
#include "util/rsthreads.h"

namespace rb = restbed;

void apiVersionHandler(const std::shared_ptr<rb::Session> session);
void createChannelHandler(const std::shared_ptr<rb::Session> session);

/**
 * Simple usage
 *  \code{.cpp}
 *    JsonApiServer jas(9092);
 *    jas.start("JsonApiServer");
 *  \endcode
 */
struct JsonApiServer : RsSingleJobThread
{
	JsonApiServer(uint16_t port);

	/// @see RsSingleJobThread
	virtual void run();

	/**
	 * @param[in] handler function which will be called to handle the requested
	 *   path, the function must be declared like:
	 *   \code{.cpp}
	 *     void functionName(const shared_ptr<restbed::Session> session)
	 *   \endcode
	 */
	void registerHandler(
	        const std::string& path,
	        const std::function<void(const std::shared_ptr<rb::Session>)>& handler );

private:
	uint16_t mPort;
	rb::Service service;
};


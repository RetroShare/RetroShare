/*******************************************************************************
 * libresapi/api/ChannelsHandler.h                                             *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Gioacchino Mazzurco <gio@eigenlab.org>                    *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include "ResourceRouter.h"

class RsGxsChannels;

namespace resource_api
{

struct ChannelsHandler : ResourceRouter
{
	ChannelsHandler(RsGxsChannels& channels);

private:
	void handleListChannels(Request& req, Response& resp);
	void handleGetChannelInfo(Request& req, Response& resp);
	void handleGetChannelContent(Request& req, Response& resp);
	void handleToggleSubscription(Request& req, Response& resp);
	void handleCreateChannel(Request& req, Response& resp);
	void handleToggleAutoDownload(Request& req, Response& resp);
	void handleTogglePostRead(Request& req, Response& resp);
	void handleCreatePost(Request& req, Response& resp);

	RsGxsChannels& mChannels;
};

} // namespace resource_api

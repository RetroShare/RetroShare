/*******************************************************************************
 * libresapi/api/LivereloadHarder.h                                            *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
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
#include "StateTokenServer.h"

namespace resource_api
{

// very simple livereload system, integrated into the existing state token system
// the response to / is only a statetoken
// if /trigger is called, then the state token is invalidaten and replaced wiht a new one
class LivereloadHandler: public ResourceRouter
{
public:
    LivereloadHandler(StateTokenServer* sts);

private:
    void handleWildcard(Request& req, Response& resp);
    void handleTrigger(Request& req, Response& resp);
    StateTokenServer* mStateTokenServer;
    StateToken mStateToken;
};
} // namespace resource_api

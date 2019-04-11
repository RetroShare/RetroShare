/*******************************************************************************
 * libresapi/api/LivereloadHarder.cpp                                          *
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
#include "LivereloadHandler.h"

namespace resource_api
{
LivereloadHandler::LivereloadHandler(StateTokenServer *sts):
    mStateTokenServer(sts), mStateToken(sts->getNewToken())
{
    addResourceHandler("*", this, &LivereloadHandler::handleWildcard);
    addResourceHandler("trigger", this, &LivereloadHandler::handleTrigger);
}

void LivereloadHandler::handleWildcard(Request &/*req*/, Response &resp)
{
    resp.mStateToken = mStateToken;
    resp.setOk();
}

void LivereloadHandler::handleTrigger(Request &/*req*/, Response &resp)
{
    mStateTokenServer->replaceToken(mStateToken);
    resp.mStateToken = mStateToken;
    resp.setOk();
}

} // namespace resource_api

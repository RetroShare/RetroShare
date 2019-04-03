/*******************************************************************************
 * libresapi/api/IdentityHandler.h                                             *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2015  electron128 <electron128@yahoo.com>                     *
 * Copyright (C) 2017  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include <retroshare/rsnotify.h>
#include <util/rsthreads.h>

#include "ResourceRouter.h"
#include "StateTokenServer.h"

struct RsIdentity;

namespace resource_api
{

class IdentityHandler: public ResourceRouter, NotifyClient
{
public:
    IdentityHandler(StateTokenServer* sts, RsNotify* notify, RsIdentity* identity);
    virtual ~IdentityHandler();

    // from NotifyClient
    // note: this may get called from foreign threads
    virtual void notifyGxsChange(const RsGxsChanges &changes);

private:
	void handleWildcard(Request& req, Response& resp);
	void handleNotOwnIdsRequest(Request& req, Response& resp);
	void handleOwnIdsRequest(Request& req, Response& resp);

	void handleExportKey(Request& req, Response& resp);
	void handleImportKey(Request& req, Response& resp);

	void handleAddContact(Request& req, Response& resp);
	void handleRemoveContact(Request& req, Response& resp);

	void handleGetIdentityDetails(Request& req, Response& resp);

	void handleGetAvatar(Request& req, Response& resp);
	void handleSetAvatar(Request& req, Response& resp);

	void handleSetBanNode(Request& req, Response& resp);
	void handleSetOpinion(Request& req, Response& resp);

    ResponseTask *handleOwn(Request& req, Response& resp);
    ResponseTask *handleCreateIdentity(Request& req, Response& resp);
	ResponseTask *handleDeleteIdentity(Request& req, Response& resp);

    StateTokenServer* mStateTokenServer;
    RsNotify* mNotify;
    RsIdentity* mRsIdentity;

    RsMutex mMtx;
    StateToken mStateToken; // mutex protected
};
} // namespace resource_api

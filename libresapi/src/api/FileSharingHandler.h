/*
 * libresapi
 *
 * Copyright (C) 2017, Konrad Dębiec <konradd@tutanota.com>
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
#pragma once

#include "ResourceRouter.h"
#include "StateTokenServer.h"

#include <retroshare/rsfiles.h>

namespace resource_api
{

class FileSharingHandler: public ResourceRouter
{
public:
	FileSharingHandler(StateTokenServer* sts, RsFiles* files);

private:
	void handleWildcard(Request& req, Response& resp);
	void handleForceCheck(Request& req, Response& resp);

	void handleGetSharedDir(Request& req, Response& resp);
	void handleSetSharedDir(Request& req, Response& resp);
	void handleUpdateSharedDir(Request& req, Response& resp);
	void handleRemoveSharedDir(Request& req, Response& resp);

	void handleGetDirectoryParent(Request& req, Response& resp);
	void handleGetDirectoryChilds(Request& req, Response& resp);

	void handleIsDownloadDirShared(Request& req, Response& resp);
	void handleShareDownloadDirectory(Request& req, Response& resp);

	void handleDownload(Request& req, Response& resp);

	StateToken mStateToken;
	StateTokenServer* mStateTokenServer;

	RsFiles* mRsFiles;
};

} // namespace resource_api

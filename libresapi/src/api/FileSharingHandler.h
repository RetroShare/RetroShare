/*******************************************************************************
 * libresapi/api/FileSharingHandler.cpp                                        *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2017, Konrad Dębiec <konradd@tutanota.com>                    *
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

#include <retroshare/rsfiles.h>
#include <retroshare/rsnotify.h>
#include <util/rsthreads.h>

namespace resource_api
{

class FileSharingHandler: public ResourceRouter, NotifyClient
{
public:
	FileSharingHandler(StateTokenServer* sts, RsFiles* files, RsNotify& notify);
	~FileSharingHandler();

	/**
	 Derived from NotifyClient
	 This function may be called from foreign thread
	*/
	virtual void notifyListChange(int list, int type);

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

	void handleGetDownloadDirectory(Request& req, Response& resp);
	void handleSetDownloadDirectory(Request& req, Response& resp);

	void handleGetPartialsDirectory(Request& req, Response& resp);
	void handleSetPartialsDirectory(Request& req, Response& resp);

	/// Token indicating change in local shared files
	StateToken mLocalDirStateToken;

	/// Token indicating change in remote (friends') shared files
	StateToken mRemoteDirStateToken;

	StateTokenServer* mStateTokenServer;

	/**
	 Protects mLocalDirStateToken and mRemoteDirStateToken that may be changed in foreign thread
	 @see FileSharingHandler::notifyListChange(...)
	*/
	RsMutex mMtx;

	RsFiles* mRsFiles;
	RsNotify& mNotify;
};

} // namespace resource_api

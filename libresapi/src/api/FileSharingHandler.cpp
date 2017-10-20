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
#include "FileSharingHandler.h"

namespace resource_api
{

FileSharingHandler::FileSharingHandler(StateTokenServer *sts, RsFiles *files):
    mStateTokenServer(sts), mRsFiles(files)
{
	addResourceHandler("*", this, &FileSharingHandler::handleWildcard);
	addResourceHandler("force_check", this, &FileSharingHandler::handleForceCheck);

	addResourceHandler("get_shared_dirs", this, &FileSharingHandler::handleGetSharedDir);
	addResourceHandler("set_shared_dir", this, &FileSharingHandler::handleSetSharedDir);
	addResourceHandler("update_shared_dir", this, &FileSharingHandler::handleUpdateSharedDir);
	addResourceHandler("remove_shared_dir", this, &FileSharingHandler::handleRemoveSharedDir);

	addResourceHandler("get_dir_parent", this, &FileSharingHandler::handleGetDirectoryParent);
	addResourceHandler("get_dir_childs", this, &FileSharingHandler::handleGetDirectoryChilds);

	addResourceHandler("is_dl_dir_shared", this, &FileSharingHandler::handleIsDownloadDirShared);
	addResourceHandler("share_dl_dir", this, &FileSharingHandler::handleShareDownloadDirectory);

	addResourceHandler("download", this, &FileSharingHandler::handleDownload);

	mStateToken = mStateTokenServer->getNewToken();
}

void FileSharingHandler::handleWildcard(Request & /*req*/, Response & /*resp*/)
{

}

void FileSharingHandler::handleForceCheck(Request&, Response& resp)
{
	mRsFiles->ForceDirectoryCheck();
	resp.setOk();
}

void FileSharingHandler::handleGetSharedDir(Request& req, Response& resp)
{
	DirDetails dirDetails;
	mRsFiles->RequestDirDetails(NULL, dirDetails, RS_FILE_HINTS_LOCAL);

	resp.mDataStream << makeKeyValue("parent_reference", *reinterpret_cast<int*>(&dirDetails.ref));
	resp.mDataStream << makeKeyValue("path", dirDetails.path);
	StreamBase &childsStream = resp.mDataStream.getStreamToMember("childs");

	for(std::vector<DirStub>::iterator it = dirDetails.children.begin(); it != dirDetails.children.end(); ++it)
	{
		DirDetails dirDetails;
		mRsFiles->RequestDirDetails((*it).ref, dirDetails, RS_FILE_HINTS_LOCAL);

		for(std::vector<DirStub>::iterator vit = dirDetails.children.begin(); vit != dirDetails.children.end(); ++vit)
		{
			DirDetails dirDetails;
			mRsFiles->RequestDirDetails((*vit).ref, dirDetails, RS_FILE_HINTS_LOCAL);

			std::string type;
			switch(dirDetails.type)
			{
			case DIR_TYPE_PERSON:
				type = "person";
				break;
			case DIR_TYPE_DIR:
				type = "folder";
				break;
			case DIR_TYPE_FILE:
				type = "file";
				break;
			}

			bool browsable = false;
			bool anon_dl = false;
			bool anon_search = false;

			if(dirDetails.flags & DIR_FLAGS_BROWSABLE)
				browsable = true;

			if(dirDetails.flags & DIR_FLAGS_ANONYMOUS_DOWNLOAD)
				anon_dl = true;

			if(dirDetails.flags & DIR_FLAGS_ANONYMOUS_SEARCH)
				anon_search = true;

			StreamBase &streamBase = childsStream.getStreamToMember()
			        << makeKeyValue("name", dirDetails.name)
			        << makeKeyValue("path", dirDetails.path)
			        << makeKeyValue("hash", dirDetails.hash.toStdString())
			        << makeKeyValue("parent_reference", *reinterpret_cast<int*>(&dirDetails.parent))
			        << makeKeyValue("reference", *reinterpret_cast<int*>(&dirDetails.ref))
			        << makeKeyValue("count", static_cast<int>(dirDetails.count))
			        << makeKeyValueReference("type", type)
			        << makeKeyValueReference("browsable", browsable)
			        << makeKeyValueReference("anon_dl", anon_dl)
			        << makeKeyValueReference("anon_search", anon_search);


			int contain_files = 0;
			int contain_folders = 0;

			if(dirDetails.count != 0)
			{
				for(std::vector<DirStub>::iterator vit = dirDetails.children.begin(); vit != dirDetails.children.end(); ++vit)
				{
					DirDetails dirDetails;
					mRsFiles->RequestDirDetails((*vit).ref, dirDetails, RS_FILE_HINTS_LOCAL);

					switch(dirDetails.type)
					{
					case DIR_TYPE_DIR:
						contain_folders++;
						break;
					case DIR_TYPE_FILE:
						contain_files++;
						break;
					}
				}
			}

			streamBase
			        << makeKeyValueReference("contain_files", contain_files)
			        << makeKeyValueReference("contain_folders", contain_folders);
		}
	}
}

void FileSharingHandler::handleSetSharedDir(Request& req, Response& resp)
{
	std::string dir;
	bool browsable = false;
	bool anon_dl = false;
	bool anon_search = false;
	req.mStream << makeKeyValueReference("directory", dir);
	req.mStream << makeKeyValueReference("browsable", browsable);
	req.mStream << makeKeyValueReference("anon_dl", anon_dl);
	req.mStream << makeKeyValueReference("anon_search", anon_search);

	SharedDirInfo sDI;
	sDI.filename = dir;
	sDI.virtualname.clear();
	sDI.shareflags.clear();

	if(browsable)
		sDI.shareflags |= DIR_FLAGS_BROWSABLE;

	if(anon_dl)
		sDI.shareflags |= DIR_FLAGS_ANONYMOUS_DOWNLOAD;

	if(anon_search)
		sDI.shareflags |= DIR_FLAGS_ANONYMOUS_SEARCH;

	if(mRsFiles->addSharedDirectory(sDI))
		resp.setOk();
	else
		resp.setFail("Couldn't share folder");
}

void FileSharingHandler::handleUpdateSharedDir(Request& req, Response& resp)
{
	std::string dir;
	std::string virtualname;
	bool browsable = false;
	bool anon_dl = false;
	bool anon_search = false;
	req.mStream << makeKeyValueReference("directory", dir);
	req.mStream << makeKeyValueReference("virtualname", virtualname);
	req.mStream << makeKeyValueReference("browsable", browsable);
	req.mStream << makeKeyValueReference("anon_dl", anon_dl);
	req.mStream << makeKeyValueReference("anon_search", anon_search);

	SharedDirInfo sDI;
	sDI.filename = dir;
	sDI.virtualname = virtualname;
	sDI.shareflags.clear();

	if(browsable)
		sDI.shareflags |= DIR_FLAGS_BROWSABLE;

	if(anon_dl)
		sDI.shareflags |= DIR_FLAGS_ANONYMOUS_DOWNLOAD;

	if(anon_search)
		sDI.shareflags |= DIR_FLAGS_ANONYMOUS_SEARCH;

	if(mRsFiles->updateShareFlags(sDI))
		resp.setOk();
	else
		resp.setFail("Couldn't update shared folder's flags");
}

void FileSharingHandler::handleRemoveSharedDir(Request& req, Response& resp)
{
	std::string dir;
	req.mStream << makeKeyValueReference("directory", dir);

	if(mRsFiles->removeSharedDirectory(dir))
		resp.setOk();
	else
		resp.setFail("Couldn't remove shared directory.");
}

void FileSharingHandler::handleGetDirectoryParent(Request& req, Response& resp)
{
	int reference;
	bool remote = false;
	bool local = true;

	req.mStream << makeKeyValueReference("reference", reference);
	req.mStream << makeKeyValueReference("remote", remote);
	req.mStream << makeKeyValueReference("local", local);

	void *ref = reinterpret_cast<void*>(reference);

	FileSearchFlags flags;
	if(remote)
		flags |= RS_FILE_HINTS_REMOTE;

	if(local)
		flags |= RS_FILE_HINTS_LOCAL;

	DirDetails dirDetails;
	mRsFiles->RequestDirDetails(ref, dirDetails, flags);
	mRsFiles->RequestDirDetails(dirDetails.parent, dirDetails, flags);

	resp.mDataStream << makeKeyValue("parent_reference", *reinterpret_cast<int*>(&dirDetails.ref));
	resp.mDataStream << makeKeyValue("path", dirDetails.path);
	StreamBase &childsStream = resp.mDataStream.getStreamToMember("childs");

	if(dirDetails.count != 0)
	{
		for(std::vector<DirStub>::iterator vit = dirDetails.children.begin(); vit != dirDetails.children.end(); ++vit)
		{
			DirDetails dirDetails;
			mRsFiles->RequestDirDetails((*vit).ref, dirDetails, flags);

			std::string type;
			switch(dirDetails.type)
			{
			case DIR_TYPE_PERSON:
				type = "person";
				break;
			case DIR_TYPE_DIR:
				type = "folder";
				break;
			case DIR_TYPE_FILE:
				type = "file";
				break;
			}

			bool browsable = false;
			bool anon_dl = false;
			bool anon_search = false;

			if(dirDetails.flags & DIR_FLAGS_BROWSABLE)
				browsable = true;

			if(dirDetails.flags & DIR_FLAGS_ANONYMOUS_DOWNLOAD)
				anon_dl = true;

			if(dirDetails.flags & DIR_FLAGS_ANONYMOUS_SEARCH)
				anon_search = true;

			StreamBase &streamBase = childsStream.getStreamToMember()
			        << makeKeyValue("name", dirDetails.name)
			        << makeKeyValue("path", dirDetails.path)
			        << makeKeyValue("hash", dirDetails.hash.toStdString())
			        << makeKeyValue("parent_reference", *reinterpret_cast<int*>(&dirDetails.parent))
			        << makeKeyValue("reference", *reinterpret_cast<int*>(&dirDetails.ref))
			        << makeKeyValue("count", static_cast<int>(dirDetails.count))
			        << makeKeyValueReference("type", type)
			        << makeKeyValueReference("browsable", browsable)
			        << makeKeyValueReference("anon_dl", anon_dl)
			        << makeKeyValueReference("anon_search", anon_search);


			int contain_files = 0;
			int contain_folders = 0;

			if(dirDetails.count != 0)
			{
				for(std::vector<DirStub>::iterator vit = dirDetails.children.begin(); vit != dirDetails.children.end(); ++vit)
				{
					DirDetails dirDetails;
					mRsFiles->RequestDirDetails((*vit).ref, dirDetails, flags);

					switch(dirDetails.type)
					{
					case DIR_TYPE_DIR:
						contain_folders++;
						break;
					case DIR_TYPE_FILE:
						contain_files++;
						break;
					}
				}
			}

			streamBase
			        << makeKeyValueReference("contain_files", contain_files)
			        << makeKeyValueReference("contain_folders", contain_folders);
		}
	}
}

void FileSharingHandler::handleGetDirectoryChilds(Request& req, Response& resp)
{
	int reference = 0;
	bool remote = false;
	bool local = true;

	req.mStream << makeKeyValueReference("reference", reference);
	req.mStream << makeKeyValueReference("remote", remote);
	req.mStream << makeKeyValueReference("local", local);

	void *ref = reinterpret_cast<void*>(reference);

	FileSearchFlags flags;
	if(remote)
		flags |= RS_FILE_HINTS_REMOTE;

	if(local)
		flags |= RS_FILE_HINTS_LOCAL;

	DirDetails dirDetails;
	mRsFiles->RequestDirDetails(ref, dirDetails, flags);

	resp.mDataStream << makeKeyValue("parent_reference", *reinterpret_cast<int*>(&dirDetails.ref));
	resp.mDataStream << makeKeyValue("path", dirDetails.path);
	resp.mDataStream << makeKeyValue("hash", dirDetails.hash.toStdString());
	StreamBase &childsStream = resp.mDataStream.getStreamToMember("childs");

	if(dirDetails.count != 0)
	{
		for(std::vector<DirStub>::iterator vit = dirDetails.children.begin(); vit != dirDetails.children.end(); ++vit)
		{
			DirDetails dirDetails;
			mRsFiles->RequestDirDetails((*vit).ref, dirDetails, flags);

			std::string type;
			switch(dirDetails.type)
			{
			case DIR_TYPE_PERSON:
				type = "person";
				break;
			case DIR_TYPE_DIR:
				type = "folder";
				break;
			case DIR_TYPE_FILE:
				type = "file";
				break;
			}

			bool browsable = false;
			bool anon_dl = false;
			bool anon_search = false;

			if(dirDetails.flags & DIR_FLAGS_BROWSABLE)
				browsable = true;

			if(dirDetails.flags & DIR_FLAGS_ANONYMOUS_DOWNLOAD)
				anon_dl = true;

			if(dirDetails.flags & DIR_FLAGS_ANONYMOUS_SEARCH)
				anon_search = true;

			StreamBase &streamBase = childsStream.getStreamToMember()
			        << makeKeyValue("name", dirDetails.name)
			        << makeKeyValue("path", dirDetails.path)
			        << makeKeyValue("hash", dirDetails.hash.toStdString())
			        << makeKeyValue("parent_reference", *reinterpret_cast<int*>(&dirDetails.parent))
			        << makeKeyValue("reference", *reinterpret_cast<int*>(&dirDetails.ref))
			        << makeKeyValue("count", static_cast<int>(dirDetails.count))
			        << makeKeyValueReference("type", type)
			        << makeKeyValueReference("browsable", browsable)
			        << makeKeyValueReference("anon_dl", anon_dl)
			        << makeKeyValueReference("anon_search", anon_search);


			int contain_files = 0;
			int contain_folders = 0;

			if(dirDetails.count != 0)
			{
				for(std::vector<DirStub>::iterator vit = dirDetails.children.begin(); vit != dirDetails.children.end(); ++vit)
				{
					DirDetails dirDetails;
					mRsFiles->RequestDirDetails((*vit).ref, dirDetails, flags);

					switch(dirDetails.type)
					{
					case DIR_TYPE_DIR:
						contain_folders++;
						break;
					case DIR_TYPE_FILE:
						contain_files++;
						break;
					}
				}
			}

			streamBase
			        << makeKeyValueReference("contain_files", contain_files)
			        << makeKeyValueReference("contain_folders", contain_folders);
		}
	}
}

void FileSharingHandler::handleIsDownloadDirShared(Request&, Response& resp)
{
	bool shared = mRsFiles->getShareDownloadDirectory();

	resp.mDataStream.getStreamToMember()
	    << makeKeyValueReference("shared", shared);

	resp.setOk();
}

void FileSharingHandler::handleShareDownloadDirectory(Request& req, Response& resp)
{
	bool share;
	req.mStream << makeKeyValueReference("share", share);

	if(mRsFiles->shareDownloadDirectory(share))
		resp.setOk();
	else
		resp.setFail("Couldn't share/unshare download directory.");
}

void FileSharingHandler::handleDownload(Request& req, Response& resp)
{
	int size;
	std::string hashString;
	std::string name;
	req.mStream << makeKeyValueReference("hash", hashString);
	req.mStream << makeKeyValueReference("name", name);
	req.mStream << makeKeyValueReference("size", size);

	std::list<RsPeerId> srcIds;
	RsFileHash hash(hashString);
	FileInfo finfo;
	mRsFiles->FileDetails(hash, RS_FILE_HINTS_REMOTE, finfo);

	for(std::list<TransferInfo>::const_iterator it(finfo.peers.begin());it!=finfo.peers.end();++it)
		srcIds.push_back((*it).peerId);

	if(!mRsFiles->FileRequest(name, hash, static_cast<uint64_t>(size), "",
	                          RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds))
	{
		resp.setOk();
		return;
	}

	resp.setFail("Couldn't download file");
}

} // namespace resource_api

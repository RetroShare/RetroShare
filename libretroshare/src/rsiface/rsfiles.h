#ifndef RS_FILES_GUI_INTERFACE_H
#define RS_FILES_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsfiles.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */


#include <list>
#include <iostream>
#include <string>

#include "rsiface/rstypes.h"

std::ostream &operator<<(std::ostream &out, const MessageInfo &info);
std::ostream &operator<<(std::ostream &out, const ChatInfo &info);

class RsFiles;
extern RsFiles  *rsFiles;


const uint32_t RS_FILE_CTRL_PAUSE	 = 0x0100;
const uint32_t RS_FILE_CTRL_START	 = 0x0200;

const uint32_t RS_FILE_CTRL_TRICKLE	 = 0x0001;
const uint32_t RS_FILE_CTRL_SLOW	 = 0x0002;
const uint32_t RS_FILE_CTRL_STANDARD	 = 0x0003;
const uint32_t RS_FILE_CTRL_FAST	 = 0x0004;
const uint32_t RS_FILE_CTRL_STREAM_AUDIO = 0x0005;
const uint32_t RS_FILE_CTRL_STREAM_VIDEO = 0x0006;


const uint32_t RS_FILE_EXTRA_DELETE	 = 0x0010;


class RsFiles
{
	public:

	RsFiles() { return; }
virtual ~RsFiles() { return; }

/****************************************/
	/* download */

/* Required Interfaces ......
 *
 * 1) Access to downloading / uploading files.
 */

/* get Details of File Transfers */
virtual bool FileDownloads(std::list<std::string> &hashs)= 0;
virtual bool FileUploads(std::list<std::string> &hashs)= 0;
virtual bool FileDetails(std::string hash, uint32_t hintflags, FileInfo &info)= 0;


/*
 * 2) Control of Downloads.
 *
 */


virtual int FileRequest(std::string fname, std::string hash, 
		uint32_t size, std::string dest, uint32_t flags)= 0;
virtual int FileCancel(std::string hash)= 0;
virtual int FileControl(std::string hash, uint32_t flags)= 0;
virtual int FileClearCompleted()= 0;


/*
 * 3) Addition of Extra Files... From File System
 * These are Hashed and stored in the 'Hidden Files' section
 * which can only be accessed if you know the hash.
 *
 * FileHash is called to start the hashing process, 
 * and add the file to the HiddenStore.
 *
 * FileHashStatus is called to lookup files 
 * and see if the hashing is completed.
 */

/* Access ftExtraList - Details */
virtual int  ExtraFileAdd(std::string fname, std::string hash, uint32_t size,
				uint32_t period, uint32_t flags)= 0;
virtual int  ExtraFileRemove(std::string hash, uin32_t flags)= 0;
virtual bool ExtraFileHash(std::string localpath, 
				uint32_t period, uint32_t flags)= 0;
virtual bool ExtraFileStatus(std::string localpath, FileInfo &info)= 0;


/*
 * 4) Search and Listing Interface 
 */

/* Directory Listing / Search Interface */
virtual int RequestDirDetails(std::string uid, std::string path, DirDetails &details)= 0;
virtual int RequestDirDetails(void *ref, DirDetails &details, uint32_t flags)= 0;

virtual int SearchKeywords(std::list<std::string> keywords, std::list<FileDetail> &results)= 0;
virtual int SearchBoolExp(Expression * exp, std::list<FileDetail> &results)= 0;

/*
 * 5) Utility Functions.
 */

virtual bool ConvertSharedFilePath(std::string path, std::string &fullpath) = 0;
virtual void ForceDirectoryCheck() = 0;
virtual bool InDirectoryCheck() = 0;


};


#endif

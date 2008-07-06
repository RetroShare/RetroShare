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

class RsFiles;
extern RsFiles  *rsFiles;

class Expression;

const uint32_t RS_FILE_CTRL_PAUSE	 = 0x0100;
const uint32_t RS_FILE_CTRL_START	 = 0x0200;

const uint32_t RS_FILE_CTRL_TRICKLE	 = 0x0001;
const uint32_t RS_FILE_CTRL_SLOW	 = 0x0002;
const uint32_t RS_FILE_CTRL_STANDARD	 = 0x0003;
const uint32_t RS_FILE_CTRL_FAST	 = 0x0004;
const uint32_t RS_FILE_CTRL_STREAM_AUDIO = 0x0005;
const uint32_t RS_FILE_CTRL_STREAM_VIDEO = 0x0006;


/************************************
 * Used To indicate where to search.
 */

const uint32_t RS_FILE_HINTS_MASK	 = 0x00ff;

const uint32_t RS_FILE_HINTS_CACHE	 = 0x0001;
const uint32_t RS_FILE_HINTS_EXTRA	 = 0x0002;
const uint32_t RS_FILE_HINTS_LOCAL	 = 0x0004;
const uint32_t RS_FILE_HINTS_REMOTE	 = 0x0008;
const uint32_t RS_FILE_HINTS_DOWNLOAD	 = 0x0010;
const uint32_t RS_FILE_HINTS_UPLOAD	 = 0x0020;

const uint32_t RS_FILE_HINTS_SPEC_ONLY	 = 0x1000;



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
virtual bool FileDownloads(std::list<std::string> &hashs) = 0;
virtual bool FileUploads(std::list<std::string> &hashs) = 0;
virtual bool FileDetails(std::string hash, uint32_t hintflags, FileInfo &info) = 0;


/*
 * 2) Control of Downloads.
 *
 */


virtual bool FileRequest(std::string fname, std::string hash, 
		uint32_t size, std::string dest, uint32_t flags) = 0;
virtual bool FileCancel(std::string hash) = 0;
virtual bool FileControl(std::string hash, uint32_t flags) = 0;
virtual bool FileClearCompleted() = 0;


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
virtual bool ExtraFileAdd(std::string fname, std::string hash, uint32_t size,
				uint32_t period, uint32_t flags) = 0;
virtual bool ExtraFileRemove(std::string hash, uint32_t flags) = 0;
virtual bool ExtraFileHash(std::string localpath, 
				uint32_t period, uint32_t flags) = 0;
virtual bool ExtraFileStatus(std::string localpath, FileInfo &info) = 0;


/*
 * 4) Search and Listing Interface 
 */

/* Directory Listing / Search Interface */
virtual int RequestDirDetails(std::string uid, std::string path, DirDetails &details) = 0;
virtual int RequestDirDetails(void *ref, DirDetails &details, uint32_t flags) = 0;

virtual int SearchKeywords(std::list<std::string> keywords, std::list<FileDetail> &results) = 0;
virtual int SearchBoolExp(Expression * exp, std::list<FileDetail> &results) = 0;

/*
 * 5) Directory Control / Shared Files Utility Functions.
 */

virtual bool ConvertSharedFilePath(std::string path, std::string &fullpath) = 0;
virtual void ForceDirectoryCheck() = 0;
virtual bool InDirectoryCheck() = 0;

virtual void    setDownloadDirectory(std::string path) = 0;
virtual void    setPartialsDirectory(std::string path) = 0;
virtual std::string getDownloadDirectory() = 0;
virtual std::string getPartialsDirectory() = 0;

virtual bool    getSharedDirectories(std::list<std::string> &dirs) = 0;
virtual bool    addSharedDirectory(std::string dir) = 0;
virtual bool    removeSharedDirectory(std::string dir) = 0;


};


#endif

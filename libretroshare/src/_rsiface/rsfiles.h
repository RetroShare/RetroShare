#ifndef RSFILES_H
#define RSFILES_H

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

#include "rstypes.h"
#include "rsdefs.h"

class Expression;

class RsFiles
{
public:

//      RsFiles is pure virtual. It doesn't need this:
//	RsFiles() { return; }
//      virtual ~RsFiles() { return; }
    RsFiles();
    virtual ~RsFiles();

/****************************************/
	/* download */


    /***
     *  Control of Downloads.
     ***/
    virtual bool FileRequest(std::string fname, std::string hash, uint64_t size,
                             std::string dest, uint32_t flags, std::list<std::string> srcIds) = 0;
    virtual bool FileCancel(std::string hash) = 0;
    virtual bool FileControl(std::string hash, uint32_t flags) = 0;
    virtual bool FileClearCompleted() = 0;

    /***
     * Download / Upload Details.
     ***/
    virtual bool FileDownloads(std::list<std::string> &hashs) = 0;
    virtual bool FileUploads(std::list<std::string> &hashs) = 0;
    virtual bool FileDetails(std::string hash, uint32_t hintflags, FileInfo &info) = 0;


    /***
     * Extra List Access
     ***/
    virtual bool ExtraFileAdd(std::string fname, std::string hash, uint64_t size,
                              uint32_t period, uint32_t flags) = 0;
    virtual bool ExtraFileRemove(std::string hash, uint32_t flags) = 0;
    virtual bool ExtraFileHash(std::string localpath,
                               uint32_t period, uint32_t flags) = 0;
    virtual bool ExtraFileStatus(std::string localpath, FileInfo &info) = 0;
    virtual bool ExtraFileMove(std::string fname, std::string hash, uint64_t size,
                               std::string destpath) = 0;

    /***
     * Directory Listing / Search Interface
     */
    virtual int RequestDirDetails(std::string uid, std::string path, DirDetails &details) = 0;
    virtual int RequestDirDetails(void *ref, DirDetails &details, uint32_t flags) = 0;

    virtual int SearchKeywords(std::list<std::string> keywords, std::list<FileDetail> &results,uint32_t flags) = 0;
    virtual int SearchBoolExp(Expression * exp, std::list<FileDetail> &results) = 0;

    /***
     * Utility Functions.
     ***/
    virtual bool ConvertSharedFilePath(std::string path, std::string &fullpath) = 0;
    virtual void ForceDirectoryCheck() = 0;
    virtual bool InDirectoryCheck() = 0;

    /***
     * Directory Control
     ***/
    virtual void    setDownloadDirectory(std::string path) = 0;
    virtual void    setPartialsDirectory(std::string path) = 0;
    virtual std::string getDownloadDirectory() = 0;
    virtual std::string getPartialsDirectory() = 0;

    virtual bool    getSharedDirectories(std::list<std::string> &dirs) = 0;
    virtual bool    addSharedDirectory(std::string dir) = 0;
    virtual bool    removeSharedDirectory(std::string dir) = 0;

};


// Extern
extern RsFiles  *rsFiles;

#endif

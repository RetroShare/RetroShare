/*
 * libretroshare/src/ft: ftserver.h
 *
 * File Transfer for RetroShare.
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

#ifndef FT_SERVER_HEADER
#define FT_SERVER_HEADER

/* 
 * ftServer.
 *
 * Top level File Transfer interface.
 * (replaces old filedexserver)
 *
 * sets up the whole File Transfer class structure.
 * sets up the File Indexing side of cache system too.
 *
 * provides rsFiles interface for external control.
 *
 */

#include <map>
#include <list>
#include <iostream>

#include "rsiface/rsfiles.h"
#include "dbase/cachestrapper.h"

#include "pqi/p3cfgmgr.h"


class p3ConnectMgr;
class p3AuthMgr;

class CacheStrapper;
class FileIndexStore;
class FileIndexMonitor;

class ftController;
class ftExtraList;

class ftServer: public RsFiles
{

	public:

	/***************************************************************/
	/******************** Setup ************************************/
	/***************************************************************/

	ftServer(CacheStrapper *cStrapper) { return; }

	/* Assign important variables */
void	setConfigDirectory(std::string path);

void	setPQInterface(PQInterface *pqi);

	/* Final Setup (once everything is assigned) */
void	SetupFtServer();

	/* add Config Items (Extra, Controller) */
void	addConfigComponents(p3ConfigMgr *mgr);

CacheTransfer *getCacheTransfer();

	/***************************************************************/
	/*************** Control Interface *****************************/
	/************** (Implements RsFiles) ***************************/
	/***************************************************************/

virtual bool FileRequest(std::string fname, std::string hash, 
				uint32_t size, std::string dest, uint32_t flags);
virtual bool FileCancel(std::string hash);
virtual bool FileControl(std::string hash, uint32_t flags);
virtual bool FileClearCompleted();

	/* get Details of File Transfers */
virtual bool FileDownloads(std::list<std::string> &hashs);
virtual bool FileUploads(std::list<std::string> &hashs);
virtual bool FileDetails(std::string hash, uint32_t hintflags, FileInfo &info);

	/* Access ftExtraList - Details */
virtual bool ExtraFileAdd(std::string fname, std::string hash, uint32_t size, 
						uint32_t period, uint32_t flags);
virtual bool ExtraFileRemove(std::string hash, uint32_t flags);
virtual bool ExtraFileHash(std::string localpath, uint32_t period, uint32_t flags);
virtual bool ExtraFileStatus(std::string localpath, FileInfo &info);

	/* Directory Listing */
virtual int RequestDirDetails(std::string uid, std::string path, DirDetails &details);
virtual int RequestDirDetails(void *ref, DirDetails &details, uint32_t flags);

	/* Search Interface */
virtual int SearchKeywords(std::list<std::string> keywords, std::list<FileDetail> &results);
virtual int SearchBoolExp(Expression * exp, std::list<FileDetail> &results);

	/* Utility Functions */
virtual bool ConvertSharedFilePath(std::string path, std::string &fullpath);
virtual void ForceDirectoryCheck();
virtual bool InDirectoryCheck();

	/* Directory Handling */
virtual void	setDownloadDirectory(std::string path);
virtual void	setPartialsDirectory(std::string path);
virtual std::string getDownloadDirectory();
virtual std::string getPartialsDirectory();

virtual bool	getSharedDirectories(std::list<std::string> &dirs);
virtual bool 	addSharedDirectory(std::string dir);
virtual bool 	removeSharedDirectory(std::string dir);

virtual int 	reScanDirs();
virtual int 	check_dBUpdate();

	/***************************************************************/
	/*************** Control Interface *****************************/
	/***************************************************************/

	/***************************************************************/
	/*************** Data Transfer Interface ***********************/
	/***************************************************************/

bool	requestData(std::string peerId, std::string hash, 
				uint64_t offset, uint32_t size);

        /******************* p3 Config Overload ************************/
	protected:
        /* Key Functions to be overloaded for Full Configuration */
virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual bool    loadList(std::list<RsItem *> load);

	private:
bool  loadConfigMap(std::map<std::string, std::string> &configMap);
        /******************* p3 Config Overload ************************/

/*************************** p3 Config Overload ********************/
	private:

	/* no need for Mutex protection - 
	 * as each component is protected independently.
	 */

	CacheStrapper *mCacheStrapper;
	ftController  *mFtController;
	ftExtraList   *mFtExtra;

        FileIndexStore *fiStore;
	FileIndexMonitor *fimon;

	RsMutex srvMutex;
	std::string mConfigPath;
	std::string mDownloadPath;
	std::string mPartialsPath;

};



#endif

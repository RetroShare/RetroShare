/*
 * "$Id: ftfiler.h,v 1.5 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * Other Bits for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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



#ifndef MRK_FT_FILER_HEADER
#define MRK_FT_FILER_HEADER

/* 
 * PQI Filer
 *
 * This managers the file transfers.
 *
 */

#include "server/ft.h"
#include "pqi/pqi.h"
#include <list>
#include <iostream>
#include <string>

const int  PQIFILE_INIT                 = 0x0000;
const int  PQIFILE_NOT_ONLINE           = 0x0001;
const int  PQIFILE_DOWNLOADING          = 0x0002;
const int  PQIFILE_COMPLETE             = 0x0004;
const int  PQIFILE_FAIL                 = 0x0010;
/* reasons for DOWNLOAD FAILURE (2nd byte) */
const int  PQIFILE_FAIL_CANCEL          = 0x0100;
const int  PQIFILE_FAIL_NOT_AVAIL       = 0x0200;
const int  PQIFILE_FAIL_NOT_OPEN        = 0x0400;
const int  PQIFILE_FAIL_NOT_SEEK        = 0x0800;
const int  PQIFILE_FAIL_NOT_WRITE       = 0x1000;
const int  PQIFILE_FAIL_NOT_READ        = 0x2000;
const int  PQIFILE_FAIL_BAD_PATH	= 0x4000;


const int TRANSFER_MODE_TRICKLE = 1;
const int TRANSFER_MODE_NORMAL  = 2;
const int TRANSFER_MODE_FAST    = 3;


const uint32_t FT_MODE_STD    = 1;
const uint32_t FT_MODE_CACHE  = 2;
const uint32_t FT_MODE_UPLOAD = 4;

class ftFileStatus
{
public:
/****
	ftFileStatus(PQFileItem *in)
	:fileItem(in), status(PQIFILE_INIT), fd(NULL), 
	total_size(0), recv_size(0),
	req_loc(0), req_size(0), lastTS(0)
	{
		return;
	}
****/

	ftFileStatus(std::string name_in, std::string hash_in, uint64_t size_in, 
					std::string destpath_in, uint32_t mode_in)
	:name(name_in), hash(hash_in), destpath(destpath_in), size(size_in), ftMode(mode_in),
	status(PQIFILE_INIT), mode(0), rate(0), fd(NULL), total_size(0), recv_size(0),
	req_loc(0), req_size(0), lastTS(0), lastDelta(0)
	{
		/* not set ...
		 * id,
		 * filename,
		 * sources
		 */
		return;
	}

	ftFileStatus(std::string id_in, std::string name_in, std::string hash_in, uint64_t size_in, 
					std::string destpath_in, uint32_t mode_in)
	:id(id_in), name(name_in), hash(hash_in), destpath(destpath_in), size(size_in), ftMode(mode_in),
	status(PQIFILE_INIT), mode(0), rate(0), fd(NULL), total_size(0), recv_size(0),
	req_loc(0), req_size(0), lastTS(0), lastDelta(0)
	{
		/* not set ...
		 * id,
		 * filename,
		 * sources
		 */
		return;
	}


	~ftFileStatus()
	{
		if (fd)
			fclose(fd);
	}


	/* data */
	std::string id;       /* current source */
	std::string name;
	std::string hash;
	std::string destpath;
	uint64_t size;

	/* new stuff */
	uint32_t ftMode;
	std::list<std::string> sources;
	uint32_t resetCount; 

	/* transfer inprogress or not */
	int status; 		
	int mode;
	float rate;

	std::string file_name;
	FILE *fd;
	uint64_t  total_size;
	/* this is the simplistic case where only inorder data 
	 * otherwise - need much more status info */
	uint64_t   recv_size; 

	/* current file data request */
	uint64_t   req_loc;
	uint32_t   req_size;

	/* timestamp */
	time_t     lastTS;
	uint32_t   lastDelta; /* send til all recved */
};


class ftfiler: public ftManager
{
public:

        ftfiler(CacheStrapper *cs)
        :ftManager(cs) { return; }

virtual ~ftfiler() { return; }

virtual bool    RequestCacheFile(std::string id, std::string path, 
				std::string hash, uint64_t size);
virtual int     getFile(std::string name, std::string hash, 
	                        uint64_t size, std::string destpath);

virtual int     cancelFile(std::string hash);
virtual int     clearFailedTransfers();

int             tick();
std::list<RsFileTransfer *> getStatus();

virtual void    setSaveBasePath(std::string s);

/************* Network Interface****************************/
virtual int            recvFileInfo(ftFileRequest *in);
virtual ftFileRequest *sendFileInfo();
 
private: 
 
virtual int 	handleFileError(std::string hash, uint32_t err);
virtual int 	handleFileNotOnline(std::string hash);
virtual int 	handleFileNotAvailable(std::string hash);
virtual int 	handleFileData(std::string hash, uint64_t offset, 
				void *data, uint32_t size);

virtual int 	handleFileRequest(     std::string id, std::string hash, 	
				uint64_t offset, uint32_t chunk);
virtual int 	handleFileCacheRequest(std::string id, std::string hash, 
				uint64_t offset, uint32_t chunk);

ftFileStatus   *findRecvFileItem(std::string hash);
void		queryInactive();


/**************** End of Interface *************************/
int 		requestData(ftFileStatus *item);

/************* PQIFILEITEM Generator ***************************
 */

ftFileStatus *	createFileCache(std::string hash);
ftFileRequest *	generateFileRequest(ftFileStatus *);
int		generateFileData(ftFileStatus *s, std::string id, uint64_t offset, uint32_t size);
//int 		sendFileNotAvail(PQFileItem *req);

/************* FILE DATA HANDLING ******************************
 */

std::string 	determineTmpFilePath(ftFileStatus *s);
std::string 	determineDestFilePath(ftFileStatus *s);

int 		initiateFileTransfer(ftFileStatus *s);
int 		resetFileTransfer(ftFileStatus *s);
int 		addFileData(ftFileStatus *s, uint64_t idx, void *data, uint32_t size);
int 		completeFileTransfer(ftFileStatus *s);


	// Data.
private:
	std::list<ftFileStatus  *> recvFiles;
	std::list<ftFileStatus  *> fileCache;
	std::list<ftFileRequest *> out_queue;

	std::string saveBasePath;
};



#endif // MRK_PQI_FILER_HEADER

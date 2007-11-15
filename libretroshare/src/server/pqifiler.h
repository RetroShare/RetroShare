/*
 * "$Id: pqifiler.h,v 1.5 2007-02-19 20:08:30 rmf24 Exp $"
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



#ifndef MRK_PQI_FILER_HEADER
#define MRK_PQI_FILER_HEADER

/* 
 * PQI Filer
 *
 * This managers the file transfers.
 *
 */

#include "pqi/pqi.h"
#include "dbase/filelook.h"
#include "dbase/filedex.h"
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

class PQFileStatus
{
public:
	PQFileStatus(PQFileItem *in)
	:fileItem(in), status(PQIFILE_INIT), fd(NULL), 
	total_size(0), recv_size(0),
	req_loc(0), req_size(0), lastTS(0)
	{
		return;
	}

	~PQFileStatus()
	{
		if (fileItem)
			delete fileItem;
		if (fd)
			fclose(fd);
	}


/* data */
	PQFileItem *fileItem;
	/* transfer inprogress or not */
	int status; 		
	int mode;
	float rate;

	std::string file_name;
	FILE *fd;
	long   total_size;
	/* this is the simplistic case where only inorder data 
	 * otherwise - need much more status info */
	long   recv_size; 

	/* current file data request */
	long   req_loc;
	int   req_size;

	/* timestamp */
	long lastTS;
	int  lastDelta; /* send til all recved */
};


class pqifiler
{
public:

#ifdef USE_FILELOOK
	pqifiler(fileLook*);
#else
	pqifiler(filedex*);
#endif

virtual ~pqifiler() { return; }

/******************* GUI Interface *************************
 */

int		getFile(PQFileItem *in);
int		cancelFile(PQFileItem *i);
int		clearFailedTransfers();


int		tick();
std::list<FileTransferItem *> getStatus();

/************* Network Interface****************************
 */

PQItem *	sendPQFileItem();
int		recvPQFileItem(PQItem *in);

void 	 	setSavePath(std::string s) { savePath = s;}
 
private: 
 
PQFileStatus   *findRecvFileItem(PQFileItem *in);
void		queryInactive();

int 		handleFileError(PQFileItem *in);
int 		handleFileNotOnline(PQFileItem *in);
int 		handleFileNotAvailable(PQFileItem *in);
int 		handleFileData(PQFileItem *in);
int 		handleFileRequest(PQFileItem *in);
int 		handleFileCacheRequest(PQFileItem *req);


int 		requestData(PQFileStatus *item);

/************* PQIFILEITEM Generator ***************************
 */

PQFileStatus *	createFileCache(PQFileItem *in);
PQFileItem *	generatePQFileRequest(PQFileStatus *);
int 		generateFileData(PQFileStatus *s, PQFileItem *req);
int 		sendFileNotAvail(PQFileItem *req);

/************* FILE DATA HANDLING ******************************
 */

std::string 	determineFilePath(PQFileItem *item);
int 		initiateFileTransfer(PQFileStatus *s);
int 		resetFileTransfer(PQFileStatus *s);
int 		addFileData(PQFileStatus *s, long idx, void *data, int size);

	// Data.
private:
	std::list<PQFileStatus *> recvFiles;
	std::list<PQFileStatus *> fileCache;
	std::list<PQItem *> out_queue;

#ifdef USE_FILELOOK
	fileLook *fileIndex;
#else
	filedex *fileIndex;
#endif

	std::string savePath;
};



#endif // MRK_PQI_FILER_HEADER

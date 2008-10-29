/*
 * libretroshare/src/ft/ ftfilecreator.h
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

#ifndef FT_FILE_CREATOR_HEADER
#define FT_FILE_CREATOR_HEADER

/* 
 * ftFileCreator
 *
 * TODO: Serialiser Load / Save.
 *
 */
#include "ftfileprovider.h"
#include <map>
class ftChunk;

class ftFileCreator: public ftFileProvider
{
public:

	ftFileCreator(std::string savepath, uint64_t size, std::string hash, uint64_t recvd);
	
	~ftFileCreator();

	 /* overloaded from FileProvider */
virtual bool 	getFileData(uint64_t offset, uint32_t chunk_size, void *data);
	bool	finished() { return getRecvd() == getFileSize(); }
	uint64_t getRecvd();
		

	/* 
	 * creation functions for FileCreator 
         */

	bool	getMissingChunk(uint64_t &offset, uint32_t &chunk);
	bool 	addFileData(uint64_t offset, uint32_t chunk_size, void *data);

protected:

virtual int initializeFileAttrs(); 

private:

	int 	notifyReceived(uint64_t offset, uint32_t chunk_size);
	/* 
         * structure to track missing chunks 
         */
	
	uint64_t mStart;
	uint64_t mEnd;

	std::map<uint64_t, ftChunk> mChunks;

};

class ftChunk {
public:
	ftChunk(uint64_t,uint64_t,time_t);
	ftChunk():offset(0), chunk(0), ts(0) {}
	~ftChunk();

	uint64_t offset;
	uint64_t chunk;
	time_t   ts;
};

#endif // FT_FILE_CREATOR_HEADER

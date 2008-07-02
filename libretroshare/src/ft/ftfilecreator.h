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
#include "util/rsthreads.h"
#include <queue>
class ftChunk;
class ftFileChunker;

class ftFileCreator: public ftFileProvider
{
public:

	ftFileCreator(std::string savepath, uint64_t size, std::string hash,std::string chunker);
	
	~ftFileCreator();

	/* overloaded from FileProvider */
//virtual bool 	getFileData(uint64_t offset, uint32_t chunk_size, void *data);
int initializeFileAttrs(); //not override?

	/* creation functions for FileCreator */
bool	getMissingChunk(uint64_t &offset, uint32_t &chunk);
bool 	addFileData(uint64_t offset, uint32_t chunk_size, void *data);

private:
	/* structure to track missing chunks */
	
	/* structure to hold*/

//	std::string save_path; use file_name from parent
//	uint64_t total_size;
	uint64_t recv_size;
	std::string hash;
	ftFileChunker *fileChunker;
	void initialize(std::string);
	RsMutex ftcMutex;
};

/*
	This class can either be specialized to follow a different splitting
	policy or have an argument to indicate different policies
*/
class ftFileChunker : public RsThread {
public:
	/* Does this require hash?? */
	ftFileChunker(uint64_t size);
	virtual ~ftFileChunker();
	/* Breaks up the file into evenly sized chunks 
	   Initializes all chunks to never_requested	
	*/
    int splitFile();
	virtual void run();
	virtual bool	getMissingChunk(uint64_t &offset, uint32_t &chunk);
	bool 	getMissingChunkRandom(uint64_t &offset, uint32_t &chunk);
	int 	notifyReceived(uint64_t offset, uint32_t chunk_size);
	int     monitor();
	void 	setMonitorPeriod(int period);
protected: 
	uint64_t file_size;
	uint64_t num_chunks;
	uint64_t std_chunk_size;
	std::vector<ftChunk*> allocationTable;
	unsigned int aggregate_status;
	int monitorPeriod;
	RsMutex chunkerMutex; /********** STACK LOCKED MTX ******/
};


class ftFileRandomizeChunker : public ftFileChunker {
public:
	ftFileRandomizeChunker(uint64_t size);
	virtual bool	getMissingChunk(uint64_t &offset, uint32_t &chunk);
	~ftFileRandomizeChunker();

};


class ftChunk {
public:
	enum Status {AVAIL, ALLOCATED, RECEIVED};
	ftChunk(uint64_t,uint64_t,time_t,Status);
	uint64_t offset;
	uint64_t max_chunk_size;
	time_t   timestamp;
	Status chunk_status;	
	
};

#endif // FT_FILE_PROVIDER_HEADER

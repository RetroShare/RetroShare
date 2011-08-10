/*
 * libretroshare/src/distrib: p3distribcacheopt.cc
 *
 *
 * Copyright 2011 by Christopher Evi-Parker
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

#include "p3distribcacheopt.h"
#include "pqi/authssl.h"

#include <fstream>
// cache id which have failed are stored under a node
// of this name/grpid
#define FAILED_CACHE_CONT "failedcachegrp"

p3DistribCacheOpt::p3DistribCacheOpt(const std::string& ownId) : mOwnId(ownId)
{
	// TODO Auto-generated constructor stub

}

p3DistribCacheOpt::~p3DistribCacheOpt()
{
	// TODO Auto-generated destructor stub
}


// check if grp exist in cache opt file
bool p3DistribCacheOpt::grpCacheOpted(const std::string & grpId)
{
	std::map<std::string, nodeCache>::iterator cit;
	if(mCacheTable.end() != (cit = mCacheTable.find(grpId)))
	{
		return cit->second.cached;
	}
	return false;
}



bool p3DistribCacheOpt::saveCacheOptFile(const std::string & filePath)
{
	std::cout << mCacheDoc.last_child().value();
	if(mCacheDoc.empty())
		return false;

	// decryption function needs file to be kept in binary
    std::ofstream hFile(filePath.c_str(), std::ios::binary | std::ios::out);
	std::ostringstream cacheStream;
	char* fileBuffer = NULL;
	int streamLength;
	char* encryptedFileBuffer = NULL;
	int outlen = 0;
	bool ok = false;

	mCacheDoc.save(cacheStream);
	streamLength = cacheStream.str().length();
	std::string cacheContent = cacheStream.str();
	fileBuffer = new char[cacheContent.size()];
	cacheContent.copy(fileBuffer, cacheContent.size(), 0);

 	ok = AuthSSL::getAuthSSL()->encrypt((void*&)encryptedFileBuffer, outlen,
			(void*&)fileBuffer, streamLength, mOwnId);

 	if(ok){
		hFile.write(encryptedFileBuffer, outlen);
		hFile.close();
 	}

	if(fileBuffer != NULL)
		delete[] fileBuffer;

	if(encryptedFileBuffer != NULL)
		delete[] encryptedFileBuffer;

	return ok;
}



bool p3DistribCacheOpt::loadCacheOptFile(const std::string & filePath)
{

	// decryption function needs file to be in binary
    std::ifstream hFile(filePath.c_str(), std::ios::binary | std::ios::in);
	int fileLength;
	char* fileLoadBuffer = NULL;
	char* decryptedCacheFile = NULL;
	int outlen = 0;
	bool ok = false;
	hFile.seekg(0, std::ios::end);
	fileLength = hFile.tellg();
	hFile.seekg(0, std::ios::beg);

	if(fileLength <= 0)
		return false;

	fileLoadBuffer = new char[fileLength];
	hFile.read(fileLoadBuffer, fileLength);
	hFile.close();

	ok = AuthSSL::getAuthSSL()->decrypt((void*&)decryptedCacheFile, outlen,
			fileLoadBuffer, fileLength);

	if(fileLoadBuffer != NULL)
		delete[] fileLoadBuffer;

	char* buffer = static_cast<char*>(pugi::get_memory_allocation_function()(outlen));

	if(ok){

		memcpy(buffer, decryptedCacheFile, outlen);

		// release resource if decryption successful
		if(decryptedCacheFile != NULL)
			delete[] decryptedCacheFile;

		ok &= mCacheDoc.load_buffer_inplace_own(buffer, outlen);

	}else{
		if(buffer !=NULL)
			delete[] buffer;
	}

	return ok;
}



void p3DistribCacheOpt::updateCacheDocument(std::vector<grpCachePair>& grps,
		std::vector<grpCachePair>& caches)
{

#ifdef DISTRIB_HISTORY_DEBUG
	std::cerr << "p3GroupDistrib::updateCacheDocument() "
			  <<  grps.size() << " Grps"
			  << std::endl;
#endif

	std::vector<grpNodePair> grpNodes;
	std::string failedCacheId = FAILED_CACHE_CONT;

	// failed cache content node is has not been created add to doc
	if(mCacheTable.find(failedCacheId) == mCacheTable.end()){

		mCacheDoc.append_child("group");
		mCacheDoc.last_child().append_child("grpId").append_child(
							pugi::node_pcdata).set_value(failedCacheId.c_str());

		grpNodes.push_back(grpNodePair(failedCacheId, mCacheDoc.last_child()));
	}

	std::map<std::string, nodeCache>::iterator nodeCache_iter;

	// for transforming int to string
	std::string subId;
	char subIdBuffer[6];

	std::vector<grpCachePair>::iterator grpIt =
			grps.begin(), msgIt = caches.begin();

	/*
	 * add pending recvd msgs and grps to cache table
	 */

	for(; grpIt != grps.end(); grpIt++){


		// make sure grp does not exist


		if(grpCacheOpted(grpIt->first)){

			// add group node
			mCacheDoc.append_child("group");

			/*** adding grp id   ***/

			mCacheDoc.last_child().append_child("grpId").append_child(
					pugi::node_pcdata).set_value(grpIt->first.c_str());

			/*** adding cache id ***/

			// add pid
			mCacheDoc.last_child().append_child("pId").append_child(
					pugi::node_pcdata).set_value(grpIt->second.first
							.c_str());

			// apparently portable int to string method
			sprintf(subIdBuffer, "%d", grpIt->second.second);
			subId = subIdBuffer;
			mCacheDoc.last_child().append_child("subId").append_child(
					pugi::node_pcdata).set_value(subId.c_str());

			grpNodes.push_back(grpNodePair(grpIt->first, mCacheDoc.last_child()));

		}
		else
		{
#ifdef DISTRIB_HISTORY_DEBUG
			std::cerr << "p3GroupDistrib::updateCacheDocument()"
					  << "\nGrp already Exists in Document!";
#endif
		}
	}

	std::map<std::string, std::set<pCacheId> > msgCacheMap;
	pugi::xml_node nodeIter;

	// create entry for all grps with empty sets
	for(; msgIt != caches.end(); msgIt++)
	{
		msgCacheMap.insert(std::make_pair(msgIt->first, std::set<pCacheId>()));
	}

	// now update document with new msg cache info
	msgIt = caches.begin();
	std::vector<grpCachePair> msgHistRestart;
	pugi::xml_node messages_node;
	pCacheId pCid;

	for(; msgIt != caches.end(); msgIt++)
	{

		// find grp in cache document
		nodeCache_iter = mCacheTable.find(msgIt->first);

		if(nodeCache_iter != mCacheTable.end()){

			pCid = pCacheId(msgIt->second.first,
					msgIt->second.second);

			nodeIter = nodeCache_iter->second.node;
			messages_node = nodeIter.child("messages");

			// if messages child does not exist, add one
			if(!messages_node)
				messages_node = nodeIter.append_child("messages");

			messages_node.append_child("msg");

			// add cache id
			messages_node.last_child().append_child("pId").append_child(
								pugi::node_pcdata).set_value(msgIt->second.first
										.c_str());
			sprintf(subIdBuffer, "%d", msgIt->second.second);
			subId = subIdBuffer;
			messages_node.last_child().append_child("subId").append_child(
								pugi::node_pcdata).set_value(subId.c_str());

		}
	}

	return;
}



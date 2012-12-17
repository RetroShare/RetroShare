/*
 * main.cpp
 *
 *  Created on: 11 Apr 2011
 *      Author: greyfox
 */

#include <string>
#include <set>
#include <iostream>

#include <common/argstream.h>
#include "pqi/pqistreamer.h"
#include "pqi/pqibin.h"
#include "serialiser/rsdistribitems.h"
#include "pqi/pqistore.h"

int storeMsg(std::map<std::string, std::list<std::string> > &msgMap, std::string filename,
					std::string grpId, std::string msgId);

pqistore *BuildSerialiser(int type, std::string filename);

int main(int argc, char **argv) 
{
	
	std::list<std::string> fileList;
	std::list<std::string>::iterator fit;
	
	for(int i = 1; i < argc; i++)
	{
		fileList.push_back(argv[i]);
	}

	std::map<std::string, std::list<std::string> > msgMap;
	std::map<std::string, std::list<std::string> >::iterator mit;

	bool printInput = true;
	bool printAll = true;
	bool printSummary = true;
	int distrib_msgs = 0;
	int signed_msgs = 0;

	for(fit = fileList.begin(); fit != fileList.end(); fit++)
	{
		if (printInput)
		{
			std::cerr << "Loading File: " << *fit;
			std::cerr << std::endl;
		}

		/* process this file */
		pqistore *store = BuildSerialiser(0, *fit);
		RsItem* item;
		RsDistribMsg *distMsg;
		RsDistribSignedMsg *signMsg;
		
		while(NULL != (item = store->GetItem()))
		{
			if (NULL != (distMsg = dynamic_cast<RsDistribMsg*>(item)))
			{
				if (printInput)
				{
					std::cerr << "Grp = " << distMsg->grpId;
					std::cerr << ", parent = " << distMsg->parentId;
					std::cerr << ", msgId = " << distMsg->msgId;
					std::cerr << std::endl;
				}
				storeMsg(msgMap, *fit, distMsg->grpId, distMsg->msgId);
				++distrib_msgs ;
			}
			if (NULL != (signMsg = dynamic_cast<RsDistribSignedMsg*>(item)))
			{
				if (printInput)
				{
					std::cerr << "Grp = " << signMsg->grpId;
					std::cerr << ", msgId = " << signMsg->msgId;
					std::cerr << ", type = " << signMsg->packet.tlvtype;
					std::cerr << ", len = " << signMsg->packet.bin_len;
					std::cerr << std::endl;
				}
				storeMsg(msgMap, *fit, signMsg->grpId, signMsg->msgId);
				++signed_msgs ;
			}
		}
		
		delete store;
	}
	for(mit = msgMap.begin(); mit != msgMap.end(); mit++)
	{
		bool print = printAll;
		if (mit->second.size() > 1)
		{
			/* duplicates */
			print = true;
		}

		if (print)
		{
			std::cerr << "MsgId: " << mit->first;	
			std::cerr << " in Files: ";

			std::list<std::string>::iterator fit;
			for(fit = mit->second.begin(); fit != mit->second.end(); fit++)
			{
				std::cerr << *fit << " ";
			}
			std::cerr << std::endl;
		}
	}
	if (printSummary)
	{
		std::cerr << "# RsDistribMsg(s): " << distrib_msgs << std::endl;
		std::cerr << "# RsDistribSignedMsg(s): " << signed_msgs << std::endl;
		std::cerr << std::endl;
	}
}

	
int storeMsg(std::map<std::string, std::list<std::string> > &msgMap, std::string filename,
					std::string grpId, std::string msgId)
{
	std::map<std::string, std::list<std::string> >::iterator mit;
	mit = msgMap.find(msgId);
	if (mit == msgMap.end())
	{
		std::list<std::string> fileList;
		fileList.push_back(filename);
		msgMap[msgId] = fileList;
	}
	else
	{
		(mit->second).push_back(filename);
	}
	return 1;
}


pqistore *BuildSerialiser(int type, std::string filename)
{ 
	BinInterface *bio = new BinFileInterface(filename.c_str(), BIN_FLAGS_READABLE);
	RsSerialiser *rsSerialiser = new RsSerialiser();
	RsSerialType *serialType = new RsDistribSerialiser();
	rsSerialiser->addSerialType(serialType);

	// can use type to add more serialiser types
	//RsSerialType *serialType = new RsDistribSerialiser();
	//rsSerialiser->addSerialType(serialType);

	pqistore *store = new pqistore(rsSerialiser, "", bio, BIN_FLAGS_READABLE);
	return store;
}



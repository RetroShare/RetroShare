/*
 * main.cpp
 *
 *  Created on: 11 Apr 2011
 *      Author: greyfox
 */

#include <string>
#include <set>
#include <iostream>

#include "argstream.h"
#include "pqi/pqistreamer.h"
#include "pqi/pqibin.h"
#include "serialiser/rsdistribitems.h"
#include "pqi/pqistore.h"

int main(int argc, char **argv) 
{
	try
	{
		argstream as(argc,argv) ;
		bool full_print = false ;

		std::string cacheFileName;

		as >> parameter('f',"file",cacheFileName,"cache file name to read",true)
			>> option('F',"full",full_print,"full dump of distrib items")
			>> help() ;

		as.defaultErrorHandling() ;

		std::cout << "opening file: " << cacheFileName << std::endl;
		// open file
		BinInterface *bio = new BinFileInterface(cacheFileName.c_str(), BIN_FLAGS_READABLE);
		RsSerialiser *rsSerialiser = new RsSerialiser();
		RsSerialType *serialType = new RsDistribSerialiser();
		rsSerialiser->addSerialType(serialType);
		pqistore *store = new pqistore(rsSerialiser, "", bio, BIN_FLAGS_READABLE);
		RsItem* item;

		std::set<std::string> already ;
		int signed_msgs = 0 ;
		int distrib_msgs = 0 ;
		int nb_duplicated = 0 ;

		// then print out contents to screen
		while(NULL != (item = store->GetItem()))
		{
			
			if(full_print)
				item->print(std::cout, 0);
			else
			{
				{
					RsDistribMsg *i = dynamic_cast<RsDistribMsg*>(item) ;
					if(i!=NULL)
					{
						std::cerr << "Grp=" << i->grpId << ", parent=" << i->parentId << ", msgId=" << i->msgId ;

						if(already.find(i->msgId)!=already.end())
						{
							std::cerr << " *** double ***" ;
							++nb_duplicated ;
						}
						else
							already.insert(i->msgId) ;
						
						std::cerr << std::endl;
						++distrib_msgs ;
					}
				}
				{
					RsDistribSignedMsg *i = dynamic_cast<RsDistribSignedMsg*>(item) ;
					if(i!=NULL)
					{
						std::cerr << "Grp=" << i->grpId << ", msgId=" << i->msgId << ", type=" << i->packet.tlvtype << ", len=" << i->packet.bin_len ;
						
						if(already.find(i->msgId)!=already.end())
						{
							std::cerr << " *** double ***" ;
							++nb_duplicated ;
						}
						else
							already.insert(i->msgId) ;
						
						std::cerr << std::endl;

						++signed_msgs ;
					}
				}
			}
		}

		std::cerr << std::endl ;
		std::cerr << "Signed messages (RsDistribMsg)      : " << distrib_msgs << std::endl;
		std::cerr << "Signed messages (RsDistribSignedMsg): " << signed_msgs << std::endl;
		std::cerr << "Duplicate messages                  : " << nb_duplicated << std::endl;

		return 0;
	}
	catch(std::exception& e)
	{
		std::cerr << "Unhandled exception: " << e.what() << std::endl;
		return 1 ;
	}
}




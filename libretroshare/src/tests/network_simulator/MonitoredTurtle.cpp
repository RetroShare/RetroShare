#include "MonitoredTurtle.h"

bool MonitoredTurtleRouter::performLocalHashSearch(const TurtleFileHash& hash,const std::string& peer_id,FileInfo& info)
{
	std::map<std::string,FileInfo>::const_iterator it( _local_files.find(hash) ) ;

	if(it != _local_files.end() )
	{
		info = it->second ;
		return true ;
	}
	else
		return false ;
}

void MonitoredTurtleRouter::provideFileHash(const std::string& hash)
{
	FileInfo& info( _local_files[hash] ) ;

	info.fname = "File 1" ;
	info.size = 100000 ;
	info.hash = hash ;
}


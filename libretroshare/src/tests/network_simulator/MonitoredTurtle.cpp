#include "MonitoredTurtle.h"

bool MonitoredTurtleRouter::performLocalHashSearch(const TurtleFileHash& hash,const RsPeerId& peer_id,FileInfo& info)
{
	std::map<RsFileHash,FileInfo>::const_iterator it( _local_files.find(hash) ) ;

	if(it != _local_files.end() )
	{
		info = it->second ;
		return true ;
	}
	else
		return false ;
}

void MonitoredTurtleRouter::provideFileHash(const RsFileHash& hash)
{
	FileInfo& info( _local_files[hash] ) ;

	info.fname = "File 1" ;
	info.size = 100000 ;
	info.hash = hash ;
}


#include "MonitoredTurtle.h"

bool MonitoredTurtleRouter::performLocalHashSearch(const TurtleFileHash& hash,FileInfo& info)
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

#include "MonitoredTurtleClient.h"

bool MonitoredTurtleClient::handleTunnelRequest(const TurtleFileHash& hash,const RsPeerId& peer_id)
{
	std::map<RsFileHash,FileInfo>::const_iterator it( _local_files.find(hash) ) ;

    return (it != _local_files.end() ) ;
}

void MonitoredTurtleClient::provideFileHash(const RsFileHash& hash)
{
	FileInfo& info( _local_files[hash] ) ;

	info.fname = "File 1" ;
	info.size = 100000 ;
	info.hash = hash ;
}


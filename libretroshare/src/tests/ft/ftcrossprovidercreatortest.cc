#include <stdexcept>
#include "ft/ftfilecreator.h"

#include "util/utest.h"
#include "util/rsdir.h"
#include "util/rsdiscspace.h"
#include <stdlib.h>

#include "util/rswin.h"
#include "ft/ftserver.h"

INITTEST();

static void createTmpFile(const std::string& name,uint64_t size,std::string& hash) ;
static void transfer(ftFileProvider *server, const std::string& server_peer_id,ftFileCreator *client,const std::string& client_peer_id) ;

static const float TRANSFER_FAIL_PROBABILITY = 0.0f ;

void validateChunksToCheck(const Sha1Map& chunk_crc_map,ftFileCreator *client,const std::string& client_name)
{
	std::vector<uint32_t> chunks ;
	client->getChunksToCheck(chunks) ;

	if(chunks.size() > 0)
		for(uint32_t i=0;i<chunks.size();++i)
		{
			std::cerr << "Validating Chunk " << chunks[i] << " for peer: " << client_name << ", with hash " << chunk_crc_map[chunks[i]].toStdString() << std::endl;
			client->verifyChunk(chunks[i],chunk_crc_map[chunks[i]]) ;
		}
}

void initChunkCrcMap(const std::string& fname,Sha1Map& map)
{
	FILE *fd = fopen(fname.c_str(),"rb") ;
	unsigned char *buf = new unsigned char[ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE] ;

	if(fd == NULL)
	{
		std::cerr << "Cannot read file " << fname << ". Something's wrong!" << std::endl;
		delete[] buf ;
		exit(1) ;
	}

	for(int i=0;i<map.size();++i)
	{
		uint32_t len ;
		if(fseeko64(fd,(uint64_t)i * (uint64_t)ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE,SEEK_SET)!=0 || 0==(len = fread(buf,1,ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE,fd))) 
		{
			std::cerr << "Cannot fseek/read from file " << fname << " at position " << (uint64_t)i * (uint64_t)ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE << std::endl;
			fclose(fd) ;
			exit(1) ;
		}
		map.set(i, RsDirUtil::sha1sum(buf,len)) ;

		std::cerr << "  chunk " << i << ": crc = " << map[i].toStdString() << std::endl;
	}
	fclose(fd) ;

	delete[] buf ;
}

int main()
{
	// We create two file creators and feed them with a file both from elsewhere
	// and from each others, as file providers.  This test should check that no
	// data race occurs while reading/writting data and exchanging chunks.  The
	// test is designed so that servers do not always have the required chunk,
	// meaning that many requests can't be fullfilled. This is a good way to
	// test if chunks availability is correctly handled.

	RsDiscSpace::setPartialsPath(".") ;

	// 1 - Create a temporary file, compute its hash
	//
	std::string fname = "source_tmp.bin" ;
	std::string fname_copy_1 = "copy_1_tmp.bin" ;
	std::string fname_copy_2 = "copy_2_tmp.bin" ;
	uint64_t size = 5000000 ;
	std::string hash = "" ;

	pthread_t seed = 8;//getpid() ;

	std::cerr << "Seeding random generator with " << seed << std::endl ;
	srand48(seed) ;
	createTmpFile(fname,size,hash) ;

	// 2 - Create one file provider for this file, and 2 file creators
	//

	ftFileProvider *server = new ftFileProvider(fname,size,hash) ;
	ftFileCreator *client1 = new ftFileCreator(fname_copy_1,size,hash,false) ;
	ftFileCreator *client2 = new ftFileCreator(fname_copy_2,size,hash,false) ;

	// 3 - Exchange chunks, and build two copies of the file.
	//
	std::string peer_id_1("client peer id 1") ;
	std::string peer_id_2("client peer id 2") ;
	std::string server_id("server peer id") ;

	// 4 - prepare a chunk crc map to verify chunks.
	
	Sha1Map chunk_crc_map(size,ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE) ;

	initChunkCrcMap(fname,chunk_crc_map) ;

	std::cerr << "Computed chunk CRC map: " << std::endl;

	for(int i=0;i<chunk_crc_map.size();++i)
		std::cerr << "  Chunk " << i << " : " << chunk_crc_map[i].toStdString() << std::endl;

	while( !(client1->finished() && client2->finished()))
	{
		// 1 - select a random client, get the data from him.

		ftFileProvider *tmpserver ;
		ftFileCreator  *tmpclient ;
		std::string tmpserver_pid ;
		std::string tmpclient_pid ;
		
		// choose client and server, randomly, provided that they are not finished.
		//
		if((!client1->finished()) && (client2->finished() || (lrand48()&1)))
		{
			//std::cerr << "###### Client 1 asking to " ;
			tmpclient = client1 ;
			tmpclient_pid = peer_id_1 ;

			if(lrand48()&1)
			{
				//std::cerr << "Client 2 " ;
				tmpserver = client2 ;
				tmpserver_pid = peer_id_2 ;
			}
			else
			{
				//std::cerr << "Server ";
				tmpserver = server ;
				tmpserver_pid = server_id ;
			}
		}
		else // client 2 is necessarily unfinished there.
		{
			//std::cerr << "###### Client 2 asking to " ;
			tmpclient = client2 ;
			tmpclient_pid = peer_id_2 ;
			
			if(lrand48()&1)
			{
				//std::cerr << "Client 1 " ;
				tmpserver = client1 ;
				tmpserver_pid = peer_id_1 ;
			}
			else
			{
				//std::cerr << "Server " ;
				tmpserver = server ;
				tmpserver_pid = server_id ;
			}

		}

		// 2 - transfer from the server to the client.

		transfer(tmpserver,tmpserver_pid,tmpclient,tmpclient_pid) ;

		printf("Client1: %08lld, Client2: %08lld, transfer from %s to %s\r",client1->getRecvd(),client2->getRecvd(),tmpserver_pid.c_str(),tmpclient_pid.c_str()) ;
		fflush(stdout) ;

		// Also get chunk checking list from each server, and validate them.

		validateChunksToCheck(chunk_crc_map,client1,"client 1") ;
		validateChunksToCheck(chunk_crc_map,client2,"client 2") ;
	}

	// hash the received data
	
	std::string hash1 ; client1->hashReceivedData(hash1) ;
	std::string hash2 ; client2->hashReceivedData(hash2) ;

	std::cout << "Hash  = " << hash  << std::endl ;
	std::cout << "Hash1 = " << hash1 << std::endl ;
	std::cout << "Hash2 = " << hash2 << std::endl ;
	
	if(hash  != hash1) FAILED("hashs 0/1 defer after transfer !") ;
	if(hash  != hash2) FAILED("hashs 0/2 defer after transfer !") ;
	if(hash1 != hash2) FAILED("hashs 1/2 defer after transfer !") ;

	FINALREPORT("File transfer test");

	return TESTRESULT();
}

void transfer(ftFileProvider *server, const std::string& server_peer_id,ftFileCreator *client,const std::string& client_peer_id)
{
	uint32_t size_hint = 128 + (lrand48()%1000) ;
	uint64_t offset = 0;
	uint32_t chunk_size = 0;
	bool toOld = false ;

	//std::cerr << " for a pending chunk of size " << size_hint ;

	if(! client->getMissingChunk(server_peer_id, size_hint, offset, chunk_size, toOld))
	{
		std::cerr << "###### Server does not have any chunk to offer: server is " << server_peer_id << ", client is " << client_peer_id << std::endl ;
		// Normally this can't be true, because the ChunkMap class assumes availability for non turtle 
		// peers.
		if(toOld)
		{
			std::cerr << "###### Map is too old. Transferring chunk map" << std::endl ;
			CompressedChunkMap map ;
			server->getAvailabilityMap(map) ;
			client->setSourceMap(server_peer_id,map) ;
		}	
		return ;
	}

	std::cerr << "###### Got chunk " << offset << ", size=" << chunk_size << std::endl ;

	void *data = malloc(chunk_size) ;
	std::cerr << "###### Asking data " << offset << ", size=" << chunk_size << std::endl ;

	if( server->getFileData(client_peer_id,offset,chunk_size,data) )
	{
		if(drand48() < TRANSFER_FAIL_PROBABILITY)
		{
			std::cerr << "simulating faulting data chunk " << offset << " + " << chunk_size << "." << std::endl;
		}
		else
		{
			client->addFileData(offset,chunk_size,data) ;
			std::cerr << "###### Added data " << offset << ", size=" << chunk_size << std::endl ;
		}
	}

	std::cerr << ", completion=" << client->getRecvd() << std::endl ;


	free(data) ;
}

void createTmpFile(const std::string& name,uint64_t S,std::string& hash)
{
	FILE *tmpf = fopen("source_tmp.bin","w") ;

	// write 1MB chunks
	uint64_t C = 1024*1024 ;
	uint8_t *data = new uint8_t[C] ;

	for(uint i=0;i<=S/C;++i)
	{
		int Cp = C ;
		if(i==S/C)
		{
			if(S%C>0)
				Cp = S%C ;
			else
				break ;
		}

		for(int k=0;k<Cp;++k)
			data[k] = lrand48()%256 ;

		size_t n = fwrite(data,1,Cp,tmpf) ;
		if(n != Cp)
		{
			std::cerr << "Wrote " << n << " expected " << Cp << std::endl;
			throw std::runtime_error("Could not write file !") ;
		}
	}
	fclose(tmpf) ;

	uint64_t size ;
	std::string path ;
	RsDirUtil::hashFile(name,path,hash,size) ;
}




#include "bitdht/bdiface.h"
#include "bitdht/bdstddht.h"
#include "bdhandler.h"

#include "bootstrap_fn.h"

bool bdSingleShotFindPeer(const std::string bootstrapfile, const std::string peerId, std::string &peer_ip, uint16_t &peer_port)
{
	/* startup dht : with a random id! */
        bdNodeId ownId;
        bdStdRandomNodeId(&ownId);

	uint16_t port = 6775;
	std::string appId = "bsId";
	BitDhtHandler dht(&ownId, port, appId, bootstrapfile);

	/* install search node */
        bdNodeId searchId;
	if (!bdStdLoadNodeId(&searchId, peerId))
	{
        	std::cerr << "bdSingleShotFindPeer(): Invalid Input Id: " << peerId;
		return false;
	}

        std::cerr << "bssdht: searching for Id: ";
        bdStdPrintNodeId(std::cerr, &searchId);
	std::cerr << std::endl;

        dht.FindNode(&searchId);

	/* run your program */
	bdId resultId;
	uint32_t status;

        resultId.id = searchId;

	while(false == dht.SearchResult(&resultId, status))
	{
		sleep(10);
	}

        std::cerr << "bdSingleShotFindPeer(): Found Result:" << std::endl;

        std::cerr << "\tId: ";
        bdStdPrintId(std::cerr, &resultId);
	std::cerr << std::endl;

	std::cerr << "\tstatus: " << status;
	std::cerr << std::endl;

	dht.shutdown();

	if ((status == BITDHT_QUERY_PEER_UNREACHABLE) ||
		(status == BITDHT_QUERY_SUCCESS))
	{
	
		peer_ip = bdnet_inet_ntoa(resultId.addr.sin_addr);
		peer_port = ntohs(resultId.addr.sin_port);
	
		std::cerr << "Answer: ";
		std::cerr << std::endl;
		std::cerr << "\tPeer IpAddress: " << peer_ip;
		std::cerr << std::endl;
		std::cerr << "\tPeer Port: " << peer_port;
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "Sorry, Cant be found!";
		std::cerr << std::endl;
	}

	return true;
}


		






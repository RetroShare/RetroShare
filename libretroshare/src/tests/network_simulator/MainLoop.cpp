
void NetworkLoop()
{
	while(true)
	{
		std::cerr<< "network loop: tick()" << std::endl;

		// Get items for each components and send them to their destination.
		//
		for(int i=0;i<network->n_nodes();++i)
		{
			RsRawItem *item ;

			while(item = network->node(i).outgoing())
			{
				PeerNode& node = network->node_by_id(item->peerId())
				item->peerId(network->node(i)->id()) ;

				node.received(item) ;
			}
		}
		usleep(500000) ;
	}
}

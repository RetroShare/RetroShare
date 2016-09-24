TODO
====

Testing the router algorithm with network simulator
	* generate a random network
	* simulate disconnections (also in GUI)
	* send messages from/to random peers and measure:
		- how many times a given message is stored
		- how much time a given message took to arrive.

Implement
	* clueing of GR from GXS (simulated in network simulator, when initing the network)

In GLobal Router, by order of priority
	* when a ACK is received for a msg that is already ACKed, we should still update the routing matrix and add a clue, but with lower prioity,
		so that the matrix gets filled with additional routes. Can be checked in the simulator

	* routing strategy:
		- when a route is known and available, always select it, but possibly add another random route, very rarely.
			Peer disconnection is likely to cause the discovery of new routes anyway.

	* we should use clues from GXS to improve the routing matrix
		- That would avoid lots of spamming.
		- allows to init the routing matrices for all keys
		- random walk will be a supplemental help, but restricted to small depth if no indication of route is available.
		- needs to be implemented in network simulator. When providing a new key, the key should be spread in the network and new clues
			should be added to the RGrouter matrix.

	* make sure the depth is accounted better:
		- if single route is known => don't limit depth
		- if no route is known => strictly limit depth
			=> add a counter which is increased when no route is available, and *reset* otherwise, so that the max number of bounce
				we can do without knowledge of the keys is limited, but the total depth has no limits.

Unsolved questions:
	* handle dead routes correctly. How?
	* should we send ACKs everywhere even upward? No, if we severely limit the depth of random walk.
	* better distribute routing events, so that the matrix gets filled better?
	* find a strategy to avoid storing too many items
	* how to handle cases where a ACK cannot be sent back? The previous peer is going to try indefinitly?
		=> the ACK will be automatically collected by another route!
	* how to make sure ACKed messages are not stored any longer than necessary?
	* send signed ACKs, so that the receiver cannot be spoofed.
	* only ACK when the message was properly received by the client service. No ACK if the client does not register that item?

================================================================================================================


The purpose of this directory is to write a Network simulator, that can have multiple turtle routers interact
together. The routers will talk to a fake link manager, which reports the peers for each node of a network graph.

Required components:
===================
	NetworkGraph: a set of friends, with connexions. Should be able to be saved to a file for debugging.

	GraphNode: a RS peer, represented by a random SSL id, a link manager, and possibly components such as file transfer, etc. 

	Main loop: a loop calling tick() on all turtle routers.

   Functions:
		* gather statistics over network load. See if tunnels are ok, improve bandwidth allocation strategy, see request broadcast.

	GUI:
		* visualization of the graph. OpenGL + qglviewer window. Show tunnels, data flow as colors, etc.
		* give quantitative information under mouse
		* the user can trigger behaviors, execute tunnel handling orders, cause file transfer, etc.


Implementation constraints
==========================
	* sendItem() and recvItems() should come from above. The class p3Service thus needs to be re-implemented to get/send the 
	  data properly.

	  	=> define subclass of p3turtle , where send() and recv() are redefined.

	* LinkMgr:
		getOwnId(), getLinkType(), getOnlineList()

		used by turtle

	* turtle needs LinkMgr and ftServer. The ftServer can be contructed from PeerMgr (not called except in ftServer::setupFtServer. not needed here.

Complilation
============
	* needs the QGLViewer-dev library (standard on ubuntu, package name is libqglviewer-qt4-dev)
	* should compile on windows and MacOS as well. Use http://www.libqglviewer.com


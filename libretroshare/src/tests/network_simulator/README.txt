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
	* needs QGLViewer-dev library (standard on ubuntu, package name is libqglviewer-qt4-dev)


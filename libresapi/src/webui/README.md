A new approach to build a webinterface for RS
=============================================

1. get JSON encoded data from the backend, data contains a state token
2. render data with react.js
3. ask the backend if the state token from step 1 expired. If yes, then start again with step 1.

Steps 1. and 3. are common for most things, only Step 2. differs. This allows to re-use code for steps 1. and 3.

BUILD / INSTALLATION
------------

	- run (requires wget, use MinGW shell on Windows)
		make
	- all output files are now in libresapi/src/webfiles
	- use the --webinterface 9090 command line parameter to enable webui in retroshare-nogui
	- set the --docroot parameter of retroshare-nogui to point to the "libresapi/src/webfiles" directory
		(or symlink from /usr/share/RetroShare0.6/webui on Linux, ./webui on Windows)
	- retroshare-gui does not have a --docroot parameter. Use symlinks then.

DEVELOPMENT
-----------

	- Ubuntu: install nodejs package
		sudo apt-get install nodejs
	- Windows: download and install nodejs from http://nodejs.org
	- Download development tools with the nodejs package manager (short npm)
		npm install
	- run Retroshare with webinterface on port 9090
	- during development, run this command (use MinGW shell on Windows)
		while true; do make ../webfiles/livereload --silent; sleep 1; done
	- the command will copy the source files to libresapi/src/webfiles if they change
	- it will trigger livereload at http://localhost:9090/api/v2/livereload/trigger

API DOCUMENTATION
-----------------

	- run
		node PeersTest.js
	- this will print the expected schema of the api output, and it will try to test it with real data
	- run retroshare-nogui with webinterface enabled on port 9090, to test if the real output of the api matches the expected schema

CONTRIBUTE
----------
	
	- if you are a web developer or want to become one
		get in contact!
	- lots of work to do, i need you!
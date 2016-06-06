A new approach to build a webinterface for RS
=============================================

1. get JSON encoded data from the backend, data contains a state token
2. render data with mithril.js
3. ask the backend if the state token from step 1 expired. If yes, then start again with step 1.

Steps 1. and 3. are common for most things, only Step 2. differs. This allows to re-use code for steps 1. and 3.

BUILD / DEVELOPMENT
------------

	- install tools
		sudo apt-get install TODO (insert package names for nodejs, ruby, sass here)
	- run this once in webui-src directory, to install more tools
		npm install
	- start build
		npm run watch
        - the build process watches files for changes, and rebuilds and reloads the page. Build output is in ./public
	- use the --webinterface 9090 command line parameter to enable webui in retroshare-nogui
	- set the --docroot parameter of retroshare-nogui to point to the "libresapi/src/webui-src/public" directory
		(or symlink from /usr/share/RetroShare06/webui on Linux, ./webui on Windows)
	- retroshare-gui does not have a --docroot parameter. Use symlinks then.

CONTRIBUTE
----------
	
	- if you are a web developer or want to become one
		get in contact!
	- lots of work to do, i need you!

TODO
----
[ ] make stylesheets or find reusable sass/css components
google material design has nice rules for color, spacing and everything: https://www.google.de/design/spec/material-design/introduction.html
[ ] find icons, maybe use google material design iconfont
[X] use urls/mithril routing for the menu. urls could replace state stored in rs.content
[X] drag and drop private key upload and import
[X] link from peer location to chat (use urls and mithril routing)
[X] add/remove friend, own cert
[X] downloads, search
[ ] make reusable infinite list controller, the js part to load data from Pagination.h (tweak Pagination.h to make everything work)
should provide forward, backward and follow-list-end
[ ] backend: view/create identities
[ ] backend: chat lobby participants list
[X] chat: send_message
[ ] backend: chat typing notifications
[ ] make routines to handle retroshare links
[ ] backend: edit shared folders
[ ] backend: view shared files
[ ] redirect if a url is not usable in the current runstate (e.g. redirect from login page to home page, after login)
[X] sort friendslist

need 4 master
-------------
[X] unsubscribe lobby
[X] unread chat message counter in menu
[X] list chat-lobby participants
[X] creating app.js on build (no need for npm on regulary build)

url-handling (brainstorming)
----------------------------
* normal weblinks (bbcode? => only with gui support)
* rslinks
  - files
  - (chatrooms)
  - forum retroshare://forum?name=Developers%27%20Discussions&id=8fd22bd8f99754461e7ba1ca8a727995
  - own cert link (paste)
  - cert-links
  - searches
  - [X] downloads pasten
  - uploads?
* enter / display urls
  - use urls in href like used for input (so it can be copy-link)
  - handle RS-urls with javascript, other with target _blank
* smilies
* Bilder
* KEEP IT SIMPLE

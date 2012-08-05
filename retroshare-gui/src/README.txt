=======================================================================================
README for RetroShare
=======================================================================================

RetroShare web site . . . . http://retroshare.sourceforge.net/
Documentation . . . . . . . http://retroshare.sourceforge.net/doc.html
Support . . . . . . . . . . http://retroshare.sourceforge.net/support.html
Forums  . . . . . . . . . . http://retroshare.sourceforge.net/forum/
Wiki  . . . . . . . . . . . http://retroshare.sourceforge.net/wiki/
The Developers site . . . . http://retroshare.sourceforge.net/developers.html
Project site  . . . . . . . https://sourceforge.net/projects/retroshare

Contact:  . . . . . . . . . retroshare@lunamutt.com ,defnax@users.sourceforge.net


Compiling + Running RetroShare (V0.5.xxx)
=========================================================================================
REQUIREMENTS
=========================================================================================

Libraries/Tools:
	* C/C++ Compiler. 	  	  (standard on Linux/cygwin)
	* Qt >= 4.6.x 		  	http://qt.nokia.com/downloads/downloads#lgpl
	* OpenSSL       	      	http://www.openssl.org/source/openssl-1.0.1c.tar.gz
	* MiniUPnP           	  	http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.3.tar.gz

Windows only:
	* Cygwin  	          	http://www.cygwin.com/cygwin/setup.exe (for compile, openssl, pthreads)
	* MinGW/Msys package  		http://sourceforge.net/projects/mingw/files/(for compile zlib, miniupnc, bzip2)
	* Pthreads            		http://sourceware.org/pthreads-win32/ 
	* Zlib                		http://www.zlib.net/
	* bzip2				http://www.bzip.org/1.0.6/bzip2-1.0.6.tar.gz
	
	Latest RetroShare sources from (SVN) sourceforge.net:	
    	svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare retroshare 

=========================================================================================

Build Scripts are avaible on SVN for Debian and Ubuntu:
http://retroshare.svn.sourceforge.net/viewvc/retroshare/trunk/build_scripts/

Latest stable OpenSSL is available at:
http://www.openssl.org

Latest miniupnpc-1.3 is avaible at:
http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.3.tar.gz


Windows only:
	pthreads: ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-8-0-release.tar.gz 
	zlib:	    http://www.zlib.net/zlib-1.2.3.tar.gz 
=========================================================================================
You can find here instructions howto compile RetroShare:
http://retroshare.sourceforge.net/wiki/index.php/Developers_Corner
	
=========================================================================================	
You can go on over to our forum when you have trouble with compiling:
http://retroshare.sourceforge.net/forum/
---------------------------------------------

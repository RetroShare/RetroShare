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
	* Qt >= 4.5.x 		  	    https://download.qt.io/official_releases/qt/4.8/4.8.7/qt-everywhere-opensource-src-4.8.7.tar.gz
	* OpenSSL       	      	http://www.openssl.org/source/openssl-0.9.8k.tar.gz
	* MiniUPnP           	  	http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.3.tar.gz
	* gpgme 		              ftp://ftp.gnupg.org/gcrypt/gpgme/gpgme-1.1.8.tar.bz2
	* libgpg-error          	ftp://ftp.gnupg.org/gcrypt/libgpg-error/libgpg-error-1.7.tar.bz2 

Windows only:
	* Cygwin  	          	  http://www.cygwin.com/cygwin/setup.exe (for openssl compile)
	* MinGW/Msys package  		http://sourceforge.net/projects/mingw/files/	 (for compile gpgme,libgpg-error)
	* Pthreads            		http://sourceware.org/pthreads-win32/ 
	* Zlib                		http://www.zlib.net/
	
	Latest RetroShare sources from (SVN) sourceforge.net:	
    	svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare retroshare 

=========================================================================================

Build Scripts are avaible on SVN for Debian and Ubuntu:
http://retroshare.svn.sourceforge.net/viewvc/retroshare/trunk/build_scripts/

Latest stable OpenSSL is available at:
http://www.openssl.org

Latest miniupnpc-1.3 is avaible at:
http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.3.tar.gz

Latest gpgme Library is avaible at:
ftp://ftp.gnupg.org/gcrypt/gpgme/gpgme-1.1.8.tar.bz2

Latest libgpg-error Library is avaible at:
ftp://ftp.gnupg.org/gcrypt/libgpg-error/libgpg-error-1.7.tar.bz2

Windows only:
	pthreads: ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-8-0-release.tar.gz 
	zlib:	    http://www.zlib.net/zlib-1.2.3.tar.gz 
=========================================================================================
You can find here instrustions howto compile libretroshare and gui:
http://retroshare.sourceforge.net/wiki/index.php/Developers_Corner
	
=========================================================================================	
You can go on over to our forum when you have trouble with compiling:
http://retroshare.sourceforge.net/forum/
---------------------------------------------

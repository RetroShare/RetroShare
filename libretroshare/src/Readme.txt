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


Compiling + Running RetroShare (V0.4.xxx)
=========================================================================================
REQUIREMENTS
=========================================================================================

Libraries/Tools:
	* C/C++ Compiler. 	  	(standard on Linux/cygwin)
	* OpenSSL-0.9.7g-xpgp 	http://www.lunamutt.com/retroshare/openssl-0.9.7g-xpgp-0.1c.tgz
	* miniupnpc           	http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.0.tar.gz
	* Qt >= 4.3.x 		  	http://trolltech.com/downloads/opensource

RetroShare Source Code: ( from sf.net/projects/retroshare)
	* Qt-GUI-XXX.tgz
	* retroshare-src-v0.4.XXX.tgz 
	
Latest RetroShare sources from (SVN) sourceforge.net:	
    svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare retroshare  

Windows Requirements:
	* Cygwin (Windows Only) 	http://www.cygwin.com/cygwin/setup.exe 
	* Pthreads (Windows Only)   http://sourceware.org/pthreads-win32/ 
	* Zlib (Windows Only)       http://www.zlib.net/
		
------------------------------------------------------------------------------------------
Build Scripts are avaible on SVN for Debian and Ubuntu:
http://retroshare.svn.sourceforge.net/viewvc/retroshare/trunk/build_scripts/

OpenSSL-0.9.7g-xpgp is available at:
http://www.lunamutt.com/retroshare/openssl-0.9.7g-xpgp-0.1c.tgz

miniupnpc-1.0 is avaible at:
http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.0.tar.gz

Linux Source package included this libraries: OpenSSL-0.9.7g-xpgp, miniupnpc-1.0, qcheckers, smplayer:
http://downloads.sourceforge.net/retroshare/retroshare-pkg-linux-src-v0.4.09b.tgz


Download/Compile as per instructions...

------------------------------------------------------------------------------------------
==========================================================================================
Howto Compile under Unix/Linux
==========================================================================================
Directory layout

This article sticks to the following scheme:
Directory 	
~ 	Your home folder
~/src/miniupnpc-X.Y 	miniupnpc directory
~/src/openssl-X-xpgp-Y 	openssl with xpgp patches
~/src/retroshare 	SVN checkout folder
~/src/retroshare/libretroshare 	libretroshare
~/src/retroshare/retroshare-gui 	Qt4 Retroshare GUI
~/lib 	All the libraries required by Retroshare GUI (libssl.a, libcrypto.a, libretroshare.a,...)


Build dependicies
------------------------------------------------------

Needed Packages for compiling Retroshare:

    * libqt4-dev
    * g++ 

$ sudo apt-get install libqt4-dev g++


Build prerequisite libraries
______________________________________________

Build miniupnpc - mini UPnP client
====================================================

If your OS already has miniupnp client as a package, you can skip this step.

Download miniupnp from http://miniupnp.free.fr/. You need the client distribution, not MiniUPNP daemon. Compatibility with different miniupnpc version: 1.0 - official version bundled with RetroShare, 1.2 works but not fully tested yet.

Build

$ cd ~/src
$ wget http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.2.tar.gz
$ tar -xzvf miniupnpc-1.2.tar.gz
$ cd miniupnpc-1.2
$ make

Testing
Test miniupnpc by running upnpc-static utility it builds. If upnpc is able to find your router (UPNP device), it will show a list of mappings on it:

$ ./upnpc-static -l
upnpc : miniupnpc library test client. (c) 2006-2008 Thomas Bernard
...
ExternalIPAddress = 1.2.3.4
0 UDP 40950->192.168.1.3:40950 'Azureus UPnP 50950 UDP' ''
1 TCP 20950->192.168.1.3:20950 'Azureus UPnP 50950 TCP' ''
...

Copy miniupnpc library to the library folder:
--------------------------------------------------

$ cp libminiupnpc.a ~/lib

--------------------------------------------------

Build openssl-xpgp (openssl with xpgp patches)
=================================================

RetroShare uses openssl SSL library with special patches to implement authentication in web of trust. You have choice of two openssl versions, openssl-0.9.7g (old openssl release from year 2005, but extensively tested with RetroShare) or openssl-0.9.8h (up to date but not fully tested).


Building openssl-0.9.7g (option 1)

You can either download patched openssl sources from http://www.lunamutt.com/retroshare/openssl-0.9.7g-xpgp-0.1c.tgz or download patches against official openssl sources from the same site if you prefer to patch openssl yourself).

Build
Important: do not run make install as it may overwrite the system openssl and could break your system

$ cd ~/src
$ wget http://www.lunamutt.com/retroshare/openssl-0.9.7g-xpgp-0.1c.tgz
$ tar -xzvf openssl-0.9.7g-xpgp-0.1c.tgz
$ cd openssl-0.9.7g-xpgp-0.1c
$ ./config --openssldir=/etc/ssl-xpgp no-shared 
$ make
...

Copy libssl and libcrypto to the library folder:
---------------------------------------------------

$ cp libssl.a ~/lib
$ cp libcrypto.a ~/lib


Building openssl-0.9.8h (option 2)
=====================================================

Under construction - do not use this yet

    * Download openssl-0.9.8h from www.openssl.org: http://www.openssl.org/source/openssl-0.9.8h.tar.gz
    * Download new files openssl-0.9.8h-xpgp-0.1c-newfiles.tgz
    * Download openssl-0.9.8h-xpgp-0.1c.patch
    * Untar the new files over openssl distribution
    * Patch openssl with xpgp patches: patch -p0 < openssl-0.9.8h-xpgp-0.1c.patch
    * Configure: ./config --openssldir=/etc/ssl-xpgp no-shared
          o no-shared ensures that only static (.a) libraries are built 

    * Run test of compiled openssl-xpgp: 

cd test ../util/shlib_wrap.sh ssltest


Build qcheckers game as a library
===================================

RetroShare includes certain games as a demo of network gaming. Unfortunately, you must compile games even if you don't plan to play them. There's currently no way to compile retroshare without games.

Obtain qcheckers patched source The easiest way is to download retroshare source bundle and extract only qcheckers patched source.

$ wget http://downloads.sourceforge.net/retroshare/retroshare-pkg-linux-src-v0.4.09b.tgz
$ tar -xzvf retroshare-pkg-linux-src-v0.4.09b.tgz "*qcheckers-svn14.tgz"
$ mv retroshare-package-v0.4.09b/tar/qcheckers-svn14.tgz ~/src
$ tar -xzvf qcheckers-svn14.tgz


Build
-----------------

$ cd ~/src/qcheckers-svn14
$ qmake
$ make
...
$ cp src/libqcheckers.a ~/lib


Build smplayer as a library
=================================

Obtain smplayer patched source The easiest way is to download retroshare source bundle and extract smplayer patched source only.

$ wget http://downloads.sourceforge.net/retroshare/retroshare-pkg-linux-src-v0.4.09b.tgz
$ tar xzvf retroshare-pkg-linux-src-v0.4.09b.tgz "*smplayer-svn-280308.tgz"
$ mv retroshare-package-v0.4.09b/tar/smplayer-svn-280308.tgz ~/src
$ tar -xzvf smplayer-svn-280308.tgz


Build
----------------------------------

$ cd ~/src/smplayer
$ qmake
$ make
...
$ cp lib/libsmplayer.a ~/lib


Build RetroShare
===================================================
Obtain latest retroshare sources from SVN

Source release is quite old at the moment and it is a good idea to obtain sources from subversion:

$ cd ~/src
$ svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk retroshare
...
Checked out revision 777.

This should create ~/src/retroshare/libretroshare, ~/src/retroshare/retroshare-gui etc.
Build libretroshare

Edit configuration files

    * Edit ~/src/retroshare/libretroshare/src/scripts/config.mk:
          o Change OS to be "OS = Linux" 

    * Edit ~/src/retroshare/libretroshare/src/scripts/config-linux.mk:
          o Change SSL_DIR to point to your openssl-xpgp directory. Check that SSL_DIR really points to openssl-xpgp folder, because it's easy to have one "../" too much: 

$ cd scripts            <-- I'm in scripts folder
$ ls ../../../../../src/openssl-0.9.7g-xpgp-0.1c          <-- that's my SSL_DIR
CHANGES         INSTALL.W32   README         demos        makevms.com   test
...

    *
          o Change UPNPC_DIR to point to your miniupnp directory
          o Change DEFINES += -DMINIUPNPC_VERSION=12 to match your miniupnpc version (10 for 1.0 or 12 for 1.2) 


Build
-------------------------

$ cd ~/src/retroshare/libretroshare/src
$ make
..
$  cp lib/libretroshare.a ~/lib

Troubleshooting
----------------------
You get miniupnpc errors, "error: too few arguments to function 'UPNPDev* upnpDiscover(int, const char*, const char*, int)'"
Possible cause: you're using miniupnpc version 1.2 but config-linux.mk says 1.0


Build retroshare GUI
===========================================================

Check
Check that all the libraries are compiled and path to libraries folder in RetroShare.pro is correct:

$ cd ~/src/retroshare/retroshare-gui/src
$ ls ../../../../lib   <--- that's my -L path from RetroShare.pro
libcrypto.a     libqcheckers.a   libsmplayer.a
libminiupnpc.a  libretroshare.a  libssl.a

If some libraries are not there, you probably forgot to copy them. If path to lib is wrong, correct it in RetroShare.pro.

Build Run
------------------------

$ cd ~/src/retroshare/retroshare-gui/src
$ qmake
$ make


Troubleshooting
Most common troubles are missing includes

    * if gcc says "find" is unknown (no matching function for call to 'find(std::_List_iterator...), add #include <algorithm>
    * if std::bad_cast is unknown, add #include <typeinfo>
    * If on the link stage you receive lot of undefined reference errors ("undefined reference to `generate_xpgp' etc), all mentioning pgp: it's likely that you link to standard libssl.a and libcrypto.a instead of openssl-xpgp versions. This can happen if libraries are not found: linker silently uses standard libraries instead, because openssl is installed on all systems. 


If compilation was successful, you should be able to start RetroShare executable:

$ ./RetroShare



------------------------------------------------------------------------------------------

Compiling the skinobject (only Required when its enabled then compile with QT 4.3.x)
------------------------------------------------------------------------------------------


1. Download skinobject from https://sourceforge.net/projects/qskinwindows/

http://downloads.sourceforge.net/qskinwindows/qskinobject-0.6.1.tar.bz2?

2. untar the qskinobject-0.6.1.tar.bz2

3. run: 
qmake
make

4.Copy the libskin.a to your retroshare libs directory.

5.then add to LIBS= -lskin -lgdi32 to the RetroShare.pro file:

example(linux):

TARGET = RetroShare 
RSLIBS = -L/path/to/your/retroshare/libs/directory/ -lretroshare -lminiupnpc -lskin -lssl -lcrypto 
LIBS = $(RSLIBS) 

example (Windows):

win32 
{
    RC_FILE = gui/images/retroshare_win.rc

    "LIBS += -L"../../winlibs" -lretroshare -lssl -lcrypto -lpthreadGC2d -lminiupnpc -lz -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32-cygwin -lskin -lgdi32
    CONFIG += qt release"
}

------------------------------------------------------------------------------------------

Compiling QCheckers game
------------------------------------------------------------------------------------------

1.qmake
2.make
3.Copy the libqcheckers.a to your retroshare libs directory.
5.then add to LIBS= -lqcheckers to the RetroShare.pro file:

Example(linux):

RSLIBS = -L/path/to/your/retroshare/libs/directory/ -lretroshare -lminiupnpc -lskin -lqcheckers -lssl -lcrypto 
LIBS = $(RSLIBS) 

Example (Windows):

win32 
{
    RC_FILE = gui/images/retroshare_win.rc

    "LIBS += -L"../../winlibs" -lretroshare -lssl -lcrypto -lpthreadGC2d  -lminiupnpc -lz -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32-cygwin -lskin -lqcheckers -lgdi32
    CONFIG += qt release"
}


------------------------------------------------------------------------------------------

Compiling SMPlayer
------------------------------------------------------------------------------------------

1.qmake
2.make
3.Copy the libsmplayer.a to your retroshare libs directory.
5.then add to LIBS  -lsmplayer to the RetroShare.pro file.


WIN XP Compilation.
------------------------------------------------

This much harder, and more perilous than the
Linux compilation: It requires both the cygwin 
and the mingw compilers... 
 
Need: 
	Cygwin development environment: 
	http://www.cygwin.com/cygwin/setup.exe 
 
	Qt4.3.x opensource development kit + MinGw: 
	http://wftp.tu-chemnitz.de/pub/Qt/qt/source/qt-win-opensource-4.3.5-mingw.exe  
 
	source code for all libraries.: 
	http://downloads.sourceforge.net/retroshare/retroshare-pkg-linux-src-v0.4.09b.tgz? 
 
	retroshare-pkg-linux-src-v0.4.09b.tgz are Libraries included: 
 
	openssl-0.9.7g-xpgp-0.1c.tgz 
	miniupnpc-1.0.tar.gz 
	smplayer-svn-280308.tgz 
 
	Libraries for Windows needs: 
	pthreads: 	http://sourceware.org/pthreads-win32/ 
	zlib: 		http://www.zlib.net/ 
 
 
In Brief: 
UNDER Cygwin: 
	(1) Compile openssl-xpgp. 
	(2) Compile miniunpnc 
	(3) Compile pthreads. 
	(4) Compile zlib. 
 
	(5) Compile libretroshare retroshare-v0.4.x 
 
UNDER Mingw:
  
    (6) Compile QCheckers    ( qmake + make ) 
	(7) Compile SMPlayer    ( qmake + make ) 
	(8) Compile the Qt-Gui. ( qmake + make )


Email me if you're having trouble:
	retroshare@lunamutt.com
	
You can go on ouer forum when you have trouble with compiling:
http://retroshare.sourceforge.net/forum/
---------------------------------------------







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
	* C/C++ Compiler. 	  	(standard on Linux/cygwin)
  * Qt >= 4.5.x 		  	  http://trolltech.com/downloads/opensource
	* OpenSSL       	      http://www.openssl.org/source/openssl-0.9.8k.tar.gz
	* MiniUPnP           	  http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.3.tar.gz
	* gpgme 			          ftp://ftp.gnupg.org/gcrypt/gpgme/gpgme-1.1.8.tar.bz2
	* libgpg-error          ftp://ftp.gnupg.org/gcrypt/libgpg-error/libgpg-error-1.7.tar.bz2 

Windows only:
	* Cygwin  	          http://www.cygwin.com/cygwin/setup.exe (for openssl compile)
	* MinGW/Msys package  http://sourceforge.net/projects/mingw/files/	 (for compile gpgme,libgpg-error)
	* Pthreads            http://sourceware.org/pthreads-win32/ 
	* Zlib                http://www.zlib.net/
	
	Latest RetroShare sources from (SVN) sourceforge.net:	
    svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare retroshare 

		
------------------------------------------------------------------------------------------
Build Scripts are avaible on SVN for Debian and Ubuntu:
http://retroshare.svn.sourceforge.net/viewvc/retroshare/trunk/build_scripts/

Latest stable OpenSSL is available at:
http://www.openssl.org/source/openssl-0.9.8k.tar.gz

Latest miniupnpc-1.3 is avaible at:
http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.3.tar.gz

Latest gpgme Library is avaible at:
ftp://ftp.gnupg.org/gcrypt/gpgme/gpgme-1.1.8.tar.bz2

Latest libgpg-error Library is avaible at:
ftp://ftp.gnupg.org/gcrypt/libgpg-error/libgpg-error-1.7.tar.bz2

Windows only:
	pthreads:   ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-8-0-release.tar.gz 
	zlib: 		  http://www.zlib.net/zlib-1.2.3.tar.gz 

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


Build RetroShare
===================================================
Obtain latest retroshare sources from SVN

Source release is quite old at the moment and it is a good idea to obtain sources from subversion:

$ cd ~/src
$ svn co https://retroshare.svn.sourceforge.net/svnroot/retroshare/trunk retroshare
...
Checked out revision 1150.

This should create ~/src/retroshare/libretroshare, ~/src/retroshare/retroshare-gui etc.
Build libretroshare

Change SSL_DIR to point to your openssl-xpgp directory. 
Check that SSL_DIR really points to openssl-xpgp folder, because it's easy to have one "../" too much: 

libtretroshare.pro:

################################### COMMON stuff ##################################

DEFINES *=  PQI_USE_XPGP MINIUPNPC_VERSION=10

SSL_DIR=../../../../openssl-0.9.7g-xpgp-0.1c
UPNPC_DIR=../../../../miniupnpc-1.0

INCLUDEPATH += . $${SSL_DIR}/include $${UPNPC_DIR}


  o Change UPNPC_DIR to point to your miniupnp directory
  o Change DEFINES += -DMINIUPNPC_VERSION=12 to match your miniupnpc version (10 for 1.0 or 12 for 1.2) 

64bit Notice:
  o If youre using a 64bit Linux you must remove the -marchi=686 option in this line: 
  BIOCFLAGS = -I $(SSL_DIR)/include ${DEFINES} -DOPENSSL_THREADS -D_REENTRANT -DDSO_DLFCN -DHAVE_DLFCN_H -DOPENSSL_NO_KRB5 -DL_ENDIAN -DTERMIO -O3 -fomit-frame-pointer -march=i686 -Wall -DSHA1_ASM -DMD5_ASM -DRMD160_ASM 


Build
-------------------------

$ cd ~/src/retroshare/libretroshare/src
$ qmake
$ make
..
$  cp lib/libretroshare.a ~/lib

Troubleshooting
----------------------
You get miniupnpc errors, "error: too few arguments to function 'UPNPDev* upnpDiscover(int, const char*, const char*, int)'"
Possible cause: you're using miniupnpc version 1.2 but config-linux.mk says 1.0

Then edit in "libretroshare/src/upnp/upnputil.c" and go on line 150 and Edit to this one:
eport, iport, iaddr, 0, proto); 


Build retroshare GUI
===========================================================

Check
Check that all the libraries are compiled and path to libraries folder in RetroShare.pro is correct:

$ cd ~/src/retroshare/retroshare-gui/src
$ ls ../../../../lib   <--- that's my -L path from RetroShare.pro
libcrypto.a
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




WIN XP Compilation.
------------------------------------------------

This much harder, and more perilous than the
Linux compilation: It requires both the cygwin 
and the mingw compilers... 
 
Need: 
	Cygwin development environment: 
	http://www.cygwin.com/cygwin/setup.exe 
 
	Qt4.5.x opensource development kit + MinGw: 
	http://get.qtsoftware.com/qtsdk/qt-sdk-win-opensource-2009.03.1.exe 
	
	MINGW and MSYS:
	http://downloads.sourceforge.net/mingw/MinGW-5.1.4.exe
	http://downloads.sourceforge.net/mingw/MSYS-1.0.11.exe
	
	miniupnpc
	http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.3.tar.gz
	
	gpgme Library 
	ftp://ftp.gnupg.org/gcrypt/gpgme/gpgme-1.1.8.tar.bz2

	libgpg-error Library
	ftp://ftp.gnupg.org/gcrypt/libgpg-error/libgpg-error-1.7.tar.bz2

	pthreads:   ftp://sourceware.org/pub/pthreads-win32/pthreads-w32-2-8-0-release.tar.gz 
	zlib: 		  http://www.zlib.net/zlib-1.2.3.tar.gz 
 
 
In Brief: 
UNDER Cygwin: 
	(1) Compile openssl. (Configure mingw + make )
	(2) Compile pthreads. 
	(3) Compile zlib. 
 
	(4) Compile libretroshare retroshare-v0.5.x 
 
UNDER MinGW/MSYS Bash shell:
  (5) Compile gpg-error (configure + make + make install)
  (6) Compile gpgme     (configure + make + make install)
   
UNDER MinGW:
  (7) Compile miniunpnc  (mingw32make.bat)
	(8) Compile the Qt-Gui. ( qmake + make )


Email me if you're having trouble:
	retroshare@lunamutt.com
	
	
You can go on ouer forum when you have trouble with compiling:
http://retroshare.sourceforge.net/forum/
---------------------------------------------







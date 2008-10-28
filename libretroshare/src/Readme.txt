
Compiling + Running RetroShare (V0.4.xxx)
------------------------------------------------------------------------------------------

Quick Requirements:
------------------------------------------------------------------------------------------
Libraries/Tools:
	C/C++ Compiler. (standard on Linux/cygwin)
	OpenSSL-0.9.7g-xpgp
	Qt-4.3 development libraries.

RetroShare Source Code: ( from sf.net/projects/retroshare)
	Qt-GUI-XXX.tgz
	retroshare-src-v0.4.XXX.tgz 

Windows Requirements:
	Cygwin (Windows Only)
	Pthreads (Windows Only)
	Zlib (Windows Only)
		
------------------------------------------------------------------------------------------
Build Scripts are avaible on SVN for Debian and Ubuntu:
http://retroshare.svn.sourceforge.net/viewvc/retroshare/trunk/build_scripts/


OpenSSL-0.9.7g-xpgp is available at:
http://www.lunamutt.com/retroshare/openssl-0.9.7g-xpgp-0.1c.tgz


Download/Compile as per instructions...

------------------------------------------------------------------------------------------

Compiling Linux
------------------------------------------------------------------------------------------

(1) compile openSSL-0.9.7g-xpgp.

(2) compile miniupnpc

(4) Modify ./make.opts 
	(4a) modify the Makefile so that:  OS=Linux or OS=Win
	(4c) Define SSL_DIR to point to openSSL-0.9.7g-xpgp.
	(4c) Define UPNPC_DIR to point to miniupnpc

(5) type: make
	This builds ./lib/libretroshare.a,
	and the various test programs.

	There is server-only (no GUI) executable
	compiled in ./rsiface/retroshare-nogui, 
	you can run this to check that its working.

------------------------------------------------------------------------------------------

Compiling Linux (Alternative Instructions from Bharath)
------------------------------------------------------------------------------------------
here's how to compiled retroshare on ubuntu linux: 
 
 compile openssl: 
 1. Get the patched version of openssl (openssl-0.9.7g-xpgp, from http://www.lunamutt.com) 
 2. run: 
 ./config  
 make 
 make test 
  
 compile miniupnpc: 
 1. Get miniupnpc library from http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.0.tar.gz  
 2. run: 
 make 
   
 install packages needed for retroshare compile:  
 sudo apt-get install libxft-dev 
 sudo apt-get install libXinerama-dev 
    
 complile retroshare: 
 1. set directories in make.opt: 
 RS_DIR=/home/dev/rs-v0.3.0-pr8/src  
 SSL_DIR=/home/dev/openssl-0.9.7g-xpgp-0.1c 
 UPNPC_DIR=/home/dev/miniupnpc
  
 2. comment out the directory declarations uncer Cygwin since that will override your directory declarations from 1. 
 3. change RSLIBS = -L$(LIBDIR) -lretroshare -L$(SSL_DIR) -lssl -lcrypto -lpthread -lminiupnpc  
    to 
  RSLIBS = -L$(LIBDIR) -lretroshare -L$(SSL_DIR) -lssl -lcrypto -lpthread -L$(UPNPC_DIR) -lminiupnpc 
 4. run: 
     make 
      
Hope this helps.


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


------------------------------------------------------------------------------------------

Compiling the Qt GUI
------------------------------------------------------------------------------------------
(1) untar the Qt-GUI source package. run qmake, 

	tar -xvzf Qt-GUI-XXXX.tgz

	cd Qt-Gui-XXX/src/

	qmake-qt4 Retroshare.pro

(2) tweak the makefile: The default makefile
	doesn't have the links to the retroshare
	libraries. It should something like this:

RSLIBS  = -L/home/dev/prog/devel/rs-v0.3.0XXX/src/lib -lretroshare -lminiupnpc
SSLLIBS = -L/home/dev/prog/devel/openssl-0.9.7g-xpgp -lssl -lcrypto
LIBS          = $(SUBLIBS)  $(RSLIBS) $(SSLLIBS)  -L/usr/lib -lQtXml -lQtGui -lQtNetwork -lQtCore -lpthread

	This should build you an executable:

	RetroShare.

------------------------------------------------
This has been compiled on the following platforms:
	(a) Debian Linux (stable/testing/unstable)
	(b) Suse Linux   (9.X/10.X)
	(c) WinXP

------------------------------------------------
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
 
	(5) Compile retroshare-v0.4.x 
 
UNDER Mingw: 
	(6) Compile SMPlayer    ( qmake + make ) 
	(7) Compile the Qt-Gui. ( qmake + make )


Email me if you're having trouble:
	retroshare@lunamutt.com
---------------------------------------------




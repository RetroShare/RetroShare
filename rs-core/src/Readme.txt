
Compiling + Running RetroShare (V0.3.0)
-------------------------------------------------------------

Quick Requirements:
---------------------------------------------
Libraries/Tools:
	C/C++ Compiler. (standard on Linux/cygwin)
	OpenSSL-0.9.7g-xpgp
	KadC Dht library
	Qt-4.2 development libraries.

RetroShare Source Code: ( from sf.net/projects/retroshare)
	Qt-GUI-XXX.tgz
	retroshare-src-v0.3.XXX.tgz 

Windows Requirements:
	Cygwin (Windows Only)
	Pthreads (Windows Only)
	Zlib (Windows Only)
---------------------------------------------

OpenSSL-0.9.7g-xpgp is available at:
http://www.lunamutt.com/retroshare/openssl-0.9.7g-xpgp-0.1c.tgz

KadC (latest) is available from sourceforge.net

Download/Compile as per instructions...

---------------------------------------------

Compiling Linux
---------------------------------------------

(1) compile openSSL-0.9.7g-xpgp.

(2) compile KadC. (and correct the library)

(4) Modify ./make.opts 
	(4a) modify the Makefile so that:  OS=Linux or OS=Win
	(4c) Define SSL_DIR to point to openSSL-0.9.7g-xpgp.
	(4c) Define KADC_DIR to point to KadC

(5) type: make
	This builds ./lib/libretroshare.a,
	and the various test programs.

	There is server-only (no GUI) executable
	compiled in ./rsiface/retroshare-nogui, 
	you can run this to check that its working.

---------------------------------------------

Compiling Linux (Alternative Instructions from Bharath)
---------------------------------------------
here's how to compiled retroshare on ubuntu linux: 
 
 compile openssl: 
 1. Get the patched version of openssl (openssl-0.9.7g-xpgp, from http://www.lunamutt.com) 
 2. run: 
 ./config  
 make 
 make test 
  
 compile KadC: 
 1. Get KadC library from http://kadc.sourceforge.net/  
 2. run: 
 make 
   
 install packages needed for retroshare compile:  
 sudo apt-get install libxft-dev 
 sudo apt-get install libXinerama-dev 
    
 complile retroshare: 
 1. set directories in make.opt: 
 RS_DIR=/home/dev/rs-v0.3.0-pr8/src  
 SSL_DIR=/home/dev/openssl-0.9.7g-xpgp-0.1c 
 KADC_DIR=/home/dev/KadC 
 2. comment out the directory declarations uncer Cygwin since that will override your directory declarations from 1. 
 3. change RSLIBS = -L$(LIBDIR) -lretroshare -L$(SSL_DIR) -lssl -lcrypto -lpthread -lKadC  
    to 
  RSLIBS = -L$(LIBDIR) -lretroshare -L$(SSL_DIR) -lssl -lcrypto -lpthread -L$(KADC_DIR) -lKadC 
 4. run: 
     make 
      
Hope this helps.

---------------------------------------------
Compiling the Qt GUI
_____________________________________________

(1) untar the Qt-GUI source package. run qmake, 

	tar -xvzf Qt-GUI-XXXX.tgz

	cd Qt-Gui-XXX/src/

	qmake-qt4 Retroshare.pro

(2) tweak the makefile: The default makefile
	doesn't have the links to the retroshare
	libraries. It should something like this:

RSLIBS  = -L/home/dev/prog/devel/rs-v0.3.0XXX/src/lib -lretroshare -lKadC
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
	Cygwin development environment
	Qt4.2 opensource development kit + MinGw.
	source code for all libraries.

In Brief:
UNDER Cygwin:
	(1) Compile openssl-xpgp.
	(2) Compile pthreads.
	(3) Compile zlib.
	(4) Compile KadC. (there are some tweaks, 
		needed to the code)

	(5) Compile retroshare-v0.3.0

UNDER Mingw:
	(6) Compile the Qt-Gui.


Email me if you're having trouble:
	retroshare@lunamutt.com
---------------------------------------------




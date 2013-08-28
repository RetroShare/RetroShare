To compile:

	- install the package dependencies. On ubuntu:
		# sudo apt-get install libglib2.0-dev libupnp-dev qt4-dev-tools libqt4-dev libssl-dev libxss-dev \
		       libgnome-keyring-dev libbz2-dev libqt4-opengl-dev libqtmultimediakit1 qtmobility-dev      \
			   libspeex-dev libspeexdsp-dev libxslt1-dev libprotobuf-dev protobuf-compiler cmake         \
			   libcurl4-openssl-dev

	- create project directory (e.g. ~/retroshare) and check out the source code
		# mkdir ~/retroshare
		# cd ~/retroshare && svn co svn://svn.code.sf.net/p/retroshare/code/trunk trunk

	- create a new directory named lib
		# mkdir lib

	- get source code for libssh-0.5.4, unzip it, and create build directory (if needed) 

		# cd lib
		# wget https://red.libssh.org/attachments/download/41/libssh-0.5.4.tar.gz
		# tar zxvf libssh-0.5.4.tar.gz
		# cd libssh-0.5.4
		# mkdir build
		# cd build
		# cmake -DWITH_STATIC_LIB=ON ..
		# make
		# cd ../../..

		NB: There is a new libssh-0.6.0rc1 which fixes some bugs from v0.5.4, 
		The procedure is the same as above, except for the following line. 
		# cmake -DWITH_STATIC_LIB=ON -DWITH_GSSAPI=OFF ..

	- get source code for sqlcipher, and build it (only needed for GXS) 

		# cd lib
		# git clone git://github.com/sqlcipher/sqlcipher.git
		# cd sqlcipher
		# ./configure --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC" \
		  LDFLAGS="-lcrypto"
		# make
		# cd ..

	- go to your svn base directory
		# cd trunk

	- go to libbitdht and compile it
		# cd libbitdht/src && qmake && make clean && make -j 4 

	- go to openpgpsdk and compile it
		# cd ../../openpgpsdk/src && qmake && make clean && make -j 4

	- go to supportlibs and compile it
		# cd ../../supportlibs/pegmarkdown && qmake && make clean && make -j 4

	- go to libretroshare and compile it
		# cd ../../libretroshare/src && qmake && make clean && make -j 4

	- go to rsctrl and compile it
		# cd ../../rsctrl/src && make &&

	- go to retroshare-nogui, and compile it
		# cd ../../retroshare-nogui/src && qmake && make clean && make -j 4

	- go to retroshare gui and compile it
		# cd ../../retroshare-gui/src && qmake && make clean && make -j 4

	- to use the SSH RS server (nogui):

		# ssh-keygen -t rsa -f rs_ssh_host_rsa_key					# this makes a RSA
		# ./retroshare-nogui -G										# generates a login+passwd hash for the RSA key used.
		# ./retroshare-nogui -S 7022 -U[SSLid] -P [Passwd hash]

	- to connect from a remote place to the server by SSH:

		# ssh -T -p 7022 [user]@[host]

		and use the command line interface to control your RS instance.

List of non backward compatible changes for V0.6:
================================================

- in rscertificate.cc, enable V_06_USE_CHECKSUM
- in p3charservice, remove all usage of _deprecated items
- turn file transfer into a service. Will break item IDs, but cleanup and
  simplify lots of code.


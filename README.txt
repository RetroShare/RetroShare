To compile:

	- install the package dependencies. On ubuntu:
		# sudo apt-get install libglib2.0-dev libupnp-dev qt4-dev-tools libqt4-dev libssl-dev libxss-dev \
		       libgnome-keyring-dev libbz2-dev libqt4-opengl-dev libqtmultimediakit1 qtmobility-dev      \
			   libspeex-dev libspeexdsp-dev libxslt1-dev libprotobuf-dev protobuf-compiler cmake         \
			   libcurl4-openssl-dev

	- get source code for libssh-0.5.4, unzip it, and create build directory (if needed) 

		# wget https://red.libssh.org/attachments/download/41/libssh-0.5.4.tar.gz
		# tar zxvf libssh-0.5.4.tar.gz
		# cd libssh-0.5.4
		# mkdir build
		# cd build
		# cmake -DWITH_STATIC_LIB=ON ..
		# make
		# cd ../..
	
	- go to retroshare-nogui, and compile it

		# cd ../../retroshare-nogui
		# qmake
		# make

	- to use the SSH RS server:
		
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


To compile:

	- get source code for libssh-0.5.2, unzip it, and create build directory (if needed) 

		# wget http://www.libssh.org/files/0.5/libssh-0.5.2.tar.gz
		# tar zxvf libssh-0.5.2.tar.gz
		# cd libssh-0.5.2
		# mkdir build
		# cd build
		# cmake -DWITH_STATIC_LIB=ON ..
		# make
		# cd ../..
	
	- build the google proto files

		# cd rsctrl/src
		# make

		Don't bother about python related errors.

	- go to retroshare-nogui, and compile it

		# cd ../../retroshare-nogui
		# qmake
		# make

	- to use the SSH RS server:
		
		# ssh-keygen -t rsa -f rs_ssh_host_rsa_key			# this makes a RSA
		# ./retroshare-nogui -G										# generates a login+passwd hash for the RSA key used.
		# ./retroshare-nogui -S 7022 -U[SSLid] -P [Passwd hash]

	- to connect from a remote place to the server by SSH:

		# ssh -T -p 7022 [user]@[host]

		and use the command line interface to control your RS instance.

List of non backward compatible changes for V0.6:
================================================

- in rscertificate.cc, enable V_06_USE_CHECKSUM
- in p3charservice, remove all usage of _deprecated items

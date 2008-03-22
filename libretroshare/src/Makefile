# FLTKGUI is not required for the Qt4,0 gui.
#

all: librs tests

librs:
	make -C	util librs
	make -C tcponudp librs
	make -C	serialiser librs
	make -C pqi librs
	make -C	dbase librs
	make -C	services librs
	make -C	dht librs
	make -C	upnp librs
	make -C	server librs
	make -C	rsserver librs

# 	make -C	rsiface librs

tests:
	make -C	util tests
	make -C tcponudp tests
	make -C	serialiser tests
	make -C pqi tests
	make -C	dbase tests
	make -C	services tests
	make -C	dht tests
	make -C	upnp tests
	make -C	server tests
	make -C	rsserver tests
	make -C	rsiface tests

clean:
	make -C	util clean
	make -C tcponudp clean
	make -C	serialiser clean
	make -C pqi clean
	make -C	dbase clean
	make -C	services clean
	make -C	dht clean
	make -C	upnp clean
	make -C	server clean
	make -C	rsserver clean
	make -C	rsiface clean

clobber:
	make -C	util clobber
	make -C tcponudp clobber
	make -C	serialiser clobber
	make -C pqi clobber
	make -C	dbase clobber
	make -C	services clobber
	make -C	dht clobber
	make -C	upnp clobber
	make -C	server clobber
	make -C	rsserver clobber
	make -C	rsiface clobber


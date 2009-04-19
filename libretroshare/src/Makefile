RS_TOP_DIR = .
###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

all: librs
# all: librs tests
# removed tests, because test programs have not been maintained since we switched to qmake for compiling libretroshare. 

librs:
	make -C	util librs
	make -C tcponudp librs
	make -C	serialiser librs
	make -C pqi librs
	make -C	dbase librs
	make -C	services librs
	make -C	dht librs
	make -C	upnp librs
	make -C	ft librs
	make -C	rsserver librs

#	make -C	server librs
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
	make -C	ft tests
	make -C	rsserver tests
	make -C	rsiface tests

#	make -C	server tests

clean:
	make -C	util clean
	make -C tcponudp clean
	make -C	serialiser clean
	make -C pqi clean
	make -C	dbase clean
	make -C	services clean
	make -C	dht clean
	make -C	upnp clean
	make -C	ft clean
	make -C	rsserver clean
	make -C	rsiface clean

#	make -C	server clean

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
	make -C	ft clobber
	make -C	rsserver clobber
	make -C	rsiface clobber
	-$(RM) lib/libretroshare.a

#	make -C	server clobber

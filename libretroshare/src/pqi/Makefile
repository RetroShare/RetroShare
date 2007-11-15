
RS_TOP_DIR = ..
include ../make.opt

# All the executables that should be generated.
EXECS = # test_p3items

BASE_OBJ = pqi_base.o pqi.o pqipacket.o \
	pqidebug.o pqisecurity.o pqinetwork.o \
	pqistreamer.o pqibin.o pqimon.o
	
TUNN_OBJ = pqitunnel.o pqitunneltst.o
LOOP_OBJ = pqiloopback.o p3loopback.o
DISC_OBJ = discItem.o p3disc.o

ifdef PQI_USE_XPGP
	SSL_OBJ = xpgpcert.o 
# Disabled for release...	EXECS += p3supernode 
else
	SSL_OBJ  = sslcert.o  
endif

SSL_OBJ  += pqissl.o pqiarchive.o pqissllistener.o

GRP_OBJ = pqiperson.o pqihandler.o pqipersongrp.o  


UDP_OBJ  = pqistunner.o pqiudpproxy.o pqissludp.o pqitunnelproxyudp.o

PRXY_OBJ = pqiproxy.o  pqitunnelproxy.o
CHAN_OBJ = p3channel.o pqichannel.o
SN_OBJ = pqisupernode.o

OBJ = $(BASE_OBJ) $(TUNN_OBJ) $(LOOP_OBJ) $(DISC_OBJ) \
	$(GRP_OBJ) $(SSL_OBJ) $(CHAN_OBJ) $(PRXY_OBJ) $(UDP_OBJ) 

# Linux only parts.
ifeq ($(OS),Linux)
	OBJ += $(SN_OBJ)
endif
	
BASE_HDR =   pqi_base.h pqi.h pqi_data.h  pqipacket.h pqisecurity.h pqinetwork.h \
		pqidebug.h pqistreamer.h pqiarchive.h  pqiindic.h pqibin.h
TUNN_HDR = pqitunnel.h  pqitunneltst.h
LOOP_HDR = p3loopback.h pqiloopback.h 
DISC_HDR = discItem.h p3disc.h
SSL_HDR  = pqissl.h pqissllistener.h xpgpcert.h 
GPR_HDR  = pqihandler.h pqiperson.h pqipersongrp.h
UDP_HDR  = pqistunner.h pqiudpproxy.h pqissludp.h pqitunnelproxyudp.h 
PRXY_HDR = pqitunnelproxy.h pqiproxy.h
CHAN_HDR = pqichannel.h p3channel.h   
SN_HDR   = pqisupernode.h 

HDR = $(BASE_HDR) $(TUNN_HDR) $(LOOP_HDR) $(DISC_HDR) \
	$(GRP_HDR) $(SSL_HDR) $(CHAN_HDR) $(PRXY_HDR) $(UDP_HDR) 

LIBPQIA = $(LIBDIR)/libpqi.a
LIBPQISO = $(LIBDIR)/libpqi.so

# decide on output format...... Lib for Unix, Objs for windows,
ifeq ($(OS),Linux)

all : librs $(EXECS) 

else

all : librs

endif

librs:  $(OBJ)
	$(AR) r $(LIBRS) $(OBJ)
	$(RANLIB) $(LIBRS)

$(OBJ) : $(HDR)

p3supernode: p3supernode.o librs
	$(CC) $(CFLAGS) -o p3supernode p3supernode.o $(RSLIBS)

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-/bin/rm $(OBJ) p3supernode.o

clobber: clean
	-/bin/rm $(EXECS) $(LIBPQIA) $(LIBPQISO) 

	

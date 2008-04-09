
RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

BASE_OBJ = pqidebug.o pqisecurity.o pqinetwork.o 
#pqi_base.o 

LOOP_OBJ = pqiloopback.o 
STREAM_OBJ = pqistreamer.o pqiarchive.o pqibin.o
MGR_OBJ = pqimonitor.o p3dhtmgr.o p3connmgr.o p3cfgmgr.o p3authmgr.o
GRP_OBJ = pqiperson.o pqihandler.o pqiservice.o pqipersongrp.o  

ifdef PQI_USE_XPGP
	SSL_OBJ = authxpgp.o   
else
	SSL_OBJ  = sslcert.o  
endif

SSL_OBJ  += pqissl.o pqissllistener.o pqisslpersongrp.o
UDP_OBJ  = pqissludp.o

OTHER_OBJ = p3notify.o

RSOBJ = $(BASE_OBJ) $(LOOP_OBJ) \
	$(STREAM_OBJ) \
	$(MGR_OBJ) \
	$(SSL_OBJ) \
	$(UDP_OBJ) \
	$(GRP_OBJ) \
	$(OTHER_OBJ)

TESTOBJ  = xpgp_id.o net_test.o dht_test.o net_test1.o
#conn_test.o

TESTS  = xpgp_id net_test dht_test net_test1
#conn_test


all: librs tests

xpgp_id: xpgp_id.o 
	$(CC) $(CFLAGS) -o xpgp_id xpgp_id.o $(LIBS)

dht_test: dht_test.o 
	$(CC) $(CFLAGS) -o dht_test dht_test.o $(LIBS)

conn_test: conn_test.o 
	$(CC) $(CFLAGS) -o conn_test conn_test.o $(LIBS)

net_test: net_test.o 
	$(CC) $(CFLAGS) -o net_test net_test.o $(LIBS)

net_test1: net_test1.o 
	$(CC) $(CFLAGS) -o net_test1 net_test1.o $(LIBS)

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################


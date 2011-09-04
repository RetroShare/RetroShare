
RS_TOP_DIR = ../..
DHT_TOP_DIR = ../../../../libbitdht/src/

##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/tests/scripts/config.mk
###############################################################

# Generic Test Harnesses.
TESTOBJ  = conn_harness.o ppg_harness.o

TESTOBJ  += pqiqos_test.o 
TESTOBJ  += net_test.o dht_test.o net_test1.o netiface_test.o dht_test.o 
TESTOBJ  += pkt_test.o pqiarchive_test.o pqiperson_test.o
TESTOBJ  += extaddrfinder_test.o dnsresolver_test.o pqiipset_test.o
TESTOBJ  += p3connmgr_reset_test.o p3connmgr_connect_test.o 
#conn_test.o 

TESTS  = net_test net_test1 netiface_test pqiarchive_test pqiperson_test dnsresolver_test extaddrfinder_test
TESTS  += pqiipset_test 
TESTS  += pqiqos_test 
TESTS  += p3connmgr_reset_test p3connmgr_connect_test
#TESTS  = p3connmgr_test1

MANUAL_TESTS = dht_test 
#conn_test

all: tests $(MANUAL_TESTS)

gpgme_tst: gpgme_tst.o 
	$(CC) $(CFLAGS) -o gpgme_tst gpgme_tst.o $(LIBS)

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

netiface_test: netiface_test.o 
	$(CC) $(CFLAGS) -o netiface_test netiface_test.o $(LIBS)

pqiarchive_test: pqiarchive_test.o pkt_test.o
	$(CC) $(CFLAGS) -o pqiarchive_test pkt_test.o pqiarchive_test.o $(LIBS)

pqiperson_test: pqiperson_test.o testconnect.o
	$(CC) $(CFLAGS) -o pqiperson_test pqiperson_test.o testconnect.o $(LIBS) -lstdc++

extaddrfinder_test: extaddrfinder_test.o 
	$(CC) $(CFLAGS) -DEXTADDRSEARCH_DEBUG=1 -o extaddrfinder_test extaddrfinder_test.o $(LIBS)

pqiqos_test: pqiqos_test.o 
	$(CC) $(CFLAGS) -o pqiqos_test pqiqos_test.o $(LIBS)

dnsresolver_test: dnsresolver_test.o 
	$(CC) $(CFLAGS) -o dnsresolver_test dnsresolver_test.o $(LIBS)

pqiipset_test: pqiipset_test.o 
	$(CC) $(CFLAGS) -o pqiipset_test pqiipset_test.o $(LIBS)

p3connmgr_reset_test: p3connmgr_reset_test.o 
	$(CC) $(CFLAGS) -o p3connmgr_reset_test p3connmgr_reset_test.o $(LIBS)

p3connmgr_connect_test: p3connmgr_connect_test.o conn_harness.o ppg_harness.o
	$(CC) $(CFLAGS) -o p3connmgr_connect_test p3connmgr_connect_test.o conn_harness.o ppg_harness.o $(LIBS)


p3connmgr_test1: p3connmgr_test1.o 
	$(CC) $(CFLAGS) -o p3connmgr_test1 p3connmgr_test1.o $(LIBS)



clobber: remove_extra_files

remove_extra_files:
	-$(RM) $(MANUAL_TESTS)

###############################################################
include $(RS_TOP_DIR)/tests/scripts/rules.mk
###############################################################


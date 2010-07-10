
LIB_TOP_DIR = ..
TEST_TOP_DIR = $(LIB_TOP_DIR)/tests

##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(TEST_TOP_DIR)/scripts/config.mk
###############################################################

# Generic Test Harnesses.
TESTOBJ  = bdmetric_test.o bdmsgs_test.o bdnode_test.o  bdspace_test.o 
TESTOBJ  += bdmgr_multitest.o bdnode_multitest1.o bdquery_test.o bdstore_test.o
TESTOBJ  += bdmidids_test.o bdnode_test2.o bdspace_test2.o udpbitdht_nettest.o
#TESTOBJ += bencode_test.o bdudp_test.o

TESTS  = bdmetric_test bdmsgs_test bdnode_test  bdspace_test 
TESTS  += bdmgr_multitest bdnode_multitest1 bdquery_test bdstore_test
TESTS  += bdmidids_test bdnode_test2 bdspace_test2 udpbitdht_nettest
#TESTS  += bencode_test bdudp_test

MANUAL_TESTS = 

all: tests $(MANUAL_TESTS)

bdmsgs_test: bdmsgs_test.o
	$(CC) $(CFLAGS) -o bdmsgs_test bdmsgs_test.o $(LIBS)

bdmetric_test: bdmetric_test.o
	$(CC) $(CFLAGS) -o bdmetric_test bdmetric_test.o $(LIBS)

bdquery_test: bdquery_test.o
	$(CC) $(CFLAGS) -o bdquery_test bdquery_test.o $(LIBS)

bdspace_test: bdspace_test.o
	$(CC) $(CFLAGS) -o bdspace_test bdspace_test.o $(LIBS)

bdspace_test2: bdspace_test2.o
	$(CC) $(CFLAGS) -o bdspace_test2 bdspace_test2.o $(LIBS)

bdnode_test: bdnode_test.o
	$(CC) $(CFLAGS) -o bdnode_test bdnode_test.o $(LIBS)

bdnode_test2: bdnode_test2.o
	$(CC) $(CFLAGS) -o bdnode_test2 bdnode_test2.o $(LIBS)

bdmidids_test: bdmidids_test.o
	$(CC) $(CFLAGS) -o bdmidids_test bdmidids_test.o $(LIBS)


bdnode_multitest1: bdnode_multitest1.o
	$(CC) $(CFLAGS) -o bdnode_multitest1 bdnode_multitest1.o $(LIBS)

bdmgr_multitest: bdmgr_multitest.o
	$(CC) $(CFLAGS) -o bdmgr_multitest bdmgr_multitest.o $(LIBS)


bdstore_test: bdstore_test.o
	$(CC) $(CFLAGS) -o bdstore_test bdstore_test.o $(LIBS)

bdudp_test: bdudp_test.o
	$(CC) $(CFLAGS) -o bdudp_test bdudp_test.o $(LIBS)

udpbitdht_nettest: udpbitdht_nettest.o
	$(CC) $(CFLAGS) -o udpbitdht_nettest udpbitdht_nettest.o $(LIBS)

bencode_test: bencode_test.o
	$(CC) $(CFLAGS) -o bencode_test bencode_test.o $(LIBS)


clobber: remove_extra_files

remove_extra_files:
	-$(RM) $(MANUAL_TESTS)

###############################################################
include $(TEST_TOP_DIR)/scripts/rules.mk
###############################################################



RS_TOP_DIR = ../..
DHT_TOP_DIR = ../../../../libbitdht/src
OPS_TOP_DIR = ../../../../openpgpsdk/src
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/tests/scripts/config.mk
###############################################################

TESTOBJ  = netsetup_test.o random_test.o memory_management_test.o pgpkey_test.o
TESTS  = netsetup_test random_test memory_management_test pgpkey_test

all: tests

pgpkey_test: pgpkey_test.o 
	$(CC) $(CFLAGS) -o pgpkey_test pgpkey_test.o $(LIBS)

netsetup_test: netsetup_test.o 
	$(CC) $(CFLAGS) -o netsetup_test netsetup_test.o $(LIBS)

random_test: random_test.o 
	$(CC) $(CFLAGS) -o random_test random_test.o $(LIBS)

memory_management_test: memory_management_test.o 
	$(CC) $(CFLAGS) -o memory_management_test memory_management_test.o $(LIBS)

###############################################################
include $(RS_TOP_DIR)/tests/scripts/rules.mk
###############################################################


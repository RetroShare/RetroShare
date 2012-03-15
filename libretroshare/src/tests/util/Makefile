
RS_TOP_DIR = ../..
DHT_TOP_DIR = ../../../../libbitdht/src
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/tests/scripts/config.mk
###############################################################

TESTOBJ  = dirtest.o sha1_test.o
TESTS  = dirtest sha1_test

all: tests

sha1_test: sha1_test.o 
	$(CC) $(CFLAGS) -o sha1_test sha1_test.o $(LIBS)
dirtest: dirtest.o 
	$(CC) $(CFLAGS) -o dirtest dirtest.o $(LIBS)

###############################################################
include $(RS_TOP_DIR)/tests/scripts/rules.mk
###############################################################


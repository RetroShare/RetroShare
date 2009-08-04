
RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

TESTOBJ  = netsetup_test.o
TESTS  = netsetup_test

all: tests

netsetup_test: netsetup_test.o 
	$(CC) $(CFLAGS) -o netsetup_test netsetup_test.o $(LIBS)

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################


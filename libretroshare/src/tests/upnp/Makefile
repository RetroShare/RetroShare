
RS_TOP_DIR = ../..
DHT_TOP_DIR = ../../../../libbitdht/src
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/tests/scripts/config.mk
###############################################################

TESTOBJ  = 
TESTS  = 

all: tests 

upnpforward: upnpforward.o 
	$(CC) $(CFLAGS) -o upnpforward upnpforward.o $(LIBS)

###############################################################
include $(RS_TOP_DIR)/tests/scripts/rules.mk
###############################################################



RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

EXTRA_CFLAGS += -I$(UPNPC_DIR) -DMINIUPNP_EXPORTS 

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

CFLAGS += $(EXTRA_CFLAGS)

RSOBJ = upnphandler.o upnputil.o

TESTOBJ  = upnptest.o

TESTS  = upnptest

all: librs tests

upnptest: $(OBJ) upnptest.o
	$(CC) $(CFLAGS) -o upnptest $(OBJ) upnptest.o $(RSLIBS)

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################


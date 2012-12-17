
RS_TOP_DIR = ../..
DHT_TOP_DIR = ../../../../libbitdht/src
OPS_TOP_DIR = ../../../../openpgpsdk/src

##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/tests/scripts/config.mk
###############################################################


TESTOBJ   = servicetest.o forumservicetest.o chatservicetest.o 
TESTOBJ  += chattest.o distribtest.o

TESTS  = chattest distribtest

all: tests

distribtest: distribtest.o servicetest.o forumservicetest.o
	$(CC) $(CFLAGS) -o distribtest distribtest.o servicetest.o \
	forumservicetest.o $(OBJ) $(LIBS)
	
chattest: chattest.o servicetest.o chatservicetest.o
	$(CC) $(CFLAGS) -o chattest chattest.o servicetest.o \
	chatservicetest.o $(OBJ) $(LIBS)


###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################



RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

RSOBJ = p3peers.o \
	p3rank.o  \
	p3msgs.o  \
	p3face-file.o \
	p3face-server.o \
	p3face-config.o \
	p3face-startup.o \
	rstypes.o \
	rsiface.o \
	p3face-msgs.o 

#	pqistrings.o \
#	p3face-people.o 
#	p3face-network.o \

#TESTOBJ  = 

#TESTS  = 

all: librs tests

#tlvbase_test : tlvbase_test.o
#	$(CC) $(CFLAGS) -o tlvbase_test tlvbase_test.o $(OBJ) $(LIBS) 

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################


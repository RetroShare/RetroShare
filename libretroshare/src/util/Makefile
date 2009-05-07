
RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

RSOBJ = rsthreads.o  rsprint.o rsnet.o rsdebug.o rsdir.o

TESTOBJ  = dirtest.o dir2test.o 

TESTS  = dirtest dir2test

all: librs tests

dirtest: $(OBJ) dirtest.o
	$(CC) $(CFLAGS) -o dirtest dirtest.o $(OBJ) $(LIBS)

dir2test: $(OBJ) dir2test.o
	$(CC) $(CFLAGS) -o dir2test dir2test.o $(OBJ) $(LIBS)

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################



RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

RSOBJ = b64.o opendhtstr.o opendht.o opendhtmgr.o

TESTOBJ  = odhtstr_test.o odhtpost_test.o odhtmgr_test.o 

TESTS  = odhtstr_test odhtpost_test odhtmgr_test

all: librs tests

#dhttest is OLD
dhttest: $(OBJ) dhttest.o
	$(CC) $(CFLAGS) -o dhttest $(OBJ) dhttest.o $(LIBS)

odhtpost_test: $(OBJ) odhtpost_test.o
	$(CC) $(CFLAGS) -o odhtpost_test $(OBJ) odhtpost_test.o $(LIBS)

odhtstr_test: $(OBJ) odhtstr_test.o
	$(CC) $(CFLAGS) -o odhtstr_test $(OBJ) odhtstr_test.o $(LIBS)

odhtmgr_test: $(OBJ) odhtmgr_test.o
	$(CC) $(CFLAGS) -o odhtmgr_test $(OBJ) odhtmgr_test.o $(LIBS)

# Extra Rule...
.c.o:
	$(CC) $(CFLAGS) -c $<

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################


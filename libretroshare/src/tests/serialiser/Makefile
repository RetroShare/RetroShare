
RS_TOP_DIR = ../..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/tests/scripts/config.mk
###############################################################

TESTOBJ  = tlvbase_test.o tlvbase_test2.o rstunnelitem_test.o
TESTOBJ += tlvitems_test.o tlvstack_test.o rsserial_test.o 
TESTOBJ += rstlvwidetest.o tlvrandom_test.o rsturtleitem_test.o
TESTOBJ += tlvtypes_test.o support.o distribitem_test.o rsmsgitem_test.o  
TESTOBJ += rsstatusitem_test.o

TESTS  = tlvbase_test tlvbase_test2 rstlvwidetest
TESTS += tlvitems_test tlvstack_test rstunnelitem_test
TESTS +=  tlvrandom_test rsserial_test rsturtleitem_test
TESTS +=  tlvtypes_test distribitem_test rsmsgitem_test
TESTS +=  rsstatusitem_test

#rsbaseitem_test

all: tests

tlvbase_test : tlvbase_test.o
	$(CC) $(CFLAGS) -o tlvbase_test tlvbase_test.o $(OBJ) $(LIBS) 

tlvbase_test2 : tlvbase_test2.o
	$(CC) $(CFLAGS) -o tlvbase_test2 tlvbase_test2.o $(OBJ) $(LIBS) 

tlvitems_test : tlvitems_test.o
	$(CC) $(CFLAGS) -o tlvitems_test tlvitems_test.o $(OBJ) $(LIBS) 

tlvstack_test : tlvstack_test.o
	$(CC) $(CFLAGS) -o tlvstack_test tlvstack_test.o $(OBJ) $(LIBS) 

rsserial_test : rsserial_test.o
	$(CC) $(CFLAGS) -o rsserial_test rsserial_test.o $(OBJ) $(LIBS) 

rsbaseitem_test : rsbaseitem_test.o
	$(CC) $(CFLAGS) -o rsbaseitem_test rsbaseitem_test.o $(OBJ) $(LIBS) 

rstlvwidetest : rstlvwidetest.o
	$(CC) $(CFLAGS) -o rstlvwidetest rstlvwidetest.o $(OBJ) $(LIBS)

tlvrandom_test : tlvrandom_test.o
	$(CC) $(CFLAGS) -o tlvrandom_test tlvrandom_test.o $(OBJ) $(LIBS)

rsturtleitem_test : rsturtleitem_test.o support.o
	$(CC) $(CFLAGS) -o rsturtleitem_test rsturtleitem_test.o support.o $(OBJ) $(LIBS)

rstunnelitem_test : rstunnelitem_test.o support.o
	$(CC) $(CFLAGS) -o rstunnelitem_test rstunnelitem_test.o support.o $(OBJ) $(LIBS)
	
tlvtypes_test : tlvtypes_test.o
	$(CC) $(CFLAGS) -o tlvtypes_test tlvtypes_test.o $(OBJ) $(LIBS)
	
distribitem_test : distribitem_test.o support.o
	$(CC) $(CFLAGS) -o distribitem_test distribitem_test.o support.o $(OBJ) $(LIBS)

rsmsgitem_test : rsmsgitem_test.o support.o
	$(CC) $(CFLAGS) -o rsmsgitem_test rsmsgitem_test.o support.o $(OBJ) $(LIBS)

rsstatusitem_test : rsstatusitem_test.o support.o
	$(CC) $(CFLAGS) -o rsstatusitem_test rsstatusitem_test.o support.o $(OBJ) $(LIBS)

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################


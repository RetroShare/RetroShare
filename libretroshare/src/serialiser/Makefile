
RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

RSOBJ  = rsserial.o rsbaseserial.o rstlvbase.o rstlvtypes.o 
RSOBJ += rstlvfileitem.o rstlvutil.o    # TLV Objs
RSOBJ += rsbaseitems.o rsconfigitems.o rsdiscitems.o # RsItems 
RSOBJ += rsmsgitems.o rsrankitems.o	# RsItems
RSOBJ += rsphotoitems.o			# RsItems
RSOBJ += rsgameitems.o			# RsItems

TESTOBJ  = tlvbase_test.o tlvbase_test2.o tlvfileitem_test.o 
TESTOBJ += tlvitems_test.o tlvstack_test.o tlvconfig_test.o 
TESTOBJ += rsserial_test.o 

#rsbaseitem_test.o 

TESTS  = tlvbase_test tlvbase_test2 tlvfileitem_test 
TESTS += tlvitems_test tlvstack_test tlvconfig_test
TESTS += 
#rsserial_test 

#rsbaseitem_test

all: librs tests

tlvbase_test : tlvbase_test.o
	$(CC) $(CFLAGS) -o tlvbase_test tlvbase_test.o $(OBJ) $(LIBS) 

tlvbase_test2 : tlvbase_test2.o
	$(CC) $(CFLAGS) -o tlvbase_test2 tlvbase_test2.o $(OBJ) $(LIBS) 

tlvfileitem_test : tlvfileitem_test.o
	$(CC) $(CFLAGS) -o tlvfileitem_test tlvfileitem_test.o $(OBJ) $(LIBS) 

tlvitems_test : tlvitems_test.o
	$(CC) $(CFLAGS) -o tlvitems_test tlvitems_test.o $(OBJ) $(LIBS) 

tlvstack_test : tlvstack_test.o
	$(CC) $(CFLAGS) -o tlvstack_test tlvstack_test.o $(OBJ) $(LIBS) 

tlvconfig_test : tlvconfig_test.o
	$(CC) $(CFLAGS) -o tlvconfig_test tlvconfig_test.o $(OBJ) $(LIBS) 

rsserial_test : rsserial_test.o
	$(CC) $(CFLAGS) -o rsserial_test rsserial_test.o $(OBJ) $(LIBS) 

rsbaseitem_test : rsbaseitem_test.o
	$(CC) $(CFLAGS) -o rsbaseitem_test rsbaseitem_test.o $(OBJ) $(LIBS) 


###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################



RS_TOP_DIR = ../..
DHT_TOP_DIR = ../../../../libbitdht/src
OPS_TOP_DIR = ../../../../openpgpsdk/src

##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/tests/scripts/config.mk
###############################################################

TESTOBJ  = ftfilemappertest.o ftfileprovidertest.o ftfilecreatortest.o ftextralisttest.o ftdataplextest.o fttransfermoduletest.o ftcrc32test.o ftcrossprovidercreatortest.o ftcontrollertest.o ftserver1test.o ftserver2test.o ftserver3test.o ftdata_test.o

TESTS  = ftfileprovidertest ftfilecreatortest ftextralisttest ftdataplextest fttransfermoduletest ftcrc32test ftcrossprovidercreatortest ftcontrollertest ftserver1test ftserver2test fttransfermoduletest ftserver3test
#ftfilemappertest 

all: tests

ftfilemappertest : ftfilemappertest.o 
	$(CC) $(CFLAGS) -o ftfilemappertest ftfilemappertest.o $(LIBS)
	
ftcontrollertest : ftcontrollertest.o 
	$(CC) $(CFLAGS) -o ftcontrollertest ftcontrollertest.o $(LIBS)
	
ftfilecreatortest : ftfilecreatortest.o 
	$(CC) $(CFLAGS) -o ftfilecreatortest ftfilecreatortest.o $(LIBS)
	
ftcrossprovidercreatortest : ftcrossprovidercreatortest.o 
	$(CC) $(CFLAGS) -o ftcrossprovidercreatortest ftcrossprovidercreatortest.o $(LIBS)

ftfileprovidertest : ftfileprovidertest.o 
	$(CC) $(CFLAGS) -o ftfileprovidertest ftfileprovidertest.o $(LIBS)

fttransfermoduletest : fttransfermoduletest.o 
	$(CC) $(CFLAGS) -o fttransfermoduletest fttransfermoduletest.o $(LIBS)

ftextralisttest : ftextralisttest.o 
	$(CC) $(CFLAGS) -o ftextralisttest ftextralisttest.o $(LIBS)

ftdataplextest : ftdataplextest.o 
	$(CC) $(CFLAGS) -o ftdataplextest ftdataplextest.o $(LIBS)

ftserver1test : ftserver1test.o 
	$(CC) $(CFLAGS) -o ftserver1test ftserver1test.o $(LIBS)

ftserver2test : ftserver2test.o 
	$(CC) $(CFLAGS) -o ftserver2test ftserver2test.o $(LIBS)

ftserver3test : ftserver3test.o 
	$(CC) $(CFLAGS) -o ftserver3test ftserver3test.o $(LIBS)

ftcrc32test : ftcrc32test.o 
	$(CC) $(CFLAGS) -O0 -g -o ftcrc32test ftcrc32test.o $(LIBS)

###############################################################
include $(RS_TOP_DIR)/tests/scripts/rules.mk
###############################################################



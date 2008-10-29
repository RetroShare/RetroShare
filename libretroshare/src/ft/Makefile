
RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

RSOBJ =  ftdata.o ftfileprovider.o ftfilecreator.o ftextralist.o \
	ftdatamultiplex.o ftfilesearch.o fttransfermodule.o ftdbase.o ftserver.o \
	ftcontroller.o pqitestor.o


TESTOBJ  = ftfileprovidertest.o ftfilecreatortest.o ftextralisttest.o ftdataplextest.o ftserver1test.o ftserver2test.o fttransfermoduletest.o ftserver3test.o


TESTS  = ftfileprovidertest ftfilecreatortest ftextralisttest ftdataplextest ftserver1test ftserver2test fttransfermoduletest ftserver3test

all: librs tests

ftfilecreatortest : ftfilecreatortest.o 
	$(CC) $(CFLAGS) -o ftfilecreatortest ftfilecreatortest.o $(LIBS)
	
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

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################



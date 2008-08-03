
RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

RSOBJ =  ftdata.o ftfileprovider.o ftfilecreator.o ftextralist.o ftdatamultiplex.o ftfilesearch.o fttransfermodule.o 

#ftserver.o

TESTOBJ  = ftfileprovidertest.o ftfilecreatortest.o ftextralisttest.o ftdataplextest.o

TESTS  = ftfileprovidertest ftfilecreatortest ftextralisttest ftdataplextest

all: librs tests

ftfilecreatortest : ftfilecreatortest.o 
	$(CC) $(CFLAGS) -o ftfilecreatortest ftfilecreatortest.o $(LIBS)
	
ftfileprovidertest : ftfileprovidertest.o 
	$(CC) $(CFLAGS) -o ftfileprovidertest ftfileprovidertest.o $(LIBS)

ftextralisttest : ftextralisttest.o 
	$(CC) $(CFLAGS) -o ftextralisttest ftextralisttest.o $(LIBS)

ftdataplextest : ftdataplextest.o 
	$(CC) $(CFLAGS) -o ftdataplextest ftdataplextest.o $(LIBS)

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################



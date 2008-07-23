
RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

RSOBJ =  ftfileprovider.o ftfilecreator.o ftextralist.o ftdatamultiplex.o 
#ftservermodule.o

TESTOBJ  = ftfileprovidertest.o ftfilecreatortest.o ftextralisttest.o

TESTS  = ftfileprovidertest ftfilecreatortest ftextralisttest

all: librs tests

ftfilecreatortest : ftfilecreatortest.o 
	$(CC) $(CFLAGS) -o ftfilecreatortest ftfilecreatortest.o $(LIBS)
	
ftfileprovidertest : ftfileprovidertest.o 
	$(CC) $(CFLAGS) -o ftfileprovidertest ftfileprovidertest.o $(LIBS)

ftextralisttest : ftextralisttest.o 
	$(CC) $(CFLAGS) -o ftextralisttest ftextralisttest.o $(LIBS)

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################



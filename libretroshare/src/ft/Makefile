
RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

RSOBJ =  ftfileprovider.o ftfilecreator.o

TESTOBJ  = ftfileprovidertest.o ftfilecreatortest.o

TESTS  = ftfileprovidertest ftfilecreatortest

all: librs tests

ftfilecreatortest : ftfilecreatortest.o $(RSOBJ)
	$(CC) $(CFLAGS) -o ftfilecreatortest ftfilecreatortest.o $(RSOBJ) $(LIBS)
	
ftfileprovidertest : ftfileprovidertest.o $(RSOBJ)
	$(CC) $(CFLAGS) -o ftfileprovidertest ftfileprovidertest.o $(RSOBJ) $(LIBS)

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################



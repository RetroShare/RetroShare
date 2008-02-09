
RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

RSOBJ =   findex.o fimonitor.o cachestrapper.o fistore.o \
	rsexpr.o

TESTOBJ =  fitest2.o fisavetest.o searchtest.o 
#ficachetest.o 

TESTS =  fitest2 fisavetest searchtest
#ficachetest 

ifeq ($(OS),Linux)
        TESTOBJ += fimontest.o
        TESTS += fimontest
endif


ifeq ($(OS),MacOSX)
        TESTOBJ += fimontest.o
        TESTS += fimontest
endif


all: librs tests

fitest2 : fitest2.o $(OBJ)
	$(CC) $(CFLAGS) -o fitest2 fitest2.o $(OBJ) $(LIBS)

fisavetest : fisavetest.o $(OBJ)
	$(CC) $(CFLAGS) -o fisavetest fisavetest.o $(OBJ) $(LIBS)

fimontest : fimontest.o $(OBJ)
	$(CC) $(CFLAGS) -o fimontest fimontest.o $(OBJ) $(LIBS)

ficachetest : ficachetest.o $(OBJ)
	$(CC) $(CFLAGS) -o ficachetest ficachetest.o $(OBJ) $(LIBS)

searchtest : searchtest.o $(OBJ)
	$(CC) $(CFLAGS) -o searchtest searchtest.o $(OBJ) $(LIBS)

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################


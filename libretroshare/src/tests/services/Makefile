
RS_TOP_DIR = ../..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/tests/scripts/config.mk
###############################################################


OBJ  += distrib_services.o Service_Test.o 

TESTS  = distrib_test_main

all: tests

distrib_test_main: distrib_test_main.o Service_Test.o distrib_services.o
	$(CC) $(CFLAGS) -o distrib_test_main distrib_test_main.o $(OBJ) $(LIBS)


###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################


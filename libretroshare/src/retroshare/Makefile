
RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

OBJ  = notifytxt.o retroshare.o

#TESTOBJ  = 

TESTS  = retroshare-nogui

all: tests

retroshare-nogui: $(OBJ)
	$(CC) $(CFLAGS) -o retroshare-nogui $(OBJ) $(LIBS) 


###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################


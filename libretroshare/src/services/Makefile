
RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

RSOBJ  = p3service.o p3chatservice.o p3msgservice.o \
	p3gamelauncher.o p3ranking.o p3disc.o \
	p3photoservice.o \
	p3distrib.o \
	p3status.o \
	p3Qblog.o \
	p3forums.o \
	p3channels.o 

# dummy forums interface.
#	p3forums-dummy.o \

TESTOBJ  = forum_test.o

TESTS  = forum_test

all: librs tests

forum_test : forum_test.o
	$(CC) $(CFLAGS) -o forum_test forum_test.o $(OBJ) $(LIBS) 


###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################


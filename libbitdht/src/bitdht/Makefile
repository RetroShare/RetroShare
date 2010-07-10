CPPFLAGS += -g -Wall -D BE_DEBUG -lpthread
LDFLAGS += -g -Wall -D BE_DEBUG -lpthread
CFLAGS += -g -Wall -D BE_DEBUG
CC = g++
LIB = -lpthread
CPPLIB = -lpthread

EXEC = bdmsgs_test bdmetric_test bdquery_test bdspace_test bdspace_test2 bdnode_test bdnode_test2 bdstore_test 
EXEC += bdnode_multitest1 bdmidids_test 
EXEC += udpbitdht_nettest bencode_test
EXEC += bdmgr_multitest
#EXEC += bdudp_test

all: $(EXEC)

OBJ = bencode.o bdmsgs.o bdobj.o
OBJ += bdpeer.o bdquery.o bdnode.o bdstore.o bdhash.o
OBJ += bdmanager.o bdstddht.o

# udp base objs
OBJ += bdthreads.o udplayer.o udpstack.o
OBJ += udpbitdht.o

bdmsgs_test: $(OBJ) bdmsgs_test.o
bdmetric_test: $(OBJ) bdmetric_test.o
bdquery_test: $(OBJ) bdquery_test.o
bdspace_test: $(OBJ) bdspace_test.o
bdspace_test2: $(OBJ) bdspace_test2.o
bdnode_test: $(OBJ) bdnode_test.o
bdnode_test2: $(OBJ) bdnode_test2.o
bdmidids_test: $(OBJ) bdmidids_test.o

bdnode_multitest1: $(OBJ) bdnode_multitest1.o
bdmgr_multitest: $(OBJ) bdmgr_multitest.o

bdstore_test: $(OBJ) bdstore_test.o
bdudp_test: $(OBJ) bdudp_test.o
udpbitdht_nettest: $(OBJ) udpbitdht_nettest.o
bencode_test: $(OBJ) bencode_test.o

clean:
	rm -f *.o core $(EXEC)

.PHONY: all clean

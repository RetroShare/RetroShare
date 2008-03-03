
RS_TOP_DIR = ..
##### Define any flags that are needed for this section #######
###############################################################

###############################################################
include $(RS_TOP_DIR)/scripts/config.mk
###############################################################

BIOOBJ = bss_tou.o 

RSOBJ  = tou_net.o udplayer.o udpsorter.o udptestfn.o
RSOBJ += tcppacket.o tcpstream.o tou.o $(BIOOBJ)

EXECS = librs  udpsock_test udpsort_test udp_server 
#test_tou pair_tou reset_tou internal_tou largefile_tou 

#TESTOBJ  = udpsock_test.o udpsort_test.o udp_server.o test_tou.o
#TESTOBJ += pair_tou.o reset_tou.o largefile_tou.o
#internal_tou.o 

#TESTS  = udpsock_test udpsort_test udp_server test_tou 
#TESTS += pair_tou reset_tou largefile_tou
#internal_tou 

all: librs tests

udpsock_test : $(OBJ) udpsock_test.o
	$(CC) $(CFLAGS) -o udpsock_test udpsock_test.o $(OBJ) $(LIBS)

udpsort_test : $(OBJ) udpsort_test.o
	$(CC) $(CFLAGS) -o udpsort_test udpsort_test.o $(OBJ) $(LIBS)

udp_server: $(OBJ) udp_server.o
	$(CC) $(CFLAGS) -o udp_server udp_server.o $(OBJ) $(LIBS)

test_tou : $(OBJ) test_tou.o
	$(CC) $(CFLAGS) -o test_tou test_tou.o $(OBJ) $(LIBS)

pair_tou : $(OBJ) pair_tou.o
	$(CC) $(CFLAGS) -o pair_tou pair_tou.o $(OBJ) $(LIBS)

reset_tou : $(OBJ) reset_tou.o
	$(CC) $(CFLAGS) -o reset_tou reset_tou.o $(OBJ) $(LIBS)

internal_tou : $(OBJ) internal_tou.o
	$(CC) $(CFLAGS) -o internal_tou internal_tou.o $(OBJ) $(LIBS)

largefile_tou : $(OBJ) largefile_tou.o
	$(CC) $(CFLAGS) -o largefile_tou largefile_tou.o $(OBJ) $(LIBS)


# Extra Rule for BIOFLAGS
.c.o:
	$(BIOCC) $(BIOCFLAGS) -c $<

###############################################################
include $(RS_TOP_DIR)/scripts/rules.mk
###############################################################



RS_TOP_DIR=..

include $(RS_TOP_DIR)/make.opt

EXECS = librs  udpsock_test udpsort_test udp_server 
#test_tou pair_tou reset_tou internal_tou largefile_tou 

OBJ  = tou_net.o udplayer.o udpsorter.o udptestfn.o
OBJ += tcppacket.o tcpstream.o tou.o

#tou.o 

all : $(OBJ) $(EXECS)

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-$(RM) $(OBJ) $(BIOOBJ) udpsock_test.o udpsort_test.o udp_server.o

#test_tou.o pair_tou.o udp_server.o reset_tou.o internal_tou.o largefile_tou.o

clobber: clean
	-$(RM) udpsock_test udpsort_test udp_server 

#test_tou pair_tou reset_tou internal_tou largefile_tou libtou.so ../lib/libtou.a

udpsock_test : $(OBJ) udpsock_test.o
	$(CC) $(CFLAGS) -o udpsock_test $(OBJ) udpsock_test.o -lpthread -L../lib -lretroshare

udpsort_test : $(OBJ) udpsort_test.o
	$(CC) $(CFLAGS) -o udpsort_test $(OBJ) udpsort_test.o -lpthread -L../lib -lretroshare

udp_server: $(OBJ) udp_server.o
	$(CC) $(CFLAGS) -o udp_server $(OBJ) udp_server.o -lpthread -L../lib -lretroshare

test_tou : $(OBJ) test_tou.o
	$(CC) $(CFLAGS) -o test_tou $(OBJ) test_tou.o $(LIBS)

pair_tou : $(OBJ) pair_tou.o
	$(CC) $(CFLAGS) -o pair_tou $(OBJ) pair_tou.o $(LIBS)

reset_tou : $(OBJ) reset_tou.o
	$(CC) $(CFLAGS) -o reset_tou $(OBJ) reset_tou.o $(LIBS)

internal_tou : $(OBJ) internal_tou.o
	$(CC) $(CFLAGS) -o internal_tou $(OBJ) internal_tou.o $(LIBS)

largefile_tou : $(OBJ) largefile_tou.o
	$(CC) $(CFLAGS) -o largefile_tou $(OBJ) largefile_tou.o $(LIBS)



# For BIO Compilation.... SSL Interface.
#


BIOOBJ = bss_tou.o 
BIOCC  = gcc
BIOCFLAGS =  -I $(SSL_DIR)/include -DOPENSSL_THREADS -D_REENTRANT -DDSO_DLFCN -DHAVE_DLFCN_H -DOPENSSL_NO_KRB5 -DL_ENDIAN -DTERMIO -O3 -fomit-frame-pointer -m486 -Wall -DSHA1_ASM -DMD5_ASM -DRMD160_ASM  


libtou.a: $(BIOOBJ) $(OBJ)
	ar rc ../lib/libtou.a $(BIOOBJ) $(OBJ)

libtou.so: $(BIOOBJ) $(OBJ)
	$(CC) -o libtou.so -shared $(BIOOBJ) $(OBJ)

librs: $(BIOOBJ) $(OBJ)	
	ar r ../lib/libretroshare.a $(BIOOBJ) $(OBJ)
	
.c.o:
	$(BIOCC) $(BIOCFLAGS) -c $<


RS_TOP_DIR = ..
include ../make.opt

OBJ = dhthandler.o
#CADKINC = /home/rmf24/prog/src/KadC
#CFLAGS += -I $(CADKINC)
#RSLIBS += -L $(CADKINC) -lKadC


all : $(OBJ) librs dhttest

dhttest: $(OBJ) dhttest.o
	$(CC) $(CFLAGS) -o dhttest $(OBJ) dhttest.o $(RSLIBS)

librs:  $(OBJ)
	$(AR) r $(LIBRS) $(OBJ)
	$(RANLIB) $(LIBRS)

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-/bin/rm $(OBJ) dhttest.o

clobber: clean
	-/bin/rm dhttest


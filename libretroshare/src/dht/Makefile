
RS_TOP_DIR = ..
include ../make.opt

OBJ = b64.o opendhtstr.o opendht.o opendhtmgr.o

#CADKINC = /home/rmf24/prog/src/KadC
#CFLAGS += -I $(CADKINC)
#RSLIBS += -L $(CADKINC) -lKadC


all : $(OBJ) librs odhtstr_test odhtpost_test odhtmgr_test

#dhttest

dhttest: $(OBJ) dhttest.o
	$(CC) $(CFLAGS) -o dhttest $(OBJ) dhttest.o $(RSLIBS)

odhtpost_test: $(OBJ) odhtpost_test.o
	$(CC) $(CFLAGS) -o odhtpost_test $(OBJ) odhtpost_test.o $(RSLIBS)

odhtstr_test: $(OBJ) odhtstr_test.o
	$(CC) $(CFLAGS) -o odhtstr_test $(OBJ) odhtstr_test.o $(RSLIBS)

odhtmgr_test: $(OBJ) odhtmgr_test.o
	$(CC) $(CFLAGS) -o odhtmgr_test $(OBJ) odhtmgr_test.o $(RSLIBS)

librs:  $(OBJ)
	$(AR) r $(LIBRS) $(OBJ)
	$(RANLIB) $(LIBRS)

.cc.o:
	$(CC) $(CFLAGS) -c $<

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-/bin/rm $(OBJ) dhttest.o

clobber: clean
	-/bin/rm dhttest


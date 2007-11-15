
RS_TOP_DIR = ..
include ../make.opt

OBJ = upnphandler.o upnputil.o

CFLAGS += -I$(UPNPC_DIR) -DMINIUPNP_EXPORTS 

all : $(OBJ) librs upnptest

upnptest: $(OBJ) upnptest.o
	$(CC) $(CFLAGS) -o upnptest $(OBJ) upnptest.o $(RSLIBS)

librs:  $(OBJ)
	$(AR) r $(LIBRS) $(OBJ)
	$(RANLIB) $(LIBRS)

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-/bin/rm $(OBJ) upnptest.o

clobber: clean
	-/bin/rm upnptest



RS_TOP_DIR = ..
include ../make.opt

OBJ =   findex.o fimonitor.o cachestrapper.o fistore.o \
	rsexpr.o

EXEC =  fitest2 fisavetest ficachetest searchtest

ifeq ($(OS),Linux)
	EXEC += fimontest 
endif

all : $(OBJ) librs $(EXEC)

librs:  $(OBJ)
	$(AR) r $(LIBRS) $(OBJ)
	$(RANLIB) $(LIBRS)

ftest: $(OBJ) ftest.o
	$(CC) $(CFLAGS) -o ftest $(OBJ) ftest.o $(RSLIBS)

looktest: $(OBJ) looktest.o
	$(CC) $(CFLAGS) -o looktest $(OBJ) looktest.o $(RSLIBS)

fitest2 : fitest2.o $(OBJ)
	$(CC) $(CFLAGS) -o fitest2 fitest2.o $(OBJ) $(RSLIBS)

fisavetest : fisavetest.o $(OBJ)
	$(CC) $(CFLAGS) -o fisavetest fisavetest.o $(OBJ) $(RSLIBS)

fimontest : fimontest.o $(OBJ)
	$(CC) $(CFLAGS) -o fimontest fimontest.o $(OBJ) $(RSLIBS)

ficachetest : ficachetest.o $(OBJ)
	$(CC) $(CFLAGS) -o ficachetest ficachetest.o $(OBJ) $(RSLIBS)

searchtest : searchtest.o $(OBJ)
	$(CC) $(CFLAGS) -o searchtest searchtest.o $(OBJ) $(RSLIBS)


.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-/bin/rm $(OBJ) ftest.o looktest.o
	-/bin/rm fitest2.o fisavetest.o fimontest.o
	-/bin/rm ficachetest.o searchtest.o

clobber: clean
	-/bin/rm $(EXEC)


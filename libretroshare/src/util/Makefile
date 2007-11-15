
RS_TOP_DIR = ..
include ../make.opt

OBJ = rsthreads.o  rsdir.o

all : $(OBJ) librs dirtest dir2test

dirtest: $(OBJ) dirtest.o
	$(CC) $(CFLAGS) -o dirtest $(OBJ) dirtest.o $(RSLIBS)

dir2test: $(OBJ) dir2test.o
	$(CC) $(CFLAGS) -o dir2test $(OBJ) dir2test.o $(RSLIBS)

librs:  $(OBJ)
	$(AR) r $(LIBRS) $(OBJ)
	$(RANLIB) $(LIBRS)

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-/bin/rm $(OBJ) dirtest.o dir2test.o

clobber: clean
	-/bin/rm dirtest dir2test



RS_TOP_DIR = ..
include ../make.opt

OBJ =   ft.o ftfiler.o hashsearch.o \
	filedexserver.o 

EXEC = ftcachetest 

all : $(OBJ) librs $(EXEC)

librs:  $(OBJ)
	$(AR) r $(LIBRS) $(OBJ)
	$(RANLIB) $(LIBRS)

ftcachetest : ftcachetest.o $(OBJ)
	$(CC) $(CFLAGS) -o ftcachetest ftcachetest.o $(OBJ) $(RSLIBS)

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-/bin/rm $(OBJ) 
	-/bin/rm ftcachetest.o 

clobber: clean
	-/bin/rm $(EXEC)


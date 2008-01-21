
RS_TOP_DIR = ..
include ../make.opt

OBJ = pqistrings.o \
	p3face-people.o p3face-file.o \
	p3face-msgs.o p3face-server.o \
	p3face-config.o \
	p3face-network.o \
	p3face-startup.o \
	rstypes.o \
	rsiface.o 

TESTS = 

all : $(OBJ) librs $(TESTS) 

librs:  $(OBJ)
	$(AR) r $(LIBRS) $(OBJ)
	$(RANLIB) $(LIBRS)

.cc.o:
	$(CC) $(CFLAGS) -c $<


clean:
	-/bin/rm $(OBJ) 

clobber: clean



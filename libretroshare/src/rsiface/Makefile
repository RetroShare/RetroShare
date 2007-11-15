
RS_TOP_DIR = ..
include ../make.opt

OBJ = notifytxt.o retroshare.o

TESTS = 

all : $(OBJ) retroshare-nogui

retroshare-nogui: $(OBJ)
	$(CC) $(RSCFLAGS) -o retroshare-nogui $(OBJ) $(RSLIBS) 
	
.cc.o:
	$(CC) $(RSCFLAGS) -c $<


clean:
	-/bin/rm $(OBJ) 

clobber: clean
	-/bin/rm retroshare-nogui



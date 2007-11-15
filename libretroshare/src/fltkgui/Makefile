
RS_TOP_DIR = ..
include ../make.opt

STRIP=strip

OBJ = fltkserver.o guitab.o Fl_Funky_Browser.o pqibrowseitem.o pqistrings.o \
	Fl_Tree_Browser.o Fl_File_Item.o fltkpqi.o alertbox.o

WIN_OBJ = retrotray.o

# add in windows objs
ifeq ($(OS),Linux)
else
	OBJ += $(WIN_OBJ)
endif

all : $(OBJ) RetroShare

RetroShare : $(OBJ) 
	$(CC) $(CFLAGS) -static -o RetroShare $(OBJ) -lfltk $(RSLIBS) 
#	$(STRIP) -s RetroShare.exe
#	$(STRIP) -s RetroShare

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-$(RM) $(OBJ)

clobber: clean
	-$(RM) RetroShare






	

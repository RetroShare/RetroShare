
CXXFLAGS = -Wall -g -I.. 
#CXXFLAGS += -arch i386 # OSX

LIBS = -L../lib -lbitdht -lpthread



EXEC : bssdht

EGOBJ = bdhandler.o bssdht.o bootstrap_fn.o

bssdht: $(EGOBJ)
	$(CXX) $(CXXFLAGS) -o bssdht $(EGOBJ) $(LIBS)



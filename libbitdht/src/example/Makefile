
CXXFLAGS = -Wall -g -I.. 
#CXXFLAGS += -arch i386 # OSX

LIBS = -L../lib -lbitdht



EXEC : bdexample

EGOBJ = bdhandler.o bdexample.o

bdexample: $(EGOBJ)
	$(CXX) $(CXXFLAGS) -o bdexample $(EGOBJ) $(LIBS)



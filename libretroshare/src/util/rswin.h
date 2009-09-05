#ifndef RS_UNIVERSAL_STUFF
#define RS_UNIVERSAL_STUFF

#if defined(WIN32) || defined(MINGW) 

#include <unistd.h>
#define sleep(x) Sleep(1000 * x)

// For win32 systems (tested on MingW+Ubuntu)
#define stat64 _stati64 


#endif



#endif
	

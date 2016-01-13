#include "util/rsmemory.h"

void *rs_safe_malloc(size_t size) 
{
    static const size_t SAFE_MEMALLOC_THRESHOLD = 1024*1024*1024 ; // 1Gb should be enough for everything!
    
    if(size == 0)
    {
        std::cerr << "(EE) Memory allocation error. A chunk of size 0 was requested. Callstack:" << std::endl;
	print_stacktrace() ;
    	return NULL ;
    }
    
    if(size > SAFE_MEMALLOC_THRESHOLD)
    {
        std::cerr << "(EE) Memory allocation error. A chunk of size 0 was requested. Callstack:" << std::endl;
	print_stacktrace() ;
    	return NULL ;
    }
    
    void *mem = malloc(size) ;
    
    if(mem == NULL)
    {
        std::cerr << "(EE) Memory allocation error for a chunk of " << size << " bytes. Callstack:" << std::endl;
	print_stacktrace() ;
    	return NULL ;
    }
    
    return mem ;
}


#pragma once

#include <stdlib.h>

// This is a scope guard to release the memory block when going of of the current scope.
// Can be very useful to auto-delete some memory on quit without the need to call free each time.
//
// Usage:
//
// {
// 	TemporaryMemoryHolder mem(size) ;
//
//		if(mem != NULL)
//			[ do something ] ;
//
//    memcopy(mem, some_other_memory, size) ;
//
// 	[do something]
//
// }	// mem gets freed automatically
//
class RsTemporaryMemory
{
	public:
		RsTemporaryMemory(size_t s)
		{
			_mem = (unsigned char *)malloc(s) ;
		}

		operator unsigned char *() { return _mem ; }

		~RsTemporaryMemory()
		{
			if(_mem != NULL)
			{
				free(_mem) ;
				_mem = NULL ;
			}
		}

	private:
		unsigned char *_mem ;

		// make it noncopyable
		RsTemporaryMemory& operator=(const RsTemporaryMemory&) { return *this ;}
		RsTemporaryMemory(const RsTemporaryMemory&) {}
};



#pragma once

#include <stdlib.h>

// This is a scope guard to release the memory block when going of of the current scope.
// Can be very useful to auto-delete some memory on quit without the need to call free each time.
//
// Usage:
//
// {
// 	unsigned char *mem = NULL ;
// 	TemporaryMemoryHolder mem_holder(mem,size) ;
//
// 	[do something]
// }	// mem gets freed automatically
//
class TemporaryMemoryHolder
{
	public:
		TemporaryMemoryHolder(unsigned char *& mem,size_t s)
			: _mem(mem)
		{
			_mem = (unsigned char *)malloc(s) ;
		}

		~TemporaryMemoryHolder()
		{
			if(_mem != NULL)
			{
				free(_mem) ;
				_mem = NULL ;
			}
		}

	private:
		unsigned char *& _mem ;
};



/*******************************************************************************
 * libretroshare/src/util: rsmemory.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Cyril Soler <csoler@users.sourceforge.net>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <stdlib.h>
#include <iostream>
#include <memory>
#include <util/stacktrace.h>

#define RS_DEFAULT_STORAGE_PARAM(Type) *std::unique_ptr<Type>(new Type)

void *rs_malloc(size_t size) ;

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
	    _mem = (unsigned char *)rs_malloc(s) ;

	    if(_mem)
		    _size = s ;
	    else
		    _size = 0 ;
    }

    operator unsigned char *() { return _mem ; }
    
    size_t size() const { return _size ; }

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
    size_t _size ;

    // make it noncopyable
    RsTemporaryMemory& operator=(const RsTemporaryMemory&) { return *this ;}
    RsTemporaryMemory(const RsTemporaryMemory&) {}
};

/*******************************************************************************
 * libretroshare/src/util: rsmemory.cc                                         *
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
#include "util/rsmemory.h"

void *rs_malloc(size_t size) 
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
        std::cerr << "(EE) Memory allocation error. A chunk of size larger than " << SAFE_MEMALLOC_THRESHOLD << " was requested. Callstack:" << std::endl;
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


/*******************************************************************************
 * libretroshare/src/util: rsmemory.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 Cyril Soler <csoler@users.sourceforge.net>                   *
 * Copyright 2019-2020 Gioacchino Mazzurco <gio@altermundi.net>                *
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

#include <cstdlib>
#include <iostream>
#include <memory>

#include "util/stacktrace.h"

/**
 * @brief Shorthand macro to declare optional functions output parameters
 * To define an optional output paramether use the following syntax
 *
\code{.cpp}
bool myFunnyFunction(
	int mandatoryParamether,
	BigType& myOptionalOutput = RS_DEFAULT_STORAGE_PARAM(BigType) )
\endcode
 *
 * The function caller then can call myFunnyFunction either passing
 * myOptionalOutput parameter or not.
 * @see RsGxsChannels methods for real usage examples.
 *
 * @details
 * When const references are used to pass function parameters it is easy do make
 * those params optional by defining a default value in the function
 * declaration, because a temp is accepted as default parameter in those cases.
 * It is not as simple when one want to make optional a non-const reference
 * parameter that is usually used as output, in that case as a temp is in theory
 * not acceptable.
 * Yet it is possible to overcome that limitation with the following trick:
 * If not passed as parameter the storage for the output parameter can be
 * dinamically allocated directly by the function call, to avoid leaking memory
 * on each function call the pointer to that storage is made unique so once the
 * function returns it goes out of scope and is automatically deleted.
 * About performance overhead: std::unique_ptr have very good performance and
 * modern compilers may be even able to avoid the dynamic allocation in this
 * case, any way the allocation would only happen if the parameter is not
 * passed, so any effect on performace would happen only in case where the
 * function is called without the parameter.
 */
#define RS_DEFAULT_STORAGE_PARAM(Type,...) *std::unique_ptr<Type>(new Type(__VA_ARGS__))

/** @brief Safely dynamic cast between std::unique_ptr of different types
 * std::unique_ptr semantic rely on the invariant that only one instance own
 * the object, when casting between differents types one would be tempted to do
 * it in a one liner that easly end up breaking that condition ending up in a
 * double delete and crash or in a silent memleak.
 * With this function one can do that with same comfort of a plain dynamic_cast,
 * plus the std::unique_ptr safety.
 * @param[inout] src reference to source pointer. If the cast is successfull it
 *	is released, otherwise it is left untouched.
 * @param[out] dst reference to destination pointer. If the cast is successful
 *	it get reseated to the object address, otherwise it is left untouched.
 * @return true on success, false otherwise
 */
template <class T_SRC, class T_DST>
bool rs_unique_cast(
        std::unique_ptr<T_SRC>& src, std::unique_ptr<T_DST>& dst )
{
	T_DST* dstPtr = dynamic_cast<T_DST*>(src.get());
	if(dstPtr)
	{
		src.release();
		dst.reset(dstPtr);
		return true;
	}
	return false;
}

/** Mark a pointer as non-owned aka it must not be deleted/freed in that context.
 * If a function take an `rs_view_ptr` as paramether it means that she will not
 * own (aka free/delete) the passed memory, instead the caller is in charge of
 * managing it. If a function return an `rs_view_ptr` it means the memory is
 * managed internally and the caller should not call free/delete on it.
 * @see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1408r0.pdf */
template<typename T> using rs_view_ptr = T*;

/** Mark a pointer as owned aka the receiving context is in charge of dealing
 * with it by free/delete once finished.
 * If a function take an `rs_owner_ptr` as paramether it means that she will own
 * (aka free/delete when finished using it) the passed memory, instead the
 * caller is NOT in charge of managing it.
 * If a function return an `rs_owner_ptr` it means the memory is NOT managed
 * internally and the caller should call free/delete on it once finished using
 * it.
 * @see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1408r0.pdf */
template<typename T> using rs_owner_ptr = T*;

void *rs_malloc(size_t size) ;

/** @deprecated use std::unique_ptr instead
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
*/
class RsTemporaryMemory
{
public:
	explicit RsTemporaryMemory(size_t s)
    {
	    _mem = (unsigned char *)rs_malloc(s) ;

	    if(_mem)
		    _size = s ;
	    else
		    _size = 0 ;
    }

    operator unsigned char *() { return _mem ; }
    
    size_t size() const { return _size ; }

	~RsTemporaryMemory() { free(_mem); }

private:
    unsigned char *_mem ;
    size_t _size ;

    // make it noncopyable
    RsTemporaryMemory& operator=(const RsTemporaryMemory&) { return *this ;}
    RsTemporaryMemory(const RsTemporaryMemory&) {}
};

/*******************************************************************************
 * libretroshare/src/util: rsinitedptr.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2010-2010 by Retroshare Team <retroshare.project@gmail.com>       *
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
/**
helper class to store a pointer
it is usefull because it initialises itself to NULL

usage:
replace
    type* ptr;
with
    inited_ptr<type> ptr;

this class can
- get assigned a pointer
- be dereferenced
- be converted back to a pointer
*/
namespace RsUtil{
template<class T> class inited_ptr{
public:
    inited_ptr(): _ptr(NULL){}
    inited_ptr(const inited_ptr<T>& other): _ptr(other._ptr){}
    inited_ptr<T>& operator= (const inited_ptr<T>& other){ _ptr = other._ptr; return *this;}
    inited_ptr<T>& operator= (T* ptr){ _ptr = ptr; return *this;}
    operator T* () const { return _ptr;}
    T* operator ->() const { return _ptr;}
    T& operator *() const { return *_ptr;}
private:
    T* _ptr;
};
}//namespace RsUtil

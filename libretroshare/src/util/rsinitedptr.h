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

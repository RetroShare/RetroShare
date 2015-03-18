#include "Operators.h"

namespace resource_api
{

StreamBase& operator <<(StreamBase& left, KeyValueReference<uint32_t> ref)
{
    if(left.serialise())
    {
        uint32_t num = ref.value;
        uint8_t digit;
        std::string str;
        while(num >= 10)
        {
            digit = num % 10;
            num   = num / 10;
            str += (char)(digit + '0');
        }
        str += (char)(num + '0');
        left << makeKeyValueReference(ref.key, str);
    }
    else
    {
        std::string str;
        left << makeKeyValueReference(ref.key, str);
        uint32_t num = 0;
        for(std::string::iterator sit = str.begin(); sit != str.end(); sit++)
        {
            uint32_t numbefore = num;
            num = num * 10;
            if(num < numbefore)
            {
                left.addErrorMsg("operator for uint32_t to std::string: oveflow");
                left.setError();
            }
            else if((*sit)<'0' || (*sit)>'9')
            {
                left.addErrorMsg("operator for uint32_t to std::string: invalid characters");
                left.setError();
            }
            else
            {
                // everything ok, can add value
                num += (*sit) - '0';
            }
        }
    }
    return left;
}




/*
template<uint32_t ID_SIZE, bool ID_UPPER, uint32_t ID_ID>
StreamBase& operator <<(StreamBase& left, t_RsGenericIdType<ID_SIZE, ID_UPPER, ID_ID>& id)
{
    if(left.serialise())
    {
        left << id.toStdString();
    }
    else
    {
        std::string str;
        left << str;
    }
    return left;
}
*/

template<class T_ID>
StreamBase& operator <<(StreamBase& left, ValueReference<T_ID>& ref)
{
    if(left.serialise())
    {
        left << makeValueReference(ref.value.toStdString());
    }
    else
    {
        std::string str;
        left << makeValueReference(str);
        T_ID id(str);
        if(id.isNull)
        {
            left.setError();
            left.addErrorMsg("operator for retroshare id value: id is null\n");
        }
        ref.value = id;
    }
    return left;
}

// idea: have external operators which do the translation form different containers to basic operations
// better idea: take input iterators as arguments, will then work with everything which has an iterator
// but what about deserilisation?
template<template <class> class ContainerT, class ValueT>
StreamBase& operator<<(StreamBase& left, ContainerT<ValueT>& right)
{
    if(left.serialise())
    {
        typename ContainerT<ValueT>::iterator vit;
        for(vit = right.begin(); vit != right.end(); vit++)
        {
            left << ValueReference<ValueT>(*vit);
        }
    }
    else
    {
        while(left.hasMore())
        {
            ValueReference<ValueT> ref;
            left << ref;
            right.push_back(ref.value);
        }
    }
    return left;
}

// maybe like this:
template<class ItertatorT>
class Array
{
public:
    Array(ItertatorT begin, ItertatorT end): begin(begin), end(end) {}
    ItertatorT begin;
    ItertatorT end;
};





} // namespace resource_api

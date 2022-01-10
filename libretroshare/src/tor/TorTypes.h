#pragma once

#include <vector>
#include <sstream>
#include <string>
#include <stdexcept>

namespace Tor
{

class NonCopiable {
public:
    NonCopiable(){}
    virtual ~NonCopiable()=default;
private:
    NonCopiable(const NonCopiable& nc) {}
    virtual NonCopiable& operator=(const NonCopiable& nc) { return *this ; }
};

class TorByteArray: public std::vector<unsigned char>
{
public:
    TorByteArray(const unsigned char *data,uint32_t len)
    {
        clear();
        for(uint32_t i=0;i<len;++i)
            push_back(data[i]);
    }
    explicit TorByteArray(const std::string& s)
    {
        clear();
        for(uint32_t i=0;i<s.length();++i)
            push_back(s[i]);
    }
    TorByteArray(uint32_t s,unsigned char c)
    {
        clear();
        resize(s,c);
    }
    TorByteArray() { clear() ; }

    bool startsWith(const TorByteArray& s) const
    {
        if(s.size() > size())
            return false;

        for(uint32_t i=0;i<s.size();++i)
            if(s[i] != data()[i])
                return false;

        return true;
    }

    int indexOf(unsigned char c) const
    {
        for(uint32_t i=0;i<size();++i)
            if(data()[i] == c)
                return i;

        return -1;
    }

    const TorByteArray& operator+= (unsigned char s) { push_back(s); return *this ;}

    TorByteArray left(uint32_t n) const
    {
        auto res = TorByteArray();
        for(size_t i=0;i<std::min((size_t)n,size());++i)
            res.push_back(data()[i]);

        return res;
    }

    const TorByteArray& operator+=(const TorByteArray& t)
    {
        for(uint32_t i=0;i<t.size();++i)
            push_back(t.data()[i]);
        return *this;
    }
    const TorByteArray& append(const std::string& s) { return operator+=(TorByteArray(s)); }
    const TorByteArray& append(char s) { return operator+=(s); }

    TorByteArray operator+(const TorByteArray& t) const
    {
        auto res = *this;
        for(uint32_t i=0;i<t.size();++i)
            res.push_back(t[i]);
        return res;
    }

    std::string toStdString() const
    {
        return std::string((const char *)data(),size());
    }

    bool contains(const TorByteArray& b) const
    {
        if(b.size() > size())
            return false;

        for(uint32_t i=0;i<size()-b.size();++i)
        {
            bool c = true;

            for(uint32_t j=0;j<b.size();++j)
                if(b[j] != data()[i+j])
                {
                    c = false;
                    break;
                }

            if(c)
                return true;
        }
        return false;
    }

    TorByteArray mid(uint32_t start,int length=-1) const
    {
        if(length==-1)
            return TorByteArray(data()+start,size()-start);

        if(length < 0 || start + length > size())
            throw std::runtime_error("Length out of range in TorByteArray::mid()");

        TorByteArray b;
        for(uint32_t i=0;i<(uint32_t)length;++i)
            b.push_back(data()[i+start]);

        return b;
    }

    static TorByteArray number(uint64_t n)
    {
        std::ostringstream o;
        o << n ;
        return TorByteArray(o.str());
    }
};

typedef std::string TorHostAddress;
}

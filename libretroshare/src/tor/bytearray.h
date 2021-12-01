#pragma once

#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>
#include <list>

#include "util/rsprint.h"
#include "util/rsdebug.h"

class ByteArray: public std::vector<unsigned char>
{
public:
    ByteArray() =default;
    ByteArray(int n) : std::vector<unsigned char>(n) {}
    ByteArray(const unsigned char *d,int n) : std::vector<unsigned char>(n) { memcpy(data(),d,n); }
    virtual ~ByteArray() =default;

    ByteArray(const std::string& c) { resize(c.size()); memcpy(data(),c.c_str(),c.size()); }
    const ByteArray& operator=(const std::string& c) { resize(c.size()); memcpy(data(),c.c_str(),c.size()); return *this; }

    bool isNull() const { return empty(); }
    ByteArray toHex() const { return ByteArray(RsUtil::BinToHex(data(),size(),0)); }
    std::string toString() const { std::string res; for(auto c:*this) res += c; return res; }

    ByteArray operator+(const ByteArray& b) const { auto res(*this); for(unsigned char c:b) res.push_back(c); return res; }
    ByteArray operator+(const std::string& b) const { return operator+(ByteArray(b)); }

    void append(const ByteArray& b) { for(auto c:b) push_back(c); }
    void append(const char *b) { for(uint32_t n=0;b[n]!=0;++n) push_back(b[n]); }

    ByteArray& operator+=(const ByteArray& b) { for(auto c:b) push_back(c);  return *this; }
    ByteArray& operator+=(const char *b) { for(uint32_t n=0;b[n]!=0;++n) push_back(b[n]); return *this;}

    ByteArray left(uint32_t l) const { auto res = *this; res.resize(std::min((uint32_t)size(),l)); return res; }
    ByteArray toUpper() const { auto res = *this; for(uint32_t i=0;i<size();++i) if( res[i]<='z' && res[i]>='a') res[i] += int('A')-int('a'); return res; }
    ByteArray toLower() const { auto res = *this; for(uint32_t i=0;i<size();++i) if( res[i]<='Z' && res[i]>='A') res[i] += int('a')-int('A'); return res; }

    int toInt() const
    {
        std::istringstream is(toString().c_str());

        int res = 0;
        is >> res ;

        return res;
    }
    bool endsWith(const ByteArray& b) const { return size() >= b.size() && !memcmp(&data()[size()-b.size()],b.data(),b.size()); }
    bool startsWith(const char *b) const
    {
        for(uint32_t n=0;b[n]!=0;++n)
            if(n >= size() || b[n]!=(*this)[n])
                return false;

        return true;
    }

    bool operator==(const char *b) const
    {
        uint32_t n;
        for(n=0;b[n]!=0;++n)
            if(n >= size() || b[n]!=(*this)[n])
                return false;

        return n==size();
    }

    ByteArray mid(uint32_t n,int s=-1) const
    {
        ByteArray res((s>=0)?s:(size()-n));
        memcpy(res.data(),&data()[n],res.size());
        return res;
    }

    int indexOf(unsigned char c,int from=0) const
    {
        for(uint32_t i=from;i<size();++i)
            if((*this)[i]==c)
                return (int)i;
        return -1;
    }

    ByteArray replace(const ByteArray& b1,const ByteArray& b2)
    {
        if(b1.empty())
        {
            RsErr() << "Attempting to replace an empty string!";
            return *this;
        }
        ByteArray res ;

        for(uint32_t i=0;i+b1.size()<=size();)
            if(!memcmp(&(*this)[i],b1.data(),b1.size()))
            {
                res.append(b2);
                i += b1.size();
            }
            else
                res.push_back((*this)[i++]);

        return res;
    }

    std::list<ByteArray> split(unsigned char sep)
    {
        std::list<ByteArray> res;
        ByteArray current_block;

        for(uint32_t i=0;i<size();++i)
            if(operator[](i) == sep)
            {
                res.push_back(current_block);
                current_block.clear();
            }
            else
                current_block += operator[](i);

        return res;
    }

    // Removes the following characters from the beginning and from the end of the array:
    //    '\t', '\n', '\v', '\f', '\r', and ' '.

    ByteArray trimmed() const
    {
        auto res(*this);

        while(!res.empty() && (    res.back() == '\t' || res.back() == '\n' || res.back() == '\v'
                                   || res.back() == '\f' || res.back() == '\r' || res.back() == ' ' ) )
            res.pop_back();

        uint32_t i=0;

        for(;i<res.size();++i)
            if(res[i] != '\t' && res[i] != '\n' && res[i] != '\v'  && res[i] != '\f' && res[i] != '\r' && res[i] != ' ')
                break;

        return res.mid(i);
    }
    
    // Removes n bytes from the end of the array
    
    void chop(uint32_t n)
    {
        resize(std::max(0,(int)size() - (int)n));
    }
    
    // Returns the last index of a given byte, -1 if not found.
    
    int lastIndexOf(unsigned char s)
    {
        for(int i=size()-1;i>=0;--i)
            if(operator[](i) == s)
                return i;
        
        return -1;
    }
};

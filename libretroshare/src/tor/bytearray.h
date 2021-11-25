#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

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
    ByteArray toUpper() const { auto res = *this; for(uint32_t i=0;i<size();++i) if( res[i]<='z' && res[i]>='a') res[i] += 'A'-'a'; return res; }

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
};

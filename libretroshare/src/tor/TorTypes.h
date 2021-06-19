#pragma once

#include <vector>
#include <string>

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
    TorByteArray(const std::string& s = std::string())
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

    bool startsWith(const std::string& s) const
    {
        if(s.length() > size())
            return false;

        for(uint32_t i=0;i<s.length();++i)
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
    const TorByteArray& append(const std::string& s) { return operator+=(s); }
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
};

typedef std::string TorHostAddress;
}
typedef Tor::TorByteArray QByteArray; // to be removed

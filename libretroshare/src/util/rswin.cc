#include "util/rswin.h"


namespace librs { namespace util {


bool ConvertUtf8ToUtf16(const std::string& source, std::wstring& dest) {
    int nbChars = MultiByteToWideChar(CP_UTF8, 0,
                                      source.c_str(), -1,
                                      0, 0);
    if(nbChars == 0)
        return false;

    wchar_t* utf16Name = new wchar_t[nbChars];
    if( MultiByteToWideChar(CP_UTF8, 0,
                            source.c_str(), -1,
                            utf16Name, nbChars) == 0) {
        return false;
    }

    dest = utf16Name;
    delete[] utf16Name;

    return true;
}

bool ConvertUtf16ToUtf8(const std::wstring& source, std::string& dest) {
    int nbChars = WideCharToMultiByte(CP_UTF8, 0,
                                      source.c_str(), -1,
                                      0, 0,
                                      0, 0);
    if(nbChars == 0)
        return false;

    char* utf8Name = new char[nbChars];
    if( WideCharToMultiByte(CP_UTF8, 0,
                            source.c_str(), -1,
                            utf8Name, nbChars,
                            0, 0) == 0) {
        return false;
    }

    dest = utf8Name;
    delete[] utf8Name;
    return true;
}


} } // librs::util

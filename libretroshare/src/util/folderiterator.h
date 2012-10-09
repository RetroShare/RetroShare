#ifndef FOLDERITERATOR_H
#define FOLDERITERATOR_H


#include <iostream>
#include <cstdio>

#ifdef WINDOWS_SYS
    #include <windows.h>
    #include <tchar.h>
    #include <stdio.h>
    #include <string.h>
#else
    #include <dirent.h>
#endif


namespace librs { namespace util {


class FolderIterator
{
public:
    FolderIterator(const std::string& folderName);
    ~FolderIterator();

    bool isValid() const    { return validity; }

    bool readdir();

    bool d_name(std::string& dest);

    bool closedir();

private:
    bool validity;

#ifdef WINDOWS_SYS
    HANDLE handle;
    bool isFirstCall;
    _WIN32_FIND_DATAW fileInfo;
#else
    DIR* handle;
    struct dirent* ent;
#endif

};


} } // librs::util


#endif // FOLDERITERATOR_H

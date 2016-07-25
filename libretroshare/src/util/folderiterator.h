#ifndef FOLDERITERATOR_H
#define FOLDERITERATOR_H


#include <stdint.h>
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

    enum { TYPE_UNKNOWN = 0x00,
           TYPE_FILE    = 0x01,
           TYPE_DIR     = 0x02
         };

    bool isValid() const    { return validity; }
    bool readdir();
    void next();

    bool d_name(std::string& dest);
    bool closedir();

    const std::string& file_name() ;
    const std::string& file_fullpath() ;
    uint64_t file_size() ;
    uint8_t file_type() ;
    time_t   file_modtime() ;

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
    void updateStatsInfo() ;

    bool mStatInfoOk ;
    time_t mFileModTime ;
    uint64_t mFileSize ;
    uint8_t mType ;
    std::string mFileName ;
    std::string mFullPath ;
    std::string mFolderName ;
};


} } // librs::util


#endif // FOLDERITERATOR_H

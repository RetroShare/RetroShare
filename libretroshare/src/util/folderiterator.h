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

    // info about current parent directory
    time_t dir_modtime() const ;

    // info about directory content

    bool isValid() const    { return validity; }
    bool readdir();
    void next();

    bool closedir();

    const std::string& file_name() ;
    const std::string& file_fullpath() ;
    uint64_t file_size() ;
    uint8_t file_type() ;
    time_t   file_modtime() ;

private:
    bool is_open;
    bool validity;

#ifdef WINDOWS_SYS
    HANDLE handle;
    bool isFirstCall;
    _WIN32_FIND_DATAW fileInfo;
#else
    DIR* handle;
    struct dirent* ent;
#endif
    bool updateFileInfo(bool &should_skip) ;

    time_t mFileModTime ;
    time_t mFolderModTime ;
    uint64_t mFileSize ;
    uint8_t mType ;
    std::string mFileName ;
    std::string mFullPath ;
    std::string mFolderName ;
};


} } // librs::util


#endif // FOLDERITERATOR_H

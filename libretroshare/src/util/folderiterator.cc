#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef WINDOWS_SYS
#include "util/rswin.h"
#endif

#include "folderiterator.h"
#include "rsstring.h"

//#define DEBUG_FOLDER_ITERATOR 1

namespace librs { namespace util {


FolderIterator::FolderIterator(const std::string& folderName)
    : mFolderName(folderName)
{
    // Grab the last modification time for the directory

    struct stat64 buf ;

#ifdef WINDOWS_SYS
    std::wstring wfullname;
    librs::util::ConvertUtf8ToUtf16(folderName, wfullname);
    if ( 0 == _wstati64(wfullname.c_str(), &buf))
#else
    if ( 0 == stat64(folderName.c_str(), &buf))
#endif
    {
        mFolderModTime = buf.st_mtime ;
    }

    // Now open directory content and read the first entry

#ifdef WINDOWS_SYS
    std::wstring utf16Name;
    if(! ConvertUtf8ToUtf16(folderName, utf16Name)) {
        validity = false;
        return;
    }

    utf16Name += L"/*.*";

    handle = FindFirstFileW(utf16Name.c_str(), &fileInfo);
    is_open = validity = handle != INVALID_HANDLE_VALUE;
#else
    handle = opendir(folderName.c_str());
    is_open = validity = handle != NULL;
    next();
#endif
}

FolderIterator::~FolderIterator()
{
    closedir();
}

void FolderIterator::next()
{
    while(readdir())
    {
        mFileName = ent->d_name ;

        if(mFileName == "." || mFileName == "..")
            continue ;

        mFullPath = mFolderName + "/" + mFileName ;

        struct stat64 buf ;

#ifdef DEBUG_FOLDER_ITERATOR
        std::cerr << "FolderIterator: next. Looking into file " << mFileName ;
#endif

#ifdef WINDOWS_SYS
        std::wstring wfullname;
        librs::util::ConvertUtf8ToUtf16(mFullPath, wfullname);
        if ( 0 == _wstati64(wfullname.c_str(), &buf))
#else
        if ( 0 == stat64(mFullPath.c_str(), &buf))
#endif
        {
            mFileModTime = buf.st_mtime ;
            mStatInfoOk = true;

            if (S_ISDIR(buf.st_mode))
            {
#ifdef DEBUG_FOLDER_ITERATOR
                std::cerr << ": is a directory" << std::endl;
#endif

                mType = TYPE_DIR ;
                mFileSize = 0 ;
                mFileModTime = buf.st_mtime;

                return ;
            }

            if (S_ISREG(buf.st_mode))
            {
#ifdef DEBUG_FOLDER_ITERATOR
                std::cerr << ": is a file" << std::endl;
#endif

                mType = TYPE_FILE ;
                mFileSize = buf.st_size;
                mFileModTime = buf.st_mtime;

                return ;
            }
        }

#ifdef DEBUG_FOLDER_ITERATOR
        std::cerr << ": is unknown skipping" << std::endl;
#endif

        mType = TYPE_UNKNOWN ;
        mFileSize = 0 ;
        mFileModTime = 0;
    }
#ifdef DEBUG_FOLDER_ITERATOR
    std::cerr << "End of directory." << std::endl;
#endif

    mType = TYPE_UNKNOWN ;
    mFileSize = 0 ;
    mFileModTime = 0;
    validity = false ;
}

bool FolderIterator::readdir()
{
    if(!validity)
        return false;

#ifdef WINDOWS_SYS
    return FindNextFileW(handle, &fileInfo) != 0;
#else
    ent = ::readdir(handle);
    return ent != NULL;
#endif
}

time_t FolderIterator::dir_modtime() const { return mFolderModTime ; }

const std::string& FolderIterator::file_fullpath() { return mFullPath ; }
const std::string& FolderIterator::file_name()     { return mFileName ; }
uint64_t           FolderIterator::file_size()     { return mFileSize ; }
time_t             FolderIterator::file_modtime()  { return mFileModTime ; }
uint8_t            FolderIterator::file_type()     { return mType ; }

bool FolderIterator::closedir()
{
    validity = false;

    if(!is_open)
        return true ;

    is_open = false ;

#ifdef WINDOWS_SYS
    return FindClose(handle) != 0;
#else
    return ::closedir(handle) == 0;
#endif
}




} } // librs::util

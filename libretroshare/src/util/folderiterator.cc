#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "folderiterator.h"
#include "rsstring.h"

namespace librs { namespace util {


FolderIterator::FolderIterator(const std::string& folderName)
    : mFolderName(folderName)
{
#ifdef WINDOWS_SYS
    std::wstring utf16Name;
    if(! ConvertUtf8ToUtf16(folderName, utf16Name)) {
        validity = false;
        return;
    }

    utf16Name += L"/*.*";

    handle = FindFirstFileW(utf16Name.c_str(), &fileInfo);
    validity = handle != INVALID_HANDLE_VALUE;
#else
    handle = opendir(folderName.c_str());
    validity = handle != NULL;
    next();
#endif
}

FolderIterator::~FolderIterator()
{
    closedir();
}

void FolderIterator::next()
{
    do {
        if(!readdir())
        {
            validity = false ;
            break ;
        }

        d_name(mFileName);
    } while(mFileName == "." || mFileName == "..") ;

    mFullPath = mFolderName + "/" + mFileName ;

    struct stat64 buf ;

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
            mType = TYPE_DIR ;
            mFileSize = 0 ;
            mFileModTime = buf.st_mtime;
        }
        else if (S_ISREG(buf.st_mode))
        {
            mType = TYPE_FILE ;
            mFileSize = buf.st_size;
            mFileModTime = buf.st_mtime;
        }
        else
        {
            mType = TYPE_UNKNOWN ;
            mFileSize = 0 ;
            mFileModTime = 0;
        }
    }
    else
    {
            mType = TYPE_UNKNOWN ;
            mFileSize = 0 ;
            mFileModTime = 0;
            validity = false ;
    }
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

bool FolderIterator::d_name(std::string& dest)
{
    if(!validity)
        return false;

#ifdef WINDOWS_SYS
    if(! ConvertUtf16ToUtf8(fileInfo.cFileName, dest)) {
        validity = false;
        return false;
    }
#else
    if(ent == 0)
        return false;
    dest = ent->d_name;
#endif

    return true;
}

const std::string& FolderIterator::file_fullpath() { return mFullPath ; }
const std::string& FolderIterator::file_name()     { return mFileName ; }
uint64_t           FolderIterator::file_size()     { return mFileSize ; }
time_t             FolderIterator::file_modtime()  { return mFileModTime ; }
uint8_t            FolderIterator::file_type()     { return mType ; }

bool FolderIterator::closedir()
{
    if(!validity)
        return false;

    validity = false;

#ifdef WINDOWS_SYS
    return FindClose(handle) != 0;
#else
    return ::closedir(handle) == 0;
#endif
}




} } // librs::util

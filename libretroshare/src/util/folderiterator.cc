#include "folderiterator.h"
#include "rswin.h"


namespace librs { namespace util {


FolderIterator::FolderIterator(const std::string& folderName)
{
#ifdef WINDOWS_SYS
    std::wstring utf16Name;
    if(! ConvertUtf8ToUtf16(folderName, utf16Name)) {
        validity = false;
        return;
    }

    utf16Name += L"\\*.*";

    handle = FindFirstFileW(utf16Name.c_str(), &fileInfo);
    validity = handle != INVALID_HANDLE_VALUE;
    isFirstCall = true;
#else
    handle = opendir(folderName.c_str());
    validity = handle != NULL;
#endif
}

FolderIterator::~FolderIterator()
{
    closedir();
}

bool FolderIterator::readdir() {
    if(!validity)
        return false;

#ifdef WINDOWS_SYS
    if(isFirstCall) {
        isFirstCall = false;
        return true;
    }
    return FindNextFileW(handle, &fileInfo) != 0;
#else
    return readdir(handle) == 0;
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
    dest = handle->d_name;
#endif

    return true;
}

bool FolderIterator::closedir()
{
    if(!validity)
        return false;

    validity = false;

#ifdef WINDOWS_SYS
    return FindClose(handle) != 0;
#else
    return closedir(handle) == 0;
#endif
}




} } // librs::util

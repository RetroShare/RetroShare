#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <errno.h>
#include "bdfile.h"

namespace librs { 
	namespace util {
		bool ConvertUtf8ToUtf16(const std::string& source, std::wstring& dest) ;
	} 
}

bool bdFile::renameFile(const std::string& from, const std::string& to)
{
	int loops = 0;

#ifdef WINDOWS_SYS
	std::wstring f;
	librs::util::ConvertUtf8ToUtf16(from, f);
	std::wstring t;
	librs::util::ConvertUtf8ToUtf16(to, t);

	while (!MoveFileEx(f.c_str(), t.c_str(), MOVEFILE_REPLACE_EXISTING))
#else
	std::string f(from),t(to) ;

	while (rename(from.c_str(), to.c_str()) < 0)
#endif
	{
#ifdef WIN32
		if (GetLastError() != ERROR_ACCESS_DENIED)
#else
		if (errno != EACCES)
#endif
			/* set errno? */
			return false ;
#ifdef WIN32
		Sleep(100000);				/* us */
#else
		usleep(100000);				/* us */
#endif

		if (loops >= 30)
			return false ;

		loops++;
	}

	return true ;
}


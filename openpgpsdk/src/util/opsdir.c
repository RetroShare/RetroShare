#include "opsdir.h"
#include "opsstring.h"

#include <fcntl.h>

int ops_open(const char* filename, int flag, int pmode)
{
#ifdef WIN32
	wchar_t *wfilename = ConvertUtf8ToUtf16(filename);
	if (!wfilename)
	{
		return -1;
	}

	int result = _wopen(wfilename, flag, pmode);
	free(wfilename);

	return result;
#else
	return open(filename, flag, pmode);
#endif
}

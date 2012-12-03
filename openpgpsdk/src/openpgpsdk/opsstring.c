#include "opsstring.h"

#ifdef WIN32
wchar_t *ConvertUtf8ToUtf16(const char* source)
{
	if (!source) {
		return NULL;
	}

#ifdef WIN32
	int nbChars = MultiByteToWideChar(CP_UTF8, 0, source, -1, 0, 0);
	if (nbChars == 0) {
		return NULL;
	}

	wchar_t* utf16Name = (wchar_t*) malloc(nbChars * sizeof(wchar_t));
	if (MultiByteToWideChar(CP_UTF8, 0, source, -1, utf16Name, nbChars) == 0) {
		free(utf16Name);
		return NULL;
	}

	return utf16Name;
#else
	// currently only for WIN32
	// convert code from rsstring.cc
	return NULL;
#endif
}

char* ConvertUtf16ToUtf8(const wchar_t *source)
{
	if (!source) {
		return NULL;
	}

#ifdef WIN32
	int nbChars = WideCharToMultiByte(CP_UTF8, 0, source, -1, 0, 0, 0, 0);
	if (nbChars == 0) {
		return NULL;
	}

	char* utf8Name = (char*) malloc(nbChars * sizeof(char));
	if (WideCharToMultiByte(CP_UTF8, 0, source, -1, utf8Name, nbChars, 0, 0) == 0) {
		free(utf8Name);
		return NULL;
	}

	return utf8Name;
#else
	// currently only for WIN32
	// convert code from rsstring.cc
	return NULL;
#endif
}
#endif

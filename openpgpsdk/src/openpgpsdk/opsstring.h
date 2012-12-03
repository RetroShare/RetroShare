#ifndef OPSSTRING_H
#define OPSSTRING_H

#ifdef WIN32
#include <wtypes.h>
#endif

#ifdef WIN32
// currently only for WIN32

// Convert strings between UTF8 and UTF16
// Don't forget to free the returned string.
wchar_t* ConvertUtf8ToUtf16(const char *source);
char*    ConvertUtf16ToUtf8(const wchar_t* source);
#endif

#endif // OPSSTRING_H

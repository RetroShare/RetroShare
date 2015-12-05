#ifndef CONTENTTYPES_H
#define CONTENTTYPES_H

#include <util/rsthreads.h>
#include <map>
#include <string>

#define DEFAULTCT "application/octet-stream"

class ContentTypes
{
public:
	static std::string cTypeFromExt(const std::string& extension);

private:
	static std::map<std::string, std::string> cache;
	static RsMutex ctmtx;
	static const char* filename;
	static bool inited;
	static void addBasic();
};

#endif // CONTENTTYPES_H

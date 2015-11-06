#include "ContentTypes.h"
#include <fstream>

RsMutex ContentTypes::ctmtx = RsMutex("CTMTX");
std::map<std::string, std::string> ContentTypes::cache;

#ifdef WINDOWS_SYS
	//Next to the executable
	const char* ContentTypes::filename = ".\\mime.types";
#else
	const char* ContentTypes::filename = "/etc/mime.types";
#endif

std::string ContentTypes::cTypeFromExt(const std::string &extension)
{
	if(extension.empty())
		return DEFAULTCT;

	RsStackMutex mtx(ctmtx);

	//looking into the cache
	std::map<std::string,std::string>::iterator it;
	it = cache.find(extension);
	if (it != cache.end())
	{
		std::cout << "Mime " + it->second + " for extension ." + extension + " was found in cache" << std::endl;
		return it->second;
	}

	//looking into mime.types
	std::string line;
	std::string ext;
	std::ifstream file(filename);
	while(getline(file, line))
	{
		if(line.empty() || line[0] == '#') continue;
		unsigned int i = line.find_first_of("\t ");
		unsigned int j;
		while(i != std::string::npos)	//tokenize
		{
			j = i;
			i = line.find_first_of("\t ", i+1);
			if(i == std::string::npos)
				ext = line.substr(j+1);
			else
				ext = line.substr(j+1, i-j-1);

			if(extension == ext)
			{
				std::string mime = line.substr(0, line.find_first_of("\t "));
				cache[extension] = mime;
				std::cout << "Mime " + mime + " for extension ." + extension + " was found in mime.types" << std::endl;
				return mime;
			}
		}
	}

	//nothing found
	std::cout << "Mime for " + extension + " was not found in " + filename + " falling back to " << DEFAULTCT << std::endl;
	cache[extension] = DEFAULTCT;
	return DEFAULTCT;
}


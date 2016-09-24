#include "ContentTypes.h"
#include <fstream>
#include <cctype>
#include <algorithm>

RsMutex ContentTypes::ctmtx = RsMutex("CTMTX");
std::map<std::string, std::string> ContentTypes::cache;
bool ContentTypes::inited = false;

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

	if(!inited)
		addBasic();

	std::string extension2(extension); //lower case
	std::transform(extension2.begin(), extension2.end(), extension2.begin(),::tolower);

	//looking into the cache
	std::map<std::string,std::string>::iterator it;
	it = cache.find(extension2);
	if (it != cache.end())
	{
		std::cout << "Mime " + it->second + " for extension ." + extension2 + " was found in cache" << std::endl;
		return it->second;
	}

	//looking into mime.types
	std::string line;
	std::string ext;
	std::ifstream file(filename);
	while(getline(file, line))
	{
		if(line.empty() || line[0] == '#') continue;
        size_t i = line.find_first_of("\t ");
        size_t j;
		while(i != std::string::npos)	//tokenize
		{
			j = i;
			i = line.find_first_of("\t ", i+1);
			if(i == std::string::npos)
				ext = line.substr(j+1);
			else
				ext = line.substr(j+1, i-j-1);

			if(extension2 == ext)
			{
				std::string mime = line.substr(0, line.find_first_of("\t "));
				cache[extension2] = mime;
				std::cout << "Mime " + mime + " for extension ." + extension2 + " was found in mime.types" << std::endl;
				return mime;
			}
		}
	}

	//nothing found
	std::cout << "Mime for " + extension2 + " was not found in " + filename + " falling back to " << DEFAULTCT << std::endl;
	cache[extension2] = DEFAULTCT;
	return DEFAULTCT;
}

//Add some basic content-types before first use.
//It keeps webui usable in the case of mime.types file not exists.
void ContentTypes::addBasic()
{
	inited = true;

	cache["html"] = "text/html";
	cache["css"] = "text/css";
	cache["js"] = "text/javascript";
	cache["jsx"] = "text/jsx";
	cache["png"] = "image/png";
	cache["jpg"] = "image/jpeg";
	cache["jpeg"] = "image/jpeg";
	cache["gif"] = "image/gif";
}


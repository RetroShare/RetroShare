#include "ftsearch_test.h"

bool 	ftSearchDummy::search(std::string /*hash*/, FileSearchFlags hintflags, FileInfo &/*info*/) const
{
	/* remove unused parameter warnings */
	(void) hintflags;

#ifdef DEBUG_SEARCH
	std::cerr << "ftSearchDummy::search(" << hash ;
	std::cerr << ", " << hintflags << ");";
	std::cerr << std::endl;
#endif
	return false;
}



#include "ftsearch_dummy.h"

bool 	ftSearchDummy::search(const std::string& /*hash*/, FileSearchFlags hintflags, FileInfo &/*info*/) const
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



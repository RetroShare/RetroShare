/*
 * "$Id: ftest.cc,v 1.5 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * Other Bits for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

/* Example Test of filedex index */

#include "filedex.h"
#include <iostream>

int main()
{
	filedex fd;
	std::string term;
	std::list<DirItem> dirs;
	std::list<std::string> terms;
	std::list<fdex *> results;
	std::list<fdex *>::iterator it;

	dirs.push_back(DirItem("/usr/local", "", 1));
	dirs.push_back(DirItem("/var/log", "", 2));

	fd.load(dirs);

	// now show the root directories...
	results.clear();
	results = fd.dirlisting("");

	std::cerr << "Root Directory Results:" << std::endl;
	for(it = results.begin(); it != results.end(); it++)
	{
		std::cerr << "Full Path: " << (*it) -> path << std::endl;
		std::cerr << "\tFile:" << (*it) -> file << std::endl;
		std::cerr << "\tExt:" << (*it) -> ext;
		std::cerr << " Size: " << (*it) -> len << std::endl;
		std::cerr << "\tDir:" << (*it) -> subdir << std::endl;
	}

	while(1)
	{
		std::cerr << "Enter Search Term :";
		std::cin >> term;
		std::cerr << "Searching For:" << term << std::endl;
		terms.clear();
		terms.push_back(term);

		results.clear();
		results = fd.search(terms, 10, false);

		std::cerr << "Search Results:" << std::endl;
		for(it = results.begin(); it != results.end(); it++)
		{
			std::cerr << "Full Path: " << (*it) -> path << std::endl;
			std::cerr << "\tFile:" << (*it) -> file << std::endl;
			std::cerr << "\tExt:" << (*it) -> ext;
			std::cerr << " Size: " << (*it) -> len << std::endl;
			std::cerr << "\tDir:" << (*it) -> subdir << std::endl;
		}
		
		results.clear();
		results = fd.dirlisting(term);

		std::cerr << "FileName Results:" << std::endl;
		for(it = results.begin(); it != results.end(); it++)
		{
			std::cerr << "Full Path: " << (*it) -> path << std::endl;
			std::cerr << "\tFile:" << (*it) -> file << std::endl;
			std::cerr << "\tExt:" << (*it) -> ext;
			std::cerr << " Size: " << (*it) -> len << std::endl;
			std::cerr << "\tDir:" << (*it) -> subdir << std::endl;
		}

		results.clear();
		results = fd.findfilename(term);

		std::cerr << "FileName Results:" << std::endl;
		for(it = results.begin(); it != results.end(); it++)
		{
			std::cerr << "Full Path: " << (*it) -> path << std::endl;
			std::cerr << "\tFile:" << (*it) -> file << std::endl;
			std::cerr << "\tExt:" << (*it) -> ext;
			std::cerr << " Size: " << (*it) -> len << std::endl;
			std::cerr << "\tDir:" << (*it) -> subdir << std::endl;
		}
	}

	//sleep(5);
	return 1;
}



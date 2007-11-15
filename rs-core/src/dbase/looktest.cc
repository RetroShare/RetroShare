/*
 * "$Id: looktest.cc,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
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




#include "filedex.h"
#include "dbase/filelook.h"
#include <iostream>

#include "pqi/pqi.h"

int main()
{
	fileLook *fl = new fileLook();
	std::string term;
	std::list<std::string> dirs;
	std::list<std::string> terms;
	std::list<fdex *> results;
	std::list<fdex *>::iterator it;

	dirs.push_back("/mnt/disc2/extra/rmf24/mp3s/good");
	dirs.push_back("/mnt/disc2/extra/rmf24/mp3s/marks");
	dirs.push_back("/mnt/disc2/extra/rmf24/mp3s/incoming");

	fl -> start(); /* background look thread */

	std::cerr << "FileLook Thread started" << std::endl;

	fl -> setSharedDirectories(dirs);

	PQFileItem *i = new PQFileItem();


	std::cerr << "test Lookup ..." << std::endl;
	fl -> lookUpDirectory(i);
	std::cerr << "Entering Main Loop" << std::endl;

	while(1)
	{
		std::cerr << "Enter Search Term :";
		std::cin >> term;
		std::cerr << "Searching For:" << term << std::endl;
		terms.clear();
		terms.push_back(term);

		PQFileItem *i = new PQFileItem();
		i -> path = term;

		std::cerr << "Root Lookup ..." << std::endl;
		fl -> lookUpDirectory(i);
		std::cerr << "LookUp done" << std::endl;

		while(NULL != (i = (PQFileItem *) fl -> getResult()))
		{
			std::cerr << "Results:" << std::endl;
			i -> print(std::cerr);
			std::cerr << std::endl;
		}

	}

	//sleep(5);
	return 1;
}



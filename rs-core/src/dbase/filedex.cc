/*
 * "$Id: filedex.cc,v 1.8 2007-02-18 21:46:49 rmf24 Exp $"
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

#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

fdex::fdex()
{
	return;
}

fdex::fdex(const char *p, const char *d, const char *f, const char *e, int l, int v)
	:path(p), subdir(d), file(f), ext(e), len(l), vis(v)
{
	return;
}


int	filedex::load(std::list<DirItem> dirs)
{
	std::list<DirItem>::iterator it;

	/* these must be done in the correct order to ensure
	 * that the visibility is correct
	 */
	for(int i = FD_VIS_EXACT_ONLY; i <= FD_VIS_LISTING; i++)
	{
		for(it = dirs.begin(); it != dirs.end(); it++)
		{
			if (i == it -> vis)
			{
				//std::cerr << "Adding Type(" << it -> vis << ") " << it -> basepath;
				//std::cerr << std::endl;

				dirtodo.push_back(*it);
			}
		}
	}
	return processdirs();
}

int	filedex::clear()
{
	std::list<fdex *>::iterator it;
	dirtodo.clear();
	dirdone.clear();

	for(it = files.begin(); it != files.end(); it++)
	{
		delete(*it);
	}
	files.clear();

	for(it = dirlist.begin(); it != dirlist.end(); it++)
	{
		delete(*it);
	}
	dirlist.clear();
	return 1;
}

std::string strtolower2(std::string in)
{
	std::string out = in;
	for(int i = 0; i < (signed) out.length(); i++)
	{
		if (isupper(out[i]))
		{
			out[i] +=  'a' - 'A';
		}
	}
	return out;
}

std::string fileextension2(std::string in)
{
	std::string out;
	bool found = false;
	for(int i = in.length() -1; (i >= 0) && (found == false); i--)
	{
		if (in[i] == '.')
		{
			found = true;
			for(int j = i+1; j < (signed) in.length(); j++)
			{
				out += in[j];
			}
		}
	}
	return strtolower2(out);
}

std::list<fdex *> filedex::search(std::list<std::string> terms, int limit, bool local)
{
	bool found;
	std::list<fdex *> newlist;
	int matches = 0;

	std::list<fdex *>::iterator fit;
	std::list<std::string>::iterator tit;

	//std::cerr << "filedex::search() looking for" << std::endl;
	//for(tit = terms.begin(); tit != terms.end(); tit++)
	//	std::cerr << "\t(" << *tit << ")" << std::endl;

	//std::cerr << "Checking:" << std::endl;

	for(fit = files.begin(); fit != files.end(); fit++)
	{
		// ignore named only files.
		if ((!local) && ((*fit)->vis < FD_VIS_SEARCH))
			continue;
		found = true;
		for(tit = terms.begin(); (tit != terms.end()) && (found); tit++)
		{
			std::string path = (*fit) -> subdir+"/"+(*fit)->file;
			std::string lowerpath = strtolower2(path);
			std::string lowerterm = strtolower2(*tit);

			//std::cerr << "\tLooking for (" << lowerterm;
			//std::cerr << ") in (" << lowerpath << ")  ";

			if (strstr(lowerpath.c_str(), lowerterm.c_str()) == NULL)
			{
				found = false;
				//std::cerr << "\tFalse..." << std::endl;
			}
			else
			{
				//std::cerr << "\tTrue..." << std::endl;
			}
		}
		if (found)
		{
			//std::cerr << "Found Matching!" << std::endl;
			newlist.push_back(*fit);
			if (++matches == limit)
			{
				//std::cerr << "Reached Limit(" << limit << ")";
				//std::cerr << "Returning Results" << std::endl;
				return newlist;
			}
		}
	}
	return newlist;
}


std::list<fdex *> filedex::dirlisting(std::string basedir)
{
	std::list<fdex *> newlist;
	std::list<fdex *>::iterator fit;

	//std::cerr << "filedex::dirlisting() looking for subdir: " << basedir << std::endl;

	//std::cerr << "Checking:" << std::endl;

	for(fit = dirlist.begin(); fit != dirlist.end(); fit++)
	{
		//std::cerr << "DCHK(" << basedir << ") vs (" << (*fit) -> subdir << ")" << std::endl;
		if (basedir == (*fit) -> subdir)
		{
			/* in the dir */
			//std::cerr << "Found Matching SubDir:";
			//std::cerr << (*fit) -> subdir << std::endl;
			newlist.push_back(*fit);
		}
	}

	for(fit = files.begin(); fit != files.end(); fit++)
	{
		//std::cerr << "FCHK(" << basedir << ") vs (" << (*fit) -> subdir << ")" << std::endl;
		if (basedir == (*fit) -> subdir)
		{
			/* in the dir */
			//std::cerr << "Found Matching File:";
			//std::cerr << (*fit) -> subdir << std::endl;
			newlist.push_back(*fit);
		}
	}
	return newlist;
}


std::list<fdex *> filedex::findfilename(std::string name)
{
	std::list<fdex *> newlist;
	std::list<fdex *>::iterator fit;

	std::cerr << "filedex::findfilename() looking for: " << name << std::endl;

	std::cerr << "Checking:" << std::endl;
	for(fit = files.begin(); fit != files.end(); fit++)
	{
		if (name == (*fit) -> file)
		{
			/* in the dir */
			std::cerr << "Found Matching File!" << std::endl;
			newlist.push_back(*fit);
		}
	}
	return newlist;
}


int	filedex::processdirs()
{
	std::list<DirItem>::iterator it;
	std::list<std::string>::iterator sit;
	std::string dirname;
	std::string subdir;
	std::string fname;
	std::string fullname;
	struct dirent *ent;
	struct stat buf;
	bool found = false;
	fdex *fx;
	int count = 0;
	int ficount = 0;
	while(dirtodo.size() > 0)
	{
		count++;
		it = dirtodo.begin();
		DirItem currDir(*it);
		dirname = currDir.getFullPath();
		dirtodo.erase(it);

		//std::cerr << "\tExamining: " << currDir.basepath;
		//std::cerr << " -/- " << currDir.subdir << std::endl;
		//std::cerr << "\t\t" <<  count << " Directories done, ";
		//std::cerr << dirtodo.size() << " to go! " << std::endl;

		// open directory.
		DIR *dir = opendir(dirname.c_str());

		if (dir != NULL) {
		// read the directory.
		while(NULL != (ent = readdir(dir)))
		{
			fname = ent -> d_name;
			fullname = dirname + "/" + fname;
			subdir  = currDir.subdir;
		//	std::cerr << "Statting dirent: " << fullname.c_str() <<std::endl;
		//	std::cerr << "Statting dirent: " << fullname <<std::endl;

			// stat each file.
			if (-1 != stat(fullname.c_str(), &buf))
			{
		//std::cerr << "buf.st_mode: " << buf.st_mode <<std::endl;
				if (S_ISDIR(buf.st_mode))
				{
		//std::cerr << "Is Directory: " << fullname << std::endl;
				  found = false;
				  // remove "." and ".." entries.
				  if ((fname == ".") || (fname == ".."))
				  { 
					  found = true;
				  }

				  // check if in list of done already.
				  for(it = dirtodo.begin(); it != dirtodo.end(); it++)
				  {
					if (it -> getFullPath() == fullname)
						found = true;
				  }
				  for(sit = dirdone.begin(); sit != dirdone.end(); sit++)
				  {
					if ((*sit) == fullname)
						found = true;
				  }
				  if (found == false)
				  {
					// add to list to read.
					DirItem ndir(currDir.basepath, subdir+"/"+fname, currDir.vis);
					// Push to the front of the list -> so processing done in order.
					dirtodo.push_front(ndir);
		//			std::cerr << "\t Adding Directory: " << fullname;
		//			std::cerr << " to dirtodo" << std::endl;

					if (currDir.vis == FD_VIS_LISTING)
					{
						// add to dirlist dbase.
						fx = new fdex(fullname.c_str(), subdir.c_str(), 
						fname.c_str(), "DIR", 0, currDir.vis);
						dirlist.push_back(fx);
		//				std::cerr << "\t Adding Directory: " << fullname;
		//				std::cerr << " to dirlist" << std::endl;
					}
				  }
				}
				else if (S_ISREG(buf.st_mode))
				{
					// add to dbase.
					fx = new fdex(fullname.c_str(), subdir.c_str(), 
						fname.c_str(), (fileextension2(fname)).c_str(), 
						buf.st_size, currDir.vis);

					//fx -> path = fullname;
					files.push_back(fx);
					ficount++;
					if (ficount % 100 == 0)
					{
				//std::cerr << "\tIndex(" << ficount << ") : ";
				//std::cerr << "\tIndexing File: " << fname;
				//std::cerr << std::endl;
					}

				}
				else
				{
					// warning.
					std::cerr << "\t Ignoring Unknown FileType";
					std::cerr << std::endl;
				}
			}
			else
			{

			}

		}
		// should check error message.
		dirdone.push_back(dirname);
		closedir(dir);
		} 
		else
		{
			//std::cerr << "Cannot Open Directory:" << dirname << std::endl;
		}

	}
	return 1;
}



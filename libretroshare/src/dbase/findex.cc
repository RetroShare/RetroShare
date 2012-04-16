/*
 * RetroShare FileCache Module: findex.cc
 *
 * Copyright 2004-2007 by Robert Fernie, Kefei Zhou.
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

#include <util/rswin.h>
#include "dbase/findex.h"
#include "retroshare/rsexpr.h"
#include "util/rsdir.h"
#include "util/rsstring.h"

#include <assert.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <sstream> // for std::stringstream
#include <tr1/unordered_set>
#include <iomanip>
#include <fstream>
#include <sys/stat.h>
#include <errno.h>

#include <openssl/sha.h>
#include <util/rsthreads.h>

// This char is used to separate fields in the file list cache. It is supposed to be
// sufficiently safe on all systems.
//
static const char FILE_CACHE_SEPARATOR_CHAR = '|' ;

/****
#define FI_DEBUG 1
 * #define FI_DEBUG_ALL 1
 ****/

static RsMutex FIndexPtrMtx("FIndexPtrMtx") ;
std::tr1::unordered_set<void*> FileIndex::_pointers ;

void FileIndex::registerEntry(void*p)
{
	RsStackMutex m(FIndexPtrMtx) ;
	_pointers.insert(p) ;
}
void FileIndex::unregisterEntry(void*p)
{
	RsStackMutex m(FIndexPtrMtx) ;
	_pointers.erase(p) ;
}
bool FileIndex::isValid(void*p)
{
	RsStackMutex m(FIndexPtrMtx) ;
	return _pointers.find(p) != _pointers.end() ;
}

DirEntry::~DirEntry()
{
	/* cleanup */
	std::map<std::string, DirEntry *>::iterator dit;
	std::map<std::string, FileEntry *>::iterator fit;

	for(dit = subdirs.begin(); dit != subdirs.end(); dit++)
	{
		FileIndex::unregisterEntry((void*)dit->second) ;
		delete (dit->second);
	}
	subdirs.clear();

	for(fit = files.begin(); fit != files.end(); fit++)
	{
		FileIndex::unregisterEntry((void*)fit->second) ;
		delete (fit->second);
	}
	files.clear();
}


int  DirEntry::checkParentPointers()
{
#ifdef FI_DEBUG
	updateChildRows();
	std::map<std::string, DirEntry *>::iterator  dit;
	for(dit = subdirs.begin(); dit != subdirs.end(); dit++)
	{
		/* debug check */
		(dit->second)->checkParentPointers();
	}
#endif
	return 1;
}



int  DirEntry::updateChildRows()
{
	/* iterate through children and set row (parent should be good) */
	std::map<std::string, DirEntry *>::iterator  dit;
	std::map<std::string, FileEntry *>::iterator fit;
	int i = 0;
	for(dit = subdirs.begin(); dit != subdirs.end(); dit++)
	{
#ifdef FI_DEBUG
		/* debug check */
		if ((dit->second)->parent != this)
		{
			std::cerr << "DirEntry::updateChildRows()";
			std::cerr << "****WARNING subdir Parent pointer invalid!";
			std::cerr << std::endl;
			(dit->second)->parent = this;
		}
#endif
		(dit->second)->row = i++;
	}

	for(fit = files.begin(); fit != files.end(); fit++)
	{
#ifdef FI_DEBUG
		/* debug check */
		if ((fit->second)->parent != this)
		{
			std::cerr << "DirEntry::updateChildRows()";
			std::cerr << "****WARNING file Parent pointer invalid!";
			std::cerr << std::endl;
			(fit->second)->parent = this;
		}
#endif
		(fit->second)->row = i++;
	}
	return 1;
}


int  DirEntry::removeDir(const std::string& name)
{
	/* if it doesn't exist - add */
	std::map<std::string, DirEntry *>::iterator it;
	DirEntry *ndir = NULL;
	if (subdirs.end() != (it = subdirs.find(name)))
	{
#ifdef FI_DEBUG_ALL
		std::cerr << "DirEntry::removeDir() Cleaning up dir: " << name;
		std::cerr << std::endl;
#endif
		ndir = (it->second);

		subdirs.erase(it);
		FileIndex::unregisterEntry((void*)ndir) ;
		delete ndir;
		/* update row counters */
		updateChildRows();
		return 1;
	}

#ifdef FI_DEBUG
	std::cerr << "DirEntry::removeDir() missing Entry: " << name;
	std::cerr << std::endl;
#endif
	return 0;
}


int  DirEntry::removeFile(const std::string& name)
{
	/* if it doesn't exist - add */
	std::map<std::string, FileEntry *>::iterator it;
	FileEntry *nfile = NULL;

	if (files.end() != (it = files.find(name)))
	{
#ifdef FI_DEBUG_ALL
		std::cerr << "DirEntry::removeFile() removing File: " << name;
		std::cerr << std::endl;
#endif
		nfile = (it->second);

		files.erase(it);
		FileIndex::unregisterEntry((void*)nfile) ;
		delete nfile;
		/* update row counters */
		updateChildRows();
		return 1;
	}

#ifdef FI_DEBUG
	std::cerr << "DirEntry::removeFile() missing Entry: " << name;
	std::cerr << std::endl;
#endif
	return 0;
}




int  DirEntry::removeOldDir(const std::string& name, time_t old)
{
	std::map<std::string, DirEntry *>::iterator it;
	DirEntry *ndir = NULL;
	if (subdirs.end() != (it = subdirs.find(name)))
	{
		ndir = (it->second);
		if (ndir->updtime < old)
		{
#ifdef FI_DEBUG_ALL
		std::cerr << "DirEntry::removeOldDir() Removing Old dir: " << name;
		std::cerr << std::endl;
#endif
			subdirs.erase(it);
			FileIndex::unregisterEntry((void*)ndir) ;
			delete ndir;

			/* update row counters */
			updateChildRows();
			return 1;
		}
#ifdef FI_DEBUG_ALL
		std::cerr << "DirEntry::removeOldDir() Keeping UptoDate dir: " << name;
		std::cerr << std::endl;
#endif
		return 0;
	}

#ifdef FI_DEBUG
	std::cerr << "DirEntry::removeDir() missing Entry: " << name;
	std::cerr << std::endl;
#endif
	return 0;
}




int  DirEntry::removeOldEntries(time_t old, bool recursive)
{
	/* remove old dirs from our lists -> then do children and files */

	/* get all dirs with old time */
	std::list<DirEntry *> removeList;
	std::map<std::string, DirEntry *>::iterator it;
	for(it = subdirs.begin(); it != subdirs.end(); it++)
	{
		if ((it->second)->updtime < old)
		{
			removeList.push_back(it->second);
		}
	}

	/* now remove the old entries */
	std::list<DirEntry *>::iterator rit;
	for(rit = removeList.begin(); rit != removeList.end(); rit++)
	{
		removeDir((*rit)->name);
	}

	if (recursive)
	{
		/* now handle children */
		for(it = subdirs.begin(); it != subdirs.end(); it++)
		{
			(it->second)->removeOldEntries(old, recursive);
		}
	}

	/* now handle files similarly */
	std::list<FileEntry *> removeFileList;
	std::map<std::string, FileEntry *>::iterator fit;
	for(fit = files.begin(); fit != files.end(); fit++)
	{
		if ((fit->second)->updtime < old)
		{
			removeFileList.push_back(fit->second);
		}
	}

	/* now remove the old entries */
	std::list<FileEntry *>::iterator rfit;
	for(rfit = removeFileList.begin(); rfit != removeFileList.end(); rfit++)
	{
		removeFile((*rfit)->name);
	}

	return 1;
}


DirEntry *DirEntry::findOldDirectory(time_t old)
{
	/* check if one of our directories is old ...
	 */

	/* get all dirs with old time */
	std::map<std::string, DirEntry *>::iterator it;
	for(it = subdirs.begin(); it != subdirs.end(); it++)
	{
		if ((it->second)->updtime < old)
		{
			return (it->second);
		}
	}

	/*
	 * else check chlidren.
	 */

	for(it = subdirs.begin(); it != subdirs.end(); it++)
	{
		DirEntry *olddir = (it->second)->findOldDirectory(old);
		if (olddir)
		{
			return olddir;
		}
	}

	return NULL;
}


DirEntry *DirEntry::findDirectory(const std::string& fpath)
{
	std::string nextdir = RsDirUtil::getRootDir(fpath);
	std::map<std::string, DirEntry *>::iterator it;
	if (subdirs.end() == (it = subdirs.find(nextdir)))
	{
#ifdef FI_DEBUG
		std::cerr << "DirEntry::findDirectory() Missing subdir:";
		std::cerr << "\"" << nextdir << "\"";
		std::cerr << std::endl;
#endif
		return NULL;
	}

	std::string rempath = RsDirUtil::removeRootDir(fpath);
	if (rempath == "")
	{
		return it->second;
	}

	return (it->second)->findDirectory(rempath);
}


int DirEntry::updateDirectories(const std::string& fpath, int new_pop, int new_modtime)
{
	int ret = 1;
	if (path != "") /* if not there -> continue down tree */
	{
		std::string nextdir = RsDirUtil::getRootDir(fpath);
		std::map<std::string, DirEntry *>::iterator it;
		if (subdirs.end() == (it = subdirs.find(nextdir)))
		{
			return 0;
		}

		std::string rempath = RsDirUtil::removeRootDir(fpath);
		ret = (it->second)->updateDirectories(rempath, new_pop, new_modtime);
	}

	if (ret) /* if full path is okay -> update and return ok */
	{
		/* this is assumes that pop always increases! */
		if (new_pop > pop)
		{
			pop = new_pop;
		}
		if (new_modtime > modtime)
		{
			modtime = new_modtime;
		}
	}
	return ret;
}

DirEntry *DirEntry::updateDir(const FileEntry& fe, time_t utime)
{
	/* if it doesn't exist - add */
	std::map<std::string, DirEntry *>::iterator it;
	DirEntry *ndir = NULL;
	if (subdirs.end() == (it = subdirs.find(fe.name)))
	{
#ifdef FI_DEBUG_ALL
		std::cerr << "DirEntry::updateDir() Adding Entry";
		std::cerr << std::endl;
#endif
		ndir = new DirEntry();
		FileIndex::registerEntry((void*)ndir) ;
		ndir -> parent = this;
		ndir -> path = path + "/" + fe.name;
		ndir -> name = fe.name;
		ndir -> pop = 0;
		ndir -> modtime = 0;
		ndir -> updtime = utime;

		subdirs[fe.name] = ndir;

		/* update row counters */
		updateChildRows();
		return ndir;
	}

#ifdef FI_DEBUG_ALL
		std::cerr << "DirEntry::updateDir() Updating Entry";
		std::cerr << std::endl;
#endif

	/* update utime */
	ndir = (it->second);
	ndir->updtime = utime;

	return ndir;
}


FileEntry *DirEntry::updateFile(const FileEntry& fe, time_t utime)
{
	/* if it doesn't exist - add */
	std::map<std::string, FileEntry *>::iterator it;
	FileEntry *nfile = NULL;
	if (files.end() == (it = files.find(fe.name)))
	{

#ifdef FI_DEBUG_ALL
		std::cerr << "DirEntry::updateFile() Adding Entry";
		std::cerr << std::endl;
#endif

		nfile = new FileEntry();
		FileIndex::registerEntry((void*)nfile) ;
		nfile -> parent = this;
		nfile -> name = fe.name;
		nfile -> hash = fe.hash;
		nfile -> size = fe.size;
		nfile -> pop  = 0;
		nfile -> modtime = fe.modtime;
		nfile -> updtime = utime;

		files[fe.name] = nfile;

		/* update row counters */
		updateChildRows();
		return nfile;
	}


#ifdef FI_DEBUG_ALL
		std::cerr << "DirEntry::updateFile() Updating Entry";
		std::cerr << std::endl;
#endif


	/* update utime */
	nfile = (it->second);
	nfile -> parent = this;
	nfile -> name = fe.name;
	nfile -> hash = fe.hash;
	nfile -> size = fe.size;
	nfile -> modtime = fe.modtime;
	nfile -> updtime = utime;
	//nfile -> pop  = 0; // not handled here.

	return nfile;
}


int FileEntry::print(std::string &out)
{
	/* print this dir, then subdirs, then files */

	rs_sprintf_append(out, "file %03d [%ld/%ld] : ", row, updtime, modtime);

	if (parent)
		out += parent->path;
	else
		out += "[MISSING PARENT]";

	rs_sprintf_append(out, " %s  [ s: %lld ] ==>   [ %s ]\n", name.c_str(), size, hash.c_str());

	return 1;
}


int DirEntry::print(std::string &out)
{
	/* print this dir, then subdirs, then files */
	rs_sprintf_append(out, "dir  %03d [%ld] : %s\n", row, updtime, path.c_str());

	std::map<std::string, DirEntry *>::iterator it;
	for(it = subdirs.begin(); it != subdirs.end(); it++)
	{
		(it->second)->print(out);
	}
	std::map<std::string, FileEntry *>::iterator fit;
	for(fit = files.begin(); fit != files.end(); fit++)
	{
		(fit->second)->print(out);
	}
	return 1;
}

FileIndex::FileIndex(const std::string& pid)
{
	root = new PersonEntry(pid);
	registerEntry(root) ;
}

FileIndex::~FileIndex()
{
	FileIndex::unregisterEntry((void*)root) ;
	delete root;
}

int	FileIndex::setRootDirectories(const std::list<std::string> &inlist, time_t updtime)
{
	/* set update time to zero */
	std::map<std::string, DirEntry *>::iterator it;
	for(it = root->subdirs.begin(); it != root->subdirs.end(); it++)
	{
		(it->second)->updtime = 0;
	}

	std::list<std::string>::const_iterator ait;
	FileEntry fe;
	time_t utime = 1;
	for(ait = inlist.begin(); ait != inlist.end(); ait++)
	{
		fe.name = (*ait);

		/* see if it exists */
		root->updateDir(fe, utime);
	}

	/* remove all dirs with zero time (non recursive) */
	root->removeOldEntries(utime, false);

	/* now flag remaining directories with correct update time */
	for(it = root->subdirs.begin(); it != root->subdirs.end(); it++)
	{
		(it->second)->updtime = updtime;
	}

	return 1;
}

void FileIndex::updateMaxModTime() 
{
	RecursUpdateMaxModTime(root) ;
}
void FileIndex::RecursUpdateMaxModTime(DirEntry *dir) 
{
	time_t max_mod_t = 0 ;

	for(std::map<std::string,DirEntry*>::iterator it(dir->subdirs.begin());it!=dir->subdirs.end();++it)
	{
		RecursUpdateMaxModTime(it->second) ;
		max_mod_t = std::max(max_mod_t, it->second->most_recent_time) ;
	}
	for(std::map<std::string,FileEntry*>::iterator it(dir->files.begin());it!=dir->files.end();++it)
		max_mod_t = std::max(max_mod_t, it->second->modtime) ;

	dir->most_recent_time = max_mod_t ;
}

int	FileIndex::getRootDirectories(std::list<std::string> &outlist)
{
	/* set update time to zero */
	std::map<std::string, DirEntry *>::iterator it;
	for(it = root->subdirs.begin(); it != root->subdirs.end(); it++)
	{
		outlist.push_back(it->first);
	}
	return 1;
}

/* update (index building) */
DirEntry *FileIndex::updateDirEntry(const std::string& fpath, const FileEntry& fe, time_t utime)
{
	/* path is to parent */
#ifdef FI_DEBUG_ALL
		std::cerr << "FileIndex::updateDirEntry() Path: \"";
		std::cerr << fpath << "\"" << " + \"" << fe.name << "\"";
		std::cerr << std::endl;
#endif
	DirEntry *parent = NULL;
	if (fpath == "")
	{
		parent = root;
	}
	else
	{
		parent = root->findDirectory(fpath);
	}

	if (!parent) {
#ifdef FI_DEBUG
		std::cerr << "FileIndex::updateDirEntry() NULL parent";
		std::cerr << std::endl;
#endif
		return NULL;
	}
	return parent -> updateDir(fe, utime);
}


FileEntry *FileIndex::updateFileEntry(const std::string& fpath, const FileEntry& fe, time_t utime)
{
	/* path is to parent */
#ifdef FI_DEBUG_ALL
		std::cerr << "FileIndex::updateFileEntry() Path: \"";
		std::cerr << fpath << "\"" << " + \"" << fe.name << "\"";
		std::cerr << std::endl;
#endif
	DirEntry *parent = root->findDirectory(fpath);

	if (!parent) {
#ifdef FI_DEBUG
		std::cerr << "FileIndex::updateFileEntry() NULL parent";
		std::cerr << std::endl;
#endif
		return NULL;
	}
	return parent -> updateFile(fe, utime);
}


DirEntry *FileIndex::findOldDirectory(time_t old)   /* finds directories older than old */
{
	DirEntry *olddir = root->findOldDirectory(old);
#ifdef FI_DEBUG

	std::cerr << "FileIndex::findOldDirectory(" << old << ") -> ";
	if (olddir)
	{
		std::cerr << olddir->path;
	}
	else
	{
		std::cerr << "NONE";
	}
	std::cerr << std::endl;

#endif
	return olddir;
}

int  	FileIndex::removeOldDirectory(const std::string& fpath, const std::string& name, time_t old)
{
	/* path is to parent */
#ifdef FI_DEBUG_ALL
		std::cerr << "FileIndex::removeOldDirectory() Path: \"";
		std::cerr << fpath << "\"" << " + \"" << name << "\"";
		std::cerr << std::endl;
#endif

	/* because of this find - we cannot get a child of
	 * root (which is what we want!)
	 */

	DirEntry *parent = root->findDirectory(fpath);
	/* for root directory case ... no subdir. */
	if (fpath == "")
	{
#ifdef FI_DEBUG
		std::cerr << "FileIndex::removeOldDirectory() removing a root dir";
		std::cerr << std::endl;
#endif
		parent = root;
	}

	if (!parent) {
#ifdef FI_DEBUG
		std::cerr << "FileIndex::removeOldDirectory() NULL parent";
		std::cerr << std::endl;
#endif
		return 0;
	}
	return parent -> removeOldDir(name, old);
}


int	FileIndex::cleanOldEntries(time_t old)  /* removes entries older than old */
{
	std::map<std::string, DirEntry *>::iterator it;
	for(it = root->subdirs.begin(); it != root->subdirs.end(); it++)
	{
		(it->second)->removeOldEntries(old, true);
	}
	return 1;
}



int     FileIndex::printFileIndex(std::string &out)
{
	out += "FileIndex::printFileIndex()\n";
	root->print(out);
	return 1;
}

int FileIndex::loadIndex(const std::string& filename, const std::string& expectedHash, uint64_t /*size*/)
{
	std::ifstream file (filename.c_str(), std::ifstream::binary);
	if (!file)
	{
#ifdef FI_DEBUG
		std::cerr << "FileIndex::loadIndex error opening file: " << filename;
		std::cerr << std::endl;
#endif
		return 0;
	}

	/* load file into memory, close file */
	char ibuf[512];
	std::stringstream ss;
	while(!file.eof())
	{
		file.read(ibuf, 512);
		ss.write(ibuf, file.gcount());
	}
	file.close();

	/* calculate hash */
	unsigned char sha_buf[SHA_DIGEST_LENGTH];
	SHA_CTX *sha_ctx = new SHA_CTX;
	SHA1_Init(sha_ctx);
	SHA1_Update(sha_ctx, ss.str().c_str(), ss.str().length());
	SHA1_Final(&sha_buf[0], sha_ctx);
	delete sha_ctx;

	std::string tmpout;
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		rs_sprintf_append(tmpout, "%02x", (unsigned int) (sha_buf[i]));
	}

	if (expectedHash != "" && expectedHash != tmpout)
	{
#ifdef FI_DEBUG
		std::cerr << "FileIndex::loadIndex expected hash does not match" << std::endl;
		std::cerr << "Expected hash: " << expectedHash << std::endl;
		std::cerr << "Hash found:    " << tmpout.str() << std::endl;
#endif
		return 0;
	}

	DirEntry *ndir = NULL;
	FileEntry *nfile = NULL;
	std::list<DirEntry *> dirlist;
	std::string word;
	char ch;

	while(ss.get(ch))
	{
		if (ch == '-')
		{
			ss.ignore(256, '\n');
			switch(dirlist.size())
			{
				/* parse error: out of directory */
				case 0:
				{
#ifdef FI_DEBUG
					std::cerr << "loadIndex error parsing saved file: " << filename;
					std::cerr << " Ran out of dirs";
					std::cerr << std::endl;
#endif
					goto error;
				}
				/* finished parse, last dir is root */
				case 1:
				{
					std::string pid = root -> id;
					FileIndex::unregisterEntry((void*)root) ;
					delete root; /* to clean up old entries */
					root = new PersonEntry(pid);
					registerEntry((void*)root) ;

					/* shallow copy of all except id */
					ndir = dirlist.back();
					dirlist.pop_back(); /* empty list */
					(*root) = (*ndir);

					/* now cleanup (can't call standard delete) */
					ndir->subdirs.clear();
					ndir->files.clear();
					FileIndex::unregisterEntry((void*)ndir) ;
					delete ndir;
					ndir = NULL;

					/* must reset parent pointers now */
					std::map<std::string, DirEntry *>::iterator it;
					for(it = root->subdirs.begin();
						it != root->subdirs.end(); it++)
					{
						(it->second)->parent = root;
					}

					break;
				}
				/* pop stack */
				default: dirlist.pop_back(); ndir = dirlist.back();
			}
			continue;
		}

		// Ignore comments
		else if (ch == '#')
		{
			ss.ignore(256, '\n');
		}

		else {
			std::vector<std::string> tokens;
			/* parse line */
			while(1)
			{
				getline(ss, word, FILE_CACHE_SEPARATOR_CHAR);
				if (ss.eof())
					goto error;
				tokens.push_back(word);
				if (ss.peek() == '\n')
				{
					ss.ignore(256, '\n');
					break;
				}

			}
			/* create new file and add it to last directory*/
			if (ch == 'f')
			{
				if (tokens.size() != 6)
				{
#ifdef FI_DEBUG
					std::cerr << "loadIndex error parsing saved file: " << filename;
					std::cerr << " File token count wrong: " << tokens.size();
					std::cerr << std::endl;
					for(unsigned int i = 0; i < tokens.size(); i++)
					{
						std::cerr << "\tToken[" << i << "]:" << tokens[i];
						std::cerr << std::endl;
					}

#endif
					goto error;
				}
				nfile = new FileEntry();
				registerEntry((void*)nfile) ;
				nfile->name = tokens[0];
				nfile->hash = tokens[1];
				nfile->size = atoll(tokens[2].c_str());
				nfile->modtime = atoi(tokens[3].c_str());
				nfile->pop = atoi(tokens[4].c_str());
				nfile->updtime = atoi(tokens[5].c_str());
				nfile->parent = ndir;
				nfile->row = ndir->subdirs.size() + ndir->files.size();
				ndir->files[nfile->name] = nfile;

			}
			/* create new dir and add to stack */
			else if (ch == 'd')
			{
				if (tokens.size() != 6)
				{
#ifdef FI_DEBUG
					std::cerr << "loadIndex error parsing saved file: " << filename;
					std::cerr << " Dir token count wrong: " << tokens.size();
					std::cerr << std::endl;
#endif
					goto error;
				}
				ndir = new DirEntry();
				registerEntry((void*)ndir) ;
				ndir->name = tokens[0];
				ndir->path = tokens[1];
				ndir->size = atoi(tokens[2].c_str());
				ndir->modtime = atoi(tokens[3].c_str());
				ndir->pop = atoi(tokens[4].c_str());
				ndir->updtime = atoi(tokens[5].c_str());
				if (!dirlist.empty())
				{
					ndir->parent = (dirlist.back());
					ndir->row = dirlist.back()->subdirs.size();
					dirlist.back()->subdirs[ndir->name] = ndir;
				}
				dirlist.push_back(ndir);
			}
		}
	}

	return 1;

	/* parse error encountered */
error:
#ifdef FI_DEBUG
	std::cerr << "loadIndex error parsing saved file: " << filename;
	std::cerr << std::endl;
#endif
	return 0;
}


int FileIndex::saveIndex(const std::string& filename, std::string &fileHash, uint64_t &size,const std::set<std::string>& forbidden_dirs)
{
	unsigned char sha_buf[SHA_DIGEST_LENGTH];
	std::string filenametmp = filename + ".tmp" ;
	std::string s;

	size = 0 ;
	fileHash = "" ;

	/* print version and header */
	s += "# FileIndex version 0.1\n";
	s += "# Dir: d name, path, parent, size, modtime, pop, updtime;\n";
	s += "# File: f name, hash, size, modtime, pop, updtime;\n";
	s += "#\n";

	/* begin recusion */
	root->writeDirInfo(s) ;

	std::map<std::string, DirEntry *>::iterator it;
	for(it = root->subdirs.begin(); it != root->subdirs.end(); it++)
	{
#ifdef FI_DEBUG
		std::cout << "writting root directory: name=" << it->second->name << ", path=" << it->second->path << std::endl ;
#endif
		if(forbidden_dirs.find(it->second->name) != forbidden_dirs.end())
		{
#ifdef FI_DEBUG
			std::cerr << "  will be suppressed." << std::endl ;
#endif
		}
		else
		{
#ifdef FI_DEBUG
			std::cerr << "  will be saved." << std::endl ;
#endif
			(it->second)->saveEntry(s);
		}
	}

	root->writeFileInfo(s) ;	// this should never do anything

	/* signal to pop directory from stack in loadIndex() */
	s += "-\n";

	/* calculate sha1 hash */
	SHA_CTX *sha_ctx = new SHA_CTX;
	SHA1_Init(sha_ctx);
	SHA1_Update(sha_ctx, s.c_str(), s.length());
	SHA1_Final(&sha_buf[0], sha_ctx);
	delete sha_ctx;

	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		rs_sprintf_append(fileHash, "%02x", (unsigned int) (sha_buf[i]));
	}

	/* finally, save to file */

	FILE *file = RsDirUtil::rs_fopen(filenametmp.c_str(), "wb");
	if (file == NULL)
	{
		std::cerr << "FileIndex::saveIndex error opening file for writting: " << filename << ". Giving up." << std::endl;
		return 0;
	}
	fprintf(file,"%s",s.c_str()) ;

	fclose(file);

	// Use a temp file name so that the file is never half saved.
	//
	if(!RsDirUtil::renameFile(filenametmp,filename))
		return false ;

	/* get the size out */
	struct stat64 buf;

	if(-1 == stat64(filename.c_str(), &buf))
	{
		std::cerr << "Can't determine size of file " << filename << ": errno = " << errno << std::endl ;
		return false ;
	}

	size=buf.st_size;

	return true;
}


std::string FixName(const std::string& _in)
{
	/* replace any , with _ */
	std::string in(_in) ;
	for(unsigned int i = 0; i < in.length(); i++)
	{
		if (in[i] == FILE_CACHE_SEPARATOR_CHAR)
		{
			in[i] = '_';
		}
	}
	return in;
}

void DirEntry::writeDirInfo(std::string& s)
{
	/* print node info */
	rs_sprintf_append(s, "d%s%c%s%c%lld%c%ld%c%d%c%ld%c\n",
					  FixName(name).c_str(), FILE_CACHE_SEPARATOR_CHAR,
					  FixName(path).c_str(), FILE_CACHE_SEPARATOR_CHAR,
					  size, FILE_CACHE_SEPARATOR_CHAR,
					  modtime, FILE_CACHE_SEPARATOR_CHAR,
					  pop, FILE_CACHE_SEPARATOR_CHAR,
					  updtime, FILE_CACHE_SEPARATOR_CHAR);
}

void DirEntry::writeFileInfo(std::string& s)
{
	/* print file info */
	std::map<std::string, FileEntry *>::iterator fit;
	for(fit = files.begin(); fit != files.end(); fit++)
	{
		rs_sprintf_append(s, "f%s%c%s%c%lld%c%ld%c%d%c%ld%c\n",
						  FixName((fit->second)->name).c_str(), FILE_CACHE_SEPARATOR_CHAR,
						  (fit->second)->hash.c_str(), FILE_CACHE_SEPARATOR_CHAR,
						  (fit->second)->size, FILE_CACHE_SEPARATOR_CHAR,
						  (fit->second)->modtime, FILE_CACHE_SEPARATOR_CHAR,
						  (fit->second)->pop, FILE_CACHE_SEPARATOR_CHAR,
						  (fit->second)->updtime, FILE_CACHE_SEPARATOR_CHAR);
	}
}

/* recusive function for traversing the dir tree in preorder */
int DirEntry::saveEntry(std::string &s)
{
	writeDirInfo(s) ;

	std::map<std::string, DirEntry *>::iterator it;
	for(it = subdirs.begin(); it != subdirs.end(); it++)
	{
		(it->second)->saveEntry(s);
	}

	writeFileInfo(s) ;

	/* signal to pop directory from stack in loadIndex() */
	s += "-\n";
	return 1;
}


int FileIndex::searchHash(const std::string& hash, std::list<FileEntry *> &results) const
{
#ifdef FI_DEBUG
	std::cerr << "FileIndex::searchHash(" << hash << ")";
	std::cerr << std::endl;
#endif
	DirEntry *ndir = NULL;
	std::list<DirEntry *> dirlist;
	dirlist.push_back(root);

	while(!dirlist.empty())
	{
		ndir = dirlist.back();
		dirlist.pop_back();
		/* add subdirs to stack */
		std::map<std::string, DirEntry *>::iterator it;
		for(it = ndir->subdirs.begin(); it != ndir->subdirs.end(); it++)
		{
			dirlist.push_back(it->second);
		}

		std::map<std::string, FileEntry *>::iterator fit;
		/* search in current dir */
		for(fit = ndir->files.begin(); fit != ndir->files.end(); fit++)
		{
			if (hash == (fit->second)->hash)
			{
				results.push_back(fit->second);
#ifdef FI_DEBUG
				std::cerr << "FileIndex::searchHash(" << hash << ")";
				std::cerr << " found: " << fit->second->name;
				std::cerr << std::endl;
#endif
			}
		}
	}

	return 0;
}


int FileIndex::searchTerms(const std::list<std::string>& terms, std::list<FileEntry *> &results) const
{
	DirEntry *ndir = NULL;
	std::list<DirEntry *> dirlist;
	dirlist.push_back(root);

	/* iterators */
	std::map<std::string, DirEntry *>::iterator it;
	std::map<std::string, FileEntry *>::iterator fit;
	std::list<std::string>::const_iterator iter;

	while(!dirlist.empty())
	{
		ndir = dirlist.back();
		dirlist.pop_back();
		for(it = ndir->subdirs.begin(); it != ndir->subdirs.end(); it++)
		{
			dirlist.push_back(it->second);
		}

		for (iter = terms.begin(); iter != terms.end(); iter ++) {
			std::string::const_iterator it2;
			const std::string &str1 = ndir->name;
			const std::string &str2 = *iter;
			it2 = std::search(str1.begin(), str1.end(), str2.begin(), str2.end(), CompareCharIC());
			if (it2 != str1.end()) {
				/* first search to see if its parent is in the results list */
				bool addDir = true;
				for (std::list<FileEntry *>::iterator rit(results.begin()); rit != results.end() && addDir; rit ++) {
					DirEntry *de = dynamic_cast<DirEntry *>(*rit);
					if (de && (de == root))
						continue;
					if (de && (de == ndir->parent))
						addDir = false;
				}
				if (addDir) {
					results.push_back((FileEntry *) ndir);
					break;
				}
			}
		}

		for(fit = ndir->files.begin(); fit != ndir->files.end(); fit++)
		{
			/* cycle through terms */
			for(iter = terms.begin(); iter != terms.end(); iter++)
			{
				/* always ignore case */
        			std::string::const_iterator it2 ;
				const std::string &str1 = fit->second->name;
				const std::string &str2 = (*iter);

                		it2 = std::search( str1.begin(), str1.end(),
                                        str2.begin(), str2.end(), CompareCharIC() );
				if (it2 != str1.end())
				{
					results.push_back(fit->second);
					break;
				}
				/* original case specific term search ******
				if (fit->second->name.find(*iter) != std::string::npos)
				{
					results.push_back(fit->second);
					break;
				}
				************/
			}
		}
	} //while

	return 0;
}

int FileIndex::searchBoolExp(Expression * exp, std::list<FileEntry *> &results)  const
{
	DirEntry *ndir = NULL;
	std::list<DirEntry *> dirlist;
	dirlist.push_back(root);

	/* iterators */
	std::map<std::string, DirEntry *>::iterator it;
	std::map<std::string, FileEntry *>::iterator fit;
	std::list<std::string>::const_iterator iter;

	while(!dirlist.empty())
	{
		ndir = dirlist.back();
		dirlist.pop_back();
		for(it = ndir->subdirs.begin(); it != ndir->subdirs.end(); it++)
		{
			dirlist.push_back(it->second);
		}

		for(fit = ndir->files.begin(); fit != ndir->files.end(); fit++)
		{
			/*Evaluate the boolean expression and add it to the results if its true*/
			bool ret = exp->eval(fit->second);
			if (ret == true){
				results.push_back(fit->second);
			}
		}
	} //while

	return 0;
}

uint32_t FileIndex::getType(void *ref) 
{
	if(ref == NULL)
		return DIR_TYPE_ROOT ;

	if(!isValid(ref))
		return DIR_TYPE_ROOT ;

	return static_cast<FileEntry*>(ref)->type() ;
}

bool FileIndex::extractData(const std::string& fpath,DirDetails& details) const
{
	void *ref = findRef(fpath) ;

	if(ref == NULL)
		return false ;

	return extractData(ref,details) ;
}

void *FileIndex::findRef(const std::string& fpath) const
{
	DirEntry *parent = root->findDirectory(fpath);

	std::cerr << "findRef() Called on " << fpath << std::endl;

	if (!parent) 
	{
//#ifdef FI_DEBUG
		std::cerr << "FileIndex::updateFileEntry() NULL parent";
		std::cerr << std::endl;
//#endif
		return false;
	}
	std::cerr << "Found parent directory: " << std::endl;
	std::cerr << "  parent.name = " << parent->name << std::endl;
	std::cerr << "  parent.path = " << parent->path << std::endl;

	if(parent->path == fpath) // directory!
	{
		std::cerr << "  fpath is a directory. Returning parent!" << std::endl;
		return parent ;
	}
	else
	{
		std::cerr << "  fpath is a file. Looking into parent directory..." << std::endl;
		/* search in current dir */
		for(std::map<std::string, FileEntry *>::iterator fit = parent->files.begin(); fit != parent->files.end(); ++fit)
		{
			std::cerr << "    trying " << parent->path + "/" + fit->second->name << std::endl;
			if(parent->path + "/" + fit->second->name == fpath)
			{
				std::cerr << "  found !" << std::endl;
				return fit->second ;
			}
		}

		std::cerr << "  (EE) not found !" << std::endl;
		return NULL ;
	}
}

bool FileIndex::extractData(void *ref,DirDetails& details) 
{
	if(!isValid(ref))
	{
#ifdef FI_DEBUG
		std::cerr << "FileIndex::extractData() asked for an invalid pointer " << (void*)ref << std::endl;
#endif
		return false ;
	}

	FileEntry *file = static_cast<FileEntry *>(ref);
	DirEntry *dir = (file->hash.empty())?static_cast<DirEntry *>(file):NULL ; // This is a hack to avoid doing a dynamic_cast

	details.children = std::list<DirStub>() ;
	time_t now = time(NULL) ;

	if (dir!=NULL) /* has children --- fill */
	{
#ifdef FI_DEBUG
		std::cerr << "FileIndex::extractData() ref=dir" << std::endl;
#endif
		/* extract all the entries */
		for(std::map<std::string,DirEntry*>::const_iterator dit(dir->subdirs.begin()); dit != dir->subdirs.end(); ++dit)
		{
			DirStub stub;
			stub.type = DIR_TYPE_DIR;
			stub.name = (dit->second) -> name;
			stub.ref  = (dit->second);

			details.children.push_back(stub);
		}

		for(std::map<std::string,FileEntry*>::const_iterator fit(dir->files.begin()); fit != dir->files.end(); ++fit)
		{
			DirStub stub;
			stub.type = DIR_TYPE_FILE;
			stub.name = (fit->second) -> name;
			stub.ref  = (fit->second);

			details.children.push_back(stub);
		}

		if(dir->parent == NULL)
			details.type = DIR_TYPE_PERSON ;
		else
			details.type = DIR_TYPE_DIR;
		details.hash = "";
		details.count = dir->subdirs.size() + dir->files.size();
		details.min_age = now - dir->most_recent_time ;
	}
	else
	{
#ifdef FI_DEBUG
		std::cerr << "FileIndexStore::extractData() ref=file" << std::endl;
#endif
		details.type = DIR_TYPE_FILE;
		details.count = file->size;
		details.min_age = now - file->modtime ;
	}

#ifdef FI_DEBUG
	std::cerr << "FileIndexStore::extractData() name: " << file->name << std::endl;
#endif
	details.ref = file;
	details.hash = file->hash;
	details.age = now - file->modtime;
	details.flags = 0;//file->pop;

	/* find parent pointer, and row */
	details.parent = file->parent ;

	details.prow = (file->parent==NULL)?0:file->parent->row ;

	if(details.type == DIR_TYPE_DIR)
	{
		details.name = file->name;
		details.path = dir->path;
	}
	else
	{
		details.name = file->name;
		details.path = (file->parent==NULL)?"":file->parent->path;
	}

	/* find peer id */
	FileEntry *f ;
	for(f=file;f->parent!=NULL;f=f->parent) ;

	details.id = static_cast<PersonEntry*>(f)->id;	// The topmost parent is necessarily a personEntrY, so we can avoid a dynamic_cast.

#ifdef FI_DEBUG
	assert(details.parent != details.ref) ;
	std::cout << "details: ref=" << (void*)ref << ", prow=" << details.prow << ", parent=" << (void*)details.parent << ", children=" ;
	for(std::list<DirStub>::iterator it(details.children.begin());it!=details.children.end();++it)
		std::cout << " " << (void*)it->ref ;
	std::cout << std::endl ;
#endif

	return true;
}


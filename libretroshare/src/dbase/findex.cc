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


#include "dbase/findex.h"
#include "rsiface/rsexpr.h"
#include "util/rsdir.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>

#include <openssl/sha.h>

/****
 * #define FI_DEBUG 1
 ****/


DirEntry::~DirEntry()
{
	/* cleanup */
	std::map<std::string, DirEntry *>::iterator dit;
	std::map<std::string, FileEntry *>::iterator fit;

	for(dit = subdirs.begin(); dit != subdirs.end(); dit++)
	{
		delete (dit->second);
	}
	subdirs.clear();

	for(fit = files.begin(); fit != files.end(); fit++)
	{
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


int  DirEntry::removeDir(std::string name)
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


int  DirEntry::removeFile(std::string name)
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




int  DirEntry::removeOldDir(std::string name, time_t old)
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


DirEntry *DirEntry::findDirectory(std::string fpath)
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


int DirEntry::updateDirectories(std::string fpath, int new_pop, int new_modtime)
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

DirEntry *DirEntry::updateDir(FileEntry fe, time_t utime)
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


FileEntry *DirEntry::updateFile(FileEntry fe, time_t utime)
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


int FileEntry::print(std::ostream &out)
{
	/* print this dir, then subdirs, then files */

	out << "file ";
	out << std::setw(3) << std::setfill('0') << row;
	out << " [" << updtime << "/" << modtime << "] : ";

	if (parent)
		out << parent->path;
	else
		out << "[MISSING PARENT]";

	out << " " << name; 
	out << "  [ s: " << size << " ] ==> ";
	out << "  [ " << hash << " ]";
        out << std::endl;
	return 1;
}


int DirEntry::print(std::ostream &out)
{
	/* print this dir, then subdirs, then files */
	out << "dir  ";
	out << std::setw(3) << std::setfill('0') << row;
	out << " [" << updtime << "] : " << path;
	out << std::endl;

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

FileIndex::FileIndex(std::string pid)
{
	root = new PersonEntry(pid);
}

FileIndex::~FileIndex()
{
	delete root;
}

int	FileIndex::setRootDirectories(std::list<std::string> inlist, time_t updtime)
{
	/* set update time to zero */
	std::map<std::string, DirEntry *>::iterator it;
	for(it = root->subdirs.begin(); it != root->subdirs.end(); it++)
	{
		(it->second)->updtime = 0;
	}

	std::list<std::string>::iterator ait;
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
DirEntry *FileIndex::updateDirEntry(std::string fpath, FileEntry fe, time_t utime)
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


FileEntry *FileIndex::updateFileEntry(std::string fpath, FileEntry fe, time_t utime)
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

int  	FileIndex::removeOldDirectory(std::string fpath, std::string name, time_t old)
{
	/* path is to parent */
#ifdef FI_DEBUG_ALL
		std::cerr << "FileIndex::removeOldDir() Path: \"";
		std::cerr << fpath << "\"" << " + \"" << name << "\"";
		std::cerr << std::endl;
#endif

	/* because of this find - we cannot get a child of 
	 * root (which is what we want!)
	 */
	DirEntry *parent = root->findDirectory(fpath);

	if (!parent) {
#ifdef FI_DEBUG
		std::cerr << "FileIndex::removeOldDir() NULL parent";
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



int     FileIndex::printFileIndex(std::ostream &out)
{
	out << "FileIndex::printFileIndex()" << std::endl;
	root->print(out);
	return 1;
}


int FileIndex::loadIndex(std::string filename, std::string expectedHash, uint64_t size)
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

	std::ostringstream tmpout;
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		tmpout << std::setw(2) << std::setfill('0') << std::hex << (unsigned int) (sha_buf[i]);
	}

	if (expectedHash != tmpout.str())
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
					delete root; /* to clean up old entries */
					root = new PersonEntry(pid);

					/* shallow copy of all except id */
					ndir = dirlist.back(); 
					dirlist.pop_back(); /* empty list */
					(*root) = (*ndir);

					/* now cleanup (can't call standard delete) */
					ndir->subdirs.clear();
					ndir->files.clear();
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
				getline(ss, word, ',');
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


int FileIndex::saveIndex(std::string filename, std::string &fileHash, uint64_t &size)
{
	unsigned char sha_buf[SHA_DIGEST_LENGTH];
	std::ofstream file (filename.c_str(), std::ofstream::binary);
	std::ostringstream oss;
	
	if (!file)
	{
#ifdef FI_DEBUG
		std::cerr << "FileIndex::saveIndex error opening file: " << filename;
		std::cerr << std::endl; 
#endif
		return 0;
	}

	/* print version and header */
	oss << "# FileIndex version 0.1" << std::endl;
	oss << "# Dir: d name, path, parent, size, modtime, pop, updtime;" << std::endl;
	oss << "# File: f name, hash, size, modtime, pop, updtime;" << std::endl;
	oss << "#" << std::endl;
	
	/* begin recusion */
	root->saveEntry(oss);

	/* calculate sha1 hash */
	SHA_CTX *sha_ctx = new SHA_CTX;	
	SHA1_Init(sha_ctx);
	SHA1_Update(sha_ctx, oss.str().c_str(), oss.str().length());
	SHA1_Final(&sha_buf[0], sha_ctx);	
	delete sha_ctx;

	std::ostringstream tmpout;
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		tmpout << std::setw(2) << std::setfill('0') << std::hex << (unsigned int) (sha_buf[i]);
	}
	fileHash = tmpout.str();

	/* finally, save to file */
	file << oss.str();

	/* get the size out */
	size=file.tellp();
	file.close();
	return 1;
}


std::string FixName(std::string in)
{
	/* replace any , with _ */
	for(unsigned int i = 0; i < in.length(); i++)
	{
		if (in[i] == ',')
		{
			in[i] = '_';
		}
	}
	return in;
}


/* recusive function for traversing the dir tree in preorder */
int DirEntry::saveEntry(std::ostringstream &oss)
{
	/* print node info */
	oss << "d";
	oss << FixName(name) << ",";
	oss << FixName(path) << ",";
	oss << size << ",";
	oss << modtime << ",";
	oss << pop << ",";
	oss << updtime << "," << std::endl;

	std::map<std::string, DirEntry *>::iterator it;
	for(it = subdirs.begin(); it != subdirs.end(); it++)
	{
		(it->second)->saveEntry(oss);
	}

	/* print file info */
	std::map<std::string, FileEntry *>::iterator fit;
	for(fit = files.begin(); fit != files.end(); fit++)
	{
		oss << "f";
		oss << FixName((fit->second)->name) << ",";
		oss << (fit->second)->hash << ",";
		oss << (fit->second)->size << ",";
		oss << (fit->second)->modtime << ",";
		oss << (fit->second)->pop << ",";
		oss << (fit->second)->updtime << "," << std::endl;
	}

	/* signal to pop directory from stack in loadIndex() */
	oss << "-" << std::endl;
	return 1;
}


int FileIndex::searchHash(std::string hash, std::list<FileEntry *> &results) const
{
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
			}
		}
	}

	return 0;
}


int FileIndex::searchTerms(std::list<std::string> terms, std::list<FileEntry *> &results) const
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


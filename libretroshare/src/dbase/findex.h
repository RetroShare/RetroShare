/*
 * RetroShare FileCache Module: findex.h
 *
 * Copyright 2004-2007 by Robert Fernie.
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

#ifndef FILE_INDEX_H
#define FILE_INDEX_H

#include <string>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <stdint.h>

class ostream;

/******************************************************************************************
 * The Key Data Types for the File Index:

  FileEntry    : Information about a single file.
  DirEntry     : Information about a directory and its children
  PersonEntry  : Information the root of a FileIndex.

  FileIndex : A Collection of root directories, with a set of functions to manipulate them. 
	In terms of retroshare, There will be a single 'Local' FileIndex used to store
	the shared files, and a set of 'Remote' FileIndices which are used to store
	the available files of all the peers.

******************************************************************************************/
/******************************************************************************************
  STILL TODO:

  (1) Load/Store a FileIndex to file...
	int FileIndex::loadIndex(FILE *input);
	int FileIndex::saveIndex(FILE *input);

      This can be done in a recursive manner, or handled completely within FileIndex.

  (2) Search Functions for Partial File Names and Hashes. 

	int FileIndex::searchHash(std::string hash, std::list<FileEntry> &results);
	int FileIndex::searchTerms(std::list<string> terms, std::list<FileEntry> &results);

      This is probably best done in a recursive manner.

      The search could also be extended to handle complex Boolean searches such as : 
      match (size > 100K) && (name contains 'Blue') .... if anyone is interested.
      But this can get quite complicated, and can be left to a later date.

******************************************************************************************/


/******************************************************************************************
 * FileEntry
 *****************************************************************************************/

class DirEntry;

class FileEntry
{
	public:
	FileEntry() :parent(NULL), row(-1) { return; }
virtual ~FileEntry() { return; }

virtual int print(std::ostream &out);

	/* Data */
	std::string name; 
	std::string hash;
	uint64_t size;         /* file size */
	time_t modtime;      /* modification time - most recent mod time for a sub entry for dirs */
	int pop;	  /* popularity rating */

	time_t updtime;      /* last updated */

	/* References for easy manipulation */
	DirEntry *parent;
	int       row;    
};

/******************************************************************************************
 * DirEntry
 *****************************************************************************************/

class DirEntry: public FileEntry
{
	public:

		/* cleanup */
virtual		~DirEntry();

		/* update local entries */
DirEntry * 	updateDir(FileEntry fe, time_t updtime);
FileEntry * 	updateFile(FileEntry fe, time_t updtime);


int  		checkParentPointers();
int 		updateChildRows();

		/* remove local entries */
int  		removeFile(std::string name);
int  		removeDir(std::string name);
int     	removeOldDir(std::string name, time_t old); /* checks ts first */

		/* recursive cleanup */
int  		removeOldEntries(time_t old, bool recursive); 

		/* recursive searches */
DirEntry *	findOldDirectory(time_t old);  
DirEntry *	findDirectory(std::string path); 

		/* recursive update directory mod/pop values */
int	 	updateDirectories(std::string path, int pop, int modtime);

		/* output */
int 		print(std::ostream &out);

int 		saveEntry(std::ostringstream &out);
void writeDirInfo(std::ostringstream&);
void writeFileInfo(std::ostringstream&);

	/* Data */
	std::string path;    /* full path (includes name) */
	std::map<std::string, DirEntry *>  subdirs;
	std::map<std::string, FileEntry *> files;

	/* Inherited members from FileEntry:
	int size 	  - count for dirs 
	std::string name; - directory name 
	std::string hash; - not used 
	int size;         - not used 
	int modtime;      - most recent modication time of any child file (recursive) 
	int pop;	  - most popular child file (recursive)  
	int updtime;      - last updated 
	*/

};

/******************************************************************************************
 * PersonEntry
 *****************************************************************************************/

class PersonEntry: public DirEntry
{
	public:
	/* cleanup */
	PersonEntry(std::string pid) : id(pid) { return; }
virtual	~PersonEntry() { return; }

DirEntry &operator=(DirEntry &src)
{
	DirEntry *pdest = this;
	(*pdest) = src;
	return src;
}

	/* Data */
	std::string id;

	/* Inherited members from FileEntry:
	int size 	  - count for dirs 
	std::string name; - directory name 
	std::string hash; - not used 
	int size;         - not used 
	int modtime;      - most recent modication time of any child file (recursive) 
	int pop;	  - most popular child file (recursive)  
	int updtime;      - last updated 
	*/

};

/******************************************************************************************
 * FileIndex
 *****************************************************************************************/

class Expression;

class FileIndex
{
	public:
		FileIndex(std::string pid);
		~FileIndex();

		/* control root entries */
		int	setRootDirectories(std::list<std::string> inlist, time_t utime);
		int	getRootDirectories(std::list<std::string> &outlist);

		/* update (index building) */
		DirEntry  * updateDirEntry(std::string path, FileEntry fe, time_t utime);
		FileEntry * updateFileEntry(std::string path, FileEntry fe, time_t utime);

		DirEntry  * findOldDirectory(time_t old); /* finds directories older than old */
		int     removeOldDirectory(std::string fpath, std::string name, time_t old);

		int	cleanOldEntries(time_t old);  /* removes entries older than old */

		/* debug */
		int	printFileIndex(std::ostream &out);

		/* load/save to file */
		int 	loadIndex(std::string filename, std::string expectedHash, uint64_t size);
		int 	saveIndex(std::string filename, std::string &fileHash, uint64_t &size, const std::set<std::string>& forbidden_roots);

		/* search through this index */
		int 	searchTerms(std::list<std::string> terms, std::list<FileEntry *> &results) const;
		int 	searchHash(std::string hash, std::list<FileEntry *> &results) const;
		int     searchBoolExp(Expression * exp, std::list<FileEntry *> &results) const;

		PersonEntry *root;
};


#endif


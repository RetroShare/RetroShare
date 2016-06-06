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
#if __MACH__
#include <unordered_set>
#else
#include <tr1/unordered_set>
#endif
#include <list>
#include <vector>
#include <stdint.h>
#include "retroshare/rstypes.h"

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

#include <util/smallobject.h>

class DirEntry;

class FileEntry: public RsMemoryManagement::SmallObject
{
	public:
	FileEntry() 
		: size(0), modtime(0), pop(0), updtime(0), parent(NULL), row(0)
	{ return; }

	virtual ~FileEntry() { return; }
	virtual uint32_t type() const { return DIR_TYPE_FILE ; }

virtual int print(std::string &out);

	/* Data */
	std::string name; 
    RsFileHash hash;
	uint64_t size;         /* file size */
	time_t modtime;      /* modification time - most recent mod time for a sub entry for dirs */
	int pop;	  /* popularity rating */

	time_t updtime;      /* last updated */

	/* References for easy manipulation */
	DirEntry *parent;
	int       row;    
	std::list<std::string> parent_groups ;
};

/******************************************************************************************
 * DirEntry
 *****************************************************************************************/

class DirEntry: public FileEntry
{
	public:

		DirEntry() : most_recent_time(0) {}
		/* cleanup */
virtual		~DirEntry();

		/* update local entries */
DirEntry * 	updateDir(const FileEntry& fe, time_t updtime);
FileEntry * 	updateFile(const FileEntry& fe, time_t updtime);


	virtual uint32_t type() const { return DIR_TYPE_DIR ; }
int  		checkParentPointers();
int 		updateChildRows();

		/* remove local entries */
int  		removeFile(const std::string& name);
int  		removeDir(const std::string& name);
int     	removeOldDir(const std::string& name, time_t old); /* checks ts first */

		/* recursive cleanup */
int  		removeOldEntries(time_t old, bool recursive); 

		/* recursive searches */
DirEntry *	findOldDirectory(time_t old);  
DirEntry *	findDirectory(const std::string& path); 

		/* recursive update directory mod/pop values */
int	 	updateDirectories(const std::string& path, int pop, int modtime);

		/* output */
int 		print(std::string &out);

int 		saveEntry(std::string &out);
void writeDirInfo(std::string&);
void writeFileInfo(std::string&);

	/* Data */
	std::string path;    /* full path (includes name) */
	std::map<std::string, DirEntry *>  subdirs;
	std::map<std::string, FileEntry *> files;

	time_t most_recent_time;      /* last updated */

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
	PersonEntry(const RsPeerId& pid) : id(pid) { return; }
virtual	~PersonEntry() { return; }

DirEntry &operator=(DirEntry &src)
{
	DirEntry *pdest = this;
	(*pdest) = src;
	return *this;
}
	virtual uint32_t type() const { return DIR_TYPE_PERSON ; }

	/* Data */
	RsPeerId id;

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
		FileIndex(const RsPeerId& pid);
		~FileIndex();

		/* control root entries */
		int	setRootDirectories(const std::list<std::string> &inlist, time_t utime);
		int	getRootDirectories(std::list<std::string> &outlist);

		/* update (index building) */
		DirEntry  * updateDirEntry(const std::string& path, const FileEntry& fe, time_t utime);
		FileEntry * updateFileEntry(const std::string& path, const FileEntry& fe, time_t utime);

		DirEntry  * findOldDirectory(time_t old); /* finds directories older than old */
		int     removeOldDirectory(const std::string& fpath, const std::string& name, time_t old);

		int	cleanOldEntries(time_t old);  /* removes entries older than old */

		/* debug */
		int	printFileIndex(std::string &out);
		int	printFileIndex(std::ostream &out);

		/* load/save to file */
        int 	loadIndex(const std::string& filename, const RsFileHash &expectedHash, uint64_t size);
        int 	saveIndex(const std::string& filename, RsFileHash &fileHash, uint64_t &size, const std::set<std::string>& forbidden_roots);

		/* search through this index */
		int 	searchTerms(const std::list<std::string>& terms, std::list<FileEntry *> &results) const;
        int 	searchHash(const RsFileHash& hash, std::list<FileEntry *> &results) const;
		int     searchBoolExp(Expression * exp, std::list<FileEntry *> &results) const;

		/// Recursively compute the maximum modification time of children.
		/// Used to easily retrieve mose recent files.
		//
		void updateMaxModTime() ;
		void RecursUpdateMaxModTime(DirEntry *) ;

		PersonEntry *root;

#ifdef __MACH__
		static std::unordered_set<void*> _pointers ;
#else
		static std::tr1::unordered_set<void*> _pointers ;
#endif
		static void registerEntry(void*p) ; 
		static void unregisterEntry(void*p) ; 
		static bool isValid(void*p)  ;

		/// Fills up details from the data contained in ref.
		//
		static bool extractData(void *ref,DirDetails& details) ;
		static uint32_t getType(void *ref) ;

		void *findRef(const std::string& path) const ;
		bool extractData(const std::string& path,DirDetails& details) const ;

		void updateHashIndex() ;
		void recursUpdateHashIndex(DirEntry *) ;

        std::map<RsFileHash,FileEntry*> _file_hashes ;
};


#endif


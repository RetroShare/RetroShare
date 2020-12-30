/*******************************************************************************
 * libretroshare/src/file_sharing: dir_hierarchy.h                             *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2016 by Mr.Alice <mralice@users.sourceforge.net>                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/
#pragma once

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "directory_storage.h"

class InternalFileHierarchyStorage
{
public:
    class FileStorageNode
    {
    public:
        static const uint32_t TYPE_UNKNOWN = 0x0000 ;
        static const uint32_t TYPE_FILE    = 0x0001 ;
        static const uint32_t TYPE_DIR     = 0x0002 ;

        virtual ~FileStorageNode() {}
        virtual uint32_t type() const =0;

        DirectoryStorage::EntryIndex parent_index;
        uint32_t row ;
    };
    class FileEntry: public FileStorageNode
    {
    public:
        FileEntry() : file_size(0), file_modtime(0) {}
        FileEntry(const std::string& name,uint64_t size,rstime_t modtime) : file_name(name),file_size(size),file_modtime(modtime) {}
        FileEntry(const std::string& name,uint64_t size,rstime_t modtime,const RsFileHash& hash) : file_name(name),file_size(size),file_modtime(modtime),file_hash(hash) {}

        virtual uint32_t type() const { return FileStorageNode::TYPE_FILE ; }
        virtual ~FileEntry() {}

        // local stuff
        std::string file_name ;
        uint64_t    file_size ;
        rstime_t      file_modtime;
        RsFileHash  file_hash ;
    };

    class DirEntry: public FileStorageNode
    {
    public:
        explicit DirEntry(const std::string& name) : dir_name(name), dir_cumulated_size(0), dir_modtime(0),dir_most_recent_time(0),dir_update_time(0) {}
        virtual ~DirEntry() {}

        virtual uint32_t type() const { return FileStorageNode::TYPE_DIR ; }

        // local stuff
        std::string dir_name ;
        std::string dir_parent_path ;
        RsFileHash  dir_hash ;
        uint64_t    dir_cumulated_size;

        std::vector<DirectoryStorage::EntryIndex> subdirs ;
        std::vector<DirectoryStorage::EntryIndex> subfiles ;

        rstime_t dir_modtime;
        rstime_t dir_most_recent_time;// recursive most recent modification time, including files and subdirs in the entire hierarchy below.
        rstime_t dir_update_time;		// last time the information was updated for that directory. Includes subdirs indexes and subfile info.
    };

    // class stuff
    InternalFileHierarchyStorage() ;

    bool load(const std::string& fname) ;
    bool save(const std::string& fname) ;

    int parentRow(DirectoryStorage::EntryIndex e);
    bool isIndexValid(DirectoryStorage::EntryIndex e) const;
    bool getChildIndex(DirectoryStorage::EntryIndex e,int row,DirectoryStorage::EntryIndex& c) const;
    bool updateSubDirectoryList(const DirectoryStorage::EntryIndex& indx, const std::set<std::string>& subdirs, const RsFileHash &random_hash_seed);
    bool removeDirectory(DirectoryStorage::EntryIndex indx)	;
    bool checkIndex(DirectoryStorage::EntryIndex indx,uint8_t type) const;
    bool updateSubFilesList(const DirectoryStorage::EntryIndex& indx,const std::map<std::string,DirectoryStorage::FileTS>& subfiles,std::map<std::string,DirectoryStorage::FileTS>& new_files);
    bool updateHash(const DirectoryStorage::EntryIndex& file_index,const RsFileHash& hash);
    bool updateFile(const DirectoryStorage::EntryIndex& file_index,const RsFileHash& hash, const std::string& fname,uint64_t size, const rstime_t modf_time);
    bool updateDirEntry(const DirectoryStorage::EntryIndex& indx, const std::string& dir_name, rstime_t most_recent_time, rstime_t dir_modtime, const std::vector<RsFileHash> &subdirs_hash, const std::vector<FileEntry> &subfiles_array);

    // TS get/set functions. Take one of the class members as argument.

    bool getTS(const DirectoryStorage::EntryIndex& index,rstime_t& TS,rstime_t DirEntry::* ) const;
    bool setTS(const DirectoryStorage::EntryIndex& index,rstime_t& TS,rstime_t DirEntry::* ) ;

    // Do a complete recursive sweep over sub-directories and files, and update the lst modf TS. This could be also performed by a cleanup method.
    // Also keeps the high level statistics up to date.

    rstime_t recursUpdateLastModfTime(const DirectoryStorage::EntryIndex& dir_index, bool &unfinished_files_present);

    // Do a complete recursive sweep over sub-directories and files, and update the cumulative size.

    uint64_t recursUpdateCumulatedSize(const DirectoryStorage::EntryIndex& dir_index);

    // hash stuff

    bool getDirHashFromIndex(const DirectoryStorage::EntryIndex& index,RsFileHash& hash) const ;
    bool getIndexFromDirHash(const RsFileHash& hash,DirectoryStorage::EntryIndex& index) ;
    bool getIndexFromFileHash(const RsFileHash& hash,DirectoryStorage::EntryIndex& index) ;

    // file/dir access and modification
    bool findSubDirectory(DirectoryStorage::EntryIndex e,const std::string& s) const ;	// returns true when s is the name of a sub-directory in the given entry e

    uint32_t mRoot ;
    std::list<uint32_t > mFreeNodes ;	// keeps a list of free nodes in order to make insert effcieint
    std::vector<FileStorageNode*> mNodes;// uses pointers to keep information about valid/invalid objects.

    void compress() ;					// use empty space in the vector, mostly due to deleted entries. This is a complicated operation, mostly due to
                                            // all the indirections used. Nodes need to be moved, renamed, etc. The operation discards all file entries that
                                            // are not referenced.

    friend class DirectoryStorage ;		// only class that can use this.
    friend class LocalDirectoryStorage ;		// only class that can use this.

    // Low level stuff. Should normally not be used externally.

    const FileStorageNode *getNode(DirectoryStorage::EntryIndex indx) const;
    const DirEntry *getDirEntry(DirectoryStorage::EntryIndex indx) const;
    const FileEntry *getFileEntry(DirectoryStorage::EntryIndex indx) const;
    uint32_t getType(DirectoryStorage::EntryIndex indx) const;
    DirectoryStorage::EntryIndex getSubFileIndex(DirectoryStorage::EntryIndex parent_index,uint32_t file_tab_index);
    DirectoryStorage::EntryIndex getSubDirIndex(DirectoryStorage::EntryIndex parent_index,uint32_t dir_tab_index);

    // search. SearchHash is logarithmic. The other two are linear.

    bool searchHash(const RsFileHash& hash, DirectoryStorage::EntryIndex &result);
    int searchBoolExp(RsRegularExpression::Expression * exp, std::list<DirectoryStorage::EntryIndex> &results) const ;
    int searchTerms(const std::list<std::string>& terms, std::list<DirectoryStorage::EntryIndex> &results) const ;		// does a logical OR between items of the list of terms

    bool check(std::string& error_string)	;// checks consistency of storage.

    void print() const;

    // gets statistics about share files

    void getStatistics(SharedDirStats& stats) const ;

private:
    void recursPrint(int depth,DirectoryStorage::EntryIndex node) const;
    static bool nodeAccessError(const std::string& s);
    static RsFileHash createDirHash(const std::string& dir_name, const RsFileHash &dir_parent_hash, const RsFileHash &random_hash_salt) ;

    // Allocates a new entry in mNodes, possible re-using an empty slot and returns its index.

    DirectoryStorage::EntryIndex allocateNewIndex();

    // Deletes an existing entry in mNodes, and keeps record of the indices that get freed.

    void deleteNode(DirectoryStorage::EntryIndex);
    void deleteFileNode(DirectoryStorage::EntryIndex);

    // Removes the given subdirectory from the parent node and all its pendign subdirs. Files are kept, and will go during the cleaning
    // phase. That allows to keep file information when moving them around.

    bool recursRemoveDirectory(DirectoryStorage::EntryIndex dir);

    // Map of the hash of all files. The file hashes are the sha1sum of the file data.
    // is used for fast search access for FT.
    // Note: We should try something faster than std::map. hash_map??
    // Unlike directories, multiple files may have the same hash. So this cannot be used for anything else than FT.

    std::map<RsFileHash,DirectoryStorage::EntryIndex> mFileHashes ;

    // The directory hashes are the sha1sum of the
    // full public path to the directory.
    // The later is used by synchronisation items in order
    // to avoid sending explicit EntryIndex values.
    // This is kept separate from mFileHashes because the two are used
    // in very different ways.
    //
    std::map<RsFileHash,DirectoryStorage::EntryIndex> mDirHashes ;

    // high level statistics on the full hierarchy. Should be kept up to date.

    uint32_t mTotalFiles ;
    uint64_t mTotalSize ;
};


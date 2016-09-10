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
        FileEntry(const std::string& name,uint64_t size,time_t modtime) : file_name(name),file_size(size),file_modtime(modtime) {}
        FileEntry(const std::string& name,uint64_t size,time_t modtime,const RsFileHash& hash) : file_name(name),file_size(size),file_modtime(modtime),file_hash(hash) {}

        virtual uint32_t type() const { return FileStorageNode::TYPE_FILE ; }
        virtual ~FileEntry() {}

        // local stuff
        std::string file_name ;
        uint64_t    file_size ;
        time_t      file_modtime;
        RsFileHash  file_hash ;
    };

    class DirEntry: public FileStorageNode
    {
    public:
        DirEntry(const std::string& name) : dir_name(name), dir_modtime(0),dir_most_recent_time(0),dir_update_time(0) {}
        virtual ~DirEntry() {}

        virtual uint32_t type() const { return FileStorageNode::TYPE_DIR ; }

        // local stuff
        std::string dir_name ;
        std::string dir_parent_path ;
        RsFileHash  dir_hash ;

        std::vector<DirectoryStorage::EntryIndex> subdirs ;
        std::vector<DirectoryStorage::EntryIndex> subfiles ;

        time_t dir_modtime;
        time_t dir_most_recent_time;	// recursive most recent modification time, including files and subdirs in the entire hierarchy below.
        time_t dir_update_time;		// last time the information was updated for that directory. Includes subdirs indexes and subfile info.
    };

    // class stuff
    InternalFileHierarchyStorage() ;

    bool load(const std::string& fname) ;
    bool save(const std::string& fname) ;

    int parentRow(DirectoryStorage::EntryIndex e);
    bool isIndexValid(DirectoryStorage::EntryIndex e) const;
    bool stampDirectory(const DirectoryStorage::EntryIndex& indx);
    bool updateSubDirectoryList(const DirectoryStorage::EntryIndex& indx,const std::map<std::string,time_t>& subdirs);
    bool removeDirectory(DirectoryStorage::EntryIndex indx)	;
    bool checkIndex(DirectoryStorage::EntryIndex indx,uint8_t type) const;
    bool updateSubFilesList(const DirectoryStorage::EntryIndex& indx,const std::map<std::string,DirectoryStorage::FileTS>& subfiles,std::map<std::string,DirectoryStorage::FileTS>& new_files);
    bool updateHash(const DirectoryStorage::EntryIndex& file_index,const RsFileHash& hash);
    bool updateFile(const DirectoryStorage::EntryIndex& file_index,const RsFileHash& hash, const std::string& fname,uint64_t size, const time_t modf_time);
    bool updateDirEntry(const DirectoryStorage::EntryIndex& indx, const std::string& dir_name, time_t most_recent_time, time_t dir_modtime, const std::vector<RsFileHash> &subdirs_hash, const std::vector<RsFileHash> &subfiles_hash);
    bool getDirUpdateTS(const DirectoryStorage::EntryIndex& index,time_t& recurs_max_modf_TS,time_t& local_update_TS);
    bool setDirUpdateTS(const DirectoryStorage::EntryIndex& index,time_t& recurs_max_modf_TS,time_t& local_update_TS);

    // Do a complete recursive sweep over sub-directories and files, and update the lst modf TS. This could be also performed by a cleanup method.

    time_t recursUpdateLastModfTime(const DirectoryStorage::EntryIndex& dir_index);

    // hash stuff

    bool getDirHashFromIndex(const DirectoryStorage::EntryIndex& index,RsFileHash& hash) const ;
    bool getIndexFromDirHash(const RsFileHash& hash,DirectoryStorage::EntryIndex& index) const ;
    bool getIndexFromFileHash(const RsFileHash& hash,DirectoryStorage::EntryIndex& index) const ;

    // file/dir access and modification
    bool findSubDirectory(DirectoryStorage::EntryIndex e,const std::string& s) const ;	// returns true when s is the name of a sub-directory in the given entry e

    uint32_t mRoot ;
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
    bool searchHash(const RsFileHash& hash,std::list<DirectoryStorage::EntryIndex>& results);

    bool check(std::string& error_string) const	;// checks consistency of storage.

    void print() const;

private:
    void recursPrint(int depth,DirectoryStorage::EntryIndex node) const;
    static bool nodeAccessError(const std::string& s);
    static RsFileHash createDirHash(const std::string& dir_name,const std::string& dir_parent_path) ;

    // Removes the given subdirectory from the parent node and all its pendign subdirs. Files are kept, and will go during the cleaning
    // phase. That allows to keep file information when moving them around.

    bool recursRemoveDirectory(DirectoryStorage::EntryIndex dir);

    // Map of the hash of all files and all directories. The file hashes are the sha1sum of the file data.
    // is used for fast search access for FT.
    // Note: We should try something faster than std::map. hash_map??

    std::map<RsFileHash,DirectoryStorage::EntryIndex> mFileHashes ;

    // The directory hashes are the sha1sum of the
    // full public path to the directory.
    // The later is used by synchronisation items in order
    // to avoid sending explicit EntryIndex values.
    // This is kept separate from mFileHashes because the two are used
    // in very different ways.
    //
    std::map<RsFileHash,DirectoryStorage::EntryIndex> mDirHashes ;
};


#pragma once

#include <string>
#include <stdint.h>
#include <list>

#include "retroshare/rsids.h"
#include "retroshare/rsfiles.h"

#define NOT_IMPLEMENTED() { std::cerr << __PRETTY_FUNCTION__ << ": not yet implemented." << std::endl; }

class RsTlvBinaryData ;
class InternalFileHierarchyStorage ;
class RsTlvBinaryData ;

class DirectoryStorage
{
	public:
        DirectoryStorage(const RsPeerId& pid) ;
        virtual ~DirectoryStorage() {}

        typedef uint32_t EntryIndex ;
        static const EntryIndex NO_INDEX = 0xffffffff;

		void save() const ;

        virtual int searchTerms(const std::list<std::string>& terms, std::list<EntryIndex> &results) const { NOT_IMPLEMENTED() ; return 0;}
        virtual int searchHash(const RsFileHash& hash, std::list<EntryIndex> &results) const ;
        virtual int searchBoolExp(Expression * exp, std::list<EntryIndex> &results) const { NOT_IMPLEMENTED() ; return 0; }

        bool getDirUpdateTS(EntryIndex index,time_t& recurs_max_modf_TS,time_t& local_update_TS) ;
        bool setDirUpdateTS(EntryIndex index,time_t  recurs_max_modf_TS,time_t  local_update_TS) ;

        uint32_t getEntryType(const EntryIndex& indx) ;	                     // returns DIR_TYPE_*, not the internal directory storage stuff.
        virtual bool extractData(const EntryIndex& indx,DirDetails& d);

		// This class allows to abstractly browse the stored directory hierarchy in a depth-first manner.
		// It gives access to sub-files and sub-directories below.
		//
		class DirIterator
		{
			public:
                DirIterator(const DirIterator& d) ;
                DirIterator(DirectoryStorage *d,EntryIndex i) ;

				DirIterator& operator++() ;
                EntryIndex operator*() const ;

                operator bool() const ;			// used in for loops. Returns true when the iterator is valid.

                // info about the directory that is pointed by the iterator

                std::string name() const ;
                time_t last_modif_time() const ; // last time a file in this directory or in the directories below has been modified.
                time_t last_update_time() const ; // last time this directory was updated
            private:
                EntryIndex mParentIndex ;		// index of the parent dir.
                uint32_t mDirTabIndex ;				// index in the vector of subdirs.
                InternalFileHierarchyStorage *mStorage ;

                friend class DirectoryStorage ;
        };
		class FileIterator
		{
			public:
                FileIterator(DirIterator& d);	// crawls all files in specified directory
                FileIterator(DirectoryStorage *d,EntryIndex e);		// crawls all files in specified directory

				FileIterator& operator++() ;
                EntryIndex operator*() const ;	// current file entry

                operator bool() const ;			// used in for loops. Returns true when the iterator is valid.

                // info about the file that is pointed by the iterator

                std::string name() const ;
                uint64_t size() const ;
                RsFileHash hash() const ;
                time_t modtime() const ;

            private:
                EntryIndex mParentIndex ;		// index of the parent dir.
                uint32_t   mFileTabIndex ;		// index in the vector of subdirs.
                InternalFileHierarchyStorage *mStorage ;
        };

        struct FileTS
        {
            uint64_t size ;
            time_t modtime;
        };

        EntryIndex root() const ;					// returns the index of the root directory entry.
        const RsPeerId& peerId() const { return mPeerId ; }
        int parentRow(EntryIndex e) const ;

        bool updateSubDirectoryList(const EntryIndex& indx, const std::map<std::string, time_t> &subdirs) ;
        bool updateSubFilesList(const EntryIndex& indx, const std::map<std::string, FileTS> &subfiles, std::map<std::string, FileTS> &new_files) ;
        bool removeDirectory(const EntryIndex& indx) ;

        bool updateFile(const EntryIndex& index,const RsFileHash& hash, const std::string& fname,  uint64_t size, time_t modf_time) ;
        bool updateHash(const EntryIndex& index,const RsFileHash& hash);

        bool getDirHashFromIndex(const EntryIndex& index,RsFileHash& hash) const ;
        bool getIndexFromDirHash(const RsFileHash& hash,EntryIndex& index) const ;

        void print();
        void cleanup();

    protected:
		void load(const std::string& local_file_name) ;
		void save(const std::string& local_file_name) ;

    private:

        void loadNextTag(const unsigned char *data, uint32_t& offset, uint8_t& entry_tag, uint32_t& entry_size) ;
        void saveNextTag(unsigned char *data,uint32_t& offset,uint8_t entry_tag,uint32_t entry_size) ;

        // debug
        void locked_check();

        // storage of internal structure. Totally hidden from the outside. EntryIndex is simply the index of the entry in the vector.

        RsPeerId mPeerId;

    protected:
        mutable RsMutex mDirStorageMtx ;

        InternalFileHierarchyStorage *mFileHierarchy ;
};

class RemoteDirectoryStorage: public DirectoryStorage
{
public:
    RemoteDirectoryStorage(const RsPeerId& pid,const std::string& fname) ;
    virtual ~RemoteDirectoryStorage() {}

    /*!
     * \brief deserialiseDirEntry
     * 			Loads a serialised directory content coming from a friend. The directory entry needs to exist already,
     * 			as it is created when updating the parent.
     *
     * \param indx		index of the directory to update
     * \param bindata   binary data to deserialise from
     * \return 			false when the directory cannot be found.
     */
    bool deserialiseUpdateDirEntry(const EntryIndex& indx,const RsTlvBinaryData& data) ;

    /*!
     * \brief checkSave
     * 			Checks the time of last saving, last modification time, and saves if needed.
     */
    void checkSave() ;

private:
    time_t mLastSavedTime ;
    bool mChanged ;
    std::string mFileName;
};

class LocalDirectoryStorage: public DirectoryStorage
{
public:
    LocalDirectoryStorage(const std::string& fname,const RsPeerId& own_id) : DirectoryStorage(own_id),mFileName(fname) {}
    virtual ~LocalDirectoryStorage() {}

    void setSharedDirectoryList(const std::list<SharedDirInfo>& lst) ;
    void getSharedDirectoryList(std::list<SharedDirInfo>& lst) ;

    void updateShareFlags(const SharedDirInfo& info) ;
    bool convertSharedFilePath(const std::string& path_with_virtual_name,std::string& fullpath) ;

    void updateTimeStamps();
    /*!
     * \brief getFileInfo Converts an index info a full file info structure.
     * \param i index in the directory structure
     * \param info structure to be filled in
     * \return false if the file does not exist, or is a directory,...
     */
    bool getFileInfo(DirectoryStorage::EntryIndex i,FileInfo& info) ;

    /*!
     * \brief getFileSharingPermissions
     * 			Computes the flags and parent groups for any index.
     * \param indx    index of the entry to compute the flags for
     * \param flags		computed flags
     * \param parent_groups computed parent groups
     * \return
     * 			false if the index is not valid
     * 			false otherwise
     */
    bool getFileSharingPermissions(const EntryIndex& indx, FileStorageFlags &flags, std::list<RsNodeGroupId> &parent_groups);

    virtual bool extractData(const EntryIndex& indx,DirDetails& d) ;

    /*!
     * \brief serialiseDirEntry
     * 			Produced a serialised directory content listing suitable for export to friends.
     *
     * \param indx					index of the directory to serialise
     * \param bindata   			binary data created by serialisation
     * \param client_id      		Peer id to be serialised to. Depending on permissions, some subdirs can be removed.
     * \return 						false when the directory cannot be found.
     */
    bool serialiseDirEntry(const EntryIndex& indx, RsTlvBinaryData& bindata, const RsPeerId &client_id) ;

private:
    bool locked_getFileSharingPermissions(const EntryIndex& indx, FileStorageFlags &flags, std::list<RsNodeGroupId>& parent_groups);
    std::string locked_findRealRootFromVirtualFilename(const std::string& virtual_rootdir) const;

    std::map<std::string,SharedDirInfo> mLocalDirs ;	// map is better for search. it->first=it->second.filename
    std::string mFileName;
};












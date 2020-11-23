/*******************************************************************************
 * libretroshare/src/file_sharing: directory_storage.h                         *
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

#include <string>
#include <stdint.h>
#include <list>

#include "retroshare/rsids.h"
#include "retroshare/rsfiles.h"
#include "util/rstime.h"

#define NOT_IMPLEMENTED() { std::cerr << __PRETTY_FUNCTION__ << ": not yet implemented." << std::endl; }

class RsTlvBinaryData ;
class InternalFileHierarchyStorage ;
class RsTlvBinaryData ;

class DirectoryStorage
{
	public:
        DirectoryStorage(const RsPeerId& pid, const std::string& fname) ;
        virtual ~DirectoryStorage() {}

        typedef uint32_t EntryIndex ;
        static const EntryIndex NO_INDEX = 0xffffffff;

		void save() const ;

        // These functions are to be used by file transfer and file search.

        virtual int searchTerms(const std::list<std::string>& terms, std::list<EntryIndex> &results) const ;
        virtual int searchBoolExp(RsRegularExpression::Expression * exp, std::list<EntryIndex> &results) const ;

        // gets/sets the various time stamps:
        //
        bool getDirectoryRecursModTime(EntryIndex index,rstime_t& recurs_max_modf_TS) const ;		// last modification time, computed recursively over all subfiles and directories
        bool getDirectoryLocalModTime (EntryIndex index,rstime_t& motime_TS) const ;				// last modification time for that index only
        bool getDirectoryUpdateTime   (EntryIndex index,rstime_t& update_TS) const ;				// last time the entry was updated. This is only used on the RemoteDirectoryStorage side.

        bool setDirectoryRecursModTime(EntryIndex index,rstime_t  recurs_max_modf_TS) ;
        bool setDirectoryLocalModTime (EntryIndex index,rstime_t  modtime_TS) ;
        bool setDirectoryUpdateTime   (EntryIndex index,rstime_t  update_TS) ;

        uint32_t getEntryType(const EntryIndex& indx) ;	                     // WARNING: returns DIR_TYPE_*, not the internal directory storage stuff.
        virtual bool extractData(const EntryIndex& indx,DirDetails& d);

		// This class allows to abstractly browse the stored directory hierarchy in a depth-first manner.
        // It gives access to sub-files and sub-directories below. When using it, the client should make sure
        // that the DirectoryStorage is properly locked, since the iterator cannot lock it.
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
                rstime_t last_modif_time() const ; // last time a file in this directory or in the directories below has been modified.
                rstime_t last_update_time() const ; // last time this directory was updated
            private:
                EntryIndex mParentIndex ;		// index of the parent dir.
                uint32_t mDirTabIndex ;				// index in the vector of subdirs.
                InternalFileHierarchyStorage *mStorage ;

                friend class DirectoryStorage ;
        };
		class FileIterator
		{
			public:
                explicit FileIterator(DirIterator& d);	// crawls all files in specified directory
                FileIterator(DirectoryStorage *d,EntryIndex e);		// crawls all files in specified directory

				FileIterator& operator++() ;
                EntryIndex operator*() const ;	// current file entry

                operator bool() const ;			// used in for loops. Returns true when the iterator is valid.

                // info about the file that is pointed by the iterator

                std::string name() const ;
                uint64_t size() const ;
                RsFileHash hash() const ;
                rstime_t modtime() const ;

            private:
                EntryIndex mParentIndex ;		// index of the parent dir.
                uint32_t   mFileTabIndex ;		// index in the vector of subdirs.
                InternalFileHierarchyStorage *mStorage ;
        };

        struct FileTS
        {
            uint64_t size ;
            rstime_t modtime;
        };

        EntryIndex root() const ;								// returns the index of the root directory entry. This is generally 0.
        const RsPeerId& peerId() const { return mPeerId ; }		// peer ID of who owns that file list.
        int parentRow(EntryIndex e) const ;						// position of the current node, in the array of children at its parent node. Used by GUI for display.
        bool getChildIndex(EntryIndex e,int row,EntryIndex& c) const;	// returns the index of the children node at position "row" in the children nodes. Used by GUI for display.

        // Sets the subdirectory/subfiles list of entry indx the supplied one, possible adding and removing directories (resp.files). New directories are set empty with
        // just a name and need to be updated later on. New files are returned in a list so that they can be sent to hash cache.
        //
        bool updateSubDirectoryList(const EntryIndex& indx, const std::set<std::string>& subdirs, const RsFileHash &random_hash_salt) ;
        bool updateSubFilesList(const EntryIndex& indx, const std::map<std::string, FileTS> &subfiles, std::map<std::string, FileTS> &new_files) ;
        bool removeDirectory(const EntryIndex& indx) ;

        // Returns the hash of the directory at the given index and reverse. This hash is set as random the first time it is used (when updating directories). It will be
        // used by the sync system to designate the directory without referring to index (index could be used to figure out the existance of hidden directories)

        bool getDirHashFromIndex(const EntryIndex& index,RsFileHash& hash) const ;	// constant cost
        bool getIndexFromDirHash(const RsFileHash& hash,EntryIndex& index) const ;	// log cost.

        // gathers statistics from the internal directory structure

        void getStatistics(SharedDirStats& stats) ;

        void print();
        void cleanup();

		/*!
		 * \brief checkSave
		 * 			Checks the time of last saving, last modification time, and saves if needed.
		 */
		void checkSave() ;

		const std::string& filename() const { return mFileName ; }

    protected:
        bool load(const std::string& local_file_name) ;
		void save(const std::string& local_file_name) ;

    private:

        // debug
        void locked_check();

        // storage of internal structure. Totally hidden from the outside. EntryIndex is simply the index of the entry in the vector.

        RsPeerId mPeerId;

    protected:
        mutable RsMutex mDirStorageMtx ;

        InternalFileHierarchyStorage *mFileHierarchy ;

		rstime_t mLastSavedTime ;
		bool mChanged ;
		std::string mFileName;
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
     * \brief lastSweepTime
     * 			returns the last time a sweep has been done over the directory in order to check update TS.
     * \return
     */
    rstime_t& lastSweepTime() { return mLastSweepTime ; }
	
	/*!
     * \brief searchHash
     * 				Looks into local database of shared files for the given hash. 
     * \param hash			hash to look for
     * \param results		Entry index of the file that is found
     * \return
     * 						true is a file is found
     * 						false otherwise.
     */
    virtual int searchHash(const RsFileHash& hash, EntryIndex& results) const ;

private:
    rstime_t mLastSweepTime ;
};

class LocalDirectoryStorage: public DirectoryStorage
{
public:
    LocalDirectoryStorage(const std::string& fname,const RsPeerId& own_id);
    virtual ~LocalDirectoryStorage() {}

    /*!
     * \brief [gs]etSharedDirectoryList
     * 			Gets/sets the list of shared directories. Each directory is supplied with a virtual name (the name the friends will see), and sharing flags/groups.
     * \param lst
     */
    void setSharedDirectoryList(const std::list<SharedDirInfo>& lst) ;
    void getSharedDirectoryList(std::list<SharedDirInfo>& lst) ;

    void updateShareFlags(const SharedDirInfo& info) ;
    bool convertSharedFilePath(const std::string& path_with_virtual_name,std::string& fullpath) ;

    virtual bool updateHash(const EntryIndex& index, const RsFileHash& hash, bool update_internal_hierarchy);
    /*!
     * \brief searchHash
     * 				Looks into local database of shared files for the given hash. Also looks for files such that the hash of the hash
     * 				matches the given hash, and returns the real hash.
     * \param hash			hash to look for
     * \param real_hash		hash such that H(real_hash) = hash, or null hash if not found.
     * \param results		Entry index of the file that is found
     * \return
     * 						true is a file is found
     * 						false otherwise.
     */
    virtual int searchHash(const RsFileHash& hash, RsFileHash &real_hash, EntryIndex &results) const ;

    /*!
     * \brief updateTimeStamps
     * 			Checks recursive TS and update the if needed.
     */
    void updateTimeStamps();

    /*!
     * \brief notifyTSChanged
     * 			Use this to force an update of the recursive TS, when calling updateTimeStamps();
     */
    void notifyTSChanged();
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
	static RsFileHash makeEncryptedHash(const RsFileHash& hash);
	bool locked_findRealHash(const RsFileHash& hash, RsFileHash& real_hash) const;
	std::string locked_getVirtualPath(EntryIndex indx) const ;
	std::string locked_getVirtualDirName(EntryIndex indx) const ;

	bool locked_getFileSharingPermissions(const EntryIndex& indx, FileStorageFlags &flags, std::list<RsNodeGroupId>& parent_groups);
	std::string locked_findRealRootFromVirtualFilename(const std::string& virtual_rootdir) const;

	std::map<std::string,SharedDirInfo> mLocalDirs ;	// map is better for search. it->first=it->second.filename
	std::map<RsFileHash,RsFileHash> mEncryptedHashes;	// map such that hash(it->second) = it->first

	bool mTSChanged ;
};












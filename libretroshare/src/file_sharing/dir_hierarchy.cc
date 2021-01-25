/*******************************************************************************
 * libretroshare/src/file_sharing: dir_hierarchy.cc                            *
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
#include <sstream>
#include <algorithm>

#include "util/rstime.h"
#include "util/rsdir.h"
#include "util/rsprint.h"
#include "retroshare/rsexpr.h"
#include "dir_hierarchy.h"
#include "filelist_io.h"
#include "file_sharing_defaults.h"

#ifdef RS_DEEP_FILES_INDEX
#	include "deep_search/filesindex.hpp"
#endif // def RS_DEEP_FILES_INDEX

//#define DEBUG_DIRECTORY_STORAGE 1

typedef FileListIO::read_error read_error;

/******************************************************************************************************************/
/*                                              Internal File Hierarchy Storage                                   */
/******************************************************************************************************************/

template<class T> typename std::set<T>::iterator erase_from_set(typename std::set<T>& s,const typename std::set<T>::iterator& it)
{
    typename std::set<T>::iterator tmp(it);
    ++tmp;
    s.erase(it) ;
    return tmp;
}

// This class handles the file hierarchy
// A Mutex is used to ensure total coherence at this level. So only abstracted operations are allowed,
// so that the hierarchy stays completely coherent between calls.

InternalFileHierarchyStorage::InternalFileHierarchyStorage() : mRoot(0)
{
    DirEntry *de = new DirEntry("") ;

    de->row=0;
    de->parent_index=0;
    de->dir_modtime=0;
    de->dir_hash=RsFileHash() ; // null hash is root by convention.

    mNodes.push_back(de) ;
    mDirHashes[de->dir_hash] = 0 ;

    mTotalSize = 0 ;
    mTotalFiles = 0 ;
}

bool InternalFileHierarchyStorage::getDirHashFromIndex(const DirectoryStorage::EntryIndex& index,RsFileHash& hash) const
{
    if(!checkIndex(index,FileStorageNode::TYPE_DIR))
        return false ;

    hash = static_cast<DirEntry*>(mNodes[index])->dir_hash ;

    return true;
}
bool InternalFileHierarchyStorage::getIndexFromDirHash(const RsFileHash& hash,DirectoryStorage::EntryIndex& index)
{
    std::map<RsFileHash,DirectoryStorage::EntryIndex>::iterator it = mDirHashes.find(hash) ;

    if(it == mDirHashes.end())
        return false;

    index = it->second;

    // make sure the hash actually points to some existing file. If not, remove it. This is a lazy update of dir hashes: when we need them, we check them.
    if(!checkIndex(index, FileStorageNode::TYPE_DIR) || static_cast<DirEntry*>(mNodes[index])->dir_hash != hash)
    {
        std::cerr << "(II) removing non existing hash from dir hash list: " << hash << std::endl;

        mDirHashes.erase(it) ;
        return false ;
    }
    return true;
}
bool InternalFileHierarchyStorage::getIndexFromFileHash(const RsFileHash& hash,DirectoryStorage::EntryIndex& index)
{
    std::map<RsFileHash,DirectoryStorage::EntryIndex>::iterator it = mFileHashes.find(hash) ;

    if(it == mFileHashes.end())
        return false;

    index = it->second;

    // make sure the hash actually points to some existing file. If not, remove it. This is a lazy update of file hashes: when we need them, we check them.
    if(!checkIndex(it->second, FileStorageNode::TYPE_FILE) || static_cast<FileEntry*>(mNodes[index])->file_hash != hash)
    {
        std::cerr << "(II) removing non existing hash from file hash list: " << hash << std::endl;

        mFileHashes.erase(it) ;
        return false ;
    }

    return true;
}

bool InternalFileHierarchyStorage::getChildIndex(DirectoryStorage::EntryIndex e,int row,DirectoryStorage::EntryIndex& c) const
{
    if(!checkIndex(e,FileStorageNode::TYPE_DIR))
        return false ;

    const DirEntry& d = *static_cast<DirEntry*>(mNodes[e]) ;

    if((uint32_t)row < d.subdirs.size())
    {
       c = d.subdirs[row] ;
       return true ;
    }

    if((uint32_t)row < d.subdirs.size() + d.subfiles.size())
    {
       c = d.subfiles[row - (int)d.subdirs.size()] ;
       return true ;
    }
    return false;
}

int InternalFileHierarchyStorage::parentRow(DirectoryStorage::EntryIndex e)
{
    if(!checkIndex(e,FileStorageNode::TYPE_DIR | FileStorageNode::TYPE_FILE) || e==0)
        return -1 ;

    return mNodes[mNodes[e]->parent_index]->row;
}

// high level modification routines

bool InternalFileHierarchyStorage::isIndexValid(DirectoryStorage::EntryIndex e) const
{
    return e < mNodes.size() && mNodes[e] != NULL ;
}

bool InternalFileHierarchyStorage::updateSubDirectoryList(const DirectoryStorage::EntryIndex& indx, const std::set<std::string>& subdirs, const RsFileHash& random_hash_seed)
{
    if(!checkIndex(indx,FileStorageNode::TYPE_DIR))
        return false;

    DirEntry& d(*static_cast<DirEntry*>(mNodes[indx])) ;

    std::set<std::string> should_create(subdirs);

    for(uint32_t i=0;i<d.subdirs.size();)
        if(subdirs.find(static_cast<DirEntry*>(mNodes[d.subdirs[i]])->dir_name) == subdirs.end())
        {
#ifdef DEBUG_DIRECTORY_STORAGE
            std::cerr << "[directory storage] Removing subdirectory " << static_cast<DirEntry*>(mNodes[d.subdirs[i]])->dir_name << " with index " << d.subdirs[i] << std::endl;
#endif

            if( !removeDirectory(d.subdirs[i]))
                i++ ;
        }
        else
        {
#ifdef DEBUG_DIRECTORY_STORAGE
            std::cerr << "[directory storage] Keeping existing subdirectory " << static_cast<DirEntry*>(mNodes[d.subdirs[i]])->dir_name << " with index " << d.subdirs[i] << std::endl;
#endif

            should_create.erase(static_cast<DirEntry*>(mNodes[d.subdirs[i]])->dir_name) ;
            ++i;
        }

    for(std::set<std::string>::const_iterator it(should_create.begin());it!=should_create.end();++it)
    {
#ifdef DEBUG_DIRECTORY_STORAGE
        std::cerr << "[directory storage] adding new subdirectory " << it->first << " at index " << mNodes.size() << std::endl;
#endif

        DirEntry *de = new DirEntry(*it) ;

        de->row = mNodes.size();
        de->parent_index = indx;
        de->dir_modtime = 0;// forces parsing.it->second;
		de->dir_parent_path = RsDirUtil::makePath(d.dir_parent_path, d.dir_name) ;
        de->dir_hash = createDirHash(de->dir_name,d.dir_hash,random_hash_seed) ;

        mDirHashes[de->dir_hash] = mNodes.size() ;

        d.subdirs.push_back(mNodes.size()) ;
        mNodes.push_back(de) ;
    }

    return true;
}

RsFileHash InternalFileHierarchyStorage::createDirHash(const std::string& dir_name, const RsFileHash& dir_parent_hash, const RsFileHash& random_hash_salt)
{
    // What we need here: a unique identifier
    // - that cannot be bruteforced to find the real directory name and path
    // - that is used by friends to refer to a specific directory.

    // Option 1: compute H(some_secret_salt + dir_name + dir_parent_path)
    // and keep the same salt so that we can re-create the hash
    //
    // Option 2: compute H(virtual_path). That only works at the level of LocalDirectoryStorage
    //
    // Option 3: just compute something random, but then we need to store it so as to not
    // confuse friends when restarting.

    RsTemporaryMemory mem(dir_name.size() + 2*RsFileHash::SIZE_IN_BYTES) ;

    memcpy( mem,                             random_hash_salt.toByteArray(),RsFileHash::SIZE_IN_BYTES) ;
    memcpy(&mem[  RsFileHash::SIZE_IN_BYTES], dir_parent_hash.toByteArray(),RsFileHash::SIZE_IN_BYTES) ;
    memcpy(&mem[2*RsFileHash::SIZE_IN_BYTES],dir_name.c_str(),              dir_name.size()) ;

    RsFileHash res = RsDirUtil::sha1sum( mem,mem.size() ) ;

#ifdef DEBUG_DIRECTORY_STORAGE
    std::cerr << "Creating new dir hash for dir " << dir_name << ", parent dir hash=" << dir_parent_hash << " seed=[hidden]" << " result is " << res << std::endl;
#endif

    return res ;
}

bool InternalFileHierarchyStorage::removeDirectory(DirectoryStorage::EntryIndex indx)	// no reference here! Very important. Otherwise, the messign we do inside can change the value of indx!!
{
    // check that it's a directory

    if(!checkIndex(indx,FileStorageNode::TYPE_DIR))
        return false;

    if(indx == 0)
        return nodeAccessError("checkIndex(): Cannot remove top level directory") ;

#ifdef DEBUG_DIRECTORY_STORAGE
    std::cerr << "(--) Removing directory " << indx << std::endl;
    print();
#endif
    // remove from parent

    DirEntry& d(*static_cast<DirEntry*>(mNodes[indx])) ;
    DirEntry& parent_dir(*static_cast<DirEntry*>(mNodes[d.parent_index]));

    for(uint32_t i=0;i<parent_dir.subdirs.size();++i)
        if(parent_dir.subdirs[i] == indx)
        {
            parent_dir.subdirs[i] = parent_dir.subdirs.back() ;
            parent_dir.subdirs.pop_back();

            recursRemoveDirectory(indx) ;
#ifdef DEBUG_DIRECTORY_STORAGE
            print();
            std::string err ;
            if(!check(err))
                std::cerr << "(EE) Error after removing subdirectory. Error string=\"" << err << "\", Hierarchy is : " << std::endl;
#endif
            return true ;
        }
    return nodeAccessError("removeDirectory(): inconsistency!!") ;
}

bool InternalFileHierarchyStorage::checkIndex(DirectoryStorage::EntryIndex indx,uint8_t type) const
{
    if(mNodes.empty() || indx==DirectoryStorage::NO_INDEX || indx >= mNodes.size() || mNodes[indx] == NULL)
        return nodeAccessError("checkIndex(): Node does not exist") ;

    if(! (mNodes[indx]->type() & type))
        return nodeAccessError("checkIndex(): Node is of wrong type") ;

    return true;
}

bool InternalFileHierarchyStorage::updateSubFilesList(const DirectoryStorage::EntryIndex& indx,const std::map<std::string,DirectoryStorage::FileTS>& subfiles,std::map<std::string,DirectoryStorage::FileTS>& new_files)
{
    if(!checkIndex(indx,FileStorageNode::TYPE_DIR))
        return false;

    DirEntry& d(*static_cast<DirEntry*>(mNodes[indx])) ;
    new_files = subfiles ;

    // remove from new_files the ones that already exist and have a modf time that is not older.

    for(uint32_t i=0;i<d.subfiles.size();)
    {
        FileEntry& f(*static_cast<FileEntry*>(mNodes[d.subfiles[i]])) ;
        std::map<std::string,DirectoryStorage::FileTS>::const_iterator it = subfiles.find(f.file_name) ;

        if(it == subfiles.end())				// file does not exist anymore => delete
        {
#ifdef DEBUG_DIRECTORY_STORAGE
            std::cerr << "[directory storage] removing non existing file " << f.file_name << " at index " << d.subfiles[i] << std::endl;
#endif

            deleteFileNode(d.subfiles[i]) ;

            d.subfiles[i] = d.subfiles[d.subfiles.size()-1] ;
            d.subfiles.pop_back();
            continue;
        }

        if(it->second.modtime != f.file_modtime || it->second.size != f.file_size)	// file is newer and/or has different size
        {
            f.file_hash.clear();																// hash needs recomputing
            f.file_modtime = it->second.modtime;
            f.file_size = it->second.size;

            mTotalSize -= f.file_size ;
            mTotalSize += it->second.size ;
        }
        new_files.erase(f.file_name) ;

        ++i;
    }

    for(std::map<std::string,DirectoryStorage::FileTS>::const_iterator it(new_files.begin());it!=new_files.end();++it)
    {
#ifdef DEBUG_DIRECTORY_STORAGE
        std::cerr << "[directory storage] adding new file " << it->first << " at index " << mNodes.size() << std::endl;
#endif

        d.subfiles.push_back(mNodes.size()) ;
        mNodes.push_back(new FileEntry(it->first,it->second.size,it->second.modtime));
        mNodes.back()->row = mNodes.size()-1;
        mNodes.back()->parent_index = indx;

        mTotalSize  += it->second.size;
        mTotalFiles += 1;
    }
    return true;
}
bool InternalFileHierarchyStorage::updateHash(const DirectoryStorage::EntryIndex& file_index,const RsFileHash& hash)
{
    if(!checkIndex(file_index,FileStorageNode::TYPE_FILE))
    {
        std::cerr << "[directory storage] (EE) cannot update file at index " << file_index << ". Not a valid index, or not a file." << std::endl;
        return false;
    }
#ifdef DEBUG_DIRECTORY_STORAGE
    std::cerr << "[directory storage] updating hash at index " << file_index << ", hash=" << hash << std::endl;
#endif

    RsFileHash& old_hash (static_cast<FileEntry*>(mNodes[file_index])->file_hash) ;
    mFileHashes[hash] = file_index ;

    old_hash = hash ;

    return true;
}
bool InternalFileHierarchyStorage::updateFile(const DirectoryStorage::EntryIndex& file_index,const RsFileHash& hash, const std::string& fname,uint64_t size, const rstime_t modf_time)
{
    if(!checkIndex(file_index,FileStorageNode::TYPE_FILE))
    {
        std::cerr << "[directory storage] (EE) cannot update file at index " << file_index << ". Not a valid index, or not a file." << std::endl;
        return false;
    }

    FileEntry& fe(*static_cast<FileEntry*>(mNodes[file_index])) ;

#ifdef DEBUG_DIRECTORY_STORAGE
    std::cerr << "[directory storage] updating file entry at index " << file_index << ", name=" << fe.file_name << " size=" << fe.file_size << ", hash=" << fe.file_hash << std::endl;
#endif
    if(mTotalSize >= fe.file_size)
		mTotalSize -= fe.file_size;

	mTotalSize += size ;

    fe.file_hash = hash;
    fe.file_size = size;
    fe.file_modtime = modf_time;
    fe.file_name = fname;

    if(!hash.isNull())
        mFileHashes[hash] = file_index ;

    return true;
}

void InternalFileHierarchyStorage::deleteFileNode(uint32_t index)
{
	if(mNodes[index] != NULL)
	{
		FileEntry& fe(*static_cast<FileEntry*>(mNodes[index])) ;

#ifdef RS_DEEP_FILES_INDEX
		DeepFilesIndex tfi(DeepFilesIndex::dbDefaultPath());
		tfi.removeFileFromIndex(fe.file_hash);
#endif

        if(mTotalSize >= fe.file_size)
			mTotalSize -= fe.file_size ;

        if(mTotalFiles > 0)
			mTotalFiles -= 1;

		delete mNodes[index] ;
		mFreeNodes.push_back(index) ;
		mNodes[index] = NULL ;
	}
}
void InternalFileHierarchyStorage::deleteNode(uint32_t index)
{
    if(mNodes[index] != NULL)
    {
        delete mNodes[index] ;
        mFreeNodes.push_back(index) ;
        mNodes[index] = NULL ;
    }
}

DirectoryStorage::EntryIndex InternalFileHierarchyStorage::allocateNewIndex()
{
    while(!mFreeNodes.empty())
    {
        uint32_t index = mFreeNodes.front();
        mFreeNodes.pop_front();

        if(mNodes[index] == NULL)
            return DirectoryStorage::EntryIndex(index) ;
    }

	mNodes.push_back(NULL) ;
	return mNodes.size()-1 ;
}

bool InternalFileHierarchyStorage::updateDirEntry(const DirectoryStorage::EntryIndex& indx,const std::string& dir_name,rstime_t most_recent_time,rstime_t dir_modtime,const std::vector<RsFileHash>& subdirs_hash,const std::vector<FileEntry>& subfiles_array)
{
    if(!checkIndex(indx,FileStorageNode::TYPE_DIR))
    {
        std::cerr << "[directory storage] (EE) cannot update dir at index " << indx << ". Not a valid index, or not an existing dir." << std::endl;
        return false;
    }
    DirEntry& d(*static_cast<DirEntry*>(mNodes[indx])) ;

#ifdef DEBUG_DIRECTORY_STORAGE
    std::cerr << "Updating dir entry: name=\"" << dir_name << "\", most_recent_time=" << most_recent_time << ", modtime=" << dir_modtime << std::endl;
#endif

    d.dir_most_recent_time = most_recent_time;
    d.dir_modtime      = dir_modtime;
    d.dir_update_time  = time(NULL);
    d.dir_name         = dir_name;

    std::map<RsFileHash,DirectoryStorage::EntryIndex> existing_subdirs ;

    for(uint32_t i=0;i<d.subdirs.size();++i)
        existing_subdirs[static_cast<DirEntry*>(mNodes[d.subdirs[i]])->dir_hash] = d.subdirs[i] ;

    d.subdirs.clear();

    // check that all subdirs already exist. If not, create.
    for(uint32_t i=0;i<subdirs_hash.size();++i)
    {
#ifdef DEBUG_DIRECTORY_STORAGE
        std::cerr << "  subdir hash = " << subdirs_hash[i] << ": " ;
#endif

        std::map<RsFileHash,DirectoryStorage::EntryIndex>::iterator it = existing_subdirs.find(subdirs_hash[i]) ;
        DirectoryStorage::EntryIndex dir_index = 0;

        if(it != existing_subdirs.end() && mNodes[it->second] != NULL && mNodes[it->second]->type() == FileStorageNode::TYPE_DIR)
        {
            dir_index = it->second ;

#ifdef DEBUG_DIRECTORY_STORAGE
            std::cerr << " already exists, at index  " << dir_index << std::endl;
#endif

            existing_subdirs.erase(it) ;
        }
        else
        {
            dir_index = allocateNewIndex() ;

            DirEntry *de = new DirEntry("") ;

            mNodes[dir_index] = de ;

			de->dir_parent_path = RsDirUtil::makePath(d.dir_parent_path, dir_name) ;
            de->dir_hash        = subdirs_hash[i];

            mDirHashes[subdirs_hash[i]] = dir_index ;

#ifdef DEBUG_DIRECTORY_STORAGE
            std::cerr << " created, at new index " << dir_index << std::endl;
#endif
        }

        d.subdirs.push_back(dir_index) ;
        mDirHashes[subdirs_hash[i]] = dir_index ;
    }
    // remove subdirs that do not exist anymore

    for(std::map<RsFileHash,DirectoryStorage::EntryIndex>::const_iterator it = existing_subdirs.begin();it!=existing_subdirs.end();++it)
    {
#ifdef DEBUG_DIRECTORY_STORAGE
        std::cerr << "  removing existing subfile that is not in the dirctory anymore: name=" << it->first << " index=" << it->second << std::endl;
#endif

        if(!checkIndex(it->second,FileStorageNode::TYPE_DIR))
        {
            std::cerr << "(EE) Cannot delete node of index " << it->second << " because it is not a file. Inconsistency error!" << std::endl;
            continue ;
        }
        recursRemoveDirectory(it->second) ;
    }

    // now update subfiles. This is more stricky because we need to not suppress hash duplicates

    std::map<std::string,DirectoryStorage::EntryIndex> existing_subfiles ;

    for(uint32_t i=0;i<d.subfiles.size();++i)
        existing_subfiles[static_cast<FileEntry*>(mNodes[d.subfiles[i]])->file_name] = d.subfiles[i] ;

    d.subfiles.clear();

    for(uint32_t i=0;i<subfiles_array.size();++i)
    {
        std::map<std::string,DirectoryStorage::EntryIndex>::iterator it = existing_subfiles.find(subfiles_array[i].file_name) ;
        const FileEntry& f(subfiles_array[i]) ;
        DirectoryStorage::EntryIndex file_index ;

#ifdef DEBUG_DIRECTORY_STORAGE
        std::cerr << "  subfile name = " << subfiles_array[i].file_name << ": " ;
#endif

        if(it != existing_subfiles.end() && mNodes[it->second] != NULL && mNodes[it->second]->type() == FileStorageNode::TYPE_FILE)
        {
            file_index = it->second ;

#ifdef DEBUG_DIRECTORY_STORAGE
            std::cerr << " already exists, at index  " << file_index << std::endl;
#endif

            if(!updateFile(file_index,f.file_hash,f.file_name,f.file_size,f.file_modtime))
                std::cerr << "(EE) Cannot update file with index " << it->second <<" and hash " << f.file_hash << ". This is very weird. Entry should have just been created and therefore should exist. Skipping." << std::endl;

            existing_subfiles.erase(it) ;
        }
        else
        {
            file_index = allocateNewIndex() ;

            mNodes[file_index] = new FileEntry(f.file_name,f.file_size,f.file_modtime,f.file_hash) ;
            mFileHashes[f.file_hash] = file_index ;
            mTotalSize += f.file_size ;
            mTotalFiles++;

#ifdef DEBUG_DIRECTORY_STORAGE
            std::cerr << " created, at new index " << file_index << std::endl;
#endif
        }

        d.subfiles.push_back(file_index) ;
    }
    // remove subfiles that do not exist anymore

    for(std::map<std::string,DirectoryStorage::EntryIndex>::const_iterator it = existing_subfiles.begin();it!=existing_subfiles.end();++it)
    {
#ifdef DEBUG_DIRECTORY_STORAGE
        std::cerr << "  removing existing subfile that is not in the dirctory anymore: name=" << it->first << " index=" << it->second << std::endl;
#endif

        if(!checkIndex(it->second,FileStorageNode::TYPE_FILE))
        {
            std::cerr << "(EE) Cannot delete node of index " << it->second << " because it is not a file. Inconsistency error!" << std::endl;
            continue ;
        }
        deleteFileNode(it->second) ;
    }

    // now update row and parent index for all subnodes

    uint32_t n=0;
    for(uint32_t i=0;i<d.subdirs.size();++i)
    {
        static_cast<DirEntry*>(mNodes[d.subdirs[i]])->dir_update_time = 0 ;	// force the update of the subdir.

        mNodes[d.subdirs[i]]->parent_index = indx ;
        mNodes[d.subdirs[i]]->row = n++ ;
    }
    for(uint32_t i=0;i<d.subfiles.size();++i)
    {
        mNodes[d.subfiles[i]]->parent_index = indx ;
        mNodes[d.subfiles[i]]->row = n++ ;
    }


    return true;
}

void InternalFileHierarchyStorage::getStatistics(SharedDirStats& stats) const
{
    stats.total_number_of_files = mTotalFiles ;
    stats.total_shared_size = mTotalSize ;
}

bool InternalFileHierarchyStorage::getTS(const DirectoryStorage::EntryIndex& index,rstime_t& TS,rstime_t DirEntry::* m) const
{
    if(!checkIndex(index,FileStorageNode::TYPE_DIR))
    {
        std::cerr << "[directory storage] (EE) cannot get TS for index " << index << ". Not a valid index or not a directory." << std::endl;
        return false;
    }

    DirEntry& d(*static_cast<DirEntry*>(mNodes[index])) ;

    TS = d.*m ;

    return true;
}

bool InternalFileHierarchyStorage::setTS(const DirectoryStorage::EntryIndex& index,rstime_t& TS,rstime_t DirEntry::* m)
{
    if(!checkIndex(index,FileStorageNode::TYPE_DIR))
    {
        std::cerr << "[directory storage] (EE) cannot get TS for index " << index << ". Not a valid index or not a directory." << std::endl;
        return false;
    }

    DirEntry& d(*static_cast<DirEntry*>(mNodes[index])) ;

    d.*m = TS;

    return true;
}

// Do a complete recursive sweep of directory hierarchy and update cumulative size of directories

uint64_t InternalFileHierarchyStorage::recursUpdateCumulatedSize(const DirectoryStorage::EntryIndex& dir_index)
{
    DirEntry& d(*static_cast<DirEntry*>(mNodes[dir_index])) ;

    uint64_t local_cumulative_size = 0;

    for(uint32_t i=0;i<d.subfiles.size();++i)
        if(mNodes[d.subfiles[i]])		// normally not needed, but an extra-security
            local_cumulative_size += static_cast<FileEntry*>(mNodes[d.subfiles[i]])->file_size;

    for(uint32_t i=0;i<d.subdirs.size();++i)
        local_cumulative_size += recursUpdateCumulatedSize(d.subdirs[i]);

    d.dir_cumulated_size = local_cumulative_size;
    return local_cumulative_size;
}
// Do a complete recursive sweep over sub-directories and files, and update the lst modf TS. This could be also performed by a cleanup method.

rstime_t InternalFileHierarchyStorage::recursUpdateLastModfTime(const DirectoryStorage::EntryIndex& dir_index,bool& unfinished_files_present)
{
    DirEntry& d(*static_cast<DirEntry*>(mNodes[dir_index])) ;

    rstime_t largest_modf_time = d.dir_modtime ;
    unfinished_files_present = false ;

    for(uint32_t i=0;i<d.subfiles.size();++i)
    {
        FileEntry *f = static_cast<FileEntry*>(mNodes[d.subfiles[i]]) ;

        if(!f->file_hash.isNull())
            largest_modf_time = std::max(largest_modf_time, f->file_modtime) ;	// only account for hashed files, since we never send unhashed files to friends.
        else
            unfinished_files_present = true ;
    }

    for(uint32_t i=0;i<d.subdirs.size();++i)
    {
        bool unfinished_files_below = false ;
        largest_modf_time = std::max(largest_modf_time,recursUpdateLastModfTime(d.subdirs[i],unfinished_files_below)) ;

        unfinished_files_present = unfinished_files_present || unfinished_files_below ;
    }

    // now if some files are not hashed in this directory, reduce the recurs time by 1, so that the TS wil be updated when all hashes are ready.

    if(unfinished_files_present && largest_modf_time > 0)
        largest_modf_time-- ;

    d.dir_most_recent_time = largest_modf_time ;

    return largest_modf_time ;
}

// Low level stuff. Should normally not be used externally.

const InternalFileHierarchyStorage::FileStorageNode *InternalFileHierarchyStorage::getNode(DirectoryStorage::EntryIndex indx) const
{
    if(checkIndex(indx,FileStorageNode::TYPE_FILE | FileStorageNode::TYPE_DIR))
        return mNodes[indx] ;
    else
        return NULL ;
}

const InternalFileHierarchyStorage::DirEntry *InternalFileHierarchyStorage::getDirEntry(DirectoryStorage::EntryIndex indx) const
{
    if(!checkIndex(indx,FileStorageNode::TYPE_DIR))
        return NULL ;

    return static_cast<DirEntry*>(mNodes[indx]) ;
}
const InternalFileHierarchyStorage::FileEntry *InternalFileHierarchyStorage::getFileEntry(DirectoryStorage::EntryIndex indx) const
{
    if(!checkIndex(indx,FileStorageNode::TYPE_FILE))
        return NULL ;

    return static_cast<FileEntry*>(mNodes[indx]) ;
}
uint32_t InternalFileHierarchyStorage::getType(DirectoryStorage::EntryIndex indx) const
{
    if(checkIndex(indx,FileStorageNode::TYPE_FILE | FileStorageNode::TYPE_DIR))
        return mNodes[indx]->type() ;
    else
        return FileStorageNode::TYPE_UNKNOWN;
}

DirectoryStorage::EntryIndex InternalFileHierarchyStorage::getSubFileIndex(DirectoryStorage::EntryIndex parent_index,uint32_t file_tab_index)
{
    if(!checkIndex(parent_index,FileStorageNode::TYPE_DIR))
        return DirectoryStorage::NO_INDEX;

    if(static_cast<DirEntry*>(mNodes[parent_index])->subfiles.size() <= file_tab_index)
        return DirectoryStorage::NO_INDEX;

    return static_cast<DirEntry*>(mNodes[parent_index])->subfiles[file_tab_index];
}
DirectoryStorage::EntryIndex InternalFileHierarchyStorage::getSubDirIndex(DirectoryStorage::EntryIndex parent_index,uint32_t dir_tab_index)
{
    if(!checkIndex(parent_index,FileStorageNode::TYPE_DIR))
        return DirectoryStorage::NO_INDEX;

    if(static_cast<DirEntry*>(mNodes[parent_index])->subdirs.size() <= dir_tab_index)
        return DirectoryStorage::NO_INDEX;

    return static_cast<DirEntry*>(mNodes[parent_index])->subdirs[dir_tab_index];
}

bool InternalFileHierarchyStorage::searchHash(const RsFileHash& hash,DirectoryStorage::EntryIndex& result)
{
    return getIndexFromFileHash(hash,result);
}

class DirectoryStorageExprFileEntry: public RsRegularExpression::ExpFileEntry
{
public:
    DirectoryStorageExprFileEntry(const InternalFileHierarchyStorage::FileEntry& fe,const InternalFileHierarchyStorage::DirEntry& parent) : mFe(fe),mDe(parent) {}

    inline virtual const std::string& file_name()       const { return mFe.file_name ; }
    inline virtual uint64_t           file_size()       const { return mFe.file_size ; }
    inline virtual const RsFileHash&  file_hash()       const { return mFe.file_hash ; }
    inline virtual rstime_t             file_modtime()    const { return mFe.file_modtime ; }
	inline virtual std::string        file_parent_path()const { return RsDirUtil::makePath(mDe.dir_parent_path, mDe.dir_name) ; }
    inline virtual uint32_t           file_popularity() const { NOT_IMPLEMENTED() ; return 0; }

private:
    const InternalFileHierarchyStorage::FileEntry& mFe ;
    const InternalFileHierarchyStorage::DirEntry& mDe ;
};

int InternalFileHierarchyStorage::searchBoolExp(RsRegularExpression::Expression * exp, std::list<DirectoryStorage::EntryIndex> &results) const
{
    for(std::map<RsFileHash,DirectoryStorage::EntryIndex>::const_iterator it(mFileHashes.begin());it!=mFileHashes.end();++it)
        if(mNodes[it->second] != NULL && exp->eval(
                    DirectoryStorageExprFileEntry(*static_cast<const FileEntry*>(mNodes[it->second]),
                                                  *static_cast<const DirEntry*>(mNodes[mNodes[it->second]->parent_index])
                                                  )))
            results.push_back(it->second);

    return 0;
}

int InternalFileHierarchyStorage::searchTerms(
        const std::list<std::string>& terms,
        std::list<DirectoryStorage::EntryIndex>& results ) const
{
    // most entries are likely to be files, so we could do a linear search over the entries tab.
    // instead we go through the table of hashes.

    for(std::map<RsFileHash,DirectoryStorage::EntryIndex>::const_iterator it(mFileHashes.begin());it!=mFileHashes.end();++it)
        if(mNodes[it->second] != NULL)
        {
            const std::string &str1 = static_cast<FileEntry*>(mNodes[it->second])->file_name;

            for(std::list<std::string>::const_iterator iter(terms.begin()); iter != terms.end(); ++iter)
            {
                /* always ignore case */
                const std::string &str2 = (*iter);

                if(str1.end() != std::search( str1.begin(), str1.end(), str2.begin(), str2.end(), RsRegularExpression::CompareCharIC() ))
                {
                    results.push_back(it->second);
                    break;
                }
            }
        }
    return 0 ;
}

bool InternalFileHierarchyStorage::check(std::string& error_string) // checks consistency of storage.
{
    // recurs go through all entries, check that all

    error_string = "";
    bool bDirOut = false;
    bool bDirDouble = false;
    bool bFileOut = false;
    bool bFileDouble = false;
    bool bOrphean = false;

    std::vector<uint32_t> hits(mNodes.size(),0) ;	// count hits of children. Should be 1 for all in the end. Otherwise there's an error.
    hits[0] = 1 ;	// because 0 is never the child of anyone

    mFreeNodes.clear();

    for(uint32_t i=0;i<mNodes.size();++i)
        if(mNodes[i] != NULL && mNodes[i]->type() == FileStorageNode::TYPE_DIR)
        {
            // stamp the kids
            DirEntry& de = *static_cast<DirEntry*>(mNodes[i]) ;

            for(uint32_t j=0;j<de.subdirs.size();)
            {
                if(de.subdirs[j] >= mNodes.size())
                {
                    if(!bDirOut){ error_string += " - Node child dir out of tab!"; bDirOut = true;}
                    de.subdirs[j] = de.subdirs.back() ;
                    de.subdirs.pop_back();
                }
                else if(hits[de.subdirs[j]] != 0)
                {
                    if(!bDirDouble){ error_string += " - Double hit on a single node dir."; bDirDouble = true;}
                    de.subdirs[j] = de.subdirs.back() ;
                    de.subdirs.pop_back();
                }
                else
                {
                    hits[de.subdirs[j]] = 1;
                    ++j ;
                }
            }
            for(uint32_t j=0;j<de.subfiles.size();)
            {
                if(de.subfiles[j] >= mNodes.size())
                {
                    if(!bFileOut){ error_string += " - Node child file out of tab!"; bFileOut = true;}
                    de.subfiles[j] = de.subfiles.back() ;
                    de.subfiles.pop_back();
                }
                else if(hits[de.subfiles[j]] != 0)
                {
                    if(!bFileDouble){ error_string += " - Double hit on a single node file."; bFileDouble = true;}
                    de.subfiles[j] = de.subfiles.back() ;
                    de.subfiles.pop_back();
                }
                else
                {
                    hits[de.subfiles[j]] = 1;
                    ++j ;
                }
            }
        }
        else if( mNodes[i] == NULL )
            mFreeNodes.push_back(i) ;

    for(uint32_t i=0;i<hits.size();++i)
        if(hits[i] == 0 && mNodes[i] != NULL)
        {
            if(!bOrphean){ error_string += " - Orphean node!"; bOrphean = true;}

            deleteNode(i) ;	// we don't care if it's a file or a dir.
        }

    return error_string.empty();;
}

void InternalFileHierarchyStorage::print() const
{
    int nfiles = 0 ;
    int ndirs = 0 ;
    int nempty = 0 ;
    int nunknown = 0;

    for(uint32_t i=0;i<mNodes.size();++i)
        if(mNodes[i] == NULL)
        {
            //std::cerr << "  Node " << i << ": empty " << std::endl;
            ++nempty ;
        }
        else if(mNodes[i]->type() == FileStorageNode::TYPE_DIR)
        {
            std::cerr << "  Node " << i << ": type=" << mNodes[i]->type() << std::endl;
            ++ndirs;
        }
        else if(mNodes[i]->type() == FileStorageNode::TYPE_FILE)
        {
            std::cerr << "  Node " << i << ": type=" << mNodes[i]->type() << std::endl;
            ++nfiles;
        }
        else
        {
            ++nunknown;
            std::cerr << "(EE) Error: unknown type node found!" << std::endl;
        }

    std::cerr << "Total nodes: " << mNodes.size() << " (" << nfiles << " files, " << ndirs << " dirs, " << nempty << " empty slots";
    if (nunknown > 0) std::cerr << ", " << nunknown << " unknown";
    std::cerr << ")" << std::endl;


    recursPrint(0,DirectoryStorage::EntryIndex(0));

    std::cerr << "Known dir hashes: " << std::endl;
    for(std::map<RsFileHash,DirectoryStorage::EntryIndex>::const_iterator  it(mDirHashes.begin());it!=mDirHashes.end();++it)
        std::cerr << "  " << it->first << " at index " << it->second << std::endl;

    std::cerr << "Known file hashes: " << std::endl;
    for(std::map<RsFileHash,DirectoryStorage::EntryIndex>::const_iterator  it(mFileHashes.begin());it!=mFileHashes.end();++it)
        std::cerr << "  " << it->first << " at index " << it->second << std::endl;
}
void InternalFileHierarchyStorage::recursPrint(int depth,DirectoryStorage::EntryIndex node) const
{
    std::string indent(2*depth,' ');

    if(mNodes[node] == NULL)
    {
        std::cerr << "EMPTY NODE !!" << std::endl;
        return ;
    }
    DirEntry& d(*static_cast<DirEntry*>(mNodes[node]));

    std::cerr << indent << "dir hash=" << d.dir_hash << ". name:" << d.dir_name << ", parent_path:" << d.dir_parent_path << ", modf time: " << d.dir_modtime << ", recurs_last_modf_time: " << d.dir_most_recent_time << ", parent: " << d.parent_index << ", row: " << d.row << ", subdirs: " ;

    for(uint32_t i=0;i<d.subdirs.size();++i)
        std::cerr << d.subdirs[i] << " " ;
    std::cerr << std::endl;

    for(uint32_t i=0;i<d.subdirs.size();++i)
        recursPrint(depth+1,d.subdirs[i]) ;

    for(uint32_t i=0;i<d.subfiles.size();++i)
        if(mNodes[d.subfiles[i]] != NULL)
        {
            FileEntry& f(*static_cast<FileEntry*>(mNodes[d.subfiles[i]]));
            std::cerr << indent << "  hash:" << f.file_hash << " ts:" << (uint64_t)f.file_modtime << "  " << f.file_size << "  " << f.file_name << ", parent: " << f.parent_index << ", row: " << f.row << std::endl;
        }
}

bool InternalFileHierarchyStorage::nodeAccessError(const std::string& s)
{
    std::cerr << "(EE) InternalDirectoryStructure: ERROR: " << s << std::endl;
    return false ;
}

// Removes the given subdirectory from the parent node and all its pendign subdirs and files.

bool InternalFileHierarchyStorage::recursRemoveDirectory(DirectoryStorage::EntryIndex dir)
{
    DirEntry& d(*static_cast<DirEntry*>(mNodes[dir])) ;

    RsFileHash hash = d.dir_hash ;

    for(uint32_t i=0;i<d.subdirs.size();++i)
        recursRemoveDirectory(d.subdirs[i]);

    for(uint32_t i=0;i<d.subfiles.size();++i)
        deleteFileNode(d.subfiles[i]);

    deleteNode(dir) ;

    mDirHashes.erase(hash) ;

    return true ;
}

bool InternalFileHierarchyStorage::save(const std::string& fname)
{
    unsigned char *buffer = NULL ;
    uint32_t buffer_size = 0 ;
    uint32_t buffer_offset = 0 ;

    unsigned char *tmp_section_data = (unsigned char*)rs_malloc(FL_BASE_TMP_SECTION_SIZE) ;

    if(!tmp_section_data)
        return false;

    uint32_t tmp_section_size = FL_BASE_TMP_SECTION_SIZE ;

    try
    {
        // Write some header

        if(!FileListIO::writeField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIRECTORY_VERSION,(uint32_t) FILE_LIST_IO_LOCAL_DIRECTORY_STORAGE_VERSION_0001)) throw std::runtime_error("Write error") ;
        if(!FileListIO::writeField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t) mNodes.size())) throw std::runtime_error("Write error") ;

        // Write all file/dir entries

        for(uint32_t i=0;i<mNodes.size();++i)
            if(mNodes[i] != NULL && mNodes[i]->type() == FileStorageNode::TYPE_FILE)
            {
                const FileEntry& fe(*static_cast<const FileEntry*>(mNodes[i])) ;

                uint32_t file_section_offset = 0 ;

                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,file_section_offset,FILE_LIST_IO_TAG_PARENT_INDEX  ,(uint32_t)fe.parent_index)) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,file_section_offset,FILE_LIST_IO_TAG_ROW           ,(uint32_t)fe.row         )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,file_section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX   ,(uint32_t)i              )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,fe.file_name             )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,fe.file_size             )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,fe.file_hash             )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,file_section_offset,FILE_LIST_IO_TAG_MODIF_TS      ,(uint32_t)fe.file_modtime)) throw std::runtime_error("Write error") ;

                if(!FileListIO::writeField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_FILE_ENTRY,tmp_section_data,file_section_offset)) throw std::runtime_error("Write error") ;
            }
            else if(mNodes[i] != NULL && mNodes[i]->type() == FileStorageNode::TYPE_DIR)
            {
                const DirEntry& de(*static_cast<const DirEntry*>(mNodes[i])) ;

                uint32_t dir_section_offset = 0 ;

                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_PARENT_INDEX   ,(uint32_t)de.parent_index      )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_ROW            ,(uint32_t)de.row               )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX    ,(uint32_t)i                    )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_FILE_NAME      ,de.dir_name                    )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_DIR_HASH       ,de.dir_hash                    )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_FILE_SIZE      ,de.dir_parent_path             )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_MODIF_TS       ,(uint32_t)de.dir_modtime       )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_UPDATE_TS      ,(uint32_t)de.dir_update_time   )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_RECURS_MODIF_TS,(uint32_t)de.dir_most_recent_time  )) throw std::runtime_error("Write error") ;

                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)de.subdirs.size())) throw std::runtime_error("Write error") ;

                for(uint32_t j=0;j<de.subdirs.size();++j)
                    if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)de.subdirs[j])) throw std::runtime_error("Write error") ;

                if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)de.subfiles.size())) throw std::runtime_error("Write error") ;

                for(uint32_t j=0;j<de.subfiles.size();++j)
                    if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)de.subfiles[j])) throw std::runtime_error("Write error") ;

                if(!FileListIO::writeField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIR_ENTRY,tmp_section_data,dir_section_offset)) throw std::runtime_error("Write error") ;
            }

        bool res = FileListIO::saveEncryptedDataToFile(fname,buffer,buffer_offset) ;

        free(buffer) ;
        free(tmp_section_data) ;

        return res ;
    }
    catch(std::exception& e)
    {
        std::cerr << "Error while writing: " << e.what() << std::endl;

        if(buffer != NULL)
            free(buffer) ;

        if(tmp_section_data != NULL)
			free(tmp_section_data) ;

        return false;
    }
}

bool InternalFileHierarchyStorage::load(const std::string& fname)
{
    unsigned char *buffer = NULL ;
    uint32_t buffer_size = 0 ;
    uint32_t buffer_offset = 0 ;

    mFreeNodes.clear();
    mTotalFiles = 0;
    mTotalSize = 0;

    try
    {
        if(!FileListIO::loadEncryptedDataFromFile(fname,buffer,buffer_size) )
            throw read_error("Cannot decrypt") ;

        // Read some header

        uint32_t version, n_nodes ;

        if(!FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIRECTORY_VERSION,version)) throw read_error(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIRECTORY_VERSION) ;
        if(version != (uint32_t) FILE_LIST_IO_LOCAL_DIRECTORY_STORAGE_VERSION_0001) throw std::runtime_error("Wrong version number") ;

        if(!FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_nodes)) throw read_error(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_RAW_NUMBER) ;

        // Write all file/dir entries

        for(uint32_t i=0;i<mNodes.size();++i)
            if(mNodes[i])
                delete mNodes[i] ;

        mNodes.clear();
        mNodes.resize(n_nodes,NULL) ;

        for(uint32_t i=0;i<mNodes.size() && buffer_offset < buffer_size;++i)	// only the 2nd condition really is needed. The first one ensures that the loop wont go forever.
        {
            unsigned char *node_section_data = NULL ;
            uint32_t node_section_size = 0 ;
            uint32_t node_section_offset = 0 ;
#ifdef DEBUG_DIRECTORY_STORAGE
            std::cerr << "reading node " << i << ", offset " << buffer_offset << " : " << RsUtil::BinToHex(&buffer[buffer_offset],std::min((int)buffer_size - (int)buffer_offset,100)) << "..." << std::endl;
#endif

            if(FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_FILE_ENTRY,node_section_data,node_section_size))
            {
                uint32_t node_index ;
                std::string file_name ;
                uint64_t file_size ;
                RsFileHash file_hash ;
                uint32_t file_modtime ;
                uint32_t row ;
                uint32_t parent_index ;

                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_PARENT_INDEX  ,parent_index)) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_PARENT_INDEX  ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_ROW           ,row         )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_ROW           ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX   ,node_index  )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX   ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,file_name   )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_NAME     ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,file_size   )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,file_hash   )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_MODIF_TS      ,file_modtime)) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_MODIF_TS      ) ;

                if(node_index >= mNodes.size())
                    mNodes.resize(node_index+1,NULL) ;

                FileEntry *fe = new FileEntry(file_name,file_size,file_modtime,file_hash);

                fe->parent_index = parent_index ;
                fe->row = row ;

                mNodes[node_index] = fe ;
                mFileHashes[fe->file_hash] = node_index ;

                mTotalFiles++ ;
                mTotalSize += file_size ;
            }
            else if(FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIR_ENTRY,node_section_data,node_section_size))
            {
                uint32_t node_index ;
                std::string dir_name ;
                std::string dir_parent_path ;
                RsFileHash dir_hash ;
                uint32_t dir_modtime ;
                uint32_t dir_update_time ;
                uint32_t dir_most_recent_time ;
                uint32_t row ;
                uint32_t parent_index ;

                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_PARENT_INDEX   ,parent_index         )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_PARENT_INDEX   ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_ROW            ,row                  )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_ROW            ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX    ,node_index           )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX    ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_NAME      ,dir_name             )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_NAME      ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_DIR_HASH       ,dir_hash             )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_DIR_HASH       ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SIZE      ,dir_parent_path      )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SIZE      ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_MODIF_TS       ,dir_modtime          )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_MODIF_TS       ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_UPDATE_TS      ,dir_update_time      )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_UPDATE_TS      ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RECURS_MODIF_TS,dir_most_recent_time )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RECURS_MODIF_TS) ;

                if(node_index >= mNodes.size())
                    mNodes.resize(node_index+1,NULL) ;

                DirEntry *de = new DirEntry(dir_name) ;
                de->dir_name         = dir_name ;
                de->dir_parent_path  = dir_parent_path ;
                de->dir_hash         = dir_hash ;
                de->dir_modtime      = dir_modtime ;
                de->dir_update_time  = dir_update_time ;
                de->dir_most_recent_time = dir_most_recent_time ;

                de->parent_index = parent_index ;
                de->row = row ;

                uint32_t n_subdirs = 0 ;
                uint32_t n_subfiles = 0 ;

                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_subdirs)) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER) ;

                for(uint32_t j=0;j<n_subdirs;++j)
                {
                    uint32_t di = 0 ;
                    if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,di)) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER) ;
                    de->subdirs.push_back(di) ;
                }

                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_subfiles)) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER) ;

                for(uint32_t j=0;j<n_subfiles;++j)
                {
                    uint32_t fi = 0 ;
                    if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,fi)) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER) ;
                    de->subfiles.push_back(fi) ;
                }
                mNodes[node_index] = de ;
                mDirHashes[de->dir_hash] = node_index ;
            }
            else
                throw read_error(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_FILE_ENTRY) ;

            free(node_section_data) ;
        }
        free(buffer) ;

        std::string err_str ;

        if(!check(err_str))
            std::cerr << "(EE) Error while loading file hierarchy " << fname << std::endl;

        recursUpdateCumulatedSize(mRoot);

        return true ;
    }
    catch(read_error& e)
    {
#ifdef DEBUG_DIRECTORY_STORAGE
        std::cerr << "Error while reading: " << e.what() << std::endl;
#endif

        if(buffer != NULL)
            free(buffer) ;
        return false;
    }
}


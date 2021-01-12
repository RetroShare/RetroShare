/*******************************************************************************
 * libretroshare/src/file_sharing: directory_storage.cc                        *
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
#include <set>

#include "util/rstime.h"
#include "serialiser/rstlvbinary.h"
#include "retroshare/rspeers.h"
#include "util/rsdir.h"
#include "util/rsstring.h"
#include "file_sharing_defaults.h"
#include "directory_storage.h"
#include "dir_hierarchy.h"
#include "filelist_io.h"

#ifdef RS_DEEP_FILES_INDEX
#	include "deep_search/filesindex.hpp"
#endif // def RS_DEEP_FILES_INDEX

//#define DEBUG_REMOTE_DIRECTORY_STORAGE 1

/******************************************************************************************************************/
/*                                                      Iterators                                                 */
/******************************************************************************************************************/

DirectoryStorage::DirIterator::DirIterator(DirectoryStorage *s,DirectoryStorage::EntryIndex i)
{
    mStorage = s->mFileHierarchy ;
    mParentIndex = i;
    mDirTabIndex = 0;
}

DirectoryStorage::FileIterator::FileIterator(DirectoryStorage *s,DirectoryStorage::EntryIndex i)
{
    mStorage = s->mFileHierarchy ;
    mParentIndex = i;
    mFileTabIndex = 0;
}

DirectoryStorage::DirIterator& DirectoryStorage::DirIterator::operator++()
{
    ++mDirTabIndex ;

    return *this;
}
DirectoryStorage::FileIterator& DirectoryStorage::FileIterator::operator++()
{
    ++mFileTabIndex ;

    return *this;
}
DirectoryStorage::EntryIndex DirectoryStorage::FileIterator::operator*() const { return mStorage->getSubFileIndex(mParentIndex,mFileTabIndex) ; }
DirectoryStorage::EntryIndex DirectoryStorage::DirIterator ::operator*() const { return mStorage->getSubDirIndex(mParentIndex,mDirTabIndex) ; }

DirectoryStorage::FileIterator::operator bool() const { return **this != DirectoryStorage::NO_INDEX; }
DirectoryStorage::DirIterator ::operator bool() const { return **this != DirectoryStorage::NO_INDEX; }

RsFileHash  DirectoryStorage::FileIterator::hash()     const { const InternalFileHierarchyStorage::FileEntry *f = mStorage->getFileEntry(**this) ; return f?(f->file_hash):RsFileHash(); }
uint64_t    DirectoryStorage::FileIterator::size()     const { const InternalFileHierarchyStorage::FileEntry *f = mStorage->getFileEntry(**this) ; return f?(f->file_size):0; }
std::string DirectoryStorage::FileIterator::name()     const { const InternalFileHierarchyStorage::FileEntry *f = mStorage->getFileEntry(**this) ; return f?(f->file_name):std::string(); }
rstime_t      DirectoryStorage::FileIterator::modtime()  const { const InternalFileHierarchyStorage::FileEntry *f = mStorage->getFileEntry(**this) ; return f?(f->file_modtime):0; }

std::string DirectoryStorage::DirIterator::name()      const { const InternalFileHierarchyStorage::DirEntry *d = mStorage->getDirEntry(**this) ; return d?(d->dir_name):std::string(); }

/******************************************************************************************************************/
/*                                                 Directory Storage                                              */
/******************************************************************************************************************/

DirectoryStorage::DirectoryStorage(const RsPeerId &pid,const std::string& fname)
    : mPeerId(pid), mDirStorageMtx("Directory storage "+pid.toStdString()),mLastSavedTime(0),mChanged(false),mFileName(fname)
{
	{
		RS_STACK_MUTEX(mDirStorageMtx) ;
		mFileHierarchy = new InternalFileHierarchyStorage();
	}
    load(fname) ;
}

DirectoryStorage::EntryIndex DirectoryStorage::root() const
{
    return EntryIndex(0) ;
}
int DirectoryStorage::parentRow(EntryIndex e) const
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    return mFileHierarchy->parentRow(e) ;
}
bool DirectoryStorage::getChildIndex(EntryIndex e,int row,EntryIndex& c) const
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    return mFileHierarchy->getChildIndex(e,row,c) ;
}

uint32_t DirectoryStorage::getEntryType(const EntryIndex& indx)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    switch(mFileHierarchy->getType(indx))
    {
    case InternalFileHierarchyStorage::FileStorageNode::TYPE_DIR:  return DIR_TYPE_DIR ;
    case InternalFileHierarchyStorage::FileStorageNode::TYPE_FILE: return DIR_TYPE_FILE ;
    default:
        return DIR_TYPE_UNKNOWN;
    }
}

bool DirectoryStorage::getDirectoryUpdateTime   (EntryIndex index,rstime_t& update_TS) const { RS_STACK_MUTEX(mDirStorageMtx) ; return mFileHierarchy->getTS(index,update_TS,&InternalFileHierarchyStorage::DirEntry::dir_update_time     ); }
bool DirectoryStorage::getDirectoryRecursModTime(EntryIndex index,rstime_t& rec_md_TS) const { RS_STACK_MUTEX(mDirStorageMtx) ; return mFileHierarchy->getTS(index,rec_md_TS,&InternalFileHierarchyStorage::DirEntry::dir_most_recent_time); }
bool DirectoryStorage::getDirectoryLocalModTime (EntryIndex index,rstime_t& loc_md_TS) const { RS_STACK_MUTEX(mDirStorageMtx) ; return mFileHierarchy->getTS(index,loc_md_TS,&InternalFileHierarchyStorage::DirEntry::dir_modtime         ); }

bool DirectoryStorage::setDirectoryUpdateTime   (EntryIndex index,rstime_t  update_TS) { RS_STACK_MUTEX(mDirStorageMtx) ; return mFileHierarchy->setTS(index,update_TS,&InternalFileHierarchyStorage::DirEntry::dir_update_time     ); }
bool DirectoryStorage::setDirectoryRecursModTime(EntryIndex index,rstime_t  rec_md_TS) { RS_STACK_MUTEX(mDirStorageMtx) ; return mFileHierarchy->setTS(index,rec_md_TS,&InternalFileHierarchyStorage::DirEntry::dir_most_recent_time); }
bool DirectoryStorage::setDirectoryLocalModTime (EntryIndex index,rstime_t  loc_md_TS) { RS_STACK_MUTEX(mDirStorageMtx) ; return mFileHierarchy->setTS(index,loc_md_TS,&InternalFileHierarchyStorage::DirEntry::dir_modtime         ); }

bool DirectoryStorage::updateSubDirectoryList(const EntryIndex& indx, const std::set<std::string> &subdirs, const RsFileHash& hash_salt)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    bool res = mFileHierarchy->updateSubDirectoryList(indx,subdirs,hash_salt) ;
    mChanged = true ;
    return res ;
}
bool DirectoryStorage::updateSubFilesList(const EntryIndex& indx,const std::map<std::string,FileTS>& subfiles,std::map<std::string,FileTS>& new_files)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    bool res = mFileHierarchy->updateSubFilesList(indx,subfiles,new_files) ;
    mChanged = true ;
    return res ;
}
bool DirectoryStorage::removeDirectory(const EntryIndex& indx)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    bool res = mFileHierarchy->removeDirectory(indx);
    mChanged = true ;

    return res ;
}

void DirectoryStorage::locked_check()
{
    std::string error ;
    if(!mFileHierarchy->check(error))
        std::cerr << "Check error: " << error << std::endl;
}

void DirectoryStorage::getStatistics(SharedDirStats& stats)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    mFileHierarchy->getStatistics(stats);
}

bool DirectoryStorage::load(const std::string& local_file_name)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    mChanged = false ;
    return mFileHierarchy->load(local_file_name);
}
void DirectoryStorage::save(const std::string& local_file_name)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    mFileHierarchy->save(local_file_name);
}
void DirectoryStorage::print()
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    mFileHierarchy->print();
}

int DirectoryStorage::searchTerms(
        const std::list<std::string>& terms,
        std::list<EntryIndex>& results ) const
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return mFileHierarchy->searchTerms(terms,results);
}
int DirectoryStorage::searchBoolExp(RsRegularExpression::Expression * exp, std::list<EntryIndex> &results) const
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return mFileHierarchy->searchBoolExp(exp,results);
}

bool DirectoryStorage::extractData(const EntryIndex& indx,DirDetails& d)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    d.children.clear() ;
    uint32_t type = mFileHierarchy->getType(indx) ;

    d.ref = (void*)(intptr_t)indx ;

    if (type == InternalFileHierarchyStorage::FileStorageNode::TYPE_DIR) /* has children --- fill */
    {
        const InternalFileHierarchyStorage::DirEntry *dir_entry = mFileHierarchy->getDirEntry(indx) ;

        /* extract all the entries */

        for(DirectoryStorage::DirIterator it(this,indx);it;++it)
        {
            DirStub stub;
            stub.type = DIR_TYPE_DIR;
            stub.name = it.name();
            stub.ref  = (void*)(intptr_t)*it; // this is updated by the caller, who knows which friend we're dealing with

            d.children.push_back(stub);
        }

        for(DirectoryStorage::FileIterator it(this,indx);it;++it)
        {
            DirStub stub;
            stub.type = DIR_TYPE_FILE;
            stub.name = it.name();
            stub.ref  = (void*)(intptr_t)*it;

            d.children.push_back(stub);
        }

        d.type = DIR_TYPE_DIR;
        d.hash.clear() ;
        d.size   = dir_entry->dir_cumulated_size;//dir_entry->subdirs.size() + dir_entry->subfiles.size();
        d.max_mtime = dir_entry->dir_most_recent_time ;
        d.mtime     = dir_entry->dir_modtime ;
        d.name    = dir_entry->dir_name;
		d.path    = RsDirUtil::makePath(dir_entry->dir_parent_path, dir_entry->dir_name) ;
        d.parent  = (void*)(intptr_t)dir_entry->parent_index ;

        if(indx == 0)
        {
            d.type = DIR_TYPE_PERSON ;
            d.name = mPeerId.toStdString();
        }
    }
    else if(type == InternalFileHierarchyStorage::FileStorageNode::TYPE_FILE)
    {
        const InternalFileHierarchyStorage::FileEntry *file_entry = mFileHierarchy->getFileEntry(indx) ;

        d.type    = DIR_TYPE_FILE;
        d.size   = file_entry->file_size;
        d.max_mtime = file_entry->file_modtime ;
        d.name    = file_entry->file_name;
        d.hash    = file_entry->file_hash;
        d.mtime     = file_entry->file_modtime;
        d.parent  = (void*)(intptr_t)file_entry->parent_index ;

        const InternalFileHierarchyStorage::DirEntry *parent_dir_entry = mFileHierarchy->getDirEntry(file_entry->parent_index);

        if(parent_dir_entry != NULL)
			d.path = RsDirUtil::makePath(parent_dir_entry->dir_parent_path, parent_dir_entry->dir_name) ;
        else
            d.path = "" ;
    }
    else
        return false;

    d.flags.clear() ;

    return true;
}

bool DirectoryStorage::getDirHashFromIndex(const EntryIndex& index,RsFileHash& hash) const
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return mFileHierarchy->getDirHashFromIndex(index,hash) ;
}
bool DirectoryStorage::getIndexFromDirHash(const RsFileHash& hash,EntryIndex& index) const
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return mFileHierarchy->getIndexFromDirHash(hash,index) ;
}

void DirectoryStorage::checkSave()
{
    rstime_t now = time(NULL);

    if(mChanged && mLastSavedTime + MIN_INTERVAL_BETWEEN_REMOTE_DIRECTORY_SAVE < now)
	{
        mFileHierarchy->recursUpdateCumulatedSize(mFileHierarchy->mRoot);

	   {
		  RS_STACK_MUTEX(mDirStorageMtx) ;
		  locked_check();
	   }

	   save(mFileName);

	   {
		  RS_STACK_MUTEX(mDirStorageMtx) ;
		  mLastSavedTime = now ;
		  mChanged = false ;
	   }
	}
}
/******************************************************************************************************************/
/*                                           Local Directory Storage                                              */
/******************************************************************************************************************/

LocalDirectoryStorage::LocalDirectoryStorage(const std::string& fname,const RsPeerId& own_id)
    : DirectoryStorage(own_id,fname)
{
	mTSChanged = false ;
}

RsFileHash LocalDirectoryStorage::makeEncryptedHash(const RsFileHash& hash)
{
    return RsDirUtil::sha1sum(hash.toByteArray(),hash.SIZE_IN_BYTES);
}
bool LocalDirectoryStorage::locked_findRealHash(const RsFileHash& hash, RsFileHash& real_hash) const
{
    std::map<RsFileHash,RsFileHash>::const_iterator it = mEncryptedHashes.find(hash) ;

    if(it == mEncryptedHashes.end())
        return false ;

    real_hash = it->second ;
    return true ;
}

int LocalDirectoryStorage::searchHash(const RsFileHash& hash, RsFileHash& real_hash, EntryIndex& result) const
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    if(locked_findRealHash(hash,real_hash) && mFileHierarchy->searchHash(real_hash,result))
        return true ;

    if(mFileHierarchy->searchHash(hash,result))
    {
        real_hash.clear();
        return true ;
    }
    return false ;
}

void LocalDirectoryStorage::setSharedDirectoryList(const std::list<SharedDirInfo>& lst)
{
	std::set<std::string> dirs_with_new_virtualname ;
    bool dirs_with_changed_flags = false ;

	{
		RS_STACK_MUTEX(mDirStorageMtx) ;

		// Chose virtual name if not supplied, and remove duplicates.

		std::set<std::string> virtual_names ;	// maps virtual to real name
		std::list<SharedDirInfo> processed_list ;

		for(std::list<SharedDirInfo>::const_iterator it(lst.begin());it!= lst.end();++it)
		{
			int i=0;
			std::string candidate_virtual_name = it->virtualname ;

			if(candidate_virtual_name.empty())
				candidate_virtual_name = RsDirUtil::getTopDir(it->filename);

			while(virtual_names.find(candidate_virtual_name) != virtual_names.end())
				rs_sprintf_append(candidate_virtual_name, "-%d", ++i);

			SharedDirInfo d(*it);
			d.virtualname = candidate_virtual_name ;
			processed_list.push_back(d) ;

			virtual_names.insert(candidate_virtual_name) ;
		}

		// now for each member of the processed list, check if it is an existing shared directory that has been changed. If so, we need to update the dir TS of that directory

		std::map<std::string,SharedDirInfo> new_dirs ;

		for(std::list<SharedDirInfo>::const_iterator it(processed_list.begin());it!=processed_list.end();++it)
		{
			std::map<std::string,SharedDirInfo>::iterator it2 = mLocalDirs.find(it->filename) ;

			if(it2 != mLocalDirs.end())
            {
                if(it2->second.virtualname != it->virtualname)
					dirs_with_new_virtualname.insert(it->filename) ;

				if(!SharedDirInfo::sameLists((*it).parent_groups,it2->second.parent_groups) || (*it).shareflags != it2->second.shareflags)
                    dirs_with_changed_flags = true ;
            }

			new_dirs[it->filename] = *it;
		}

		mLocalDirs = new_dirs ;
		mTSChanged = true ;
	}

    // now update the TS off-mutex.

	for(DirIterator dirit(this,root());dirit;++dirit)
		if(dirs_with_new_virtualname.find(dirit.name()) != dirs_with_new_virtualname.end())
		{
			std::cerr << "Updating TS of local dir \"" << dirit.name() << "\" with changed virtual name" << std::endl;
			setDirectoryLocalModTime(*dirit,time(NULL));
		}

    if(dirs_with_changed_flags)
        setDirectoryLocalModTime(0,time(NULL)) ;
}

void LocalDirectoryStorage::getSharedDirectoryList(std::list<SharedDirInfo>& lst)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    lst.clear();

    for(std::map<std::string,SharedDirInfo>::iterator it(mLocalDirs.begin());it!=mLocalDirs.end();++it)
        lst.push_back(it->second) ;
}
void LocalDirectoryStorage::updateShareFlags(const SharedDirInfo& info)
{
    bool changed = false ;

    {
        RS_STACK_MUTEX(mDirStorageMtx) ;

        std::map<std::string,SharedDirInfo>::iterator it = mLocalDirs.find(info.filename) ;

        if(it == mLocalDirs.end())
        {
            std::cerr << "(EE) LocalDirectoryStorage::updateShareFlags: directory \"" << info.filename << "\" not found" << std::endl;
            return ;
        }

        // we compare the new info with the old one. If the two group lists have a different order, they will be seen as different. Not a big deal. We just
        // want to make sure that if they are different, flags get updated.

        if(!SharedDirInfo::sameLists(it->second.parent_groups,info.parent_groups) || it->second.filename != info.filename || it->second.shareflags != info.shareflags || it->second.virtualname != info.virtualname)
        {
            it->second = info;

#ifdef DEBUG_LOCAL_DIRECTORY_STORAGE
            std::cerr << "Updating dir mod time because flags at level 0 have changed." << std::endl;
#endif
            changed = true ;
        }
    }

    if(changed)
    {
        setDirectoryLocalModTime(0,time(NULL)) ;
        mTSChanged = true ;
    }
}

bool LocalDirectoryStorage::convertSharedFilePath(const std::string& path, std::string& fullpath)
{
    std::string shpath  = RsDirUtil::removeRootDir(path);
    std::string basedir = RsDirUtil::getRootDir(path);
    std::string realroot ;
    {
        RS_STACK_MUTEX(mDirStorageMtx) ;
        realroot = locked_findRealRootFromVirtualFilename(basedir);
    }

    if (realroot.empty())
        return false;

    /* construct full name */
    fullpath = realroot + "/";
    fullpath += shpath;

    return true;
}

void LocalDirectoryStorage::notifyTSChanged()
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    mTSChanged = true ;
}
void LocalDirectoryStorage::updateTimeStamps()
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    if(mTSChanged)
    {
#ifdef DEBUG_LOCAL_DIRECTORY_STORAGE
        std::cerr << "Updating recursive TS for local shared dirs..." << std::endl;
#endif

        bool unfinished_files_below ;

        rstime_t last_modf_time = mFileHierarchy->recursUpdateLastModfTime(EntryIndex(0),unfinished_files_below) ;
        mTSChanged = false ;

#ifdef DEBUG_LOCAL_DIRECTORY_STORAGE
        std::cerr << "LocalDirectoryStorage: global last modf time is " << last_modf_time << " (which is " << time(NULL) - last_modf_time << " secs ago), unfinished files below=" << unfinished_files_below << std::endl;
#else
		// remove unused variable warning
		// variable is only used for debugging
		(void)last_modf_time;
#endif
    }
}

bool LocalDirectoryStorage::updateHash(
        const EntryIndex& index, const RsFileHash& hash,
        bool update_internal_hierarchy )
{
	bool ret = false;

	{
		RS_STACK_MUTEX(mDirStorageMtx);

		mEncryptedHashes[makeEncryptedHash(hash)] = hash ;
		mChanged = true ;

#ifdef DEBUG_LOCAL_DIRECTORY_STORAGE
		std::cerr << "Updating index of hash " << hash << " update_internal="
		          << update_internal_hierarchy << std::endl;
#endif

		ret = (!update_internal_hierarchy) ||
		        mFileHierarchy->updateHash(index,hash);
	} // RS_STACK_MUTEX(mDirStorageMtx);

#ifdef RS_DEEP_FILES_INDEX
	FileInfo fInfo;
	if( ret && getFileInfo(index, fInfo) &&
	        fInfo.storage_permission_flags & DIR_FLAGS_ANONYMOUS_SEARCH )
	{
		DeepFilesIndex dfi(DeepFilesIndex::dbDefaultPath());
		ret &= dfi.indexFile(fInfo.path, fInfo.fname, hash);
	}
#endif // def RS_DEEP_FILES_INDEX

	return ret;
}
std::string LocalDirectoryStorage::locked_findRealRootFromVirtualFilename(const std::string& virtual_rootdir) const
{
    /**** MUST ALREADY BE LOCKED ****/

    std::map<std::string, SharedDirInfo>::const_iterator cit = mLocalDirs.find(virtual_rootdir) ;

    if (cit == mLocalDirs.end())
    {
        std::cerr << "(EE) locked_findRealRootFromVirtualFilename() Invalid RootDir: " << virtual_rootdir << std::endl;
        return std::string();
    }
    return cit->second.filename;
}

bool LocalDirectoryStorage::extractData(const EntryIndex& indx,DirDetails& d)
{
    bool res = DirectoryStorage::extractData(indx,d) ;

    if(!res)
        return false;

    // here we should update the file sharing flags

    return getFileSharingPermissions(indx,d.flags,d.parent_groups) ;
}

bool LocalDirectoryStorage::getFileInfo(DirectoryStorage::EntryIndex i,FileInfo& info)
{
    DirDetails d;
    extractData(i,d) ;

    if(d.type != DIR_TYPE_FILE)
    {
        std::cerr << "(EE) LocalDirectoryStorage: asked for file info for index " << i << " which is not a file." << std::endl;
        return false;
    }

    info.storage_permission_flags = d.flags; 	// Combination of the four RS_DIR_FLAGS_*. Updated when the file is a local stored file.
    info.parent_groups            = d.parent_groups;
    info.transfer_info_flags      = TransferRequestFlags();		// various flags from RS_FILE_HINTS_*
    info.path                     = d.path + "/" + d.name;
    info.fname                    = d.name;
    info.hash                     = d.hash;
    info.size                     = d.size;

    // all this stuff below is not useful in this case.

    info.mId = 0; /* (GUI) Model Id -> unique number */
    info.ext.clear();
    info.avail = 0; /* how much we have */
    info.rank = 0;
    info.age = 0;
    info.queue_position =0;
    info.searchId = 0;      /* 0 if none */

    /* Transfer Stuff */
    info.transfered = 0;
    info.tfRate = 0; /* in kbytes */
    info.downloadStatus = FT_STATE_COMPLETE ;
    //std::list<TransferInfo> peers;

    info.priority  = SPEED_NORMAL;
    info.lastTS = 0;

    return true;
}

bool LocalDirectoryStorage::getFileSharingPermissions(const EntryIndex& indx,FileStorageFlags& flags,std::list<RsNodeGroupId>& parent_groups)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return locked_getFileSharingPermissions(indx,flags,parent_groups) ;
}

bool LocalDirectoryStorage::locked_getFileSharingPermissions(const EntryIndex& indx, FileStorageFlags& flags, std::list<RsNodeGroupId> &parent_groups)
{
    flags.clear() ;
    parent_groups.clear();

    std::string base_dir;

    const InternalFileHierarchyStorage::FileStorageNode *n = mFileHierarchy->getNode(indx) ;

    if(n == NULL)
        return false ;

    for(DirectoryStorage::EntryIndex i=((n->type()==InternalFileHierarchyStorage::FileStorageNode::TYPE_FILE)?((intptr_t)n->parent_index):indx);;)
    {
        const InternalFileHierarchyStorage::DirEntry *e = mFileHierarchy->getDirEntry(i) ;

        if(e == NULL)
            break ;

        if(e->parent_index == 0)
        {
            base_dir = e->dir_name ;
            break ;
        }
        i = e->parent_index ;
    }

    if(!base_dir.empty())
    {
        std::map<std::string,SharedDirInfo>::const_iterator it = mLocalDirs.find(base_dir) ;

        if(it == mLocalDirs.end())
        {
            std::cerr << "(II) base directory \"" << base_dir << "\" not found in shared dir list." << std::endl;
            return false ;
        }

        flags = it->second.shareflags;
        parent_groups = it->second.parent_groups;
    }

    return true;
}

std::string LocalDirectoryStorage::locked_getVirtualDirName(EntryIndex indx) const
{
    if(indx == 0)
        return std::string() ;

    const InternalFileHierarchyStorage::DirEntry *dir = mFileHierarchy->getDirEntry(indx);

    if(dir->parent_index != 0)
        return dir->dir_name ;

   std::map<std::string,SharedDirInfo>::const_iterator it = mLocalDirs.find(dir->dir_name) ;

   if(it == mLocalDirs.end())
   {
       std::cerr << "(EE) Cannot find real name " << dir->dir_name << " at level 1 among shared dirs. Bug?" << std::endl;
       return std::string() ;
   }

   return it->second.virtualname ;
}
std::string LocalDirectoryStorage::locked_getVirtualPath(EntryIndex indx) const
{
    if(indx == 0)
        return std::string() ;

    std::string res ;
    const InternalFileHierarchyStorage::DirEntry *dir = mFileHierarchy->getDirEntry(indx);

    while(dir->parent_index != 0)
    {
        dir = mFileHierarchy->getDirEntry(dir->parent_index) ;
        res += dir->dir_name + "/"+ res ;
    }

   std::map<std::string,SharedDirInfo>::const_iterator it = mLocalDirs.find(dir->dir_name) ;

   if(it == mLocalDirs.end())
   {
       std::cerr << "(EE) Cannot find real name " << dir->dir_name << " at level 1 among shared dirs. Bug?" << std::endl;
       return std::string() ;
   }
   return it->second.virtualname + "/" + res;
}

bool LocalDirectoryStorage::serialiseDirEntry(const EntryIndex& indx,RsTlvBinaryData& bindata,const RsPeerId& client_id)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    const InternalFileHierarchyStorage::DirEntry *dir = mFileHierarchy->getDirEntry(indx);

#ifdef DEBUG_LOCAL_DIRECTORY_STORAGE
    std::cerr << "Serialising Dir entry " << std::hex << indx << " for client id " << client_id << std::endl;
#endif
    if(dir == NULL)
    {
        std::cerr << "(EE) serialiseDirEntry: ERROR. Cannot find entry " << (void*)(intptr_t)indx << std::endl;
        return false;
    }

    // compute list of allowed subdirs
    std::vector<RsFileHash> allowed_subdirs ;
    FileStorageFlags node_flags ;
    std::list<RsNodeGroupId> node_groups ;

    // for each subdir, compute the node flags and groups, then ask rsPeers to compute the mask that result from these flags for the particular peer supplied in parameter

    for(uint32_t i=0;i<dir->subdirs.size();++i)
        if(indx != 0 || (locked_getFileSharingPermissions(dir->subdirs[i],node_flags,node_groups) && (rsPeers->computePeerPermissionFlags(client_id,node_flags,node_groups) & RS_FILE_HINTS_BROWSABLE)))
        {
            RsFileHash hash ;
            if(!mFileHierarchy->getDirHashFromIndex(dir->subdirs[i],hash))
            {
              std::cerr << "(EE) Cannot get hash from subdir index " << dir->subdirs[i] << ". Weird bug." << std::endl ;
              return false;
            }
            allowed_subdirs.push_back(hash) ;
#ifdef DEBUG_LOCAL_DIRECTORY_STORAGE
            std::cerr << "  pushing subdir " << hash << ", array position=" << i << " indx=" << dir->subdirs[i] << std::endl;
#endif
        }
#ifdef DEBUG_LOCAL_DIRECTORY_STORAGE
        else
            std::cerr << "  not pushing subdir " << hash << ", array position=" << i << " indx=" << dir->subdirs[i] << ": permission denied for this peer." << std::endl;
#endif

    // now count the files that do not have a null hash (meaning the hash has indeed been computed)

    uint32_t allowed_subfiles = 0 ;

    for(uint32_t i=0;i<dir->subfiles.size();++i)
    {
        const InternalFileHierarchyStorage::FileEntry *file = mFileHierarchy->getFileEntry(dir->subfiles[i]) ;
        if(file != NULL && !file->file_hash.isNull())
            allowed_subfiles++ ;
    }

    unsigned char *section_data = (unsigned char *)rs_malloc(FL_BASE_TMP_SECTION_SIZE) ;

    if(!section_data)
        return false ;

    uint32_t section_size = FL_BASE_TMP_SECTION_SIZE;
    uint32_t section_offset = 0;

    // we need to send:
    //	- the name of the directory, its TS
    //	- the index entry for each subdir (the updte TS are exchanged at a higher level)
    //	- the file info for each subfile
    //
    std::string virtual_dir_name = locked_getVirtualDirName(indx) ;

    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_DIR_NAME       ,virtual_dir_name                   )) { free(section_data); return false ;}
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_RECURS_MODIF_TS,(uint32_t)dir->dir_most_recent_time)) { free(section_data); return false ;}
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_MODIF_TS       ,(uint32_t)dir->dir_modtime         )) { free(section_data); return false ;}

    // serialise number of subdirs and number of subfiles

    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)allowed_subdirs.size()  )) { free(section_data); return false ;}
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)allowed_subfiles        )) { free(section_data); return false ;}

    // serialise subdirs entry indexes

    for(uint32_t i=0;i<allowed_subdirs.size();++i)
        if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX ,allowed_subdirs[i]  )) { free(section_data); return false ;}

    // serialise directory subfiles, with info for each of them

    unsigned char *file_section_data = (unsigned char *)rs_malloc(FL_BASE_TMP_SECTION_SIZE) ;

    if(!file_section_data)
    {
        free(section_data);
        return false ;
    }

    uint32_t file_section_size = FL_BASE_TMP_SECTION_SIZE ;

    for(uint32_t i=0;i<dir->subfiles.size();++i)
    {
        uint32_t file_section_offset = 0 ;

        const InternalFileHierarchyStorage::FileEntry *file = mFileHierarchy->getFileEntry(dir->subfiles[i]) ;

        if(file == NULL || file->file_hash.isNull())
        {
            std::cerr << "(II) skipping unhashed or Null file entry " << dir->subfiles[i] << " to get/send file info." << std::endl;
            continue ;
        }

        if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,file->file_name   )) { free(section_data);free(file_section_data);return false ;}
        if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,file->file_size   )) { free(section_data);free(file_section_data);return false ;}
        if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,file->file_hash   )) { free(section_data);free(file_section_data);return false ;}
        if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_MODIF_TS      ,(uint32_t)file->file_modtime)) { free(section_data);free(file_section_data);return false ;}

        // now write the whole string into a single section in the file

        if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_REMOTE_FILE_ENTRY,file_section_data,file_section_offset)) { free(section_data); free(file_section_data);return false ;}

#ifdef DEBUG_LOCAL_DIRECTORY_STORAGE
        std::cerr << "  pushing subfile " << file->hash << ", array position=" << i << " indx=" << dir->subfiles[i] << std::endl;
#endif
    }
	free(file_section_data) ;

#ifdef DEBUG_LOCAL_DIRECTORY_STORAGE
    std::cerr << "Serialised dir entry to send for entry index " << (void*)(intptr_t)indx << ". Data size is " << section_size << " bytes" << std::endl;
#endif

    bindata.bin_data = realloc(section_data,section_offset) ;	// This discards the possibly unused trailing bytes in the end of section_data
    bindata.bin_len = section_offset ;

    return true ;
}


/******************************************************************************************************************/
/*                                           Remote Directory Storage                                              */
/******************************************************************************************************************/

RemoteDirectoryStorage::RemoteDirectoryStorage(const RsPeerId& pid,const std::string& fname)
    : DirectoryStorage(pid,fname)
{
    mLastSweepTime = time(NULL) - (RSRandom::random_u32() % DELAY_BETWEEN_REMOTE_DIRECTORIES_SWEEP) ;

    std::cerr << "Loaded remote directory for peer " << pid << ", inited last sweep time to " << time(NULL) - mLastSweepTime << " secs ago." << std::endl;
#ifdef DEBUG_REMOTE_DIRECTORY_STORAGE
    mFileHierarchy->print();
#endif
}

bool RemoteDirectoryStorage::deserialiseUpdateDirEntry(const EntryIndex& indx,const RsTlvBinaryData& bindata)
{
    const unsigned char *section_data = (unsigned char*)bindata.bin_data ;
    uint32_t section_size = bindata.bin_len ;
    uint32_t section_offset=0 ;

#ifdef DEBUG_REMOTE_DIRECTORY_STORAGE
    std::cerr << "RemoteDirectoryStorage::deserialiseDirEntry(): deserialising directory content for friend " << peerId() << ", and directory " << indx << std::endl;
#endif

    std::string dir_name ;
    uint32_t most_recent_time ,dir_modtime ;

    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_DIR_NAME       ,dir_name        )) return false ;
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_RECURS_MODIF_TS,most_recent_time)) return false ;
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_MODIF_TS       ,dir_modtime     )) return false ;

#ifdef DEBUG_REMOTE_DIRECTORY_STORAGE
    std::cerr << "  dir name           : \"" << dir_name << "\"" << std::endl;
    std::cerr << "  most recent time   : " << most_recent_time << std::endl;
    std::cerr << "  modification time  : " << dir_modtime << std::endl;
#endif

    // serialise number of subdirs and number of subfiles

    uint32_t n_subdirs,n_subfiles ;

    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_subdirs  )) return false ;
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_subfiles )) return false ;

#ifdef DEBUG_REMOTE_DIRECTORY_STORAGE
    std::cerr << "  number of subdirs  : " << n_subdirs << std::endl;
    std::cerr << "  number of files    : " << n_subfiles << std::endl;
#endif

    // serialise subdirs entry indexes

    std::vector<RsFileHash> subdirs_hashes ;
    RsFileHash subdir_hash ;

    for(uint32_t i=0;i<n_subdirs;++i)
    {
        if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX ,subdir_hash)) return false ;

        subdirs_hashes.push_back(subdir_hash) ;
    }

    // deserialise directory subfiles, with info for each of them

    std::vector<InternalFileHierarchyStorage::FileEntry> subfiles_array ;

    // Pre-allocate file_section_data, so that read_field does not need to do it many times.

    unsigned char *file_section_data = (unsigned char *)rs_malloc(FL_BASE_TMP_SECTION_SIZE) ;

    if(!file_section_data)
        return false ;

    uint32_t file_section_size = FL_BASE_TMP_SECTION_SIZE ;

    for(uint32_t i=0;i<n_subfiles;++i)
    {
        // Read the full data section for the file

        if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_REMOTE_FILE_ENTRY,file_section_data,file_section_size)) { free(file_section_data); return false ; }

        uint32_t file_section_offset = 0 ;

        InternalFileHierarchyStorage::FileEntry f;
        uint32_t modtime =0;

        if(!FileListIO::readField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,f.file_name   )) { free(file_section_data); return false ; }
        if(!FileListIO::readField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,f.file_size   )) { free(file_section_data); return false ; }
        if(!FileListIO::readField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,f.file_hash   )) { free(file_section_data); return false ; }
        if(!FileListIO::readField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_MODIF_TS      ,modtime       )) { free(file_section_data); return false ; }

        f.file_modtime = modtime ;

        subfiles_array.push_back(f) ;
    }
	free(file_section_data) ;

    RS_STACK_MUTEX(mDirStorageMtx) ;
#ifdef DEBUG_REMOTE_DIRECTORY_STORAGE
    std::cerr << "  updating dir entry..." << std::endl;
#endif

    // First create the entries for each subdir and each subfile, if needed.
    if(!mFileHierarchy->updateDirEntry(indx,dir_name,most_recent_time,dir_modtime,subdirs_hashes,subfiles_array))
    {
        std::cerr << "(EE) Cannot update dir entry with index " << indx << ": entry does not exist." << std::endl;
        return false ;
    }

    mChanged = true ;

    return true ;
}

int RemoteDirectoryStorage::searchHash(const RsFileHash& hash, EntryIndex& result) const
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    return mFileHierarchy->searchHash(hash,result);
}




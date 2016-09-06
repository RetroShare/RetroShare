#include <set>
#include "serialiser/rstlvbinary.h"
#include "retroshare/rspeers.h"
#include "util/rsdir.h"
#include "util/rsstring.h"
#include "file_sharing_defaults.h"
#include "directory_storage.h"
#include "dir_hierarchy.h"
#include "filelist_io.h"

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
time_t      DirectoryStorage::FileIterator::modtime()  const { const InternalFileHierarchyStorage::FileEntry *f = mStorage->getFileEntry(**this) ; return f?(f->file_modtime):0; }

std::string DirectoryStorage::DirIterator::name()      const { const InternalFileHierarchyStorage::DirEntry *d = mStorage->getDirEntry(**this) ; return d?(d->dir_name):std::string(); }

/******************************************************************************************************************/
/*                                                 Directory Storage                                              */
/******************************************************************************************************************/

DirectoryStorage::DirectoryStorage(const RsPeerId &pid)
    : mPeerId(pid), mDirStorageMtx("Directory storage "+pid.toStdString())
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    mFileHierarchy = new InternalFileHierarchyStorage();
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
bool DirectoryStorage::getDirUpdateTS(EntryIndex index,time_t& recurs_max_modf_TS,time_t& local_update_TS)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return mFileHierarchy->getDirUpdateTS(index,recurs_max_modf_TS,local_update_TS) ;
}
bool DirectoryStorage::setDirUpdateTS(EntryIndex index,time_t  recurs_max_modf_TS,time_t  local_update_TS)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return mFileHierarchy->setDirUpdateTS(index,recurs_max_modf_TS,local_update_TS) ;
}

bool DirectoryStorage::updateSubDirectoryList(const EntryIndex& indx,const std::map<std::string,time_t>& subdirs)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    bool res = mFileHierarchy->updateSubDirectoryList(indx,subdirs) ;
    locked_check() ;
    return res ;
}
bool DirectoryStorage::updateSubFilesList(const EntryIndex& indx,const std::map<std::string,FileTS>& subfiles,std::map<std::string,FileTS>& new_files)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    bool res = mFileHierarchy->updateSubFilesList(indx,subfiles,new_files) ;
    locked_check() ;
    return res ;
}
bool DirectoryStorage::removeDirectory(const EntryIndex& indx)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    bool res = mFileHierarchy->removeDirectory(indx);

    locked_check();
    return res ;
}

void DirectoryStorage::locked_check()
{
    std::string error ;
    if(!mFileHierarchy->check(error))
        std::cerr << "Check error: " << error << std::endl;
}

bool DirectoryStorage::updateFile(const EntryIndex& index,const RsFileHash& hash,const std::string& fname, uint64_t size,time_t modf_time)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return mFileHierarchy->updateFile(index,hash,fname,size,modf_time);
}
bool DirectoryStorage::updateHash(const EntryIndex& index,const RsFileHash& hash)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return mFileHierarchy->updateHash(index,hash);
}

int DirectoryStorage::searchHash(const RsFileHash& hash, std::list<EntryIndex> &results) const
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return mFileHierarchy->searchHash(hash,results);
}

// static const uint8_t DIRECTORY_STORAGE_TAG_FILE_HASH         =  0x01 ;
// static const uint8_t DIRECTORY_STORAGE_TAG_FILE_NAME         =  0x02 ;
// static const uint8_t DIRECTORY_STORAGE_TAG_FILE_SIZE         =  0x03 ;
// static const uint8_t DIRECTORY_STORAGE_TAG_DIR_NAME          =  0x04 ;
// static const uint8_t DIRECTORY_STORAGE_TAG_MODIF_TS          =  0x05 ;
// static const uint8_t DIRECTORY_STORAGE_TAG_RECURS_MODIF_TS   =  0x06 ;

void DirectoryStorage::loadNextTag(const unsigned char *data,uint32_t& offset,uint8_t& entry_tag,uint32_t& entry_size)
{
    entry_tag = data[offset++] ;
}
void DirectoryStorage::saveNextTag(unsigned char *data, uint32_t& offset, uint8_t entry_tag, uint32_t entry_size)
{
}

void DirectoryStorage::load(const std::string& local_file_name)
{
    std::cerr << "DirectoryStorage::load()" << std::endl;

    // first load the header, than all fields.

    RS_STACK_MUTEX(mDirStorageMtx) ;
    mFileHierarchy->load(local_file_name);
}
void DirectoryStorage::save(const std::string& local_file_name)
{
    std::cerr << "DirectoryStorage::Save()" << std::endl;

    RS_STACK_MUTEX(mDirStorageMtx) ;
    mFileHierarchy->save(local_file_name);

    // first write the header, than all fields.
}
void DirectoryStorage::print()
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    std::cerr << "LocalDirectoryStorage:" << std::endl;
    mFileHierarchy->print();
}

bool LocalDirectoryStorage::getFileInfo(DirectoryStorage::EntryIndex i,FileInfo& info)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    NOT_IMPLEMENTED();
    return true;
}

bool DirectoryStorage::extractData(const EntryIndex& indx,DirDetails& d)
{
    d.children.clear() ;
    time_t now = time(NULL) ;

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
        d.count   = dir_entry->subdirs.size() + dir_entry->subfiles.size();
        d.min_age = now - dir_entry->dir_most_recent_time ;
        d.name    = dir_entry->dir_name;
        d.path    = dir_entry->dir_parent_path + "/" + dir_entry->dir_name ;
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
        d.count   = file_entry->file_size;
        d.min_age = now - file_entry->file_modtime ;
        d.name    = file_entry->file_name;
        d.hash    = file_entry->file_hash;
        d.age     = now - file_entry->file_modtime;
        d.parent  = (void*)(intptr_t)file_entry->parent_index ;

        const InternalFileHierarchyStorage::DirEntry *parent_dir_entry = mFileHierarchy->getDirEntry(file_entry->parent_index);

        if(parent_dir_entry != NULL)
            d.path = parent_dir_entry->dir_parent_path + "/" + parent_dir_entry->dir_name + "/" ;
        else
            d.path = "" ;
    }
    else
        return false;

    d.flags.clear() ;

    return true;
}
/******************************************************************************************************************/
/*                                           Local Directory Storage                                              */
/******************************************************************************************************************/

void LocalDirectoryStorage::setSharedDirectoryList(const std::list<SharedDirInfo>& lst)
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

    mLocalDirs.clear();

    for(std::list<SharedDirInfo>::const_iterator it(processed_list.begin());it!=processed_list.end();++it)
        mLocalDirs[it->filename] = *it;
}
void LocalDirectoryStorage::getSharedDirectoryList(std::list<SharedDirInfo>& lst)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    lst.clear();

    for(std::map<std::string,SharedDirInfo>::iterator it(mLocalDirs.begin());it!=mLocalDirs.end();++it)
        lst.push_back(it->second) ;
}

static bool sameLists(const std::list<RsNodeGroupId>& l1,const std::list<RsNodeGroupId>& l2)
{
    std::list<RsNodeGroupId>::const_iterator it1(l1.begin()) ;
    std::list<RsNodeGroupId>::const_iterator it2(l2.begin()) ;

    for(; (it1!=l1.end() && it2!=l2.end());++it1,++it2)
        if(*it1 != *it2)
            return false ;

    return it1 == l1.end() && it2 == l2.end() ;
}

void LocalDirectoryStorage::updateShareFlags(const SharedDirInfo& info)
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

    if(!sameLists(it->second.parent_groups,info.parent_groups) || it->second.filename != info.filename || it->second.shareflags != info.shareflags || it->second.virtualname != info.virtualname)
    {
        it->second = info;
        mFileHierarchy->stampDirectory(0) ;

        std::cerr << "Updating dir mod time because flags at level 0 have changed." << std::endl;
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

void LocalDirectoryStorage::updateTimeStamps()
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    time_t last_modf_time = mFileHierarchy->recursUpdateLastModfTime(EntryIndex(0)) ;

    std::cerr << "LocalDirectoryStorage: global last modf time is " << last_modf_time << " (which is " << time(NULL) - last_modf_time << " secs ago)" << std::endl;
}

std::string LocalDirectoryStorage::locked_findRealRootFromVirtualFilename(const std::string& virtual_rootdir) const
{
    /**** MUST ALREADY BE LOCKED ****/

    std::map<std::string, SharedDirInfo>::const_iterator cit = mLocalDirs.find(virtual_rootdir) ;

    if (cit == mLocalDirs.end())
    {
        std::cerr << "FileIndexMonitor::locked_findRealRoot() Invalid RootDir: " << virtual_rootdir << std::endl;
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
            std::cerr << "(EE) very weird bug: base directory \"" << base_dir << "\" not found in shared dir list." << std::endl;
            return false ;
        }

        flags = it->second.shareflags;
        parent_groups = it->second.parent_groups;
    }

    return true;
}

bool LocalDirectoryStorage::serialiseDirEntry(const EntryIndex& indx,RsTlvBinaryData& bindata,const RsPeerId& client_id)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    const InternalFileHierarchyStorage::DirEntry *dir = mFileHierarchy->getDirEntry(indx);

    if(dir == NULL)
    {
        std::cerr << "(EE) serialiseDirEntry: ERROR. Cannot find entry " << (void*)(intptr_t)indx << std::endl;
        return false;
    }

    // compute list of allowed subdirs
    std::vector<EntryIndex> allowed_subdirs ;
    FileStorageFlags node_flags ;
    std::list<RsNodeGroupId> node_groups ;

    // for each subdir, compute the node flags and groups, then ask rsPeers to compute the mask that result from these flags for the particular peer supplied in parameter

    for(uint32_t i=0;i<dir->subdirs.size();++i)
        if(indx != 0 || (locked_getFileSharingPermissions(dir->subdirs[i],node_flags,node_groups) && (rsPeers->computePeerPermissionFlags(client_id,node_flags,node_groups) & RS_FILE_HINTS_BROWSABLE)))
            allowed_subdirs.push_back(dir->subdirs[i]) ;

    unsigned char *section_data = NULL;
    uint32_t section_size = 0;
    uint32_t section_offset = 0;

    // we need to send:
    //	- the name of the directory, its TS
    //	- the index entry for each subdir (the updte TS are exchanged at a higher level)
    //	- the file info for each subfile
    //

    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_DIR_NAME       ,dir->dir_name        )) return false ;
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_RECURS_MODIF_TS,(uint32_t)dir->dir_most_recent_time)) return false ;
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_MODIF_TS       ,(uint32_t)dir->dir_modtime     )) return false ;

    // serialise number of subdirs and number of subfiles

    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)allowed_subdirs.size()  )) return false ;
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)dir->subfiles.size()    )) return false ;

    // serialise subdirs entry indexes

    for(uint32_t i=0;i<allowed_subdirs.size();++i)
        if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX ,(uint32_t)allowed_subdirs[i]  )) return false ;

    // serialise directory subfiles, with info for each of them

    for(uint32_t i=0;i<dir->subfiles.size();++i)
    {
        unsigned char *file_section_data = NULL ;
        uint32_t file_section_offset = 0 ;
        uint32_t file_section_size = 0;

        const InternalFileHierarchyStorage::FileEntry *file = mFileHierarchy->getFileEntry(dir->subfiles[i]) ;

        if(file == NULL)
        {
            std::cerr << "(EE) cannot reach file entry " << dir->subfiles[i] << " to get/send file info." << std::endl;
            continue ;
        }

        if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX   ,(uint32_t)dir->subfiles[i]  )) return false ;
        if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,file->file_name   )) return false ;
        if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,file->file_size   )) return false ;
        if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,file->file_hash   )) return false ;
        if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_MODIF_TS      ,(uint32_t)file->file_modtime)) return false ;

        // now write the whole string into a single section in the file

        if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_REMOTE_FILE_ENTRY,file_section_data,file_section_offset)) return false ;

        free(file_section_data) ;
    }

    std::cerr << "Serialised dir entry to send for entry index " << (void*)(intptr_t)indx << ". Data size is " << section_size << " bytes" << std::endl;

    bindata.bin_data = section_data ;
    bindata.bin_len = section_offset ;

    return true ;
}

/******************************************************************************************************************/
/*                                           Remote Directory Storage                                              */
/******************************************************************************************************************/

RemoteDirectoryStorage::RemoteDirectoryStorage(const RsPeerId& pid,const std::string& fname)
    : DirectoryStorage(pid),mLastSavedTime(0),mChanged(false),mFileName(fname)
{
    load(fname) ;

    std::cerr << "***************************************" << std::endl;
    std::cerr << "Loaded following directory for peer " << pid << std::endl;
    mFileHierarchy->print();
    std::cerr << "***************************************" << std::endl;
}

void RemoteDirectoryStorage::checkSave()
{
    time_t now = time(NULL);

    if(mChanged && mLastSavedTime + MIN_INTERVAL_BETWEEN_REMOTE_DIRECTORY_SAVE < now)
    {
        save(mFileName);
        mLastSavedTime = now ;
    }
}

bool RemoteDirectoryStorage::deserialiseUpdateDirEntry(const EntryIndex& indx,const RsTlvBinaryData& bindata)
{
    const unsigned char *section_data = (unsigned char*)bindata.bin_data ;
    uint32_t section_size = bindata.bin_len ;
    uint32_t section_offset=0 ;

    std::cerr << "RemoteDirectoryStorage::deserialiseDirEntry(): deserialising directory content for friend " << peerId() << ", and directory " << indx << std::endl;

    std::string dir_name ;
    uint32_t most_recent_time ,dir_modtime ;

    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_DIR_NAME       ,dir_name        )) return false ;
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_RECURS_MODIF_TS,most_recent_time)) return false ;
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_MODIF_TS       ,dir_modtime     )) return false ;

    std::cerr << "  dir name           : \"" << dir_name << "\"" << std::endl;
    std::cerr << "  most recent time   : " << most_recent_time << std::endl;
    std::cerr << "  modification time  : " << dir_modtime << std::endl;

    // serialise number of subdirs and number of subfiles

    uint32_t n_subdirs,n_subfiles ;

    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_subdirs  )) return false ;
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_subfiles )) return false ;

    std::cerr << "  number of subdirs  : " << n_subdirs << std::endl;
    std::cerr << "  number of files    : " << n_subfiles << std::endl;

    // serialise subdirs entry indexes

    std::vector<EntryIndex> subdirs_array ;
    uint32_t subdir_index ;

    for(uint32_t i=0;i<n_subdirs;++i)
    {
        if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX ,subdir_index)) return false ;

        subdirs_array.push_back(EntryIndex(subdir_index)) ;
    }

    // deserialise directory subfiles, with info for each of them
    std::vector<EntryIndex> subfiles_array ;
    std::vector<std::string> subfiles_name ;
    std::vector<uint64_t> subfiles_size ;
    std::vector<RsFileHash> subfiles_hash ;
    std::vector<time_t> subfiles_modtime ;

    for(uint32_t i=0;i<n_subfiles;++i)
    {
        // Read the full data section for the file

        unsigned char *file_section_data = NULL ;
        uint32_t file_section_size = 0;

        if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_REMOTE_FILE_ENTRY,file_section_data,file_section_size)) return false ;

        uint32_t file_section_offset = 0 ;

        uint32_t entry_index ;
        std::string entry_name ;
        uint64_t entry_size ;
        RsFileHash entry_hash ;
        uint32_t entry_modtime ;

        if(!FileListIO::readField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX   ,entry_index  )) return false ;
        if(!FileListIO::readField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,entry_name   )) return false ;
        if(!FileListIO::readField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,entry_size   )) return false ;
        if(!FileListIO::readField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,entry_hash   )) return false ;
        if(!FileListIO::readField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_MODIF_TS      ,entry_modtime)) return false ;

        free(file_section_data) ;

        subfiles_array.push_back(entry_index) ;
        subfiles_name.push_back(entry_name) ;
        subfiles_size.push_back(entry_size) ;
        subfiles_hash.push_back(entry_hash) ;
        subfiles_modtime.push_back(entry_modtime) ;
    }

    RS_STACK_MUTEX(mDirStorageMtx) ;

    std::cerr << "  updating dir entry..." << std::endl;

    // first create the entries for each subdir and each subfile.
    if(!mFileHierarchy->updateDirEntry(indx,dir_name,most_recent_time,dir_modtime,subdirs_array,subfiles_array))
    {
        std::cerr << "(EE) Cannot update dir entry with index " << indx << ": entry does not exist." << std::endl;
        return false ;
    }

    // then update the subfiles
    for(uint32_t i=0;i<subfiles_array.size();++i)
    {
        std::cerr << "  updating file entry " << subfiles_hash[i] << std::endl;

        if(!mFileHierarchy->updateFile(subfiles_array[i],subfiles_hash[i],subfiles_name[i],subfiles_size[i],subfiles_modtime[i]))
            std::cerr << "(EE) Cannot update file with index " << subfiles_array[i] << ". This is very weird. Entry should have just been created and therefore should exist. Skipping." << std::endl;
    }
    mChanged = true ;

    return true ;
}






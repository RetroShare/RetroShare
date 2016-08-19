#include <set>
#include "util/rsdir.h"
#include "util/rsstring.h"
#include "directory_storage.h"

#define DEBUG_DIRECTORY_STORAGE 1

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

class InternalFileHierarchyStorage
{
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
        virtual uint32_t type() const { return FileStorageNode::TYPE_FILE ; }
        virtual ~FileEntry() {}

        // local stuff
        std::string file_name ;
        uint64_t    file_size ;
        RsFileHash  file_hash ;
        time_t      file_modtime ;
    };

    class DirEntry: public FileStorageNode
    {
    public:
        DirEntry(const std::string& name) : dir_name(name) {}
        virtual ~DirEntry() {}

        virtual uint32_t type() const { return FileStorageNode::TYPE_DIR ; }

        // local stuff
        std::string dir_name ;
        std::string dir_parent_path ;

        std::vector<DirectoryStorage::EntryIndex> subdirs ;
        std::vector<DirectoryStorage::EntryIndex> subfiles ;

        time_t most_recent_time;
    };

    // class stuff
    InternalFileHierarchyStorage() : mRoot(0)
    {
        mNodes.push_back(new DirEntry("")) ;
        mNodes.back()->row=0;
        mNodes.back()->parent_index=0;
    }

    int parentRow(DirectoryStorage::EntryIndex e)
    {
        if(!checkIndex(e,FileStorageNode::TYPE_DIR | FileStorageNode::TYPE_FILE) || e==0)
            return -1 ;

        return mNodes[mNodes[e]->parent_index]->row;
    }

    // high level modification routines

    bool isIndexValid(DirectoryStorage::EntryIndex e) const
    {
        return e < mNodes.size() && mNodes[e] != NULL ;
    }

    bool updateSubDirectoryList(const DirectoryStorage::EntryIndex& indx,const std::set<std::string>& subdirs)
    {
        if(!checkIndex(indx,FileStorageNode::TYPE_DIR))
            return false;

        DirEntry& d(*static_cast<DirEntry*>(mNodes[indx])) ;

        std::set<std::string> should_create(subdirs);

        for(uint32_t i=0;i<d.subdirs.size();)
            if(subdirs.find(static_cast<DirEntry*>(mNodes[d.subdirs[i]])->dir_name) == subdirs.end())
            {
                std::cerr << "[directory storage] Removing subdirectory " << static_cast<DirEntry*>(mNodes[d.subdirs[i]])->dir_name << " with index " << d.subdirs[i] << std::endl;

                if( !removeDirectory(d.subdirs[i]))
                    i++ ;
            }
            else
            {
                std::cerr << "[directory storage] Keeping existing subdirectory " << static_cast<DirEntry*>(mNodes[d.subdirs[i]])->dir_name << " with index " << d.subdirs[i] << std::endl;

                should_create.erase(static_cast<DirEntry*>(mNodes[d.subdirs[i]])->dir_name) ;
                ++i;
            }

        for(std::set<std::string>::const_iterator it(should_create.begin());it!=should_create.end();++it)
        {
            std::cerr << "[directory storage] adding new subdirectory " << *it << " at index " << mNodes.size() << std::endl;

            d.subdirs.push_back(mNodes.size()) ;
            mNodes.push_back(new DirEntry(*it));
            mNodes.back()->row = mNodes.size()-1;
            mNodes.back()->parent_index = indx;
        }

        return true;
    }
    bool removeDirectory(const DirectoryStorage::EntryIndex& indx)
    {
        // check that it's a directory

        if(!checkIndex(indx,FileStorageNode::TYPE_DIR))
            return false;

        if(indx == 0)
            return nodeAccessError("checkIndex(): Cannot remove top level directory") ;

        // remove from parent

        DirEntry& d(*static_cast<DirEntry*>(mNodes[indx])) ;
        DirEntry& parent_dir(*static_cast<DirEntry*>(mNodes[d.parent_index]));

        for(uint32_t i=0;i<parent_dir.subdirs.size();++i)
            if(parent_dir.subdirs[i] == indx)
                    return recursRemoveDirectory(indx) ;

        return nodeAccessError("removeDirectory(): inconsistency!!") ;
    }

    bool checkIndex(DirectoryStorage::EntryIndex indx,uint8_t type) const
    {
        if(mNodes.empty() || indx==DirectoryStorage::NO_INDEX || indx >= mNodes.size() || mNodes[indx] == NULL)
            return nodeAccessError("checkIndex(): Node does not exist") ;

        if(! (mNodes[indx]->type() & type))
            return nodeAccessError("checkIndex(): Node is of wrong type") ;

        return true;
    }

    bool updateSubFilesList(const DirectoryStorage::EntryIndex& indx,const std::map<std::string,DirectoryStorage::FileTS>& subfiles,std::map<std::string,DirectoryStorage::FileTS>& new_files)
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
                std::cerr << "[directory storage] removing non existing file " << f.file_name << " at index " << d.subfiles[i] << std::endl;

                delete mNodes[d.subfiles[i]] ;
                mNodes[d.subfiles[i]] = NULL ;

                d.subfiles[i] = d.subfiles[d.subfiles.size()-1] ;
                d.subfiles.pop_back();
                continue;
            }

            if(it->second.modtime != f.file_modtime || it->second.size != f.file_size)	// file is newer and/or has different size
            {
                f.file_hash.clear();																// hash needs recomputing
                f.file_modtime = it->second.modtime;
                f.file_size = it->second.size;
            }
            else
                new_files.erase(f.file_name) ;

            ++i;
        }

        for(std::map<std::string,DirectoryStorage::FileTS>::const_iterator it(new_files.begin());it!=new_files.end();++it)
        {
            std::cerr << "[directory storage] adding new file " << it->first << " at index " << mNodes.size() << std::endl;

            d.subfiles.push_back(mNodes.size()) ;
            mNodes.push_back(new FileEntry(it->first,it->second.size,it->second.modtime));
            mNodes.back()->row = mNodes.size()-1;
            mNodes.back()->parent_index = indx;
        }
        return true;
    }
    bool updateHash(const DirectoryStorage::EntryIndex& file_index,const RsFileHash& hash)
    {
        if(!checkIndex(file_index,FileStorageNode::TYPE_FILE))
        {
            std::cerr << "[directory storage] (EE) cannot update file at index " << file_index << ". Not a valid index, or not a file." << std::endl;
            return false;
        }

        std::cerr << "[directory storage] updating hash at index " << file_index << ", hash=" << hash << std::endl;

        static_cast<FileEntry*>(mNodes[file_index])->file_hash = hash ;
        return true;
    }
    bool updateFile(const DirectoryStorage::EntryIndex& file_index,const RsFileHash& hash, const std::string& fname,uint64_t size, const time_t modf_time)
    {
        if(!checkIndex(file_index,FileStorageNode::TYPE_FILE))
        {
            std::cerr << "[directory storage] (EE) cannot update file at index " << file_index << ". Not a valid index, or not a file." << std::endl;
            return false;
        }

        FileEntry& fe(*static_cast<FileEntry*>(mNodes[file_index])) ;

        std::cerr << "[directory storage] updating file entry at index " << file_index << ", name=" << fe.file_name << " size=" << fe.file_size << ", hash=" << fe.file_hash << std::endl;

        fe.file_hash = hash;
        fe.file_size = size;
        fe.file_modtime = modf_time;

        return true;
    }

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

    const DirEntry *getDirEntry(DirectoryStorage::EntryIndex indx) const
    {
        if(!checkIndex(indx,FileStorageNode::TYPE_DIR))
            return NULL ;

        return static_cast<DirEntry*>(mNodes[indx]) ;
    }
    const FileEntry *getFileEntry(DirectoryStorage::EntryIndex indx) const
    {
        if(!checkIndex(indx,FileStorageNode::TYPE_FILE))
            return NULL ;

        return static_cast<FileEntry*>(mNodes[indx]) ;
    }
    uint32_t getType(DirectoryStorage::EntryIndex indx) const
    {
        if(checkIndex(indx,FileStorageNode::TYPE_FILE))
            return FileStorageNode::TYPE_FILE;
        else if(checkIndex(indx,FileStorageNode::TYPE_DIR))
            return FileStorageNode::TYPE_DIR;
        else
            return FileStorageNode::TYPE_UNKNOWN;
    }

    DirectoryStorage::EntryIndex getSubFileIndex(DirectoryStorage::EntryIndex parent_index,uint32_t file_tab_index)
    {
        if(!checkIndex(parent_index,FileStorageNode::TYPE_DIR))
            return DirectoryStorage::NO_INDEX;

        if(static_cast<DirEntry*>(mNodes[parent_index])->subfiles.size() <= file_tab_index)
            return DirectoryStorage::NO_INDEX;

        return static_cast<DirEntry*>(mNodes[parent_index])->subfiles[file_tab_index];
    }
    DirectoryStorage::EntryIndex getSubDirIndex(DirectoryStorage::EntryIndex parent_index,uint32_t dir_tab_index)
    {
        if(!checkIndex(parent_index,FileStorageNode::TYPE_DIR))
            return DirectoryStorage::NO_INDEX;

        if(static_cast<DirEntry*>(mNodes[parent_index])->subdirs.size() <= dir_tab_index)
            return DirectoryStorage::NO_INDEX;

        return static_cast<DirEntry*>(mNodes[parent_index])->subdirs[dir_tab_index];
    }

    bool check()	// checks consistency of storage.
    {
        return true;
    }

    void print() const
    {
        int nfiles = 0 ;
        int ndirs = 0 ;
        int nempty = 0 ;
        int nunknown = 0;

        for(uint32_t i=0;i<mNodes.size();++i)
            if(mNodes[i] == NULL)
            {
                std::cerr << "  Node " << i << ": empty " << std::endl;
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

        std::cerr << "Total nodes: " << mNodes.size() << " (" << nfiles << " files, " << ndirs << " dirs, " << nempty << " empty slots)" << std::endl;

        recursPrint(0,DirectoryStorage::EntryIndex(0));
    }
private:
    void recursPrint(int depth,DirectoryStorage::EntryIndex node) const
    {
        std::string indent(2*depth,' ');

        if(mNodes[node] == NULL)
        {
            std::cerr << "EMPTY NODE !!" << std::endl;
            return ;
        }
        DirEntry& d(*static_cast<DirEntry*>(mNodes[node]));

        std::cerr << indent << "dir:" << d.dir_name << ", subdirs: " ;
        for(int i=0;i<d.subdirs.size();++i)
            std::cerr << d.subdirs[i] << " " ;
        std::cerr << std::endl;

        for(int i=0;i<d.subdirs.size();++i)
            recursPrint(depth+1,d.subdirs[i]) ;

        for(int i=0;i<d.subfiles.size();++i)
        {
            FileEntry& f(*static_cast<FileEntry*>(mNodes[d.subfiles[i]]));
            std::cerr << indent << "  hash:" << f.file_hash << " ts:" << (uint64_t)f.file_modtime << "  " << f.file_size << "  " << f.file_name << std::endl;
        }
    }

    static bool nodeAccessError(const std::string& s)
    {
        std::cerr << "(EE) InternalDirectoryStructure: ERROR: " << s << std::endl;
        return false ;
    }

    // Removes the given subdirectory from the parent node and all its pendign subdirs. Files are kept, and will go during the cleaning
    // phase. That allows to keep file information when moving them around.

    bool recursRemoveDirectory(DirectoryStorage::EntryIndex dir)
    {
        DirEntry& d(*static_cast<DirEntry*>(mNodes[dir])) ;

        for(uint32_t i=0;i<d.subdirs.size();++i)
            recursRemoveDirectory(d.subdirs[i]);

        delete mNodes[dir] ;
        mNodes[dir] = NULL ;

        return true ;
    }

    std::map<RsFileHash,DirectoryStorage::EntryIndex> mHashes ; // used for fast search access
};

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

DirectoryStorage::DirectoryStorage(const std::string& local_file_name, const RsPeerId &pid)
    : mFileName(local_file_name),mPeerId(pid), mDirStorageMtx("Directory storage "+local_file_name)
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

bool DirectoryStorage::updateSubDirectoryList(const EntryIndex& indx,const std::set<std::string>& subdirs)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return mFileHierarchy->updateSubDirectoryList(indx,subdirs) ;
}
bool DirectoryStorage::updateSubFilesList(const EntryIndex& indx,const std::map<std::string,FileTS>& subfiles,std::map<std::string,FileTS>& new_files)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return mFileHierarchy->updateSubFilesList(indx,subfiles,new_files) ;
}
bool DirectoryStorage::removeDirectory(const EntryIndex& indx)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    return mFileHierarchy->removeDirectory(indx);
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
    // first load the header, than all fields.
}
void DirectoryStorage::save(const std::string& local_file_name)
{
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
#warning todo

    return true;
}

bool DirectoryStorage::extractData(const EntryIndex& indx,DirDetails& d)
{
    d.children.clear() ;
    time_t now = time(NULL) ;

    std::cerr << "LocalDirectoryStorage::extractData(). Index=" << indx << std::endl;

    const InternalFileHierarchyStorage::DirEntry *dir_entry = mFileHierarchy->getDirEntry(indx) ;

    d.ref = (void*)(intptr_t)indx ;

    if (dir_entry != NULL) /* has children --- fill */
    {
#ifdef DEBUG_DIRECTORY_STORAGE
        std::cerr << "FileIndex::extractData() ref=dir" << std::endl;
#endif
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
        d.min_age = now - dir_entry->most_recent_time ;
        d.name    = dir_entry->dir_name;
        d.path    = dir_entry->dir_parent_path + "/" + dir_entry->dir_name ;
        d.parent  = (void*)(intptr_t)dir_entry->parent_index ;

        if(indx == 0)
        {
            d.type = DIR_TYPE_PERSON ;
            d.name = mPeerId.toStdString();
        }
    }
    else
    {
        const InternalFileHierarchyStorage::FileEntry *file_entry = mFileHierarchy->getFileEntry(indx) ;
#ifdef DEBUG_DIRECTORY_STORAGE
        std::cerr << "FileIndexStore::extractData() ref=file" << std::endl;
#endif
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

void LocalDirectoryStorage::updateShareFlags(const SharedDirInfo& info)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;

    std::map<std::string,SharedDirInfo>::iterator it = mLocalDirs.find(info.virtualname) ;

    if(it == mLocalDirs.end())
    {
        std::cerr << "(EE) LocalDirectoryStorage::updateShareFlags: directory \"" << info.filename << "\" not found" << std::endl;
        return ;
    }
    it->second = info;
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

    d.flags.clear() ;

    /* find parent pointer, and row */

    std::cerr << "LocalDirectoryStorage::extractData(): indx=" << indx << " Returning this:" << std::endl;
    std::cerr << d << std::endl;

    return true;
}

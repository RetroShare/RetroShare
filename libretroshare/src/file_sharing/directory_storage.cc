#include <set>
#include "directory_storage.h"

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
        DirEntry(const std::string& name,DirectoryStorage::EntryIndex parent) : dir_name(name),parent_index(parent) {}
        virtual ~DirEntry() {}

        virtual uint32_t type() const { return FileStorageNode::TYPE_DIR ; }

        // local stuff
        std::string dir_name ;
        DirectoryStorage::EntryIndex parent_index;

        std::vector<DirectoryStorage::EntryIndex> subdirs ;
        std::vector<DirectoryStorage::EntryIndex> subfiles ;
    };

    // class stuff
    InternalFileHierarchyStorage() : mRoot(0)
    {
        mNodes.push_back(new DirEntry("",0)) ;
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
                removeDirectory(d.subdirs[i]) ;
            else
            {
                should_create.erase(static_cast<DirEntry*>(mNodes[d.subdirs[i]])->dir_name) ;
                ++i;
            }

        for(std::set<std::string>::const_iterator it(should_create.begin());it!=should_create.end();++it)
        {
            d.subdirs.push_back(mNodes.size()) ;
            mNodes.push_back(new DirEntry(*it,indx));
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

        if(mNodes[indx]->type() != type)
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
                d.subfiles[i] = d.subfiles[d.subfiles.size()-1] ;
                d.subfiles.pop_back();

                continue;
            }

            if(it->second.modtime != f.file_modtime || it->second.size != f.file_size)	// file is newer and/or has different size
                f.file_hash.clear();																// hash needs recomputing
            else
                new_files.erase(f.file_name) ;

            ++i;
        }

        for(std::map<std::string,DirectoryStorage::FileTS>::const_iterator it(new_files.begin());it!=new_files.end();++it)
        {
            d.subfiles.push_back(mNodes.size()) ;
            mNodes.push_back(new FileEntry(it->first,it->second.size,it->second.modtime));
        }
        return true;
    }
    bool updateHash(const DirectoryStorage::EntryIndex& file_index,const RsFileHash& hash)
    {
        if(!checkIndex(file_index,FileStorageNode::TYPE_FILE))
            return false;

        static_cast<FileEntry*>(mNodes[file_index])->file_hash = hash ;
        return true;
    }
    bool updateFile(const DirectoryStorage::EntryIndex& file_index,const RsFileHash& hash, const std::string& fname,uint64_t size, const time_t modf_time)
    {
        if(!checkIndex(file_index,FileStorageNode::TYPE_FILE))
            return false;

        FileEntry& fe(*static_cast<FileEntry*>(mNodes[file_index])) ;

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
        DirEntry& d(*static_cast<DirEntry*>(mNodes[node]));

        std::cerr << indent << "dir:" << d.dir_name << std::endl;

        for(int i=0;i<d.subdirs.size();++i)
            recursPrint(depth+1,d.subdirs[i]) ;

        for(int i=0;i<d.subfiles.size();++i)
        {
            FileEntry& f(*static_cast<FileEntry*>(mNodes[d.subfiles[i]]));
            std::cerr << indent << "  hash:" << f.file_hash << " ts:" << std::fill(8) << (uint64_t)f.file_modtime << "  " << f.file_size << "  " << f.file_name << std::endl;
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

/******************************************************************************************************************/
/*                                                 Directory Storage                                              */
/******************************************************************************************************************/

DirectoryStorage::DirectoryStorage(const std::string& local_file_name)
    : mFileName(local_file_name), mDirStorageMtx("Directory storage "+local_file_name)
{
    mFileHierarchy = new InternalFileHierarchyStorage();
}

DirectoryStorage::EntryIndex DirectoryStorage::root() const
{
    return EntryIndex(0) ;
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
/******************************************************************************************************************/
/*                                           Local Directory Storage                                              */
/******************************************************************************************************************/

void LocalDirectoryStorage::setSharedDirectoryList(const std::list<SharedDirInfo>& lst)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    mLocalDirs = lst ;
}
void LocalDirectoryStorage::getSharedDirectoryList(std::list<SharedDirInfo>& lst)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    lst = mLocalDirs ;
}

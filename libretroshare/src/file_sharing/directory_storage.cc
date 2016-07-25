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
        static const uint32_t TYPE_FILE = 0x0001 ;
        static const uint32_t TYPE_DIR  = 0x0002 ;

        virtual ~FileStorageNode() {}
        virtual uint32_t type() const =0;
    };
    class FileEntry: public FileStorageNode
    {
    public:
        FileEntry(const std::string& name) : file_name(name) {}
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

    bool isIndexValid(DirectoryStorage::EntryIndex e)
    {
        return e < mNodes.size() && mNodes[e] != NULL ;
    }

    bool updateSubDirectoryList(const DirectoryStorage::EntryIndex& indx,const std::set<std::string>& subdirs)
    {
        if(indx >= mNodes.size() || mNodes[indx] == NULL)
            return nodeAccessError("updateSubDirectoryList(): Node does not exist") ;

        if(mNodes[indx]->type() != FileStorageNode::TYPE_DIR)
            return nodeAccessError("updateSubDirectoryList(): Node is not a directory") ;

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

        if(indx >= mNodes.size() || mNodes[indx] == NULL)
            return nodeAccessError("removeDirectory(): Node does not exist") ;

        if(mNodes[indx]->type() != FileStorageNode::TYPE_DIR)
            return nodeAccessError("removeDirectory(): Node is not a directory") ;

        if(indx == 0)
            return nodeAccessError("removeDirectory(): Cannot remove top level directory") ;

        // remove from parent

        DirEntry& d(*static_cast<DirEntry*>(mNodes[indx])) ;
        DirEntry& parent_dir(*static_cast<DirEntry*>(mNodes[d.parent_index]));

        for(uint32_t i=0;i<parent_dir.subdirs.size();++i)
            if(parent_dir.subdirs[i] == indx)
                    return recursRemoveDirectory(indx) ;

        return nodeAccessError("removeDirectory(): inconsistency!!") ;
    }

    bool updateSubFilesList(const DirectoryStorage::EntryIndex& indx,const std::set<std::string>& subfiles,std::set<std::string>& new_files)
    {
        if(indx >= mNodes.size() || mNodes[indx] == NULL)
            return nodeAccessError("updateSubFilesList(): Node does not exist") ;

        if(mNodes[indx]->type() != FileStorageNode::TYPE_DIR)
            return nodeAccessError("updateSubFilesList(): Node is not a directory") ;

        DirEntry& d(*static_cast<DirEntry*>(mNodes[indx])) ;
        new_files = subfiles ;

        for(uint32_t i=0;i<d.subfiles.size();)
            if(subfiles.find(static_cast<FileEntry*>(mNodes[d.subfiles[i]])->file_name) == subfiles.end())
            {
                d.subfiles[i] = d.subfiles[d.subfiles.size()-1] ;
                d.subfiles.pop_back();
            }
            else
            {
                new_files.erase(static_cast<FileEntry*>(mNodes[d.subfiles[i]])->file_name) ;
                ++i;
            }

        for(std::set<std::string>::const_iterator it(new_files.begin());it!=new_files.end();++it)
        {
            d.subfiles.push_back(mNodes.size()) ;
            mNodes.push_back(new FileEntry(*it));
        }
        return true;
    }
    void updateFile(const DirectoryStorage::EntryIndex& parent_dir,const RsFileHash& hash, const std::string& fname, const uint32_t modf_time) ;
    void updateDirectory(const DirectoryStorage::EntryIndex& parent_dir,const std::string& dname) ;

    // file/dir access and modification
    bool findSubDirectory(DirectoryStorage::EntryIndex e,const std::string& s) const ;	// returns true when s is the name of a sub-directory in the given entry e

    uint32_t mRoot ;

    std::vector<FileStorageNode*> mNodes;// uses pointers to keep information about valid/invalid objects.

    void compress() ;					// use empty space in the vector, mostly due to deleted entries. This is a complicated operation, mostly due to
                                            // all the indirections used. Nodes need to be moved, renamed, etc. The operation discards all file entries that
                                            // are not referenced.

    friend class DirectoryStorage ;		// only class that can use this.

    // low level stuff. Should normally not be used externally.

    DirectoryStorage::EntryIndex getSubFileIndex(DirectoryStorage::EntryIndex parent_index,uint32_t file_tab_index)
    {
        if(parent_index >= mNodes.size() || mNodes[parent_index] == NULL || mNodes[parent_index]->type() != FileStorageNode::TYPE_DIR)
            return DirectoryStorage::NO_INDEX;

        if(static_cast<DirEntry*>(mNodes[parent_index])->subfiles.size() <= file_tab_index)
            return DirectoryStorage::NO_INDEX;

        return static_cast<DirEntry*>(mNodes[parent_index])->subfiles[file_tab_index];
    }
    DirectoryStorage::EntryIndex getSubDirIndex(DirectoryStorage::EntryIndex parent_index,uint32_t dir_tab_index)
    {
        if(parent_index >= mNodes.size() || mNodes[parent_index] == NULL || mNodes[parent_index]->type() != FileStorageNode::TYPE_DIR)
            return DirectoryStorage::NO_INDEX;

        if(static_cast<DirEntry*>(mNodes[parent_index])->subdirs.size() <= dir_tab_index)
            return DirectoryStorage::NO_INDEX;

        return static_cast<DirEntry*>(mNodes[parent_index])->subfiles[dir_tab_index];
    }

    bool check()	// checks consistency of storage.
    {
        return true;
    }

    void print()
    {
    }
private:
    bool nodeAccessError(const std::string& s)
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
DirectoryStorage::EntryIndex DirectoryStorage::DirIterator::operator*() const
{
    return mStorage->getSubDirIndex(mParentIndex,mDirTabIndex) ;
}
DirectoryStorage::EntryIndex DirectoryStorage::FileIterator::operator*() const
{
    return mStorage->getSubFileIndex(mParentIndex,mFileTabIndex) ;
}
DirectoryStorage::DirIterator::operator bool() const
{
    return **this != DirectoryStorage::NO_INDEX;
}
DirectoryStorage::FileIterator::operator bool() const
{
    return **this != DirectoryStorage::NO_INDEX;
}

/******************************************************************************************************************/
/*                                                 Directory Storage                                              */
/******************************************************************************************************************/

DirectoryStorage::DirIterator DirectoryStorage::root()
{
    return DirIterator(this,EntryIndex(0)) ;
}

void DirectoryStorage::updateSubDirectoryList(const EntryIndex& indx,const std::set<std::string>& subdirs)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    mFileHierarchy->updateSubDirectoryList(indx,subdirs) ;
}
void DirectoryStorage::updateSubFilesList(const EntryIndex& indx,const std::set<std::string>& subfiles,std::set<std::string>& new_files)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    mFileHierarchy->updateSubFilesList(indx,subfiles,new_files) ;
}
void DirectoryStorage::removeDirectory(const EntryIndex& indx)
{
    RS_STACK_MUTEX(mDirStorageMtx) ;
    mFileHierarchy->removeDirectory(indx);
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

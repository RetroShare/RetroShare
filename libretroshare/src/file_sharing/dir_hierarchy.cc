#include "util/rsdir.h"

#include "dir_hierarchy.h"
#include "filelist_io.h"

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

InternalFileHierarchyStorage::InternalFileHierarchyStorage() : mRoot(0)
{
    DirEntry *de = new DirEntry("") ;

    de->row=0;
    de->parent_index=0;
    de->dir_modtime=0;

    mNodes.push_back(de) ;
#warning not very elegant. We should remove the leading /
    mDirHashes[computeDirHash("/")] = 0 ;
}

RsFileHash InternalFileHierarchyStorage::computeDirHash(const std::string& dir_path)
{
    return RsDirUtil::sha1sum((unsigned char*)dir_path.c_str(),dir_path.length()) ;
}
bool InternalFileHierarchyStorage::getDirHashFromIndex(const DirectoryStorage::EntryIndex& index,RsFileHash& hash) const
{
    if(!checkIndex(index,FileStorageNode::TYPE_DIR))
        return false ;

    DirEntry& d = *static_cast<DirEntry*>(mNodes[index]) ;

    hash = computeDirHash( d.dir_parent_path + "/" + d.dir_name ) ;

    std::cerr << "Computing dir hash from index " << index << ". Dir=\"" << d.dir_parent_path + "/" + d.dir_name << "\" hash=" << hash << std::endl;

    return true;
}
bool InternalFileHierarchyStorage::getIndexFromDirHash(const RsFileHash& hash,DirectoryStorage::EntryIndex& index) const
{
    std::map<RsFileHash,DirectoryStorage::EntryIndex>::const_iterator it = mDirHashes.find(hash) ;

    if(it == mDirHashes.end())
        return false;

    index = it->second;
    return true;
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
bool InternalFileHierarchyStorage::stampDirectory(const DirectoryStorage::EntryIndex& indx)
{
    if(!checkIndex(indx,FileStorageNode::TYPE_DIR))
        return false;

    static_cast<DirEntry*>(mNodes[indx])->dir_modtime = time(NULL) ;
    return true;
}

bool InternalFileHierarchyStorage::updateSubDirectoryList(const DirectoryStorage::EntryIndex& indx,const std::map<std::string,time_t>& subdirs)
{
    if(!checkIndex(indx,FileStorageNode::TYPE_DIR))
        return false;

    DirEntry& d(*static_cast<DirEntry*>(mNodes[indx])) ;

    std::map<std::string,time_t> should_create(subdirs);

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

    for(std::map<std::string,time_t>::const_iterator it(should_create.begin());it!=should_create.end();++it)
    {
        std::cerr << "[directory storage] adding new subdirectory " << it->first << " at index " << mNodes.size() << std::endl;

        DirEntry *de = new DirEntry(it->first) ;

        de->row = mNodes.size();
        de->parent_index = indx;
        de->dir_modtime = it->second;

        d.subdirs.push_back(mNodes.size()) ;
        mNodes.push_back(de) ;
    }

    return true;
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

            bool res = recursRemoveDirectory(indx) ;
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
bool InternalFileHierarchyStorage::updateHash(const DirectoryStorage::EntryIndex& file_index,const RsFileHash& hash)
{
    if(!checkIndex(file_index,FileStorageNode::TYPE_FILE))
    {
        std::cerr << "[directory storage] (EE) cannot update file at index " << file_index << ". Not a valid index, or not a file." << std::endl;
        return false;
    }

    std::cerr << "[directory storage] updating hash at index " << file_index << ", hash=" << hash << std::endl;

    RsFileHash& old_hash (static_cast<FileEntry*>(mNodes[file_index])->file_hash) ;

    old_hash = hash ;

    return true;
}
bool InternalFileHierarchyStorage::updateFile(const DirectoryStorage::EntryIndex& file_index,const RsFileHash& hash, const std::string& fname,uint64_t size, const time_t modf_time)
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
    fe.file_name = fname;

    return true;
}

bool InternalFileHierarchyStorage::updateDirEntry(const DirectoryStorage::EntryIndex& indx,const std::string& dir_name,time_t most_recent_time,time_t dir_modtime,const std::vector<RsFileHash>& subdirs_hash,const std::vector<RsFileHash>& subfiles_hash)
{
    if(!checkIndex(indx,FileStorageNode::TYPE_DIR))
    {
        std::cerr << "[directory storage] (EE) cannot update dir at index " << indx << ". Not a valid index, or not an existing dir." << std::endl;
        return false;
    }
    DirEntry& d(*static_cast<DirEntry*>(mNodes[indx])) ;

    d.dir_most_recent_time = most_recent_time;
    d.dir_modtime      = dir_modtime;
    d.dir_update_time  = time(NULL);
    d.dir_name         = dir_name;

    d.subdirs.clear();
    d.subfiles.clear();

    // check that all subdirs already exist. If not, create.
    for(uint32_t i=0;i<subdirs_hash.size();++i)
    {
        DirectoryStorage::EntryIndex dir_index = 0;
        std::map<RsFileHash,DirectoryStorage::EntryIndex>::const_iterator it = mDirHashes.find(subdirs_hash[i]) ;

        if(it == mDirHashes.end() || it->second >= mNodes.size())
        {
            // find an epty slot
            int found = -1 ;

            for(uint32_t j=0;j<mNodes.size();++j)
                if(mNodes[i] == NULL)
                {
                    found = j;
                    break;
                }

            if(found < 0)
            {
                dir_index = mNodes.size() ;
                mNodes.push_back(NULL) ;
            }
            else
                dir_index = found;

            mDirHashes[subdirs_hash[i]] = dir_index ;
        }
        else
        {
            dir_index = it->second;

            if(mNodes[dir_index] != NULL && mNodes[dir_index]->type() != FileStorageNode::TYPE_DIR)
            {
                delete mNodes[dir_index] ;
                mNodes[dir_index] = NULL ;
            }
        }
        FileStorageNode *& node(mNodes[dir_index]);
        if(!node)
            node = new DirEntry("");

        d.subdirs.push_back(dir_index) ;

        ((DirEntry*&)node)->dir_parent_path = d.dir_parent_path + "/" + dir_name ;
        node->row = i ;
        node->parent_index = indx ;
    }
    for(uint32_t i=0;i<subfiles_hash.size();++i)
    {
        DirectoryStorage::EntryIndex file_index = 0;
        std::map<RsFileHash,DirectoryStorage::EntryIndex>::const_iterator it = mFileHashes.find(subfiles_hash[i]) ;

        if(it == mFileHashes.end())
        {
            // find an epty slot
            int found = -1;

            for(uint32_t j=0;j<mNodes.size();++j)
                if(mNodes[i] == NULL)
                {
                    found = j;
                    break;
                }

            if(found < 0)
            {
                file_index = mNodes.size() ;
                mNodes.push_back(NULL) ;
            }
            else
                file_index = found;

            mFileHashes[subfiles_hash[i]] = file_index ;
        }
        else
        {
            file_index = it->second ;

            if(mNodes[file_index] != NULL && mNodes[file_index]->type() != FileStorageNode::TYPE_FILE)
            {
                delete mNodes[file_index] ;
                mNodes[file_index] = NULL ;
            }

            file_index = it->second;
        }
        FileStorageNode *& node(mNodes[file_index]);
        if(!node)
            node = new FileEntry("",0,0,subfiles_hash[i]);

        d.subfiles.push_back(file_index) ;

        node->row = subdirs_hash.size()+i ;
        node->parent_index = indx ;
    }

    return true;
}

bool InternalFileHierarchyStorage::getDirUpdateTS(const DirectoryStorage::EntryIndex& index,time_t& recurs_max_modf_TS,time_t& local_update_TS)
{
    if(!checkIndex(index,FileStorageNode::TYPE_DIR))
    {
        std::cerr << "[directory storage] (EE) cannot update TS for index " << index << ". Not a valid index or not a directory." << std::endl;
        return false;
    }

    DirEntry& d(*static_cast<DirEntry*>(mNodes[index])) ;

    recurs_max_modf_TS = d.dir_most_recent_time ;
    local_update_TS    = d.dir_update_time ;

    return true;
}
bool InternalFileHierarchyStorage::setDirUpdateTS(const DirectoryStorage::EntryIndex& index,time_t& recurs_max_modf_TS,time_t& local_update_TS)
{
    if(!checkIndex(index,FileStorageNode::TYPE_DIR))
    {
        std::cerr << "[directory storage] (EE) cannot update TS for index " << index << ". Not a valid index or not a directory." << std::endl;
        return false;
    }

    DirEntry& d(*static_cast<DirEntry*>(mNodes[index])) ;

    d.dir_most_recent_time = recurs_max_modf_TS ;
    d.dir_update_time  = local_update_TS    ;

    return true;
}

// Do a complete recursive sweep over sub-directories and files, and update the lst modf TS. This could be also performed by a cleanup method.

time_t InternalFileHierarchyStorage::recursUpdateLastModfTime(const DirectoryStorage::EntryIndex& dir_index)
{
    DirEntry& d(*static_cast<DirEntry*>(mNodes[dir_index])) ;

    time_t largest_modf_time = d.dir_modtime ;

    for(uint32_t i=0;i<d.subfiles.size();++i)
        largest_modf_time = std::max(largest_modf_time,static_cast<FileEntry*>(mNodes[d.subfiles[i]])->file_modtime) ;

    for(uint32_t i=0;i<d.subdirs.size();++i)
        largest_modf_time = std::max(largest_modf_time,recursUpdateLastModfTime(d.subdirs[i])) ;

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

bool InternalFileHierarchyStorage::searchHash(const RsFileHash& hash,std::list<DirectoryStorage::EntryIndex>& results)
{
    std::map<RsFileHash,DirectoryStorage::EntryIndex>::const_iterator it = mFileHashes.find(hash);

    if( it != mFileHashes.end() )
    {
        results.clear();
        results.push_back(it->second) ;
        return true ;
    }
    else
        return false;
}

bool InternalFileHierarchyStorage::check(std::string& error_string) const	// checks consistency of storage.
{
    // recurs go through all entries, check that all

    std::vector<uint32_t> hits(mNodes.size(),0) ;	// count hits of children. Should be 1 for all in the end. Otherwise there's an error.
    hits[0] = 1 ;	// because 0 is never the child of anyone

    for(uint32_t i=0;i<mNodes.size();++i)
        if(mNodes[i] != NULL && mNodes[i]->type() == FileStorageNode::TYPE_DIR)
        {
            // stamp the kids
            const DirEntry& de = *static_cast<DirEntry*>(mNodes[i]) ;

            for(uint32_t j=0;j<de.subdirs.size();++j)
            {
                if(de.subdirs[j] >= mNodes.size())
                {
                    error_string = "Node child out of tab!" ;
                    return false ;
                }
                if(hits[de.subdirs[j]] != 0)
                {
                    error_string = "Double hit on a single node" ;
                    return false;
                }
                hits[de.subdirs[j]] = 1;
            }
            for(uint32_t j=0;j<de.subfiles.size();++j)
            {
                if(de.subfiles[j] >= mNodes.size())
                {
                    error_string = "Node child out of tab!" ;
                    return false ;
                }
                if(hits[de.subfiles[j]] != 0)
                {
                    error_string = "Double hit on a single node" ;
                    return false;
                }
                hits[de.subfiles[j]] = 1;
            }

        }

    for(uint32_t i=0;i<hits.size();++i)
        if(hits[i] == 0 && mNodes[i] != NULL)
        {
            error_string = "Orphean node!" ;
            return false;
        }

    return true;
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
void InternalFileHierarchyStorage::recursPrint(int depth,DirectoryStorage::EntryIndex node) const
{
    std::string indent(2*depth,' ');

    if(mNodes[node] == NULL)
    {
        std::cerr << "EMPTY NODE !!" << std::endl;
        return ;
    }
    DirEntry& d(*static_cast<DirEntry*>(mNodes[node]));

    std::cerr << indent << "dir:" << d.dir_name << ", modf time: " << d.dir_modtime << ", recurs_last_modf_time: " << d.dir_most_recent_time << ", parent: " << d.parent_index << ", row: " << d.row << ", subdirs: " ;

    for(uint32_t i=0;i<d.subdirs.size();++i)
        std::cerr << d.subdirs[i] << " " ;
    std::cerr << std::endl;

    for(uint32_t i=0;i<d.subdirs.size();++i)
        recursPrint(depth+1,d.subdirs[i]) ;

    for(uint32_t i=0;i<d.subfiles.size();++i)
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

// Removes the given subdirectory from the parent node and all its pendign subdirs. Files are kept, and will go during the cleaning
// phase. That allows to keep file information when moving them around.

bool InternalFileHierarchyStorage::recursRemoveDirectory(DirectoryStorage::EntryIndex dir)
{
    DirEntry& d(*static_cast<DirEntry*>(mNodes[dir])) ;

    for(uint32_t i=0;i<d.subdirs.size();++i)
        recursRemoveDirectory(d.subdirs[i]);

    for(uint32_t i=0;i<d.subfiles.size();++i)
    {
        delete mNodes[d.subfiles[i]] ;
        mNodes[d.subfiles[i]] = NULL ;
    }
    delete mNodes[dir] ;
    mNodes[dir] = NULL ;

    return true ;
}

bool InternalFileHierarchyStorage::save(const std::string& fname)
{
    unsigned char *buffer = NULL ;
    uint32_t buffer_size = 0 ;
    uint32_t buffer_offset = 0 ;

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

                unsigned char *file_section_data = NULL ;
                uint32_t file_section_offset = 0 ;
                uint32_t file_section_size = 0;

                if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_PARENT_INDEX  ,(uint32_t)fe.parent_index)) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_ROW           ,(uint32_t)fe.row         )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX   ,(uint32_t)i              )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,fe.file_name             )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,fe.file_size             )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,fe.file_hash             )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(file_section_data,file_section_size,file_section_offset,FILE_LIST_IO_TAG_MODIF_TS      ,(uint32_t)fe.file_modtime)) throw std::runtime_error("Write error") ;

                if(!FileListIO::writeField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_FILE_ENTRY,file_section_data,file_section_offset)) throw std::runtime_error("Write error") ;

                free(file_section_data) ;
            }
            else if(mNodes[i] != NULL && mNodes[i]->type() == FileStorageNode::TYPE_DIR)
            {
                const DirEntry& de(*static_cast<const DirEntry*>(mNodes[i])) ;

                unsigned char *dir_section_data = NULL ;
                uint32_t dir_section_offset = 0 ;
                uint32_t dir_section_size = 0;

                if(!FileListIO::writeField(dir_section_data,dir_section_size,dir_section_offset,FILE_LIST_IO_TAG_PARENT_INDEX   ,(uint32_t)de.parent_index      )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(dir_section_data,dir_section_size,dir_section_offset,FILE_LIST_IO_TAG_ROW            ,(uint32_t)de.row               )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(dir_section_data,dir_section_size,dir_section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX    ,(uint32_t)i                    )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(dir_section_data,dir_section_size,dir_section_offset,FILE_LIST_IO_TAG_FILE_NAME      ,de.dir_name                    )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(dir_section_data,dir_section_size,dir_section_offset,FILE_LIST_IO_TAG_FILE_SIZE      ,de.dir_parent_path             )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(dir_section_data,dir_section_size,dir_section_offset,FILE_LIST_IO_TAG_MODIF_TS       ,(uint32_t)de.dir_modtime       )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(dir_section_data,dir_section_size,dir_section_offset,FILE_LIST_IO_TAG_UPDATE_TS      ,(uint32_t)de.dir_update_time   )) throw std::runtime_error("Write error") ;
                if(!FileListIO::writeField(dir_section_data,dir_section_size,dir_section_offset,FILE_LIST_IO_TAG_RECURS_MODIF_TS,(uint32_t)de.dir_most_recent_time  )) throw std::runtime_error("Write error") ;

                if(!FileListIO::writeField(dir_section_data,dir_section_size,dir_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)de.subdirs.size())) throw std::runtime_error("Write error") ;

                for(uint32_t j=0;j<de.subdirs.size();++j)
                    if(!FileListIO::writeField(dir_section_data,dir_section_size,dir_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)de.subdirs[j])) throw std::runtime_error("Write error") ;

                if(!FileListIO::writeField(dir_section_data,dir_section_size,dir_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)de.subfiles.size())) throw std::runtime_error("Write error") ;

                for(uint32_t j=0;j<de.subfiles.size();++j)
                    if(!FileListIO::writeField(dir_section_data,dir_section_size,dir_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)de.subfiles[j])) throw std::runtime_error("Write error") ;

                if(!FileListIO::writeField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIR_ENTRY,dir_section_data,dir_section_offset)) throw std::runtime_error("Write error") ;

                free(dir_section_data) ;
            }

        bool res = FileListIO::saveEncryptedDataToFile(fname,buffer,buffer_offset) ;

        free(buffer) ;
        return res ;
    }
    catch(std::exception& e)
    {
        std::cerr << "Error while writing: " << e.what() << std::endl;

        if(buffer != NULL)
            free(buffer) ;
        return false;
    }
}

bool InternalFileHierarchyStorage::load(const std::string& fname)
{
    unsigned char *buffer = NULL ;
    uint32_t buffer_size = 0 ;
    uint32_t buffer_offset = 0 ;

    try
    {
        if(!FileListIO::loadEncryptedDataFromFile(fname,buffer,buffer_size) )
            throw std::runtime_error("Read error") ;

        // Read some header

        uint32_t version, n_nodes ;

        if(!FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIRECTORY_VERSION,version)) throw std::runtime_error("Read error") ;
        if(version != (uint32_t) FILE_LIST_IO_LOCAL_DIRECTORY_STORAGE_VERSION_0001) throw std::runtime_error("Wrong version number") ;

        if(!FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_nodes)) throw std::runtime_error("Read error") ;

        // Write all file/dir entries

        for(uint32_t i=0;i<mNodes.size();++i)
            if(mNodes[i])
                delete mNodes[i] ;

        mNodes.clear();
        mNodes.resize(n_nodes,NULL) ;

        for(uint32_t i=0;i<mNodes.size();++i)
        {
            unsigned char *node_section_data = NULL ;
            uint32_t node_section_size = 0 ;
            uint32_t node_section_offset = 0 ;

            if(FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_FILE_ENTRY,node_section_data,node_section_size))
            {
                uint32_t node_index ;
                std::string file_name ;
                uint64_t file_size ;
                RsFileHash file_hash ;
                uint32_t file_modtime ;
                uint32_t row ;
                uint32_t parent_index ;

                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_PARENT_INDEX  ,parent_index)) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_ROW           ,row         )) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX   ,node_index  )) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,file_name   )) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,file_size   )) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,file_hash   )) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_MODIF_TS      ,file_modtime)) throw std::runtime_error("Read error") ;

                if(node_index >= mNodes.size())
                    mNodes.resize(node_index+1,NULL) ;

                FileEntry *fe = new FileEntry(file_name,file_size,file_modtime,file_hash);

                fe->parent_index = parent_index ;
                fe->row = row ;

                mNodes[node_index] = fe ;
            }
            else if(FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIR_ENTRY,node_section_data,node_section_size))
            {
                uint32_t node_index ;
                std::string dir_name ;
                std::string dir_parent_path ;
                uint32_t dir_modtime ;
                uint32_t dir_update_time ;
                uint32_t dir_most_recent_time ;
                uint32_t row ;
                uint32_t parent_index ;

                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_PARENT_INDEX   ,parent_index         )) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_ROW            ,row                  )) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_ENTRY_INDEX    ,node_index           )) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_NAME      ,dir_name             )) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SIZE      ,dir_parent_path      )) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_MODIF_TS       ,dir_modtime          )) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_UPDATE_TS      ,dir_update_time      )) throw std::runtime_error("Read error") ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RECURS_MODIF_TS,dir_most_recent_time )) throw std::runtime_error("Read error") ;

                if(node_index >= mNodes.size())
                    mNodes.resize(node_index+1,NULL) ;

                DirEntry *de = new DirEntry(dir_name) ;
                de->dir_name         = dir_name ;
                de->dir_parent_path  = dir_parent_path ;
                de->dir_modtime      = dir_modtime ;
                de->dir_update_time  = dir_update_time ;
                de->dir_most_recent_time = dir_most_recent_time ;

                de->parent_index = parent_index ;
                de->row = row ;

                uint32_t n_subdirs = 0 ;
                uint32_t n_subfiles = 0 ;

                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_subdirs)) throw std::runtime_error("Read error") ;

                for(uint32_t j=0;j<n_subdirs;++j)
                {
                    uint32_t di = 0 ;
                    if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,di)) throw std::runtime_error("Read error") ;
                    de->subdirs.push_back(di) ;
                }

                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_subfiles)) throw std::runtime_error("Read error") ;

                for(uint32_t j=0;j<n_subfiles;++j)
                {
                    uint32_t fi = 0 ;
                    if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,fi)) throw std::runtime_error("Read error") ;
                    de->subfiles.push_back(fi) ;
                }
                mNodes[node_index] = de ;
            }
            else
                throw std::runtime_error("Unknown node section.") ;

            free(node_section_data) ;
        }
        free(buffer) ;
        return true ;
    }
    catch(std::exception& e)
    {
        std::cerr << "Error while reading: " << e.what() << std::endl;

        if(buffer != NULL)
            free(buffer) ;
        return false;
    }
}


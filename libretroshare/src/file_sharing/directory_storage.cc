#include "directory_storage.h"

class InternalFileHierarchyStorage
{
    class FileStorageNode
    {
    public:
        static const uint32_t TYPE_FILE = 0x0001 ;
        static const uint32_t TYPE_DIR  = 0x0002 ;

        virtual uint32_t type() const =0;
    };
    class FileEntry: public FileStorageNode
    {
    public:
        virtual uint32_t type() const { return FileStorageNode::TYPE_FILE ; }

    };

    class DirEntry: public FileStorageNode
    {
    public:
        virtual uint32_t type() const { return FileStorageNode::TYPE_DIR ; }

        std::set<DirectoryStorage::EntryIndex> subdirs ;
        std::set<DirectoryStorage::EntryIndex> subfiles ;
    };

    // file/dir access and modification
    bool findSubDirectory(DirectoryStorage::EntryIndex e,const std::string& s) const ;	// returns true when s is the name of a sub-directory in the given entry e

    uint32_t root ;

    std::vector<FileStorageNode*> mNodes;// uses pointers to keep information about valid/invalid objects.

    void compress() ;					// use empty space in the vector, mostly due to deleted entries.

    friend class DirectoryStorage ;		// only class that can use this.
};

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

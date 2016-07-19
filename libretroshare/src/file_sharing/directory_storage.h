#include <string>

static const uint8_t DIRECTORY_STORAGE_VERSION               =  0x01 ;

static const uint8_t DIRECTORY_STORAGE_TAG_FILE_HASH         =  0x01 ;
static const uint8_t DIRECTORY_STORAGE_TAG_FILE_HASH         =  0x01 ;
static const uint8_t DIRECTORY_STORAGE_TAG_FILE_NAME         =  0x02 ;
static const uint8_t DIRECTORY_STORAGE_TAG_FILE_SIZE         =  0x03 ;
static const uint8_t DIRECTORY_STORAGE_TAG_DIR_NAME          =  0x04 ;
static const uint8_t DIRECTORY_STORAGE_TAG_MODIF_TS          =  0x05 ;
static const uint8_t DIRECTORY_STORAGE_TAG_RECURS_MODIF_TS   =  0x06 ;

class DirectoryStorage
{
	public:
		DirectoryStorage(const std::string& local_file_name) ;

		void save() const ;

		virtual int searchTerms(const std::list<std::string>& terms, std::list<EntryIndex> &results) const;
		virtual int searchHash(const RsFileHash& hash, std::list<EntryIndex> &results) const;
		virtual int searchBoolExp(Expression * exp, std::list<EntryIndex> &results) const;

		void getFileDetails(EntryIndex i) ;

		// This class allows to abstractly browse the stored directory hierarchy in a depth-first manner.
		// It gives access to sub-files and sub-directories below.
		//
		class DirIterator
		{
			public:
				DirIterator(const DirectoryStorage& d) ;

				DirIterator& operator++() ;
				EntryIndex operator*() const ;		// current directory entry

				bool operator()() const ;				// used in for loops. Returns true when the iterator is valid.
		};
		class FileIterator
		{
			public:
				FileIterator(const DirectoryStorage& d) ;

				FileIterator& operator++() ;
				EntryIndex operator*() const ;		// current file entry

				bool operator()() const ;				// used in for loops. Returns true when the iterator is valid.
		};

	private:
		void load(const std::string& local_file_name) ;
		void save(const std::string& local_file_name) ;

		void loadNextTag(const void *data,uint32_t& offset,uint8_t& entry_tag,uint32_t& entry_size) ;
		void saveNextTag(void *data,uint32_t& offset,uint8_t entry_tag,uint32_t entry_size) ;
};

class RemoteDirectoryStorage: public DirectoryStorage
{
};

class LocalDirectoryStorage: public DirectoryStorage
{
};


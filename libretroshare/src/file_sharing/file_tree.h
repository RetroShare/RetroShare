#include "retroshare/rsfiles.h"

class FileTreeImpl: public FileTree
{
public:
	FileTreeImpl()
	{
		mTotalFiles = 0 ;
		mTotalSize = 0 ;
	}

	virtual std::string toRadix64() const ;
	virtual bool getDirectoryContent(uint32_t index,std::string& name,std::vector<uint32_t>& subdirs,std::vector<FileData>& subfiles) const ;
	virtual void print() const ;

	bool serialise(unsigned char *& data,uint32_t& data_size) const ;
	bool deserialise(unsigned char* data, uint32_t data_size) ;

protected:
	void recurs_print(uint32_t index,const std::string& indent) const;

	static void recurs_buildFileTree(FileTreeImpl& ft,uint32_t index,const DirDetails& dd,bool remote);

	struct DirData {
		std::string name;
		std::vector<uint32_t> subdirs ;
		std::vector<uint32_t> subfiles ;
	};
	std::vector<FileData> mFiles ;
	std::vector<DirData> mDirs ;

	friend class FileTree ;
};

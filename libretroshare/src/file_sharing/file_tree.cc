/*******************************************************************************
 * libretroshare/src/file_sharing: file_tree.cc                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
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
#include <iomanip>
#include <util/radix64.h>
#include <util/rsdir.h>

#include "file_sharing_defaults.h"
#include "filelist_io.h"
#include "file_tree.h"

std::string FileTreeImpl::toRadix64() const
{
	unsigned char *buff = NULL ;
	uint32_t size = 0 ;

	serialise(buff,size) ;

	std::string res ;

	Radix64::encode(buff,size,res) ;

	free(buff) ;
	return res ;
}

FileTree *FileTree::create(const std::string& radix64_string)
{
	FileTreeImpl *ft = new FileTreeImpl ;

	std::vector<uint8_t> mem = Radix64::decode(radix64_string);
	ft->deserialise(mem.data(),mem.size()) ;

	return ft ;
}

void FileTreeImpl::recurs_buildFileTree(FileTreeImpl& ft,uint32_t index,const DirDetails& dd,bool remote,bool remove_top_dirs)
{
	if(ft.mDirs.size() <= index)
		ft.mDirs.resize(index+1) ;

	if(remove_top_dirs)
		ft.mDirs[index].name = RsDirUtil::getTopDir(dd.name) ;
	else
		ft.mDirs[index].name = dd.name ;

	ft.mDirs[index].subfiles.clear();
	ft.mDirs[index].subdirs.clear();

	DirDetails dd2 ;

	FileSearchFlags flags = remote?RS_FILE_HINTS_REMOTE:RS_FILE_HINTS_LOCAL ;

	for(uint32_t i=0;i<dd.children.size();++i)
		if(rsFiles->RequestDirDetails(dd.children[i].ref,dd2,flags))
		{
			if(dd.children[i].type == DIR_TYPE_FILE)
			{
				FileTree::FileData f ;
				f.name = dd2.name ;
				f.size = dd2.count ;
				f.hash = dd2.hash ;

				ft.mDirs[index].subfiles.push_back(ft.mFiles.size()) ;
				ft.mFiles.push_back(f) ;

				ft.mTotalFiles++ ;
				ft.mTotalSize += f.size ;
			}
			else if(dd.children[i].type == DIR_TYPE_DIR)
			{
				ft.mDirs[index].subdirs.push_back(ft.mDirs.size());
				recurs_buildFileTree(ft,ft.mDirs.size(),dd2,remote,remove_top_dirs) ;
			}
			else
				std::cerr << "(EE) Unsupported DirDetails type." << std::endl;
		}
		else
			std::cerr << "(EE) Cannot request dir details for pointer " << dd.children[i].ref << std::endl;
}

bool FileTreeImpl::getDirectoryContent(uint32_t index,std::string& name,std::vector<uint32_t>& subdirs,std::vector<FileData>& subfiles) const
{
	if(index >= mDirs.size())
		return false ;

	name = mDirs[index].name;
	subdirs = mDirs[index].subdirs ;

	subfiles.clear() ;
	for(uint32_t i=0;i<mDirs[index].subfiles.size();++i)
		subfiles.push_back(mFiles[mDirs[index].subfiles[i]]);

	return true ;
}

FileTree *FileTree::create(const DirDetails& dd, bool remote,bool remove_top_dirs)
{
	FileTreeImpl *ft = new FileTreeImpl ;

	FileTreeImpl::recurs_buildFileTree(*ft,0,dd,remote,remove_top_dirs) ;

	return ft ;
}

typedef FileListIO::read_error read_error ;

bool FileTreeImpl::deserialise(unsigned char *buffer,uint32_t buffer_size)
{
    uint32_t buffer_offset = 0 ;

    mTotalFiles = 0;
    mTotalSize = 0;

    try
    {
        // Read some header

        uint32_t version,n_dirs,n_files ;

        if(!FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIRECTORY_VERSION,version)) throw read_error(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIRECTORY_VERSION) ;
        if(version != (uint32_t) FILE_LIST_IO_LOCAL_DIRECTORY_TREE_VERSION_0001) throw std::runtime_error("Wrong version number") ;

        if(!FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_files)) throw read_error(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_RAW_NUMBER) ;
        if(!FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_dirs))  throw read_error(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_RAW_NUMBER) ;

        // Write all file/dir entries

		mFiles.resize(n_files) ;
		mDirs.resize(n_dirs) ;

		unsigned char *node_section_data = NULL ;
		uint32_t node_section_size = 0 ;

        for(uint32_t i=0;i<mFiles.size() && buffer_offset < buffer_size;++i)	// only the 2nd condition really is needed. The first one ensures that the loop wont go forever.
        {
            uint32_t node_section_offset = 0 ;
#ifdef DEBUG_DIRECTORY_STORAGE
            std::cerr << "reading node " << i << ", offset " << buffer_offset << " : " << RsUtil::BinToHex(&buffer[buffer_offset],std::min((int)buffer_size - (int)buffer_offset,100)) << "..." << std::endl;
#endif

            if(FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_FILE_ENTRY,node_section_data,node_section_size))
            {
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,mFiles[i].name   )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_NAME     ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,mFiles[i].size   )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ) ;
                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,mFiles[i].hash   )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH) ;

                mTotalFiles++ ;
                mTotalSize += mFiles[i].size ;
            }
            else
                throw read_error(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_FILE_ENTRY) ;
		}

        for(uint32_t i=0;i<mDirs.size() && buffer_offset < buffer_size;++i)
		{
            uint32_t node_section_offset = 0 ;

            if(FileListIO::readField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIR_ENTRY,node_section_data,node_section_size))
            {
				DirData& de(mDirs[i]) ;

                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_NAME,de.name )) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_FILE_NAME      ) ;

                uint32_t n_subdirs = 0 ;
                uint32_t n_subfiles = 0 ;

                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_subdirs)) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER) ;

                for(uint32_t j=0;j<n_subdirs;++j)
                {
                    uint32_t di = 0 ;
                    if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,di)) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER) ;
                    de.subdirs.push_back(di) ;
                }

                if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,n_subfiles)) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER) ;

                for(uint32_t j=0;j<n_subfiles;++j)
                {
                    uint32_t fi = 0 ;
                    if(!FileListIO::readField(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,fi)) throw read_error(node_section_data,node_section_size,node_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER) ;
                    de.subfiles.push_back(fi) ;
                }
            }
            else
                throw read_error(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIR_ENTRY) ;
        }
		free(node_section_data) ;

        return true ;
    }
    catch(read_error& e)
    {
#ifdef DEBUG_DIRECTORY_STORAGE
        std::cerr << "Error while reading: " << e.what() << std::endl;
#endif
        return false;
    }
	return true ;
}

bool FileTreeImpl::serialise(unsigned char *& buffer,uint32_t& buffer_size) const
{
	buffer = 0 ;
    uint32_t buffer_offset = 0 ;

    unsigned char *tmp_section_data = (unsigned char*)rs_malloc(FL_BASE_TMP_SECTION_SIZE) ;

    if(!tmp_section_data)
        return false;

    uint32_t tmp_section_size = FL_BASE_TMP_SECTION_SIZE ;

    try
    {
        // Write some header

        if(!FileListIO::writeField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIRECTORY_VERSION,(uint32_t) FILE_LIST_IO_LOCAL_DIRECTORY_TREE_VERSION_0001)) throw std::runtime_error("Write error") ;
        if(!FileListIO::writeField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t) mFiles.size())) throw std::runtime_error("Write error") ;
        if(!FileListIO::writeField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t) mDirs.size())) throw std::runtime_error("Write error") ;

        // Write all file/dir entries

        for(uint32_t i=0;i<mFiles.size();++i)
		{
			const FileData& fe(mFiles[i]) ;

			uint32_t file_section_offset = 0 ;

			if(!FileListIO::writeField(tmp_section_data,tmp_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,fe.name        )) throw std::runtime_error("Write error") ;
			if(!FileListIO::writeField(tmp_section_data,tmp_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,fe.size        )) throw std::runtime_error("Write error") ;
			if(!FileListIO::writeField(tmp_section_data,tmp_section_size,file_section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,fe.hash        )) throw std::runtime_error("Write error") ;

			if(!FileListIO::writeField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_FILE_ENTRY,tmp_section_data,file_section_offset)) throw std::runtime_error("Write error") ;
		}

        for(uint32_t i=0;i<mDirs.size();++i)
		{
			const DirData& de(mDirs[i]) ;

			uint32_t dir_section_offset = 0 ;

			if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_FILE_NAME      ,de.name            )) throw std::runtime_error("Write error") ;
			if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)de.subdirs.size()))  throw std::runtime_error("Write error") ;

			for(uint32_t j=0;j<de.subdirs.size();++j)
				if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)de.subdirs[j]))  throw std::runtime_error("Write error") ;

			if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)de.subfiles.size())) throw std::runtime_error("Write error") ;

			for(uint32_t j=0;j<de.subfiles.size();++j)
				if(!FileListIO::writeField(tmp_section_data,tmp_section_size,dir_section_offset,FILE_LIST_IO_TAG_RAW_NUMBER,(uint32_t)de.subfiles[j])) throw std::runtime_error("Write error") ;

			if(!FileListIO::writeField(buffer,buffer_size,buffer_offset,FILE_LIST_IO_TAG_LOCAL_DIR_ENTRY,tmp_section_data,dir_section_offset))         throw std::runtime_error("Write error") ;
		}

        free(tmp_section_data) ;
		buffer_size = buffer_offset;

        return true ;
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

void FileTreeImpl::print() const
{
	std::cerr << "File hierarchy: name=" << mDirs[0].name << " size=" << mTotalSize << std::endl;
	recurs_print(0,"  ") ;
}

void FileTreeImpl::recurs_print(uint32_t index,const std::string& indent) const
{
	if(index >= mDirs.size())
	{
		std::cerr << "(EE) inconsistent FileTree structure" << std::endl;
		return;
	}
	std::cerr << indent << mDirs[index].name << std::endl;

	for(uint32_t i=0;i<mDirs[index].subdirs.size();++i)
		recurs_print(mDirs[index].subdirs[i],indent+"  ") ;

	for(uint32_t i=0;i<mDirs[index].subfiles.size();++i)
	{
		const FileData& fd(mFiles[mDirs[index].subfiles[i]]) ;

		std::cerr << indent << "  " << fd.hash << " " << std::setprecision(8) << fd.size << " " << fd.name << std::endl;
	}
}

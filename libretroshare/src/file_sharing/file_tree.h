/*
 * RetroShare C++ File lists IO methods.
 *
 *      file_sharing/file_tree.h
 *
 * Copyright 2017 by csoler
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare.project@gmail.com".
 *
 */

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

	static void recurs_buildFileTree(FileTreeImpl& ft, uint32_t index, const DirDetails& dd, bool remote, bool remove_top_dirs);

	struct DirData {
		std::string name;
		std::vector<uint32_t> subdirs ;
		std::vector<uint32_t> subfiles ;
	};
	std::vector<FileData> mFiles ;
	std::vector<DirData> mDirs ;

	friend class FileTree ;
};

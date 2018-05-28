/*******************************************************************************
 * libretroshare/src/file_sharing: file_tree.h                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 by Cyril Soler <csoler@users.sourceforge.net>                *
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

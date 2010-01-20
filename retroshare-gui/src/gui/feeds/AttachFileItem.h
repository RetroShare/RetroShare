/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef _ATTACH_FILE_ITEM_DIALOG_H
#define _ATTACH_FILE_ITEM_DIALOG_H

#include "ui_AttachFileItem.h"

#include <string>
#include <stdint.h>

const uint32_t AFI_MASK_STATE  		= 0x000f;
const uint32_t AFI_MASK_TYPE   		= 0x00f0;
const uint32_t AFI_MASK_FT     		= 0x0f00;

const uint32_t AFI_STATE_ERROR 		= 0x0001;
const uint32_t AFI_STATE_EXTRA 		= 0x0002;
const uint32_t AFI_STATE_REMOTE 	= 0x0003;
const uint32_t AFI_STATE_DOWNLOAD 	= 0x0004;
const uint32_t AFI_STATE_LOCAL 		= 0x0005;
const uint32_t AFI_STATE_UPLOAD 	= 0x0006;

const uint32_t AFI_TYPE_CHANNEL 	= 0x0010;
const uint32_t AFI_TYPE_ATTACH 		= 0x0020;

class AttachFileItem : public QWidget, private Ui::AttachFileItem
{
  Q_OBJECT

public:
  	/** Default Constructor */
  	AttachFileItem(std::string localpath);
    AttachFileItem(std::string hash, std::string name, uint64_t size,
					uint32_t flags, std::string srcId);

  	/** Default Destructor */

	std::string FileHash() { return mFileHash; }
	std::string FileName() { return mFileName; }
	uint64_t    FileSize() { return mFileSize; }
	std::string FilePath() { return mPath; }

	void updateItemStatic();

  bool done();
	bool ready();
	uint32_t getState();

public  slots:

private slots:

  void cancel();

	void updateItem();

private:

	void Setup();


	std::string mPath;
	std::string mFileHash;
	std::string mFileName;
	uint64_t    mFileSize;
	std::string mSrcId;

	uint32_t    mMode;
	uint32_t    mType;
	uint64_t    mDivisor;

	/* for display purposes */
	float amountDone;

signals:
		void fileFinished(AttachFileItem * AttachFileItem);

};



#endif


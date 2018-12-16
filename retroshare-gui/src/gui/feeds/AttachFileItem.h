/*******************************************************************************
 * gui/feeds/AttachFileItem.h                                                  *
 *                                                                             *
 * Copyright (c) 2008, Robert Fernie   <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef _ATTACH_FILE_ITEM_DIALOG_H
#define _ATTACH_FILE_ITEM_DIALOG_H

#include <retroshare/rsfiles.h>
#include "ui_AttachFileItem.h"
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
	AttachFileItem(const QString& localpath,TransferRequestFlags flags);
    AttachFileItem(const RsFileHash& hash, const QString& name, uint64_t size, uint32_t flags,TransferRequestFlags tflags, const std::string& srcId);

    const RsFileHash& FileHash() { return mFileHash; }
	const QString& FileName() { return mFileName; }
	uint64_t       FileSize() { return mFileSize; }
	const QString& FilePath() { return mPath; }

	void updateItemStatic();

	bool done();
	bool ready();
	uint32_t getState();

private slots:
	void cancel();
	void updateItem();

private:
	void Setup();

	QString    mPath;
    RsFileHash mFileHash;
	QString     mFileName;
	uint64_t    mFileSize;
	std::string mSrcId;

	uint32_t    mMode;
	uint32_t    mType;
	uint64_t    mDivisor;
	TransferRequestFlags mFlags ;

	/* for display purposes */
	float amountDone;

signals:
	void fileFinished(AttachFileItem * AttachFileItem);
};

#endif


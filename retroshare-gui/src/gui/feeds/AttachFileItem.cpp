/*******************************************************************************
 * gui/feeds/AttachFileItem.cpp                                                *
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

#include <QTimer>

#include "AttachFileItem.h"

#include <retroshare/rsfiles.h>

/****
 * #define DEBUG_ITEM 1
 ****/

/*******************************************************************
 * AttachFileItem fully controls the file transfer from the gui side
 *
 * Display: (In order)
 *
 * 1) HASHING
 * 2) REMOTE / DOWNLOAD
 * 3) LOCAL
 * 4) LOCAL / UPLOAD
 *
 * Behaviours:
 * a) Addition to General Dialog (1), (2), (3)
 *   (i) (1), request Hash -> (3).
 *   (ii) (2), download complete -> (3)
 *   (iii) (3) 
 *
 * b) Message/Blog/Channel (2), (3)
 *   (i) (2), download complete -> (3)
 *   (ii) (3)
 *
 * c) Transfers (2), (4)
 *   (i) (2)
 *   (ii) (3)
 *
 *
 */



const uint32_t AFI_DEFAULT_PERIOD 	= (30 * 3600 * 24); /* 30 Days */

/** Constructor */
AttachFileItem::AttachFileItem(const RsFileHash& hash, const QString& name, uint64_t size, uint32_t flags,TransferRequestFlags tflags, const std::string& srcId)
:QWidget(NULL), mFileHash(hash), mFileName(name), mFileSize(size), mSrcId(srcId),mFlags(tflags)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

	mMode = flags & AFI_MASK_STATE;
	mType = flags & AFI_MASK_TYPE;

	if (mMode == AFI_STATE_EXTRA)
	{
		mMode = AFI_STATE_ERROR;
	}
	/**** Enable ****
	*****/

	/* all other states are possible */

	if (!rsFiles) 
	{
		mMode = AFI_STATE_ERROR;
	}

	Setup();
}

/** Constructor */
AttachFileItem::AttachFileItem(const QString& path,TransferRequestFlags flags)
:QWidget(NULL), mPath(path), mFileSize(0),mFlags(flags)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);

	mMode = AFI_STATE_EXTRA;
	mType = AFI_TYPE_ATTACH;

	/* ask for Files to hash/prepare it for us */
	if ((!rsFiles) || (!rsFiles->ExtraFileHash(path.toUtf8().constData(), AFI_DEFAULT_PERIOD, flags)))
	{
		mMode = AFI_STATE_ERROR;
	}

	Setup();
}

void AttachFileItem::Setup()
{
  connect( cancelButton, SIGNAL( clicked( void ) ), this, SLOT( cancel ( void ) ) );

  /* once off check - if remote, check if we have it 
   * NB: This check might be expensive - and it'll happen often!
   */
#ifdef DEBUG_ITEM
  std::cerr << "AttachFileItem::Setup(): " << mFileName;
  std::cerr << std::endl;
#endif

  if (mMode == AFI_STATE_REMOTE)
  {
	FileInfo fi;
	FileSearchFlags hintflags = RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_SPEC_ONLY;

	/* look up path */
	if (rsFiles->FileDetails(mFileHash, hintflags, fi))
	{
#ifdef DEBUG_ITEM
		std::cerr << "AttachFileItem::Setup() STATE=>Local Found File";
		std::cerr << std::endl;
		std::cerr << "AttachFileItem::Setup() path: " << fi.path;
		std::cerr << std::endl;
#endif
		mMode = AFI_STATE_LOCAL;
		mPath = QString::fromUtf8(fi.path.c_str());
	}
  }

  updateItemStatic();
  updateItem();
}


bool AttachFileItem::done()
{
	return (mMode >= AFI_STATE_LOCAL);
}

bool AttachFileItem::ready()
{
	return (mMode >= AFI_STATE_REMOTE);
}

void AttachFileItem::updateItemStatic()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "AttachFileItem::updateItemStatic(): " << mFileName;
	std::cerr << std::endl;
#endif

	QString filename = mFileName;
	mDivisor = 1;

	if (mFileSize > 10000000) /* 10 Mb */
	{
		progressBar->setRange(0, mFileSize / 1000000);
		progressBar->setFormat("%v MB");
		mDivisor = 1000000;
	}
	else if (mFileSize > 10000) /* 10 Kb */
	{
		progressBar->setRange(0, mFileSize / 1000);
		progressBar->setFormat("%v kB");
		mDivisor = 1000;
	}
	else 
	{
		progressBar->setRange(0, mFileSize);
		progressBar->setFormat("%v B");
		mDivisor = 1;
	}

	/* get full path for local file */
	if ((mMode == AFI_STATE_LOCAL) || (mMode == AFI_STATE_UPLOAD))
	{
#ifdef DEBUG_ITEM
		std::cerr << "AttachFileItem::updateItemStatic() STATE=Local/Upload checking path";
		std::cerr << std::endl;
#endif
		if (mPath == "")
		{
			FileInfo fi;
			FileSearchFlags hintflags = RS_FILE_HINTS_UPLOAD | RS_FILE_HINTS_LOCAL
								| RS_FILE_HINTS_SPEC_ONLY;

			/* look up path */
			if (!rsFiles->FileDetails(mFileHash, hintflags, fi))
			{
				mMode = AFI_STATE_ERROR;
#ifdef DEBUG_ITEM
			std::cerr << "AttachFileItem::updateItemStatic() STATE=>Error No Details";
			std::cerr << std::endl;
#endif
			}
			else
			{
#ifdef DEBUG_ITEM
			std::cerr << "AttachFileItem::updateItemStatic() Updated Path";
			std::cerr << std::endl;
#endif
				mPath = QString::fromUtf8(fi.path.c_str());
			}
		}
	}

	/* do buttons + display */
	switch (mMode)
	{
		case AFI_STATE_ERROR:
			progressBar->setRange(0, 100);
			progressBar->setFormat("ERROR");

			cancelButton->setEnabled(false);
		
			progressBar->setValue(0);
			filename = tr("[ERROR])") + " " + filename;

			break;

		case AFI_STATE_EXTRA:
			filename = mPath;

			progressBar->setRange(0, 100);
			progressBar->setFormat("HASHING");

			cancelButton->setEnabled(false);

			progressBar->setValue(0);
			filename = "[EXTRA] " + filename;

			break;

		case AFI_STATE_REMOTE:
			cancelButton->setEnabled(false);

			progressBar->setValue(0);
			filename = "[REMOTE] " + filename;

			break;

		case AFI_STATE_DOWNLOAD:
			cancelButton->setEnabled(true);
			filename = "[DOWNLOAD] " + filename;

			break;

		case AFI_STATE_LOCAL:
			cancelButton->setEnabled(false);

			progressBar->setValue(mFileSize / mDivisor);
			filename = "[LOCAL] " + filename;

			break;

		case AFI_STATE_UPLOAD:
			cancelButton->setEnabled(false);
			filename = "[UPLOAD] " + filename;

			break;
	}


	switch(mType)
	{
		case AFI_TYPE_CHANNEL:
		{
			if (mMode == AFI_STATE_LOCAL)
			{
			}
			else
			{
			}
		}
			break;
		case AFI_TYPE_ATTACH:
		{
			cancelButton->setEnabled(true);
			cancelButton->setToolTip("Remove Attachment");
		}
			break;
		default:
			break;
	}

	fileLabel->setText(filename);
	fileLabel->setToolTip(filename);
}

void AttachFileItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "AttachFileItem::updateItem():" << mFileName;
	std::cerr << std::endl;
#endif

	/* Extract File Details */
	/* Update State if necessary */

	FileInfo fi;
	bool stateChanged = false;
	int msec_rate = 1000;

	if ((mMode == AFI_STATE_ERROR) || (mMode == AFI_STATE_LOCAL))
	{
#ifdef DEBUG_ITEM
		std::cerr << "AttachFileItem::updateItem() STATE=Local/Error ignore";
		std::cerr << std::endl;
#endif
		/* ignore - dead file, or done */
	}
	else if (mMode == AFI_STATE_EXTRA)
	{
#ifdef DEBUG_ITEM
		std::cerr << "AttachFileItem::updateItem() STATE=Extra File";
		std::cerr << std::endl;
#endif
		/* check for file status */
		if (rsFiles->ExtraFileStatus(mPath.toUtf8().constData(), fi))
		{
#ifdef DEBUG_ITEM
			std::cerr << "AttachFileItem::updateItem() STATE=>Local";
			std::cerr << std::endl;
#endif
			mMode = AFI_STATE_LOCAL;

			/* fill in file details */
			mFileName = QString::fromUtf8(fi.fname.c_str());
			mFileSize = fi.size;
			mFileHash = fi.hash;

			/* have path already! */

			stateChanged = true;
		}
	}
	else
	{
		FileSearchFlags hintflags(0u);
		switch(mMode)
		{
			case AFI_STATE_REMOTE:
				hintflags = RS_FILE_HINTS_DOWNLOAD | RS_FILE_HINTS_SPEC_ONLY;
				break;
			case AFI_STATE_DOWNLOAD:
				hintflags = RS_FILE_HINTS_DOWNLOAD | RS_FILE_HINTS_SPEC_ONLY;
				break;
			case AFI_STATE_UPLOAD:
				hintflags = RS_FILE_HINTS_UPLOAD | RS_FILE_HINTS_SPEC_ONLY;
				break;
		}

		bool detailsOk = rsFiles->FileDetails(mFileHash, hintflags, fi);

		/* have details - see if state has changed */
		switch(mMode)
		{
			case AFI_STATE_REMOTE:
#ifdef DEBUG_ITEM
				std::cerr << "AttachFileItem::updateItem() STATE=Remote";
				std::cerr << std::endl;
#endif
				/* is it downloading? */
				if (detailsOk)
				{
#ifdef DEBUG_ITEM
					std::cerr << "AttachFileItem::updateItem() STATE=>Download";
					std::cerr << std::endl;
#endif
					/* downloading */
					mMode = AFI_STATE_DOWNLOAD;
					stateChanged = true;
				}
				break;
			case AFI_STATE_DOWNLOAD:
#ifdef DEBUG_ITEM
					std::cerr << "AttachFileItem::updateItem() STATE=Download";
					std::cerr << std::endl;
#endif

				if (!detailsOk)
				{
#ifdef DEBUG_ITEM
					std::cerr << "AttachFileItem::updateItem() STATE=>Remote";
					std::cerr << std::endl;
#endif
					mMode = AFI_STATE_REMOTE;
					stateChanged = true;
				}
				else
				{
					/* has it completed? */
					if (fi.avail == mFileSize)
					{
#ifdef DEBUG_ITEM
						std::cerr << "AttachFileItem::updateItem() STATE=>Local";
						std::cerr << std::endl;
#endif
						/* save path */
						/* update progress */
						mMode = AFI_STATE_LOCAL;
						mPath = QString::fromUtf8(fi.path.c_str());
						stateChanged = true;
					}
					progressBar->setValue(fi.avail / mDivisor);
				}
				break;
			case AFI_STATE_UPLOAD:
#ifdef DEBUG_ITEM
				std::cerr << "AttachFileItem::updateItem() STATE=Upload";
				std::cerr << std::endl;
#endif

				if (detailsOk)
				{
					progressBar->setValue(fi.avail / mDivisor);
				}

				/* update progress */
				break;
		}
	}

	/****** update based on new state ******/
	if (stateChanged)
	{
		updateItemStatic();
	}

	uint32_t repeat = 0;

	switch (mMode)
	{
		case AFI_STATE_ERROR:
			repeat = 0;
			break;

		case AFI_STATE_EXTRA:
			repeat = 1;
			msec_rate = 5000; /* slow */
			break;

		case AFI_STATE_REMOTE:
			repeat = 1;
			msec_rate = 30000; /* very slow */
			break;

		case AFI_STATE_DOWNLOAD:
			repeat = 1;
			msec_rate = 2000; /* should be download rate dependent */
			break;

		case AFI_STATE_LOCAL:
			repeat = 0;
			emit fileFinished(this);
			hide(); // auto hide
			break;

		case AFI_STATE_UPLOAD:
			repeat = 1;
			msec_rate = 2000; /* should be download rate dependent */
			break;
	}
	
	if (repeat)
	{
#ifdef DEBUG_ITEM
		std::cerr << "AttachFileItem::updateItem() callback for update!";
		std::cerr << std::endl;
#endif
	  	QTimer::singleShot( msec_rate, this, SLOT(updateItem( void ) ));
	}
}

void AttachFileItem::cancel()
{
#ifdef DEBUG_ITEM
	std::cerr << "AttachFileItem::cancel()";
	std::cerr << std::endl;
#endif
	//set the state to error mode
	mMode = AFI_STATE_ERROR;

	/* Only occurs - if it is downloading */
	if (mType == AFI_TYPE_ATTACH)
	{
		hide();
	}
	else
	{
		rsFiles->FileCancel(mFileHash);
	}
}

uint32_t AttachFileItem::getState()
{
    return mMode;
}

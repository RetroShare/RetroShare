/*******************************************************************************
 * gui/feeds/SubFileItem.cpp                                                   *
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
#include <QMessageBox>
#include <QFileDialog>
#include <QUrl>
#include <QMovie>

#include "SubFileItem.h"

#include <gui/common/RsUrlHandler.h>
#include <retroshare/rsfiles.h>
#include "util/misc.h"
#include "gui/RetroShareLink.h"

//#include <retroshare/rschannels.h>

/****
 * #define DEBUG_ITEM 1
 ****/

/*******************************************************************
 * SubFileItem fully controls the file transfer from the gui side
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



//const uint32_t SFI_DEFAULT_PERIOD 	= (30 * 3600 * 24); /* 30 Days */

/** Constructor */
SubFileItem::SubFileItem(const RsFileHash &hash, const std::string &name, const std::string &path, uint64_t size, uint32_t flags, const RsPeerId &srcId)
:QWidget(NULL), mPath(path), mFileHash(hash), mFileName(name), mFileSize(size), mSrcId(srcId)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

	mMode = flags & SFI_MASK_STATE;
	mType = flags & SFI_MASK_TYPE;
	mFlag = flags & SFI_MASK_FLAG;

	/**** Enable ****
	*****/

	/* all other states are possible */

	if (!rsFiles) 
	{
		mMode = SFI_STATE_ERROR;
	}

	Setup();
}


void SubFileItem::Setup()
{
  connect( playButton, SIGNAL( clicked( ) ), this, SLOT( play ( ) ) );
  connect( downloadButton, SIGNAL( clicked( ) ), this, SLOT( download ( ) ) );
  connect( cancelButton, SIGNAL( clicked( ) ), this, SLOT( cancel( ) ) );
  connect( deleteButton, SIGNAL( clicked( ) ), this, SLOT( del( ) ) );
  connect( copyLinkButton, SIGNAL( clicked( ) ), this, SLOT( copyLink ( ) ) );
  connect( saveButton, SIGNAL( clicked( ) ), this, SLOT( save ( ) ) );

  /* once off check - if remote, check if we have it 
   * NB: This check might be expensive - and it'll happen often!
   */
#ifdef DEBUG_ITEM
  std::cerr << "SubFileItem::Setup(): " << mFileName;
  std::cerr << std::endl;
#endif

  if (mMode == SFI_STATE_REMOTE)
  {
	FileInfo fi;

	/* look up path */
	if (rsFiles->alreadyHaveFile(mFileHash, fi))
	{
#ifdef DEBUG_ITEM
		std::cerr << "SubFileItem::Setup() STATE=>Local Found File";
		std::cerr << std::endl;
		std::cerr << "SubFileItem::Setup() path: " << fi.path;
		std::cerr << std::endl;
#endif
		mMode = SFI_STATE_LOCAL;
		mPath = fi.path;
	}
  }

  deleteButton->setVisible(mFlag & SFI_FLAG_ALLOW_DELETE);
  downloadButton->setVisible(mMode < SFI_STATE_LOCAL);
  cancelButton->setVisible(mMode < SFI_STATE_LOCAL);

  smaller();
  updateItemStatic();
  updateItem();
}


void SubFileItem::del()
{
    emit wantsToBeDeleted();
}

bool SubFileItem::done()
{
	return (mMode >= SFI_STATE_LOCAL);
}

bool SubFileItem::ready()
{
	return (mMode >= SFI_STATE_REMOTE);
}

void SubFileItem::updateItemStatic()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "SubFileItem::updateItemStatic(): " << mFileName;
	std::cerr << std::endl;
#endif

	QString filename = QString::fromUtf8(mFileName.c_str());
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
	if (((mMode == SFI_STATE_LOCAL) || (mMode == SFI_STATE_UPLOAD)))
	{
#ifdef DEBUG_ITEM
		std::cerr << "SubFileItem::updateItemStatic() STATE=Local/Upload checking path";
		std::cerr << std::endl;
#endif
		if (mPath == "")
		{
			FileInfo fi;
			FileSearchFlags hintflags = RS_FILE_HINTS_UPLOAD | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_SPEC_ONLY | RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_BROWSABLE;

			/* look up path */
			if (!rsFiles->FileDetails(mFileHash, hintflags, fi))
			{
				if(mFlag & SFI_FLAG_ASSUME_FILE_READY)
					mMode = SFI_STATE_REMOTE;
				else
					mMode = SFI_STATE_ERROR;
#ifdef DEBUG_ITEM
			std::cerr << "SubFileItem::updateItemStatic() STATE=>Error No Details";
			std::cerr << std::endl;
#endif
			}
			else
			{
#ifdef DEBUG_ITEM
			std::cerr << "SubFileItem::updateItemStatic() Updated Path";
			std::cerr << std::endl;
#endif
				mPath = fi.path;
			}
		}
	}

	/* do buttons + display */
	switch (mMode)
	{
		case SFI_STATE_ERROR:
			progressBar->setRange(0, 100);
			progressBar->setFormat(tr("ERROR"));

			playButton->setEnabled(false);
			downloadButton->setEnabled(false);
			cancelButton->setEnabled(false);

			progressBar->setValue(0);
			filename = "[" + tr("ERROR") + "] " + filename;

			break;

		case SFI_STATE_EXTRA:
			filename = QString::fromUtf8(mPath.c_str());

			progressBar->setRange(0, 100);
			progressBar->setFormat("HASHING");

			playButton->setEnabled(false);
			downloadButton->setEnabled(false);
			cancelButton->setEnabled(false);

			progressBar->setValue(0);
			filename = "[" + tr("EXTRA") + "] " + filename;

			break;

		case SFI_STATE_REMOTE:
			playButton->setEnabled(false);
			downloadButton->setEnabled(true);
			downloadButton->setVisible(true);
			cancelButton->setEnabled(false);

			progressBar->setValue(0);
			filename = "[" + tr("REMOTE") + "] " + filename + " (" + misc::friendlyUnit(mFileSize) + ")";

			break;

		case SFI_STATE_DOWNLOAD:
			playButton->setEnabled(false);
			downloadButton->setEnabled(false);
			cancelButton->setEnabled(true);
			filename = "[" + tr("DOWNLOAD") + "] " + filename;

			break;

		case SFI_STATE_LOCAL:
			playButton->setEnabled(true);
			downloadButton->setEnabled(false);
			cancelButton->setEnabled(false);

			progressBar->setValue(mFileSize / mDivisor);
			filename = "[" + tr("LOCAL") + "] " + filename + " (" + misc::friendlyUnit(mFileSize) + ")";

			break;

		case SFI_STATE_UPLOAD:
			playButton->setEnabled(true);
			downloadButton->setEnabled(false);
			cancelButton->setEnabled(false);
			filename = "[" + tr("UPLOAD") + "] " + filename;

			break;
	}

	saveButton->hide();

	switch(mType)
	{
		case SFI_TYPE_CHANNEL:
		{
			saveButton->show();
			if (mMode == SFI_STATE_LOCAL)
			{
				saveButton->setEnabled(true);
			}
			else
			{
				saveButton->setEnabled(false);
			}
			if (mFlag & SFI_FLAG_CREATE) {
				cancelButton->setEnabled(true); // channel files which are extra files are removed
				cancelButton->setToolTip(tr("Remove Attachment"));
			}
		}
			break;
		case SFI_TYPE_ATTACH:
		{
			playButton->hide();
			downloadButton->hide();
			cancelButton->setEnabled(true);
			cancelButton->setToolTip(tr("Remove Attachment"));
		}
			break;
		default:
			break;
	}

	fileLabel->setText(filename);
	fileLabel->setToolTip(filename);
}

void SubFileItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "SubFileItem::updateItem():" << mFileName;
	std::cerr << std::endl;
#endif

	/* Extract File Details */
	/* Update State if necessary */

	FileInfo fi;
	bool stateChanged = false;
	int msec_rate = 1000;

	if ((mMode == SFI_STATE_ERROR) || (mMode == SFI_STATE_LOCAL))
	{
#ifdef DEBUG_ITEM
		std::cerr << "SubFileItem::updateItem() STATE=Local/Error ignore";
		std::cerr << std::endl;
#endif
		/* ignore - dead file, or done */

		if (mMode == SFI_STATE_ERROR) {
			/* updateStatic once */
			stateChanged = true;
		}
	}
	else if (mMode == SFI_STATE_EXTRA)
	{
#ifdef DEBUG_ITEM
		std::cerr << "SubFileItem::updateItem() STATE=Extra File";
		std::cerr << std::endl;
#endif
		/* check for file status */
		if (rsFiles->ExtraFileStatus(mPath, fi))
		{
#ifdef DEBUG_ITEM
			std::cerr << "SubFileItem::updateItem() STATE=>Local";
			std::cerr << std::endl;
#endif
			mMode = SFI_STATE_LOCAL;

			/* fill in file details */
			mFileName = fi.fname;
			mFileSize = fi.size;
			mFileHash = fi.hash;

			/* have path already! */

			stateChanged = true;
		}
	}
	else
	{
		FileSearchFlags hintflags(0u) ;
		switch(mMode)
		{
			case SFI_STATE_REMOTE:
				hintflags = RS_FILE_HINTS_DOWNLOAD | RS_FILE_HINTS_SPEC_ONLY;
				break;
			case SFI_STATE_DOWNLOAD:
				hintflags = RS_FILE_HINTS_DOWNLOAD | RS_FILE_HINTS_SPEC_ONLY;
				break;
			case SFI_STATE_UPLOAD:
				hintflags = RS_FILE_HINTS_UPLOAD | RS_FILE_HINTS_SPEC_ONLY;
				break;
		}

		bool detailsOk = rsFiles->FileDetails(mFileHash, hintflags, fi);

		/* have details - see if state has changed */
		switch(mMode)
		{
			case SFI_STATE_REMOTE:
#ifdef DEBUG_ITEM
				std::cerr << "SubFileItem::updateItem() STATE=Remote";
				std::cerr << std::endl;
#endif
				/* is it downloading? */
				if (detailsOk)
				{
#ifdef DEBUG_ITEM
					std::cerr << "SubFileItem::updateItem() STATE=>Download";
					std::cerr << std::endl;
#endif
					/* downloading */
					mMode = SFI_STATE_DOWNLOAD;
					stateChanged = true;
				}
				break;
			case SFI_STATE_DOWNLOAD:
#ifdef DEBUG_ITEM
					std::cerr << "SubFileItem::updateItem() STATE=Download";
					std::cerr << std::endl;
#endif

				if (!detailsOk)
				{
#ifdef DEBUG_ITEM
					std::cerr << "SubFileItem::updateItem() STATE=>Remote";
					std::cerr << std::endl;
#endif
					mMode = SFI_STATE_REMOTE;
					stateChanged = true;
				}
				else
				{
					/* has it completed? */
					if (fi.avail == mFileSize)
					{
#ifdef DEBUG_ITEM
						std::cerr << "SubFileItem::updateItem() STATE=>Local";
						std::cerr << std::endl;
#endif
						/* save path */
						/* update progress */
						mMode = SFI_STATE_LOCAL;
						mPath = fi.path;
						stateChanged = true;
					}
					progressBar->setValue(fi.avail / mDivisor);
				}
				break;
			case SFI_STATE_UPLOAD:
#ifdef DEBUG_ITEM
				std::cerr << "SubFileItem::updateItem() STATE=Upload";
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
		case SFI_STATE_ERROR:
			repeat = 0;
			break;

		case SFI_STATE_EXTRA:
			repeat = 1;
			msec_rate = 5000; /* slow */
			break;

		case SFI_STATE_REMOTE:
			repeat = 1;
			msec_rate = 30000; /* very slow */
			break;

		case SFI_STATE_DOWNLOAD:
			repeat = 1;
			msec_rate = 2000; /* should be download rate dependent */
			break;

		case SFI_STATE_LOCAL:
			repeat = 0;
			emit fileFinished(this);
			break;

		case SFI_STATE_UPLOAD:
			repeat = 1;
			msec_rate = 2000; /* should be download rate dependent */
			break;
	}

	
	if (repeat)
	{
#ifdef DEBUG_ITEM
		std::cerr << "SubFileItem::updateItem() callback for update!";
		std::cerr << std::endl;
#endif
	  	QTimer::singleShot( msec_rate, this, SLOT(updateItem( void ) ));
	}

}




void SubFileItem::smaller()
{
#ifdef DEBUG_ITEM
	std::cerr << "SubFileItem::cancel()";
	std::cerr << std::endl;
#endif

#if 0
	expandFrame->hide();
#endif
}

void SubFileItem::toggle()
{
#if 0
	if (expandFrame->isHidden())
	{
		expandFrame->show();
	}
	else
	{
		expandFrame->hide();
	}
#endif
}

void SubFileItem::cancel()
{
#ifdef DEBUG_ITEM
	std::cerr << "SubFileItem::cancel()";
	std::cerr << std::endl;
#endif
	//set the state to error mode
	mMode = SFI_STATE_ERROR;

	/* Only occurs - if it is downloading */
	if (((mType == SFI_TYPE_ATTACH) || (mType == SFI_TYPE_CHANNEL)) && (mFlag & SFI_FLAG_CREATE))
	{
		rsFiles->ExtraFileRemove(FileHash());//, RS_FILE_REQ_ANONYMOUS_ROUTING | RS_FILE_REQ_EXTRA);
		mPath = "";
        del();
        return;	// do not update!
	}
	else
	{
		rsFiles->FileCancel(mFileHash);
	}

	updateItem();
}


void SubFileItem::play()
{
	FileInfo info;
	FileSearchFlags flags = RS_FILE_HINTS_DOWNLOAD | RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_NETWORK_WIDE;


	if (!rsFiles->FileDetails( mFileHash, flags, info))
		return;

	if (done()) {

		/* open file with a suitable application */
		QFileInfo qinfo;
		qinfo.setFile(info.path.c_str());
		if (qinfo.exists()) {
			if (!RsUrlHandler::openUrl(QUrl::fromLocalFile(qinfo.absoluteFilePath()))) {
				std::cerr << "openTransfer(): can't open file " << info.path << std::endl;
			}
		}else{
			QMessageBox::information(this, tr("Play File"),
					tr("File %1 does not exist at location.").arg(info.path.c_str()));
			return;
		}
	} else {
		/* rise a message box for incompleted download file */
		QMessageBox::information(this, tr("Play File"),
				tr("File %1 is not completed.").arg(info.fname.c_str()));
		return;
	}

}

void SubFileItem::download()
{
#ifdef DEBUG_ITEM
	std::cerr << "SubFileItem::download()";
	std::cerr << std::endl;
#endif

	std::list<RsPeerId> sources ;

	std::string destination;
#if 0
	if (!mChannelId.empty() && mType == SFI_TYPE_CHANNEL) {
		ChannelInfo ci;
		if (rsChannels->getChannelInfo(mChannelId, ci)) {
			destination = ci.destination_directory;
		}
	}
#endif
	

	// Add possible direct sources.
	//
	FileInfo finfo ;
	rsFiles->FileDetails(mFileHash,RS_FILE_HINTS_REMOTE,finfo) ;

	for(std::vector<TransferInfo>::const_iterator it(finfo.peers.begin());it!=finfo.peers.end();++it)
		sources.push_back((*it).peerId) ;

	// TEMP
	std::cerr << "SubFileItem::download() Calling File Request";
	std::cerr << std::endl;

	if (!mSrcId.isNull())
		sources.push_back(mSrcId);
	
	rsFiles->FileRequest(mFileName, mFileHash, mFileSize, destination, RS_FILE_REQ_ANONYMOUS_ROUTING, sources);

	downloadButton->setEnabled(false);

	updateItem();
}


void SubFileItem::save()
{
#ifdef DEBUG_ITEM
	std::cerr << "SubFileItem::save()";
	std::cerr << std::endl;
#endif

	FileInfo fInfo;

	if (mType == SFI_TYPE_CHANNEL)
	{
		/* only enable these function for Channels. */

		/* find out where they want to save it */
		QString startpath = "";
 		QString dir = QFileDialog::getExistingDirectory(this, tr("Save Channel File"),
						 startpath,
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

		std::string destpath = dir.toStdString();

		if (destpath != "")
		{
			bool copied = rsFiles->ExtraFileMove(mFileName, mFileHash, mFileSize, destpath);

			// may be manually downloaded channel file
			if(!copied){
				rsFiles->FileDetails(mFileHash, RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_EXTRA, fInfo);

				if(fInfo.path != "")
				{

					destpath += "/" + fInfo.fname;
					rsFiles->copyFile(fInfo.path, destpath);
				}
			}
		}
	}
	else
	{
	}
}

uint32_t SubFileItem::getState() {
    return mMode;
}

bool SubFileItem::isDownloadable(bool &startable)
{
	/* Check buttons. Not good, but it works. */
	bool visible = downloadButton->isVisibleTo(this);
	startable = visible && downloadButton->isEnabled();

	return visible;
}

bool SubFileItem::isPlayable(bool &startable)
{
	/* Check buttons. Not good, but it works. */
	bool visible = playButton->isVisibleTo(this);
	startable = visible && playButton->isEnabled();
	loadpicture();
	
	return visible;
}

void SubFileItem::mediatype()
{
	/* check if the file is a media file and change text */
	playButton->setText(tr("Play"));
	playButton->setToolTip(tr("Play Media"));
	playButton->setIcon(QIcon(":/icons/png/play.png"));
}

void SubFileItem::nomediatype()
{
	/* check if the file is not a media file and change text */
	playButton->setText(tr("Open"));
	playButton->setToolTip(tr("Open File"));
	playButton->setIcon(QIcon());
}

void SubFileItem::picturetype()
{
	/* check if the file is not a picture & hide it */
	imageFrame->hide();
}

void SubFileItem::copyLink()
{
    if (mFileName.empty() || mFileHash.isNull()) {
		return;
	}

	RetroShareLink link = RetroShareLink::createFile(QString::fromUtf8(mFileName.c_str()), mFileSize, QString::fromStdString(mFileHash.toStdString()));
	if (link.valid()) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
	}
}

void SubFileItem::loadpicture()
{
	FileInfo info;
	FileSearchFlags flags = RS_FILE_HINTS_DOWNLOAD | RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_NETWORK_WIDE;


	if (!rsFiles->FileDetails( mFileHash, flags, info))
		return;

	if (done()) {

		/* display the picture into the label */
		QFileInfo qinfo;
		qinfo.setFile(info.path.c_str());
		QString extension = qinfo.suffix().toUpper(); 
		QImageReader reader(qinfo.absoluteFilePath());
		int count = reader.imageCount();
		
		if (qinfo.exists()) {
			
			if((extension == "GIF" || extension == "WEBP" ) && count > 1){
				movie = new QMovie(qinfo.absoluteFilePath());
				//imageLabel->setAttribute(Qt::WA_NoSystemBackground);
				imageLabel->setMovie(movie);
				movie->start();
			}else{
				QPixmap pixmap = QPixmap(qinfo.absoluteFilePath());
				QPixmap scaledpixmap = pixmap.scaled(640,480, Qt::KeepAspectRatio, Qt::SmoothTransformation);

				imageLabel->setPixmap(scaledpixmap);
			}
		}
	}
}

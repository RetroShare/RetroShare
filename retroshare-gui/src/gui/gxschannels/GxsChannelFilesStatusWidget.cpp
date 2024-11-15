/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelFilesStatusWidget.cpp          *
 *                                                                             *
 * Copyright 2014 by Retroshare Team   <retroshare.project@gmail.com>          *
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

#include <QMenu>
#include <QTimer>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QUrl>

#include "GxsChannelFilesStatusWidget.h"
#include "ui_GxsChannelFilesStatusWidget.h"
#include "gui/common/RsUrlHandler.h"
#include "gui/common/FilesDefs.h"
#include "util/misc.h"

#include "retroshare/rsfiles.h"

GxsChannelFilesStatusWidget::GxsChannelFilesStatusWidget(const RsGxsFile &file, QWidget *parent,bool used_as_editor) :
    QWidget(parent), mFile(file), mUsedAsEditor(used_as_editor),ui(new Ui::GxsChannelFilesStatusWidget)
{
	ui->setupUi(this);

	mState = STATE_REMOTE;

	setSize(mFile.mSize);

	/* Connect signals */
	connect(ui->downloadPushButton, SIGNAL(clicked()), this, SLOT(download()));
	connect(ui->resumeToolButton, SIGNAL(clicked()), this, SLOT(resume()));
	connect(ui->pauseToolButton, SIGNAL(clicked()), this, SLOT(pause()));
    connect(ui->cancelToolButton, SIGNAL(clicked()), this, SLOT(cancel()));
	connect(ui->openFilePushButton, SIGNAL(clicked()), this, SLOT(openFile()));

	ui->downloadPushButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/download.png"));

    ui->openFolderPushButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/folderopen.png"));
    ui->openFolderPushButton->setToolTip(tr("Open folder"));

    connect(ui->openFolderPushButton, SIGNAL(clicked()), this, SLOT(openFolder()));
	
	check();
}

GxsChannelFilesStatusWidget::~GxsChannelFilesStatusWidget()
{
	delete ui;
}

void GxsChannelFilesStatusWidget::setSize(uint64_t size)
{
	mDivisor = 1;

	if (size > 10000000) {
		/* 10 Mb */
		ui->progressBar->setFormat("%v MB");
		mDivisor = 1000000;
	} else if (size > 10000) {
		/* 10 Kb */
		ui->progressBar->setFormat("%v kB");
		mDivisor = 1000;
	} else {
		ui->progressBar->setFormat("%v B");
		mDivisor = 1;
	}

	ui->progressBar->setRange(0, size / mDivisor);
}

void GxsChannelFilesStatusWidget::check()
{
	FileInfo fileInfo;

    if(haveFile(fileInfo))
    {
		mState = STATE_LOCAL;
		setSize(fileInfo.size);
		
		/* check if the file is a media file */
		if (!misc::isPreviewable(QFileInfo(QString::fromUtf8(fileInfo.path.c_str())).suffix()))
		{ 
			/* check if the file is not a media file and change text */
			ui->openFilePushButton->setText(tr("Open file"));
		} else {
			ui->openFilePushButton->setText(tr("Play"));
			ui->openFilePushButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/play.png"));
		}
    }
    else
    {
        switch (fileInfo.downloadStatus)
        {
			case FT_STATE_WAITING:
				mState = STATE_WAITING;
				break;
			case FT_STATE_DOWNLOADING:
                if (fileInfo.avail == fileInfo.size)
					mState = STATE_LOCAL;
                else
					mState = STATE_DOWNLOAD;

				setSize(fileInfo.size);
				ui->progressBar->setValue(fileInfo.avail / mDivisor);
				break;
            case FT_STATE_COMPLETE:		// this should not happen, since the case is handled earlier
                mState = STATE_ERROR;
				break;
			case FT_STATE_QUEUED:
				mState = STATE_WAITING;
				break;
			case FT_STATE_PAUSED:
				mState = STATE_PAUSED;
				break;
			case FT_STATE_CHECKING_HASH:
				mState = STATE_CHECKING;
				break;
        default:
                mState = STATE_REMOTE;
                break;
        }
    }

	int repeat = 0;
	QString statusText;

	switch (mState) {
	case STATE_ERROR:
		repeat = 0;

		ui->downloadPushButton->hide();
		ui->resumeToolButton->hide();
		ui->pauseToolButton->hide();
		ui->cancelToolButton->hide();
		ui->progressBar->hide();
		ui->openFilePushButton->hide();
        ui->openFolderPushButton->hide();

		statusText = tr("Error");

		break;

	case STATE_REMOTE:
		repeat = 30000;

		ui->downloadPushButton->show();
		ui->resumeToolButton->hide();
		ui->pauseToolButton->hide();
		ui->cancelToolButton->hide();
		ui->progressBar->hide();
		ui->openFilePushButton->hide();
        ui->openFolderPushButton->hide();

		break;

	case STATE_DOWNLOAD:
		repeat = 1000;

		ui->downloadPushButton->hide();
		ui->resumeToolButton->hide();
		ui->pauseToolButton->show();
		ui->cancelToolButton->show();
		ui->progressBar->show();
		ui->openFilePushButton->hide();
        ui->openFolderPushButton->hide();

		break;

	case STATE_PAUSED:
		repeat = 1000;

		ui->downloadPushButton->hide();
		ui->resumeToolButton->show();
		ui->pauseToolButton->hide();
		ui->cancelToolButton->show();
		ui->progressBar->hide();
		ui->openFilePushButton->hide();
        ui->openFolderPushButton->hide();

		statusText = tr("Paused");

		break;

	case STATE_WAITING:
		repeat = 1000;

		ui->downloadPushButton->hide();
		ui->resumeToolButton->hide();
		ui->pauseToolButton->show();
		ui->cancelToolButton->show();
		ui->progressBar->hide();
		ui->openFilePushButton->hide();
        ui->openFolderPushButton->hide();

		statusText = tr("Waiting");

		break;

	case STATE_CHECKING:
		repeat = 1000;

		ui->downloadPushButton->hide();
		ui->resumeToolButton->hide();
		ui->pauseToolButton->hide();
		ui->cancelToolButton->show();
		ui->progressBar->hide();
		ui->openFilePushButton->hide();
        ui->openFolderPushButton->hide();

		statusText = tr("Checking");

		break;

	case STATE_LOCAL:
		repeat = 60000;

		ui->downloadPushButton->hide();
		ui->resumeToolButton->hide();
		ui->pauseToolButton->hide();
		ui->cancelToolButton->hide();
		ui->progressBar->hide();
		ui->openFilePushButton->show();
        ui->openFolderPushButton->show();

		break;
	}

	if (statusText.isEmpty()) {
		ui->label->clear();
		ui->label->hide();
	} else {
		ui->label->setText(statusText);
		ui->label->show();
	}

	if (repeat) {
	  	QTimer::singleShot(repeat, this, SLOT(check()));
	}
}

void GxsChannelFilesStatusWidget::download()
{
	std::list<RsPeerId> sources;

	std::string destination;
//#if 0
//	if (!mChannelId.empty() && mType == SFI_TYPE_CHANNEL) {
//		ChannelInfo ci;
//		if (rsChannels->getChannelInfo(mChannelId, ci)) {
//			destination = ci.destination_directory;
//		}
//	}
//#endif

	// Add possible direct sources.
	FileInfo fileInfo;
	rsFiles->FileDetails(mFile.mHash, RS_FILE_HINTS_REMOTE, fileInfo);

	for(std::vector<TransferInfo>::const_iterator it = fileInfo.peers.begin(); it != fileInfo.peers.end(); ++it) {
		sources.push_back((*it).peerId);
	}

	rsFiles->FileRequest(mFile.mName, mFile.mHash, mFile.mSize, destination, RS_FILE_REQ_ANONYMOUS_ROUTING, sources);

	emit onButtonClick();// Signals the parent widget to e.g. update the downloadable file count
	check();
}

void GxsChannelFilesStatusWidget::pause()
{
	rsFiles->FileControl(mFile.mHash, RS_FILE_CTRL_PAUSE);

	emit onButtonClick();// Signals the parent widget to e.g. update the downloadable file count
	check();
}

void GxsChannelFilesStatusWidget::resume()
{
	rsFiles->FileControl(mFile.mHash, RS_FILE_CTRL_START);

	emit onButtonClick();// Signals the parent widget to e.g. update the downloadable file count
	check();
}

void GxsChannelFilesStatusWidget::cancel()
{
    // When QMessgeBox asks for cancel confirmtion, this makes the widget lose focus => since it is an editor widget,
    // it gets destroyed by the parent list widget => subsequent code after the QMessageBox runs over a deleted object => crash
    // In summary: no QMessageBox here when the Status widget is used as an editor.

    if(!mUsedAsEditor)
        if ((QMessageBox::question(this, "", tr("Are you sure that you want to cancel and delete the file?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No)) == QMessageBox::No) {
            return;
        }

	rsFiles->FileCancel(mFile.mHash);

	emit onButtonClick();// Signals the parent widget to e.g. update the downloadable file count
    check();
}

void GxsChannelFilesStatusWidget::openFolder()
{
	FileInfo fileInfo;
    if (!haveFile(fileInfo))
		return;

    QFileInfo finfo;
    finfo.setFile(QString::fromUtf8(fileInfo.path.c_str()));

    /* open folder with a suitable application */
    if (!RsUrlHandler::openUrl(QUrl::fromLocalFile(finfo.absolutePath()))) {
        if(!mUsedAsEditor)
            QMessageBox::warning(this, "", QString("%1 %2").arg(tr("Can't open folder"), finfo.absolutePath()));
        else
            RsErr() << "Can't open folder " << finfo.absolutePath().toStdString() ;
    }
}

bool GxsChannelFilesStatusWidget::haveFile(FileInfo& info)
{
    bool already_has_file = rsFiles->alreadyHaveFile(mFile.mHash, info);

    if(!already_has_file)
        if(!(rsFiles->FileDetails(mFile.mHash, RS_FILE_HINTS_DOWNLOAD | RS_FILE_HINTS_SPEC_ONLY, info) && info.downloadStatus==FT_STATE_COMPLETE))
            return false;

    // We need the code below because FileDetails() returns fileInfo.path as the directory when the file in COMPLETE and
    // as a full path when the file is shared. The former is inconsistent with the documentation in rstypes.h, but I'm not
    // sure what are the implications of changing the code in libretroshare so that the full path is always returned.

    QFileInfo finfo;

    if(QDir(QString::fromUtf8(info.path.c_str())).exists())
        finfo.setFile(QString::fromUtf8(info.path.c_str()),QString::fromUtf8(info.fname.c_str()));
    else if(QFile(QString::fromUtf8(info.path.c_str())).exists())
        finfo.setFile(QString::fromUtf8(info.path.c_str()));
    else
    {
        RsErr() << "Cannot find file!" << std::endl;
        return false;
    }

    info.path = finfo.absoluteFilePath().toStdString();
    return true;
}

void GxsChannelFilesStatusWidget::openFile()
{
	FileInfo fileInfo;
    if(!haveFile(fileInfo))
		return;

    QFileInfo finfo;
    finfo.setFile(QString::fromUtf8(fileInfo.path.c_str()));

    if (finfo.exists()) {
        if (!RsUrlHandler::openUrl(QUrl::fromLocalFile(finfo.absoluteFilePath()))) {
			std::cerr << "GxsChannelFilesStatusWidget(): can't open file " << fileInfo.path << std::endl;
		}
	}else{
        if(!mUsedAsEditor)
            QMessageBox::information(this, tr("Play File"),
                    tr("File %1 does not exist at location.").arg(fileInfo.path.c_str()));
        else
            RsErr() << "File " << fileInfo.path << " does not exist at location." ;

        return;
	}
}

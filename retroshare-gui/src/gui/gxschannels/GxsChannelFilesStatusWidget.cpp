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

#include <QTimer>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QUrl>

#include "GxsChannelFilesStatusWidget.h"
#include "ui_GxsChannelFilesStatusWidget.h"
#include "gui/common/RsUrlHandler.h"

#include "retroshare/rsfiles.h"

GxsChannelFilesStatusWidget::GxsChannelFilesStatusWidget(const RsGxsFile &file, QWidget *parent) :
    QWidget(parent), mFile(file), ui(new Ui::GxsChannelFilesStatusWidget)
{
	ui->setupUi(this);

	mState = STATE_REMOTE;

	setSize(mFile.mSize);

	/* Connect signals */
	connect(ui->downloadToolButton, SIGNAL(clicked()), this, SLOT(download()));
	connect(ui->resumeToolButton, SIGNAL(clicked()), this, SLOT(resume()));
	connect(ui->pauseToolButton, SIGNAL(clicked()), this, SLOT(pause()));
	connect(ui->cancelToolButton, SIGNAL(clicked()), this, SLOT(cancel()));
	connect(ui->openFolderToolButton, SIGNAL(clicked()), this, SLOT(openFolder()));

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
	if (rsFiles->alreadyHaveFile(mFile.mHash, fileInfo)) {
		mState = STATE_LOCAL;
		setSize(fileInfo.size);
	} else {
		FileInfo fileInfo;
		bool detailsOk = rsFiles->FileDetails(mFile.mHash, RS_FILE_HINTS_DOWNLOAD | RS_FILE_HINTS_SPEC_ONLY, fileInfo);

		if (detailsOk) {
			switch (fileInfo.downloadStatus) {
			case FT_STATE_WAITING:
				mState = STATE_WAITING;
				break;
			case FT_STATE_DOWNLOADING:
				if (fileInfo.avail == fileInfo.size) {
					mState = STATE_LOCAL;
				} else {
					mState = STATE_DOWNLOAD;
				}
				setSize(fileInfo.size);
				ui->progressBar->setValue(fileInfo.avail / mDivisor);
				break;
			case FT_STATE_COMPLETE:
				mState = STATE_DOWNLOAD;
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
			case FT_STATE_FAILED:
				mState = STATE_ERROR;
				break;
			}
		} else {
			mState = STATE_REMOTE;
		}
	}

	int repeat = 0;
	QString statusText;

	switch (mState) {
	case STATE_ERROR:
		repeat = 0;

		ui->downloadToolButton->hide();
		ui->resumeToolButton->hide();
		ui->pauseToolButton->hide();
		ui->cancelToolButton->hide();
		ui->progressBar->hide();
		ui->openFolderToolButton->hide();

		statusText = tr("Error");

		break;

	case STATE_REMOTE:
		repeat = 30000;

		ui->downloadToolButton->show();
		ui->resumeToolButton->hide();
		ui->pauseToolButton->hide();
		ui->cancelToolButton->hide();
		ui->progressBar->hide();
		ui->openFolderToolButton->hide();

		break;

	case STATE_DOWNLOAD:
		repeat = 1000;

		ui->downloadToolButton->hide();
		ui->resumeToolButton->hide();
		ui->pauseToolButton->show();
		ui->cancelToolButton->show();
		ui->progressBar->show();
		ui->openFolderToolButton->hide();

		break;

	case STATE_PAUSED:
		repeat = 1000;

		ui->downloadToolButton->hide();
		ui->resumeToolButton->show();
		ui->pauseToolButton->hide();
		ui->cancelToolButton->show();
		ui->progressBar->hide();
		ui->openFolderToolButton->hide();

		statusText = tr("Paused");

		break;

	case STATE_WAITING:
		repeat = 1000;

		ui->downloadToolButton->hide();
		ui->resumeToolButton->hide();
		ui->pauseToolButton->show();
		ui->cancelToolButton->show();
		ui->progressBar->hide();
		ui->openFolderToolButton->hide();

		statusText = tr("Waiting");

		break;

	case STATE_CHECKING:
		repeat = 1000;

		ui->downloadToolButton->hide();
		ui->resumeToolButton->hide();
		ui->pauseToolButton->hide();
		ui->cancelToolButton->show();
		ui->progressBar->hide();
		ui->openFolderToolButton->hide();

		statusText = tr("Checking");

		break;

	case STATE_LOCAL:
		repeat = 60000;

		ui->downloadToolButton->hide();
		ui->resumeToolButton->hide();
		ui->pauseToolButton->hide();
		ui->cancelToolButton->hide();
		ui->progressBar->hide();
		ui->openFolderToolButton->show();

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

	check();
}

void GxsChannelFilesStatusWidget::pause()
{
	rsFiles->FileControl(mFile.mHash, RS_FILE_CTRL_PAUSE);

	check();
}

void GxsChannelFilesStatusWidget::resume()
{
	rsFiles->FileControl(mFile.mHash, RS_FILE_CTRL_START);

	check();
}

void GxsChannelFilesStatusWidget::cancel()
{
	if ((QMessageBox::question(this, "", tr("Are you sure that you want to cancel and delete the file?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No)) == QMessageBox::No) {
		return;
	}

	rsFiles->FileCancel(mFile.mHash);

	check();
}

void GxsChannelFilesStatusWidget::openFolder()
{
	FileInfo fileInfo;
	if (!rsFiles->alreadyHaveFile(mFile.mHash, fileInfo)) {
		return;
	}

	/* open folder with a suitable application */
	QDir dir = QFileInfo(QString::fromUtf8(fileInfo.path.c_str())).absoluteDir();
	if (dir.exists()) {
		if (!RsUrlHandler::openUrl(QUrl::fromLocalFile(dir.absolutePath()))) {
			QMessageBox::warning(this, "", QString("%1 %2").arg(tr("Can't open folder"), dir.absolutePath()));
		}
	}
}

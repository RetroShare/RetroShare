/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/PhotoDialog.cpp                           *
 *                                                                             *
 * Copyright (C) 2018 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#include <iostream>
#include <QVBoxLayout>
#include "PhotoDialog.h"
#include "ui_PhotoDialog.h"
#include "retroshare/rsidentity.h"
#include "gui/gxs/GxsCommentDialog.h"

#define IMAGE_FULLSCREEN          ":/icons/fullscreen.png"
#define IMAGE_FULLSCREENEXIT      ":/icons/fullscreen-exit.png"
#define IMAGE_SHOW                ":/icons/png/down-arrow.png"
#define IMAGE_HIDE                ":/icons/png/up-arrow.png"

PhotoDialog::PhotoDialog(RsPhoto *rs_photo, const RsPhotoPhoto &photo, QWidget *parent) :
	QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
	ui(new Ui::PhotoDialog), mRsPhoto(rs_photo), mPhotoQueue(new TokenQueue(mRsPhoto->getTokenService(), this)),
	mPhotoDetails(photo),
	mCommentsCreated(false)
{
	ui->setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );

	connect(ui->fullscreenButton, SIGNAL(clicked()),this, SLOT(setFullScreen()));
	connect(ui->commentsButton, SIGNAL(clicked()),this, SLOT(toggleComments()));
	connect(ui->detailsButton, SIGNAL(clicked()),this, SLOT(toggleDetails()));

	setUp();
}

PhotoDialog::~PhotoDialog()
{
	delete ui;
	delete mPhotoQueue;
}

void PhotoDialog::setUp()
{
	QPixmap qtn;
	qtn.loadFromData(mPhotoDetails.mLowResImage.mData, mPhotoDetails.mLowResImage.mSize);
	ui->label_Photo->setPixmap(qtn);
	ui->label_Photo->setVisible(true);

	// set size of label to match image.
	ui->label_Photo->setMinimumSize(ui->label_Photo->sizeHint());
	// alternative is to scale contents.
	// ui->label_Photo->setScaledContents(true);
	// Neither are ideal. sizeHint is potentially too large.
	// scaled contents - doesn't respect Aspect Ratio...
	//
	// Ideal soln: 
	// Allow both, depending on Zoom Factor.
	// Auto: use Scale, with correct aspect ratio.
	// answer here:  https://stackoverflow.com/questions/8211982/qt-resizing-a-qlabel-containing-a-qpixmap-while-keeping-its-aspect-ratio
	// Fixed %, then manually scale to that, with scroll area.

	ui->lineEdit_Title->setText(QString::fromStdString(mPhotoDetails.mMeta.mMsgName));
	ui->albumGroup->setTitle( tr("Album") + " / " + QString::fromStdString(mPhotoDetails.mMeta.mMsgName));

	ui->frame_comments->setVisible(false);
	ui->frame_details->setVisible(false);
}

void PhotoDialog::toggleDetails()
{
	if (ui->frame_details->isVisible()) {
		ui->frame_details->setVisible(false);
		ui->detailsButton->setIcon(QIcon(IMAGE_SHOW));
	} else {
		ui->frame_details->setVisible(true);
		ui->detailsButton->setIcon(QIcon(IMAGE_HIDE));
	}
}

void PhotoDialog::toggleComments()
{
	if (ui->frame_comments->isVisible()) {
		ui->frame_comments->setVisible(false);
	} else {
		if (mCommentsCreated) {
			ui->frame_comments->setVisible(true);
		} else {
			RsGxsId current_author;
			// create CommentDialog.
			RsGxsCommentService *commentService = dynamic_cast<RsGxsCommentService *>(mRsPhoto);
			GxsCommentDialog *commentDialog = new GxsCommentDialog(this,current_author, mRsPhoto->getTokenService(), commentService);

			// TODO: Need to fetch all msg versions, otherwise - won't get all the comments.
			// For the moment - use current msgid.
			// Needs to be passed to PhotoDialog, or fetched here.

			RsGxsGroupId grpId = mPhotoDetails.mMeta.mGroupId;
			RsGxsMessageId msgId = mPhotoDetails.mMeta.mMsgId;

			std::set<RsGxsMessageId> msgv;
			msgv.insert(msgId);
			msgv.insert(mPhotoDetails.mMeta.mOrigMsgId); // if duplicate will be ignored.

			commentDialog->commentLoad(grpId, msgv,msgId);

			// insert into frame.
			QVBoxLayout *vbox = new QVBoxLayout();

			vbox->addWidget(commentDialog);
			ui->frame_comments->setLayout(vbox);

			ui->frame_comments->setVisible(true);
			mCommentsCreated = true;
		}
	}
}

/*************** message loading **********************/

void PhotoDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "PhotoShare::loadRequest()";
	std::cerr << std::endl;

	if (queue == mPhotoQueue)
	{
		/* now switch on req */
		switch(req.mType)
		{
			case TOKENREQ_MSGINFO:
			{
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_LIST:
						loadList(req.mToken);
						break;
					default:
						std::cerr << "PhotoShare::loadRequest() ERROR: MSG INVALID TYPE";
						std::cerr << std::endl;
						break;
				}
				break;
			}

			default:
			{
				std::cerr << "PhotoShare::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
			}
		}
	}

}

void PhotoDialog::loadList(uint32_t token)
{
	GxsMsgReq msgIds;
	mRsPhoto->getMsgList(token, msgIds);
	RsTokReqOptions opts;

	// just use data as no need to worry about getting comments
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
	uint32_t reqToken;
	mPhotoQueue->requestMsgInfo(reqToken, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, 0);
}

void PhotoDialog::setFullScreen()
{
  if (!isFullScreen()) {
	// hide menu & toolbars

#ifdef Q_OS_LINUX
	show();
	raise();
	setWindowState( windowState() | Qt::WindowFullScreen );
#else
	setWindowState( windowState() | Qt::WindowFullScreen );
	show();
	raise();
#endif
	ui->fullscreenButton->setIcon(QIcon(IMAGE_FULLSCREENEXIT));
  } else {

	setWindowState( windowState() ^ Qt::WindowFullScreen );
	show();
	ui->fullscreenButton->setIcon(QIcon(IMAGE_FULLSCREEN));
  }
}

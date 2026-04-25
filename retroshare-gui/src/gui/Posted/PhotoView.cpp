/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PhotoView.cpp                                *
 *                                                                             *
 * Copyright (C) 2020 by RetroShare Team       <retroshare.project@gmail.com>  *
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

#include "PhotoView.h"
#include "ui_PhotoView.h"

#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QClipboard>
#include <QBuffer>
#include <QMovie>

#include "BoardPostImageHelper.h"

#include "gui/gxs/GxsIdDetails.h"
#include "gui/RetroShareLink.h"
#include "util/misc.h"

#include <retroshare/rsidentity.h>
#include <retroshare/rsposted.h>

/** Constructor */
PhotoView::PhotoView(QWidget *parent)
: QDialog(parent, Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint),
	ui(new Ui::PhotoView)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui->setupUi(this);

  setAttribute(Qt::WA_DeleteOnClose, true);
  
  // Hide navigation buttons by default
  ui->prevButton->hide();
  ui->nextButton->hide();
  
  connect(ui->shareButton, SIGNAL(clicked()), this, SLOT(copyMessageLink()));
  connect(ui->prevButton, &QToolButton::clicked, this, &PhotoView::goToPrevious);
  connect(ui->nextButton, &QToolButton::clicked, this, &PhotoView::goToNext);

}

/** Destructor */
PhotoView::~PhotoView()
{
	// Note: mMovie is NOT deleted here - Qt parent-child handles cleanup
	// (caller sets movie->setParent(this) before calling setMovie)
	if (mMovie) {
		mMovie->stop();
	}
	delete ui;
}

void PhotoView::setPixmap(const QPixmap& pixmap) 
{
	ui->photoLabel->setPixmap(pixmap);
	
	if (mPosts.size() <= 1){
		this->adjustSize();
	}
}

void PhotoView::setMovie(QMovie* movie)
{
	// Note: Previous movie cleanup is handled by Qt parent-child mechanism
	// Caller sets movie->setParent(this) before calling this
	if (mMovie) {
		mMovie->stop();
		// Don't delete - Qt parent-child handles it
	}
	
	mMovie = movie;
	
	if (mMovie) {
		ui->photoLabel->setMovie(mMovie);
		mMovie->start();
		// Loop animation when finished
		connect(mMovie, &QMovie::finished, mMovie, &QMovie::start);
	}
	
	this->adjustSize();
}

void PhotoView::setTitle(const QString& text) 
{
	ui->titleLabel->setText(text);
}

void PhotoView::setName(const RsGxsId& authorID) 
{
	ui->nameLabel->setId(authorID);
	
	RsIdentityDetails idDetails ;
	rsIdentity->getIdDetails(authorID,idDetails);

	QPixmap pixmap ;

	if(idDetails.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idDetails.mAvatar.mData, idDetails.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
			pixmap = GxsIdDetails::makeDefaultIcon(authorID,GxsIdDetails::SMALL);
			
	ui->avatarWidget->setPixmap(pixmap);
}

void PhotoView::setTime(const QString& text) 
{
	ui->timeLabel->setText(text);
}

void PhotoView::setGroupId(const RsGxsGroupId &groupId) 
{
	mGroupId = groupId;
}

void PhotoView::setMessageId(const RsGxsMessageId& messageId) 
{
	mMessageId = messageId ;
}

void PhotoView::copyMessageLink()
{
	RetroShareLink link = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_POSTED, mGroupId, mMessageId, ui->titleLabel->text());

	if (link.valid()) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
		QMessageBox::information(NULL,tr("information"),tr("The Retrohare link was copied to your clipboard.")) ;
	}
}

void PhotoView::setGroupNameString(const QString& name)
{
	ui->nameLabel->setText("@" + name);
}

void PhotoView::setPosts(const QList<RsPostedPost>& posts, int currentIndex)
{
    mPosts = posts;
    mCurrentIndex = currentIndex;
    
    if (mPosts.size() > 1) {
        // Apply constraints only when multiple photos exist
        this->resize(800, 600);
    }

    // Show buttons only if there is more than one post to navigate
    bool showNavigation = (mPosts.size() > 1);
    ui->prevButton->setVisible(showNavigation);
    ui->nextButton->setVisible(showNavigation);

    updateDisplay();
}

void PhotoView::goToPrevious()
{
    if (mCurrentIndex > 0) {
        mCurrentIndex--;
        updateDisplay();
    }
}

void PhotoView::goToNext()
{
    if (mCurrentIndex < mPosts.size() - 1){
        mCurrentIndex++;
        updateDisplay();
    }
}

void PhotoView::updateDisplay()
{
    if (mCurrentIndex < 0 || mCurrentIndex >= mPosts.size()) return;

    const RsPostedPost& post = mPosts[mCurrentIndex];

    // Emit the signal with the current message ID
    emit postChanged(post.mMeta.mMsgId);

    QString timestamp = misc::timeRelativeToNow(post.mMeta.mPublishTs);

    // Set Title, ID, and Time using existing methods
    setTitle(QString::fromUtf8(post.mMeta.mMsgName.c_str()));
    setName(post.mMeta.mAuthorId);
    setTime(timestamp);
    setGroupId(post.mMeta.mGroupId);
    setMessageId(post.mMeta.mMsgId);

    // Check if animated image
    QString format;
    if (BoardPostImageHelper::isAnimatedImage(post.mImage.mData, post.mImage.mSize, &format))
    {
        // Animated GIF/WEBP - use QMovie in popup
        QMovie* movie = BoardPostImageHelper::createMovieFromData(post.mImage.mData, post.mImage.mSize);
        if (movie)
        {
            movie->setParent(this); // Ensure cleanup
            setMovie(movie);
            movie->start();
        }
    }
    else
    {
        // Static image - use QPixmap
        QPixmap pixmap;
        GxsIdDetails::loadPixmapFromData(post.mImage.mData, post.mImage.mSize, pixmap,GxsIdDetails::ORIGINAL);
        setPixmap(pixmap);
    }
    
    // Enable/Disable buttons based on bounds
    ui->prevButton->setEnabled(mCurrentIndex > 0);
    ui->nextButton->setEnabled(mCurrentIndex < mPosts.size() - 1);
}

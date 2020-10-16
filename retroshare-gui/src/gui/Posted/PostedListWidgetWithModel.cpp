/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/PostedListWidgetWithModel.cpp            *
 *                                                                             *
 * Copyright 2013 by Robert Fernie     <retroshare.project@gmail.com>          *
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

#include <QDateTime>
#include <QMenu>
#include <QSignalMapper>
#include <QPainter>
#include <QMessageBox>

#include "retroshare/rsgxscircles.h"

#include "ui_PostedListWidgetWithModel.h"
#include "gui/feeds/GxsChannelPostItem.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxs/GxsCommentDialog.h"
#include "util/misc.h"
#include "gui/Posted/PostedCreatePostDialog.h"
#include "gui/common/UIStateHelper.h"
#include "gui/common/RSTabWidget.h"
#include "gui/settings/rsharesettings.h"
#include "gui/feeds/SubFileItem.h"
#include "gui/notifyqt.h"
#include "gui/Identity/IdDialog.h"
#include "gui/RetroShareLink.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "util/qtthreadsutils.h"
#include "gui/common/FilesDefs.h"

#include "gui/MainWindow.h"

#include "PostedListWidgetWithModel.h"
#include "PostedPostsModel.h"
#include "BoardPostDisplayWidget.h"

#include <algorithm>

#define ROLE_PUBLISH FEED_TREEWIDGET_SORTROLE

/****
 * #define DEBUG_POSTED
 ***/

/* View mode */
#define VIEW_MODE_FEEDS  1
#define VIEW_MODE_FILES  2

// Determine the Shape and size of cells as a factor of the font height. An aspect ratio of 3/4 is what's needed
// for the image, so it's important that the height is a bit larger so as to leave some room for the text.
//
//
#define IMAGE_COPYLINK     ":/images/copyrslink.png"
#define IMAGE_AUTHOR       ":/images/user/personal64.png"

Q_DECLARE_METATYPE(RsPostedPost);

// Delegate used to paint into the table of thumbnails

//===================================================================================================================================
//==                                                     PostedPostDelegate                                                        ==
//===================================================================================================================================

std::ostream& operator<<(std::ostream& o,const QSize& s) { return o << s.width() << " x " << s.height() ; }

void PostedPostDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	if((option.state & QStyle::State_Selected)) // Avoids double display. The selected widget is never exactly the size of the rendered one,
        return;                                 // so when selected, we only draw the selected one.

	// prepare
	painter->save();
	painter->setClipRect(option.rect);

	RsPostedPost post = index.data(Qt::UserRole).value<RsPostedPost>() ;

	painter->save();

    painter->fillRect( option.rect, option.palette.background());
	painter->restore();

    QPixmap pixmap(option.rect.size());
    pixmap.fill(option.palette.alternateBase().color()); // use base() instead to have all widgets the same background

    if(mDisplayMode == BoardPostDisplayWidget_compact::DISPLAY_MODE_COMPACT)
    {
        BoardPostDisplayWidget_compact w(post,displayFlags(post.mMeta.mMsgId),nullptr);

        w.setFixedSize(option.rect.size());

        w.updateGeometry();
        w.adjustSize();
        w.render(&pixmap,QPoint(0,0),QRegion(),QWidget::DrawChildren );// draw the widgets, not the background
    }
    else
    {
        BoardPostDisplayWidget_card w(post,displayFlags(post.mMeta.mMsgId),nullptr);

        w.setFixedSize(option.rect.size());
        w.updateGeometry();
        w.adjustSize();
        w.render(&pixmap,QPoint(0,0),QRegion(),QWidget::DrawChildren );// draw the widgets, not the background
    }

#ifdef TODO
	if(IS_MSG_UNREAD(post.mMeta.mMsgStatus) || IS_MSG_NEW(post.mMeta.mMsgStatus))
	{
		QPainter p(&pixmap);
		QFontMetricsF fm(option.font);
		p.drawPixmap(mZoom*QPoint(6.2*fm.height(),6.9*fm.height()),FilesDefs::getPixmapFromQtResourcePath(STAR_OVERLAY_IMAGE).scaled(mZoom*7*fm.height(),mZoom*7*fm.height(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
	}
#endif

//    // debug
//    if(index.row()==0 && index.column()==0)
//    {
//        QFile file("yourFile.png");
//        file.open(QIODevice::WriteOnly);
//        pixmap.save(&file, "PNG");
//        std::cerr << "Saved pxmap to png" << std::endl;
//    }
    //std::cerr << "option.rect = " << option.rect.width() << "x" << option.rect.height() << ". fm.height()=" << QFontMetricsF(option.font).height() << std::endl;

	painter->save();
	painter->drawPixmap(option.rect.topLeft(), pixmap /*,.scaled(option.rect.width(),option.rect.width()*w.height()/(float)w.width(),Qt::KeepAspectRatio,Qt::SmoothTransformation)*/);
	painter->restore();
}

QSize PostedPostDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // This is the only place where we actually set the size of cells

	RsPostedPost post = index.data(Qt::UserRole).value<RsPostedPost>() ;

    if(mDisplayMode == BoardPostDisplayWidget_compact::DISPLAY_MODE_COMPACT)
    {
        BoardPostDisplayWidget_compact w(post,displayFlags(post.mMeta.mMsgId),nullptr);
        w.adjustSize();
        return w.size();
    }
    else
    {
        BoardPostDisplayWidget_card w(post,displayFlags(post.mMeta.mMsgId),nullptr);
        w.adjustSize();
        return w.size();
    }
}
void PostedPostDelegate::expandItem(RsGxsMessageId msgId,bool expanded)
{
    std::cerr << __PRETTY_FUNCTION__ << ": received expandItem signal. b="  << expanded << std::endl;
    if(expanded)
        mExpandedItems.insert(msgId);
    else
        mExpandedItems.erase(msgId);

    mPostListWidget->forceRedraw();
}

uint8_t PostedPostDelegate::displayFlags(const RsGxsMessageId &id) const
{
    uint8_t flags=0;

    if(mExpandedItems.find(id) != mExpandedItems.end())
        flags |= BoardPostDisplayWidget_compact::SHOW_NOTES;

    if(mShowCommentItems.find(id) != mShowCommentItems.end())
        flags |= BoardPostDisplayWidget_compact::SHOW_COMMENTS;

    return flags;
}

QWidget *PostedPostDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex& index) const
{
    RsPostedPost post = index.data(Qt::UserRole).value<RsPostedPost>() ;

    if(index.column() == RsPostedPostsModel::COLUMN_POSTS)
    {
        QWidget *w ;

        if(mDisplayMode==BoardPostDisplayWidget_compact::DISPLAY_MODE_COMPACT)
            w = new BoardPostDisplayWidget_compact(post,displayFlags(post.mMeta.mMsgId),parent);
        else
            w = new BoardPostDisplayWidget_card(post,displayFlags(post.mMeta.mMsgId),parent);

        QObject::connect(w,SIGNAL(vote(RsGxsGrpMsgIdPair,bool)),mPostListWidget,SLOT(voteMsg(RsGxsGrpMsgIdPair,bool)));
        QObject::connect(w,SIGNAL(expand(RsGxsMessageId,bool)),this,SLOT(expandItem(RsGxsMessageId,bool)));
        QObject::connect(w,SIGNAL(commentsRequested(const RsGxsMessageId&,bool)),mPostListWidget,SLOT(openComments(const RsGxsMessageId&)));
        QObject::connect(w,SIGNAL(changeReadStatusRequested(const RsGxsMessageId&,bool)),mPostListWidget,SLOT(changeReadStatus(const RsGxsMessageId&,bool)));

        // All other interactions with the widget should cause the msg to be set as read.
        QObject::connect(w,SIGNAL(thumbnailOpenned()),mPostListWidget,SLOT(markCurrentPostAsRead()));
        QObject::connect(w,SIGNAL(vote(RsGxsGrpMsgIdPair,bool)),mPostListWidget,SLOT(markCurrentPostAsRead()));
        QObject::connect(w,SIGNAL(expand(RsGxsMessageId,bool)),this,SLOT(markCurrentPostAsRead()));
        QObject::connect(w,SIGNAL(commentsRequested(const RsGxsMessageId&,bool)),mPostListWidget,SLOT(markCurrentPostAsRead()));
        QObject::connect(w,SIGNAL(shareButtonClicked()),mPostListWidget,SLOT(markCurrentPostAsRead()));
        QObject::connect(w,SIGNAL(copylinkClicked()),mPostListWidget,SLOT(copyMessageLink()));

        w->setFixedSize(option.rect.size());
        w->adjustSize();
        w->updateGeometry();
        w->adjustSize();

        return w;
    }
    else
        return NULL;
}
void PostedPostDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
	editor->setGeometry(option.rect);
}

//===================================================================================================================================
//==                                                 PostedListWidgetWithModel                                                     ==
//===================================================================================================================================

/** Constructor */
PostedListWidgetWithModel::PostedListWidgetWithModel(const RsGxsGroupId& postedId, QWidget *parent) :
	GxsMessageFrameWidget(rsPosted, parent),
	ui(new Ui::PostedListWidgetWithModel)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

    ui->postsTree->setModel(mPostedPostsModel = new RsPostedPostsModel());
    ui->postsTree->setItemDelegate(mPostedPostsDelegate = new PostedPostDelegate(this));
    ui->postsTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);	// prevents bug on w10, since row size depends on widget width
    ui->postsTree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);// more beautiful if we scroll at pixel level
    ui->postsTree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->postsTree->setPlaceholderText(tr("No files in this post, or no post selected"));
    ui->postsTree->setSortingEnabled(true);
    ui->postsTree->sortByColumn(0, Qt::AscendingOrder);
    ui->postsTree->setAutoSelect(true);

    ui->idChooser->setFlags(IDCHOOSER_ID_REQUIRED);

    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
    ui->tabWidget->hideCloseButton(0);
    ui->tabWidget->hideCloseButton(1);

    connect(ui->sortStrategy_CB,SIGNAL(currentIndexChanged(int)),this,SLOT(updateSorting(int)));
    connect(ui->nextButton,SIGNAL(clicked()),this,SLOT(next10Posts()));
    connect(ui->prevButton,SIGNAL(clicked()),this,SLOT(prev10Posts()));

    connect(ui->postsTree,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(postContextMenu(const QPoint&)));
    connect(ui->viewModeButton,SIGNAL(clicked()),this,SLOT(switchDisplayMode()));

    connect(mPostedPostsModel,SIGNAL(boardPostsLoaded()),this,SLOT(postPostLoad()));

    QFontMetricsF fm(font());

	/* Setup UI helper */

	/* Connect signals */
	connect(ui->submitPostButton,    SIGNAL(clicked()),            this, SLOT(createMsg()));
	connect(ui->subscribeToolButton, SIGNAL(subscribe(bool)),      this, SLOT(subscribeGroup(bool)));
    connect(ui->filter_LE,           SIGNAL(textChanged(QString)), this, SLOT(filterItems(QString)));
	connect(NotifyQt::getInstance(), SIGNAL(settingsChanged()),this, SLOT(settingsChanged()));

	/* add filter actions */
    ui->postsTree->setPlaceholderText(tr("Thumbnails"));
    //ui->postsTree->setMinimumWidth(COLUMN_SIZE_FONT_FACTOR_W*QFontMetricsF(font()).height()+1);

    connect(ui->postsTree,SIGNAL(sizeChanged(QSize)),this,SLOT(handlePostsTreeSizeChange(QSize)));

	/* load settings */
	processSettings(true);

	/* Initialize subscribe button */
	QIcon icon;
	icon.addPixmap(QPixmap(":/images/redled.png"), QIcon::Normal, QIcon::On);
	icon.addPixmap(QPixmap(":/images/start.png"), QIcon::Normal, QIcon::Off);

    // ui->commentsDialog->setTokenService(rsPosted->getTokenService(),rsPosted);

	/* Initialize GUI */
	// setAutoDownload(false);

	settingsChanged();
    setGroupId(postedId);

    mPostedPostsDelegate->setDisplayMode(BoardPostDisplayWidget_compact::DISPLAY_MODE_CARD);

    switchDisplayMode();	// makes everything consistent and chooses classic view as default
    updateSorting(ui->sortStrategy_CB->currentIndex());

	mEventHandlerId = 0;
	// Needs to be asynced because this function is called by another thread!
	rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
    {
        RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this );
    }, mEventHandlerId, RsEventType::GXS_POSTED );
}

void PostedListWidgetWithModel::switchDisplayMode()
{
    if(mPostedPostsDelegate->getDisplayMode() == BoardPostDisplayWidget_compact::DISPLAY_MODE_CARD)
    {
        ui->viewModeButton->setIcon(FilesDefs::getIconFromQtResourcePath(":images/classic.png"));
        ui->viewModeButton->setToolTip(tr("Click to switch to card view"));

        mPostedPostsDelegate->setDisplayMode(BoardPostDisplayWidget_compact::DISPLAY_MODE_COMPACT);
    }
    else
    {
        ui->viewModeButton->setIcon(FilesDefs::getIconFromQtResourcePath(":images/card.png"));
        ui->viewModeButton->setToolTip(tr("Click to switch to compact view"));

        mPostedPostsDelegate->setDisplayMode(BoardPostDisplayWidget_compact::DISPLAY_MODE_CARD);
    }
    mPostedPostsModel->triggerRedraw();
}

void PostedListWidgetWithModel::updateSorting(int s)
{
    switch(s)
    {
    default:
    case 0:   mPostedPostsModel->setSortingStrategy(RsPostedPostsModel::SORT_NEW_SCORE); break;
    case 1:   mPostedPostsModel->setSortingStrategy(RsPostedPostsModel::SORT_TOP_SCORE); break;
    case 2:   mPostedPostsModel->setSortingStrategy(RsPostedPostsModel::SORT_HOT_SCORE); break;
    }
}

void PostedListWidgetWithModel::handlePostsTreeSizeChange(QSize size)
{
    std::cerr << "resizing!"<< std::endl;
    mPostedPostsDelegate->setCellWidth(size.width());
    mPostedPostsModel->triggerRedraw();
}

void PostedListWidgetWithModel::filterItems(QString text)
{
	QStringList lst = text.split(" ",QString::SkipEmptyParts) ;

    uint32_t count;
	mPostedPostsModel->setFilter(lst,count) ;

	ui->showLabel->setText(QString::number(mPostedPostsModel->displayedStartPostIndex()+1)+" - "+QString::number(std::min(mPostedPostsModel->filteredPostsCount(),mPostedPostsModel->displayedStartPostIndex()+10+1)));
}

void PostedListWidgetWithModel::next10Posts()
{
    if(mPostedPostsModel->displayedStartPostIndex() + 10 < mPostedPostsModel->filteredPostsCount())
    {
        mPostedPostsModel->setPostsInterval(10+mPostedPostsModel->displayedStartPostIndex(),10);
        ui->showLabel->setText(QString::number(mPostedPostsModel->displayedStartPostIndex()+1)+" - "+QString::number(std::min(mPostedPostsModel->filteredPostsCount(),mPostedPostsModel->displayedStartPostIndex()+10+1)));
    }
}

void PostedListWidgetWithModel::prev10Posts()
{
	if((int)mPostedPostsModel->displayedStartPostIndex() - 10 >= 0)
    {
        mPostedPostsModel->setPostsInterval(mPostedPostsModel->displayedStartPostIndex()-10,10);
        ui->showLabel->setText(QString::number(mPostedPostsModel->displayedStartPostIndex()+1)+" - "+QString::number(std::min(mPostedPostsModel->filteredPostsCount(),mPostedPostsModel->displayedStartPostIndex()+10+1)));
    }
}

void PostedListWidgetWithModel::showAuthorInPeople()
{
    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();

    if(!index.isValid())
        throw std::runtime_error("No post under mouse!");

    RsPostedPost post = index.data(Qt::UserRole).value<RsPostedPost>() ;

    if(post.mMeta.mMsgId.isNull())
        throw std::runtime_error("Post has empty MsgId!");

    if(post.mMeta.mAuthorId.isNull())
    {
        std::cerr << "(EE) GxsForumThreadWidget::loadMsgData_showAuthorInPeople() ERROR Missing Message Data...";
        std::cerr << std::endl;
    }

    /* window will destroy itself! */
    IdDialog *idDialog = dynamic_cast<IdDialog*>(MainWindow::getPage(MainWindow::People));

    if (!idDialog)
        return ;

    MainWindow::showWindow(MainWindow::People);
    idDialog->navigate(RsGxsId(post.mMeta.mAuthorId));
}
void PostedListWidgetWithModel::postContextMenu(const QPoint&)
{
    QMenu menu(this);

	menu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyMessageLink()));
    menu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_AUTHOR), tr("Show author in People tab"), this, SLOT(showAuthorInPeople()));

#ifdef TODO
    // This feature is not implemented yet in libretroshare.

	if(IS_GROUP_PUBLISHER(mGroup.mMeta.mSubscribeFlags))
		menu.addAction(FilesDefs::getIconFromQtResourcePath(":/images/edit_16.png"), tr("Edit"), this, SLOT(editPost()));
#endif

	menu.exec(QCursor::pos());
}

void PostedListWidgetWithModel::copyMessageLink()
{
    try
	{
		if (groupId().isNull())
			throw std::runtime_error("No channel currently selected!");

		QModelIndex index = ui->postsTree->selectionModel()->currentIndex();

		if(!index.isValid())
			throw std::runtime_error("No post under mouse!");

		RsPostedPost post = index.data(Qt::UserRole).value<RsPostedPost>() ;

		if(post.mMeta.mMsgId.isNull())
			throw std::runtime_error("Post has empty MsgId!");

		RetroShareLink link = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_POSTED, groupId(), post.mMeta.mMsgId, QString::fromUtf8(post.mMeta.mMsgName.c_str()));

		if (!link.valid())
			throw std::runtime_error("Link is not valid");

		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
		QMessageBox::information(NULL,tr("information"),tr("The Retrohare link was copied to your clipboard.")) ;
	}
    catch(std::exception& e)
    {
        QMessageBox::critical(NULL,tr("Link creation error"),tr("Link could not be created: ")+e.what());
    }
}

#ifdef TODO
void PostedListWidgetWithModel::editPost()
{
    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();
	RsPostedPost post = index.data(Qt::UserRole).value<RsPostedPost>() ;

	CreatePostedMsg *msgDialog = new CreatePostedMsg(post.mMeta.mGroupId,post.mMeta.mMsgId);
    msgDialog->show();
}
#endif

void PostedListWidgetWithModel::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
	const RsGxsPostedEvent *e = dynamic_cast<const RsGxsPostedEvent*>(event.get());

	if(!e)
		return;

	switch(e->mPostedEventCode)
	{
        case RsPostedEventCode::NEW_MESSAGE:     // [[fallthrough]];
        {
            // special treatment here because the message might be a comment, so we need to refresh the comment tab if openned

            for(int i=2;i<ui->tabWidget->count();++i)
            {
                auto *t = dynamic_cast<GxsCommentDialog*>(ui->tabWidget->widget(i));

                if(t->groupId() == e->mPostedGroupId)
                    t->refresh();
            }
        }
        case RsPostedEventCode::NEW_POSTED_GROUP:     // [[fallthrough]];
		case RsPostedEventCode::UPDATED_POSTED_GROUP: // [[fallthrough]];
		case RsPostedEventCode::UPDATED_MESSAGE:
		{
			if(e->mPostedGroupId == groupId())
				updateDisplay(true);
    	}

		default:
		break;
	}
}

#ifdef TO_REMOVE
void PostedListWidgetWithModel::showPostDetails()
{
    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();
	RsPostedPost post = index.data(Qt::UserRole).value<RsPostedPost>() ;
	
	QTextDocument doc;
	doc.setHtml(post.mMsg.c_str());

    if(post.mMeta.mPublishTs == 0)
    {
        ui->postDetails_TE->clear();
        ui->postLogo_LB->hide();
		ui->postName_LB->hide();
		ui->postTime_LB->hide();
		mChannelPostFilesModel->clear();

        return;
    }

	ui->postLogo_LB->show();
	ui->postName_LB->show();
	ui->postTime_LB->show();

    if(index.row()==0 && index.column()==0)
        std::cerr << "here" << std::endl;

	std::cerr << "showPostDetails: setting mSelectedPost to current post Id " << post.mMeta.mMsgId << ". Previous value: " << mSelectedPost << std::endl;
    mSelectedPost = post.mMeta.mMsgId;

    std::list<ChannelPostFileInfo> files;
    for(auto& file:post.mFiles)
        files.push_back(ChannelPostFileInfo(file,post.mMeta.mPublishTs));

    mChannelPostFilesModel->setFiles(files);

    auto all_msgs_versions(post.mOlderVersions);
    all_msgs_versions.insert(post.mMeta.mMsgId);

    ui->commentsDialog->commentLoad(post.mMeta.mGroupId, all_msgs_versions, post.mMeta.mMsgId);

    std::cerr << "Showing details about selected index : "<< index.row() << "," << index.column() << std::endl;

    ui->postDetails_TE->setText(RsHtml().formatText(NULL, QString::fromUtf8(post.mMsg.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));

	QPixmap postImage;

	if (post.mThumbnail.mData != NULL)
		GxsIdDetails::loadPixmapFromData(post.mThumbnail.mData, post.mThumbnail.mSize, postImage,GxsIdDetails::ORIGINAL);
	else
		postImage = FilesDefs::getPixmapFromQtResourcePath(ChannelPostThumbnailView::CHAN_DEFAULT_IMAGE);

    int W = QFontMetricsF(font()).height() * 8;

    // Using fixed width so that the post will not displace the text when we browse.

	ui->postLogo_LB->setPixmap(postImage);
	ui->postLogo_LB->setFixedSize(W,postImage.height()/(float)postImage.width()*W);

	ui->postName_LB->setText(QString::fromUtf8(post.mMeta.mMsgName.c_str()));
	ui->postName_LB->setFixedWidth(W);
	ui->postTime_LB->setText(QDateTime::fromMSecsSinceEpoch(post.mMeta.mPublishTs*1000).toString("MM/dd/yyyy, hh:mm"));
	ui->postTime_LB->setFixedWidth(W);

    //ui->channelPostFiles_TV->resizeColumnToContents(RsGxsChannelPostFilesModel::COLUMN_FILES_FILE);
    //ui->channelPostFiles_TV->resizeColumnToContents(RsGxsChannelPostFilesModel::COLUMN_FILES_SIZE);
    ui->channelPostFiles_TV->setAutoSelect(true);

 	// Now also set the post as read

	if(IS_MSG_UNREAD(post.mMeta.mMsgStatus) || IS_MSG_NEW(post.mMeta.mMsgStatus))
	{
		RsGxsGrpMsgIdPair postId;
		postId.second = post.mMeta.mMsgId;
		postId.first  = post.mMeta.mGroupId;

		RsThread::async([postId]() { rsGxsChannels->markRead(postId, true) ; } );
	}
}
#endif

void PostedListWidgetWithModel::updateGroupData()
{
	if(groupId().isNull())
		return;

	RsThread::async([this]()
	{
		std::vector<RsPostedGroup> groups;

		if(!rsPosted->getBoardsInfo(std::list<RsGxsGroupId>{ groupId() }, groups))
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to get boards group value for group: " << groupId() << std::endl;
			return;
		}

		if(groups.size() != 1)
		{
			RsErr() << __PRETTY_FUNCTION__ << " cannot retrieve posted group data for group ID " << groupId() << ": ERROR." << std::endl;
			return;
		}

		RsQThreadUtils::postToObject( [this,groups]()
        {
            bool group_changed = (groups[0].mMeta.mGroupId!=mGroup.mMeta.mGroupId);

            mGroup = groups[0];
			mPostedPostsModel->updateBoard(groupId());

            insertBoardDetails(mGroup);

            if(group_changed)
                while(ui->tabWidget->widget(2) != nullptr)
                    tabCloseRequested(2);

            emit groupDataLoaded();
            emit groupChanged(this);		// signals the parent widget to e.g. update the group tab name
        } );
	});
}

void PostedListWidgetWithModel::postPostLoad()
{
    std::cerr << "Post channel load..." << std::endl;

	if(!mSelectedPost.isNull())
    {
    	QModelIndex index = mPostedPostsModel->getIndexOfMessage(mSelectedPost);

        std::cerr << "Setting current index to " << index.row() << ","<< index.column() << " for current post "
                  << mSelectedPost.toStdString() << std::endl;

		ui->postsTree->selectionModel()->setCurrentIndex(index,QItemSelectionModel::ClearAndSelect);
		ui->postsTree->scrollTo(index);//May change if model reloaded
		ui->postsTree->setFocus();
    }
    else
        std::cerr << "No pre-selected channel post." << std::endl;

	whileBlocking(ui->showLabel)->setText(QString::number(mPostedPostsModel->displayedStartPostIndex()+1)+" - "+QString::number(std::min(mPostedPostsModel->filteredPostsCount(),mPostedPostsModel->displayedStartPostIndex()+10+1)));
	whileBlocking(ui->filter_LE)->setText(QString());
}

void PostedListWidgetWithModel::forceRedraw()
{
    if(mPostedPostsModel)
        mPostedPostsModel->deepUpdate();
}

void PostedListWidgetWithModel::updateDisplay(bool complete)
{
#ifdef DEBUG_CHANNEL
    std::cerr << "udateDisplay: groupId()=" << groupId()<< std::endl;
#endif
	if(groupId().isNull())
    {
#ifdef DEBUG_CHANNEL
        std::cerr << "  group_id=0. Return!"<< std::endl;
#endif
		return;
    }

	if(mGroup.mMeta.mGroupId.isNull() && !groupId().isNull())
    {
#ifdef DEBUG_FORUMS
        std::cerr << "  inconsistent group data. Reloading!"<< std::endl;
#endif
		complete = true;
    }

	if(complete) 	// need to update the group data, reload the messages etc.
	{
		updateGroupData();

		return;
	}
}
PostedListWidgetWithModel::~PostedListWidgetWithModel()
{
	rsEvents->unregisterEventsHandler(mEventHandlerId);
	// save settings
	processSettings(false);

	delete ui;
}

void PostedListWidgetWithModel::processSettings(bool load)
{
	Settings->beginGroup(QString("ChannelPostsWidget"));

	if (load)
	{
        // state of ID Chooser combobox
        RsGxsId gxs_id(Settings->value("IDChooser", QString::fromStdString(RsGxsId().toStdString())).toString().toStdString());

        if(!gxs_id.isNull() && rsIdentity->isOwnId(gxs_id))
            ui->idChooser->setChosenId(gxs_id);
	}
	else
	{
        // state of ID Chooser combobox
        RsGxsId id;

        if(ui->idChooser->getChosenId(id))
            Settings->setValue("IDChooser", QString::fromStdString(id.toStdString()));
    }

	Settings->endGroup();
}

void PostedListWidgetWithModel::settingsChanged()
{
}

QString PostedListWidgetWithModel::groupName(bool)
{
    return QString::fromUtf8(mGroup.mMeta.mGroupName.c_str());
}

void PostedListWidgetWithModel::groupNameChanged(const QString &name)
{
}

QIcon PostedListWidgetWithModel::groupIcon()
{
	/* CHANNEL IMAGE */
	QPixmap postedImage;
	if (mGroup.mGroupImage.mData != NULL)
		GxsIdDetails::loadPixmapFromData(mGroup.mGroupImage.mData, mGroup.mGroupImage.mSize, postedImage,GxsIdDetails::ORIGINAL);
	else
        postedImage = FilesDefs::getPixmapFromQtResourcePath(BoardPostDisplayWidget_compact::DEFAULT_BOARD_IMAGE);

	return QIcon(postedImage);
}

void PostedListWidgetWithModel::setAllMessagesReadDo(bool read, uint32_t &token)
{
    if (groupId().isNull() || !IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags))
        return;

    QModelIndex src_index;

    mPostedPostsModel->setAllMsgReadStatus(read);

}

void PostedListWidgetWithModel::openComments(const RsGxsMessageId& msgId)
{
    QModelIndex index = mPostedPostsModel->getIndexOfMessage(msgId);

    if(!index.isValid())
        return;

    RsGxsId current_author;
    ui->idChooser->getChosenId(current_author);

    RsPostedPost post = index.data(Qt::UserRole).value<RsPostedPost>() ;
    auto *commentDialog = new GxsCommentDialog(this,current_author,rsPosted->getTokenService(),rsPosted);

    std::set<RsGxsMessageId> msg_versions({post.mMeta.mMsgId});
    commentDialog->commentLoad(post.mMeta.mGroupId, msg_versions, post.mMeta.mMsgId);

    QString title = QString::fromUtf8(post.mMeta.mMsgName.c_str());
    if(title.length() > 30)
        title = title.left(27) + "...";

    ui->tabWidget->addTab(commentDialog,title);
}

void PostedListWidgetWithModel::markCurrentPostAsRead()
{
    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();

    if(!index.isValid())
        throw std::runtime_error("No post under mouse!");

    mPostedPostsModel->setMsgReadStatus(index,true);
}

void PostedListWidgetWithModel::changeReadStatus(const RsGxsMessageId& msgId,bool b)
{
    QModelIndex index=mPostedPostsModel->getIndexOfMessage(msgId);
    mPostedPostsModel->setMsgReadStatus(index, b);
}
void PostedListWidgetWithModel::tabCloseRequested(int index)
{
    std::cerr << "GxsCommentContainer::tabCloseRequested(" << index << ")";
    std::cerr << std::endl;

    if (index != 0)
    {
        QWidget *comments = ui->tabWidget->widget(index);
        ui->tabWidget->removeTab(index);
        delete comments;
    }
    else
    {
        std::cerr << "GxsCommentContainer::tabCloseRequested() Not closing First Tab";
        std::cerr << std::endl;
    }
}

void PostedListWidgetWithModel::createMsg()
{
	if (groupId().isNull()) {
		return;
	}

	if (!IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags)) {
		return;
	}
    RsGxsId author_id;
    ui->idChooser->getChosenId(author_id);
std::cerr << "Chosing default ID " << author_id<< std::endl;
    PostedCreatePostDialog *msgDialog = new PostedCreatePostDialog(rsPosted,groupId(),author_id);
	msgDialog->show();

	/* window will destroy itself! */
}

void PostedListWidgetWithModel::insertBoardDetails(const RsPostedGroup& group)
{
    // save selection if needed

	/* IMAGE */
	QPixmap chanImage;
	if (group.mGroupImage.mData != NULL) {
		GxsIdDetails::loadPixmapFromData(group.mGroupImage.mData, group.mGroupImage.mSize, chanImage,GxsIdDetails::ORIGINAL);
	} else {
        chanImage = QPixmap(BoardPostDisplayWidget_compact::DEFAULT_BOARD_IMAGE);
	}
    if(group.mMeta.mGroupName.empty())
		ui->namelabel->setText(tr("[No name]"));
    else
		ui->namelabel->setText(QString::fromUtf8(group.mMeta.mGroupName.c_str()));

	ui->logoLabel->setPixmap(chanImage);
    ui->logoLabel->setFixedSize(QSize(ui->logoLabel->height()*chanImage.width()/(float)chanImage.height(),ui->logoLabel->height())); // make the logo have the same aspect ratio than the original image

    ui->submitPostButton->setEnabled(IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags));

	ui->subscribeToolButton->setSubscribed(IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags));
	ui->subscribeToolButton->setEnabled(true);

	RetroShareLink link;

	if (IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags))
		ui->subscribeToolButton->setText(tr("Subscribed") + " " + QString::number(group.mMeta.mPop) );
    else
		ui->subscribeToolButton->setText(tr("Subscribe"));

	ui->infoPosts->setText(QString::number(group.mMeta.mVisibleMsgCount));

	if(group.mMeta.mLastPost==0)
		ui->infoLastPost->setText(tr("Never"));
	else
		ui->infoLastPost->setText(DateTime::formatLongDateTime(group.mMeta.mLastPost));
	QString formatDescription = QString::fromUtf8(group.mDescription.c_str());

	unsigned int formatFlag = RSHTML_FORMATTEXT_EMBED_LINKS;

	// embed smileys ?
	if (Settings->valueFromGroup(QString("ChannelPostsWidget"), QString::fromUtf8("Emoteicons_ChannelDecription"), true).toBool())
		formatFlag |= RSHTML_FORMATTEXT_EMBED_SMILEYS;

	formatDescription = RsHtml().formatText(NULL, formatDescription, formatFlag);

	ui->infoDescription->setText(formatDescription);
	ui->infoAdministrator->setId(group.mMeta.mAuthorId) ;

	link = RetroShareLink::createMessage(group.mMeta.mAuthorId, "");
	ui->infoAdministrator->setText(link.toHtml());

	ui->createdinfolabel->setText(DateTime::formatLongDateTime(group.mMeta.mPublishTs));

	QString distrib_string ( "[unknown]" );

	switch(group.mMeta.mCircleType)
	{
	case GXS_CIRCLE_TYPE_PUBLIC: distrib_string = tr("Public") ;
		break ;
	case GXS_CIRCLE_TYPE_EXTERNAL:
	{
		RsGxsCircleDetails det ;

		// !! What we need here is some sort of CircleLabel, which loads the circle and updates the label when done.

		if(rsGxsCircles->getCircleDetails(group.mMeta.mCircleId,det))
			distrib_string = tr("Restricted to members of circle \"")+QString::fromUtf8(det.mCircleName.c_str()) +"\"";
		else
			distrib_string = tr("Restricted to members of circle ")+QString::fromStdString(group.mMeta.mCircleId.toStdString()) ;
	}
		break ;
	case GXS_CIRCLE_TYPE_YOUR_EYES_ONLY: distrib_string = tr("Your eyes only");
		break ;
	case GXS_CIRCLE_TYPE_LOCAL: distrib_string = tr("You and your friend nodes");
		break ;
	default:
		std::cerr << "(EE) badly initialised group distribution ID = " << group.mMeta.mCircleType << std::endl;
	}

	ui->infoDistribution->setText(distrib_string);
#ifdef TODO
	ui->infoWidget->show();
	ui->feedWidget->hide();
	ui->fileWidget->hide();

	//ui->feedToolButton->setEnabled(false);
	//ui->fileToolButton->setEnabled(false);
#endif
}

#ifdef TODO
int PostedListWidgetWithModel::viewMode()
{
	if (ui->feedToolButton->isChecked()) {
		return VIEW_MODE_FEEDS;
	} else if (ui->fileToolButton->isChecked()) {
		return VIEW_MODE_FILES;
	}

	/* Default */
	return VIEW_MODE_FEEDS;
}
#endif

#ifdef TODO
/*static*/ bool PostedListWidgetWithModel::filterItem(FeedItem *feedItem, const QString &text, int filter)
{
	GxsChannelPostItem *item = dynamic_cast<GxsChannelPostItem*>(feedItem);
	if (!item) {
		return true;
	}

	bool bVisible = text.isEmpty();

	if (!bVisible)
	{
		switch(filter)
		{
			case FILTER_TITLE:
				bVisible = item->getTitleLabel().contains(text,Qt::CaseInsensitive);
			break;
			case FILTER_MSG:
				bVisible = item->getMsgLabel().contains(text,Qt::CaseInsensitive);
			break;
			case FILTER_FILE_NAME:
			{
				std::list<SubFileItem *> fileItems = item->getFileItems();
				std::list<SubFileItem *>::iterator lit;
				for(lit = fileItems.begin(); lit != fileItems.end(); ++lit)
				{
					SubFileItem *fi = *lit;
					QString fileName = QString::fromUtf8(fi->FileName().c_str());
					bVisible = (bVisible || fileName.contains(text,Qt::CaseInsensitive));
				}
				break;
			}
			default:
				bVisible = true;
			break;
		}
	}

	return bVisible;
}

void PostedListWidget::createPostItemFromMetaData(const RsGxsMsgMetaData& meta,bool related)
{
	GxsChannelPostItem *item = NULL;
    RsGxsChannelPost post;

    if(!meta.mOrigMsgId.isNull())
    {
		FeedItem *feedItem = ui->feedWidget->findFeedItem(GxsChannelPostItem::computeIdentifier(meta.mOrigMsgId)) ;
		item = dynamic_cast<GxsChannelPostItem*>(feedItem);

        if(item)
		{
            post = feedItem->post();
			ui->feedWidget->removeFeedItem(item) ;

            post.mOlderVersions.insert(post.mMeta.mMsgId);

			GxsChannelPostItem *item = new GxsChannelPostItem(this, 0, post, true, false,post.mOlderVersions);
			ui->feedWidget->addFeedItem(item, ROLE_PUBLISH, QDateTime::fromTime_t(post.mMeta.mPublishTs));

			return ;
		}
    }

	if (related)
    {
		FeedItem *feedItem = ui->feedWidget->findFeedItem(GxsChannelPostItem::computeIdentifier(meta.mMsgId)) ;
		item = dynamic_cast<GxsChannelPostItem*>(feedItem);
	}
	if (item)
    {
		item->setPost(post);
		ui->feedWidget->setSort(item, ROLE_PUBLISH, QDateTime::fromTime_t(meta.mPublishTs));
	}
    else
    {
		GxsChannelPostItem *item = new GxsChannelPostItem(this, 0, meta.mGroupId,meta.mMsgId, true, true);
		ui->feedWidget->addFeedItem(item, ROLE_PUBLISH, QDateTime::fromTime_t(post.mMeta.mPublishTs));
	}
#ifdef TODO
	ui->fileWidget->addFiles(post, related);
#endif
}

void PostedListWidget::createPostItem(const RsGxsChannelPost& post, bool related)
{
	GxsChannelPostItem *item = NULL;

    const RsMsgMetaData& meta(post.mMeta);

    if(!meta.mOrigMsgId.isNull())
    {
		FeedItem *feedItem = ui->feedWidget->findFeedItem(GxsChannelPostItem::computeIdentifier(meta.mOrigMsgId)) ;
		item = dynamic_cast<GxsChannelPostItem*>(feedItem);

        if(item)
		{
            std::set<RsGxsMessageId> older_versions(item->olderVersions()); // we make a copy because the item will be deleted
			ui->feedWidget->removeFeedItem(item) ;

            older_versions.insert(meta.mMsgId);

			GxsChannelPostItem *item = new GxsChannelPostItem(this, 0, mGroup.mMeta,meta.mMsgId, true, false,older_versions);
			ui->feedWidget->addFeedItem(item, ROLE_PUBLISH, QDateTime::fromTime_t(meta.mPublishTs));

			return ;
		}
    }

	if (related)
    {
		FeedItem *feedItem = ui->feedWidget->findFeedItem(GxsChannelPostItem::computeIdentifier(meta.mMsgId)) ;
		item = dynamic_cast<GxsChannelPostItem*>(feedItem);
	}
	if (item)
    {
		item->setPost(post);
		ui->feedWidget->setSort(item, ROLE_PUBLISH, QDateTime::fromTime_t(meta.mPublishTs));
	}
    else
    {
		GxsChannelPostItem *item = new GxsChannelPostItem(this, 0, mGroup.mMeta,meta.mMsgId, true, true);
		ui->feedWidget->addFeedItem(item, ROLE_PUBLISH, QDateTime::fromTime_t(meta.mPublishTs));
	}

	ui->fileWidget->addFiles(post, related);
}

void PostedListWidget::fillThreadCreatePost(const QVariant &post, bool related, int current, int count)
{
	/* show fill progress */
	if (count) {
		ui->progressBar->setValue(current * ui->progressBar->maximum() / count);
	}

	if (!post.canConvert<RsGxsChannelPost>()) {
		return;
	}

	createPostItem(post.value<RsGxsChannelPost>(), related);
}
#endif

void PostedListWidgetWithModel::blank()
{
#ifdef TODO
	ui->postButton->setEnabled(false);
	ui->subscribeToolButton->setEnabled(false);

	ui->channelName_LB->setText(tr("No Channel Selected"));
	ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/channels.png"));
	ui->infoPosts->setText("");
	ui->infoLastPost->setText("");
	ui->infoAdministrator->setText("");
	ui->infoDistribution->setText("");
	ui->infoCreated->setText("");
	ui->infoDescription->setText("");

	mChannelPostsModel->clear();
	mChannelPostFilesModel->clear();
	ui->postDetails_TE->clear();
	ui->postLogo_LB->hide();
	ui->postName_LB->hide();
	ui->postTime_LB->hide();
#endif
	groupNameChanged(QString());
}

bool PostedListWidgetWithModel::navigate(const RsGxsMessageId& msgId)
{
	QModelIndex index = mPostedPostsModel->getIndexOfMessage(msgId);

    if(!index.isValid())
    {
        std::cerr << "(EE) Cannot navigate to msg " << msgId << " in board " << mGroup.mMeta.mGroupId << ": index unknown. Setting mNavigatePendingMsgId." << std::endl;

        mSelectedPost = msgId;		// not found. That means the forum may not be loaded yet. So we keep that post in mind, for after loading.
		return true;						// we have to return true here, otherwise the caller will intepret the async loading as an error.
    }

	ui->postsTree->selectionModel()->setCurrentIndex(index,QItemSelectionModel::ClearAndSelect);
	ui->postsTree->scrollTo(index);//May change if model reloaded
	ui->postsTree->setFocus();

	return true;
}

void PostedListWidgetWithModel::subscribeGroup(bool subscribe)
{
	RsGxsGroupId grpId(groupId());
	if (grpId.isNull()) return;

	RsThread::async([=]()
	{
        uint32_t token;
		rsPosted->subscribeToGroup(token,grpId, subscribe);
	} );
}

void PostedListWidgetWithModel::voteMsg(RsGxsGrpMsgIdPair msg,bool up_or_down)
{
    RsGxsId voter_id ;
    if(ui->idChooser->getChosenId(voter_id) != GxsIdChooser::KnowId)
    {
        std::cerr << "(EE) No id returned by GxsIdChooser. Somthing's wrong?" << std::endl;
        return;
    }

    rsPosted->voteForPost(up_or_down,msg.first,msg.second,voter_id);
}

#ifdef TODO
class PostedPostsReadData
{
public:
	PostedPostsReadData(bool read)
	{
		mRead = read;
		mLastToken = 0;
	}

public:
	bool mRead;
	uint32_t mLastToken;
};

static void setAllMessagesReadCallback(FeedItem *feedItem, void *data)
{
	GxsChannelPostItem *channelPostItem = dynamic_cast<GxsChannelPostItem*>(feedItem);
	if (!channelPostItem) {
		return;
	}

	GxsChannelPostsReadData *readData = (GxsChannelPostsReadData*) data;
	bool isRead = !channelPostItem->isUnread() ;

	if(channelPostItem->isLoaded() && (isRead == readData->mRead))
		return ;

	RsGxsGrpMsgIdPair msgPair = std::make_pair(channelPostItem->groupId(), channelPostItem->messageId());
	rsGxsChannels->setMessageReadStatus(readData->mLastToken, msgPair, readData->mRead);
}

void PostedListWidgetWithModel::setAllMessagesReadDo(bool read, uint32_t &token)
{
	if (groupId().isNull() || !IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags)) {
		return;
	}

	GxsChannelPostsReadData data(read);
	//ui->feedWidget->withAll(setAllMessagesReadCallback, &data);

	token = data.mLastToken;
}

#endif

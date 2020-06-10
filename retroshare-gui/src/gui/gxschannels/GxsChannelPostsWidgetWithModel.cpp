/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelPostsWidget.cpp                *
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
#include <QSignalMapper>
#include <QPainter>

#include "retroshare/rsgxscircles.h"

#include "ui_GxsChannelPostsWidgetWithModel.h"
#include "gui/feeds/GxsChannelPostItem.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/misc.h"
#include "gui/gxschannels/CreateGxsChannelMsg.h"
#include "gui/common/UIStateHelper.h"
#include "gui/settings/rsharesettings.h"
#include "gui/feeds/SubFileItem.h"
#include "gui/notifyqt.h"
#include "gui/RetroShareLink.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "util/qtthreadsutils.h"

#include "GxsChannelPostsWidgetWithModel.h"
#include "GxsChannelPostsModel.h"
#include "GxsChannelPostFilesModel.h"
#include "GxsChannelFilesStatusWidget.h"

#include <algorithm>

#define CHAN_DEFAULT_IMAGE ":/icons/png/channels.png"

#define ROLE_PUBLISH FEED_TREEWIDGET_SORTROLE

/****
 * #define DEBUG_CHANNEL
 ***/

static const int mTokenTypeGroupData = 1;

static const int CHANNEL_TABS_DETAILS= 0;
static const int CHANNEL_TABS_POSTS  = 1;

/* View mode */
#define VIEW_MODE_FEEDS  1
#define VIEW_MODE_FILES  2

// Size of thumbnails as a function of the height of the font. An aspect ratio of 3/4 is good.

#define THUMBNAIL_W  4
#define THUMBNAIL_H  6

// Determine the Shape and size of cells as a factor of the font height. An aspect ratio of 3/4 is what's needed
// for the image, so it's important that the height is a bit larger so as to leave some room for the text.
//
//
#define COLUMN_SIZE_FONT_FACTOR_W  6
#define COLUMN_SIZE_FONT_FACTOR_H  10

// This variable determines the zoom factor on the text below thumbnails. 2.0 is mostly correct for all screen.
#define THUMBNAIL_OVERSAMPLE_FACTOR 2.0

Q_DECLARE_METATYPE(RsGxsFile)

// Class to paint the thumbnails with title

class ThumbnailView: public QWidget
{
public:
    ThumbnailView(const RsGxsChannelPost& post,QWidget *parent=NULL)
        : QWidget(parent)
    {
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *lb = new QLabel(this);
        lb->setScaledContents(true);
        layout->addWidget(lb);

        QLabel *lt = new QLabel(this);
        layout->addWidget(lt);

		setLayout(layout);

		setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);

        // now fill the data

		QPixmap thumbnail;
		GxsIdDetails::loadPixmapFromData(post.mThumbnail.mData, post.mThumbnail.mSize, thumbnail,GxsIdDetails::ORIGINAL);

        lb->setPixmap(thumbnail);

		QFontMetricsF fm(font());
		int W = THUMBNAIL_OVERSAMPLE_FACTOR * THUMBNAIL_W * fm.height() ;
		int H = THUMBNAIL_OVERSAMPLE_FACTOR * THUMBNAIL_H * fm.height() ;

        lb->setFixedSize(W,H);

        lt->setText(QString::fromUtf8(post.mMeta.mMsgName.c_str()));
        lt->setMaximumWidth(W);
        lt->setWordWrap(true);

        adjustSize();
        update();
    }
};

// Delegate used to paint into the table of thumbnails

void ChannelPostDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	// prepare
	painter->save();
	painter->setClipRect(option.rect);

	RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;

	painter->save();
    painter->fillRect( option.rect, option.backgroundBrush);
    painter->restore();

	ThumbnailView w(post);

	QPixmap pixmap(w.size());

    if(option.state & QStyle::State_Selected)
		pixmap.fill(QRgb(0xff308dc7));	// I dont know how to grab the backgroud color for selected objects automatically.
	else
		pixmap.fill(QRgb(0x00ffffff));	// choose a fully transparent background

	w.render(&pixmap,QPoint(),QRegion(),QWidget::DrawChildren );// draw the widgets, not the background

    // debug
 	// if(index.row()==0 && index.column()==0)
	// {
	// 	QFile file("yourFile.png");
	// 	file.open(QIODevice::WriteOnly);
	// 	pixmap.save(&file, "PNG");
 	//  std::cerr << "Saved pxmap to png" << std::endl;
	// }
    //std::cerr << "option.rect = " << option.rect.width() << "x" << option.rect.height() << ". fm.height()=" << QFontMetricsF(option.font).height() << std::endl;

	painter->drawPixmap(option.rect.topLeft(),
                        pixmap.scaled(option.rect.width(),option.rect.width()*w.height()/(float)w.width(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
}

QSize ChannelPostDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // This is the only place where we actually set the size of cells

    QFontMetricsF fm(option.font);

    return QSize(COLUMN_SIZE_FONT_FACTOR_W*fm.height(),COLUMN_SIZE_FONT_FACTOR_H*fm.height());
}

QWidget *ChannelPostFilesDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex& index) const
{
	RsGxsFile file = index.data(Qt::UserRole).value<RsGxsFile>() ;

    if(index.column() == RsGxsChannelPostFilesModel::COLUMN_FILES_FILE)
		return new GxsChannelFilesStatusWidget(file,parent);
    else
        return NULL;
}
void ChannelPostFilesDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
	editor->setGeometry(option.rect);
}

void ChannelPostFilesDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	RsGxsFile file = index.data(Qt::UserRole).value<RsGxsFile>() ;

	// prepare
	painter->save();
	painter->setClipRect(option.rect);

    painter->save();

    painter->fillRect( option.rect, option.backgroundBrush);
	//optionFocusRect.backgroundColor = option.palette.color(colorgroup, (option.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Background);
    painter->restore();

    switch(index.column())
    {
    case RsGxsChannelPostFilesModel::COLUMN_FILES_NAME: painter->drawText(option.rect,Qt::AlignLeft | Qt::AlignVCenter," " + QString::fromUtf8(file.mName.c_str()));
        	break;
    case RsGxsChannelPostFilesModel::COLUMN_FILES_SIZE: painter->drawText(option.rect,Qt::AlignRight | Qt::AlignVCenter,misc::friendlyUnit(qulonglong(file.mSize)));
        	break;
    case RsGxsChannelPostFilesModel::COLUMN_FILES_FILE: {

		GxsChannelFilesStatusWidget w(file);

        w.setFixedWidth(option.rect.width());
		w.setFixedHeight(option.rect.height());

		QPixmap pixmap(w.size());
		pixmap.fill(QRgb(0x00ffffff));	// choose a fully transparent background
		w.render(&pixmap,QPoint(),QRegion(),QWidget::DrawChildren );// draw the widgets, not the background

        painter->drawPixmap(option.rect.topLeft(),pixmap);
    }
        break;

    default:
            painter->drawText(option.rect,Qt::AlignLeft,QString("[No data]"));
		break;
    }
}

QSize ChannelPostFilesDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	RsGxsFile file = index.data(Qt::UserRole).value<RsGxsFile>() ;

    QFontMetricsF fm(option.font);

    switch(index.column())
    {
    case RsGxsChannelPostFilesModel::COLUMN_FILES_NAME: return QSize(1.1*fm.width(QString::fromUtf8(file.mName.c_str())),fm.height());
    case RsGxsChannelPostFilesModel::COLUMN_FILES_SIZE: return QSize(1.1*fm.width(misc::friendlyUnit(qulonglong(file.mSize))),fm.height());
    default:
    case RsGxsChannelPostFilesModel::COLUMN_FILES_FILE: return QSize(option.rect.width(),GxsChannelFilesStatusWidget(file).height());
    }
}

class RsGxsChannelPostFilesProxyModel: public QSortFilterProxyModel
{
public:
    RsGxsChannelPostFilesProxyModel(QObject *parent = NULL): QSortFilterProxyModel(parent) {}

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
    {
		return left.data(RsGxsChannelPostFilesModel::SortRole) < right.data(RsGxsChannelPostFilesModel::SortRole) ;
    }

    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
	{
		if(filter_list.empty())
			return true;

		QString name = sourceModel()->data(sourceModel()->index(source_row,RsGxsChannelPostFilesModel::COLUMN_FILES_NAME,source_parent)).toString();

		for(auto& s:filter_list)
			if(!name.contains(s,Qt::CaseInsensitive))
				return false;

        return true;
	}

    void setFilterList(const QStringList& str)
    {
        filter_list = str;
        invalidateFilter();
    }

private:
    QStringList filter_list;
};

/** Constructor */
GxsChannelPostsWidgetWithModel::GxsChannelPostsWidgetWithModel(const RsGxsGroupId &channelId, QWidget *parent) :
	GxsMessageFrameWidget(rsGxsChannels, parent),
	ui(new Ui::GxsChannelPostsWidgetWithModel)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	ui->postsTree->setModel(mChannelPostsModel = new RsGxsChannelPostsModel());
    ui->postsTree->setItemDelegate(new ChannelPostDelegate());

	mChannelPostFilesModel = new RsGxsChannelPostFilesModel(this);

    mChannelPostFilesProxyModel = new RsGxsChannelPostFilesProxyModel(this);
    mChannelPostFilesProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mChannelPostFilesProxyModel->setSourceModel(mChannelPostFilesModel);
	mChannelPostFilesProxyModel->setDynamicSortFilter(true);

    ui->channelPostFiles_TV->setModel(mChannelPostFilesProxyModel);
    ui->channelPostFiles_TV->setItemDelegate(new ChannelPostFilesDelegate());
    ui->channelPostFiles_TV->setPlaceholderText(tr("Post files"));
    ui->channelPostFiles_TV->setSortingEnabled(true);
    ui->channelPostFiles_TV->sortByColumn(0, Qt::AscendingOrder);

    ui->channelFiles_TV->setPlaceholderText(tr("No files in the channel, or no channel selected"));
    ui->channelFiles_TV->setModel(mChannelFilesModel = new RsGxsChannelPostFilesModel());
    ui->channelFiles_TV->setItemDelegate(new ChannelPostFilesDelegate());

	connect(ui->channelPostFiles_TV->header(),SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(sortColumn(int,Qt::SortOrder)));
    connect(ui->postsTree->selectionModel(),SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),this,SLOT(showPostDetails()));
    connect(mChannelPostsModel,SIGNAL(channelLoaded()),this,SLOT(updateChannelFiles()));

    QFontMetricsF fm(font());

    for(int i=0;i<mChannelPostsModel->columnCount();++i)
		ui->postsTree->setColumnWidth(i,COLUMN_SIZE_FONT_FACTOR_W*fm.height());

	/* Setup UI helper */

	/* Connect signals */
	connect(ui->postButton, SIGNAL(clicked()), this, SLOT(createMsg()));
	connect(ui->subscribeToolButton, SIGNAL(subscribe(bool)), this, SLOT(subscribeGroup(bool)));
	connect(NotifyQt::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));

	ui->postButton->setText(tr("Add new post"));
	
	/* add filter actions */
    ui->filterLineEdit->setPlaceholderText(tr("Search..."));
	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));

    ui->postsTree->setPlaceholderText(tr("Thumbnails"));
    ui->postsTree->setMinimumWidth(COLUMN_SIZE_FONT_FACTOR_W*QFontMetricsF(font()).height()+1);

    connect(ui->postsTree,SIGNAL(sizeChanged(QSize)),this,SLOT(handlePostsTreeSizeChange(QSize)));

	//ui->nameLabel->setMinimumWidth(20);

	/* Initialize feed widget */
	//ui->feedWidget->setSortRole(ROLE_PUBLISH, Qt::DescendingOrder);
	//ui->feedWidget->setFilterCallback(filterItem);

	/* load settings */
	processSettings(true);

	/* Initialize subscribe button */
	QIcon icon;
	icon.addPixmap(QPixmap(":/images/redled.png"), QIcon::Normal, QIcon::On);
	icon.addPixmap(QPixmap(":/images/start.png"), QIcon::Normal, QIcon::Off);
	mAutoDownloadAction = new QAction(icon, "", this);
	mAutoDownloadAction->setCheckable(true);
	connect(mAutoDownloadAction, SIGNAL(triggered()), this, SLOT(toggleAutoDownload()));

	ui->subscribeToolButton->addSubscribedAction(mAutoDownloadAction);

    ui->commentsDialog->setTokenService(rsGxsChannels->getTokenService(),rsGxsChannels);

	/* Initialize GUI */
	setAutoDownload(false);
	settingsChanged();

	mChannelPostsModel->updateChannel(channelId);

	mEventHandlerId = 0;
	// Needs to be asynced because this function is called by another thread!
	rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
    {
        RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this );
    }, mEventHandlerId, RsEventType::GXS_CHANNELS );
}

void GxsChannelPostsWidgetWithModel::sortColumn(int col,Qt::SortOrder so)
{
    std::cerr << "Sorting!!"<< std::endl;
    mChannelPostFilesProxyModel->sort(col,so);
}

void GxsChannelPostsWidgetWithModel::handlePostsTreeSizeChange(QSize s)
{
//    adjustSize();
//
    int n_columns = std::max(1,(int)floor(s.width() / (COLUMN_SIZE_FONT_FACTOR_W*QFontMetricsF(font()).height())));
    std::cerr << "nb columns: " << n_columns << std::endl;

    if(n_columns != mChannelPostsModel->columnCount())
		mChannelPostsModel->setNumColumns(n_columns);
}

void GxsChannelPostsWidgetWithModel::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
	const RsGxsChannelEvent *e = dynamic_cast<const RsGxsChannelEvent*>(event.get());

	if(!e)
		return;

	switch(e->mChannelEventCode)
	{
		case RsChannelEventCode::NEW_CHANNEL:     // [[fallthrough]];
		case RsChannelEventCode::UPDATED_CHANNEL: // [[fallthrough]];
		case RsChannelEventCode::NEW_MESSAGE:     // [[fallthrough]];
		case RsChannelEventCode::UPDATED_MESSAGE:
			if(e->mChannelGroupId == groupId())
				updateDisplay(true);
		break;
//		case RsChannelEventCode::READ_STATUS_CHANGED:
//			if (FeedItem *feedItem = ui->feedWidget->findFeedItem(GxsChannelPostItem::computeIdentifier(e->mChannelMsgId)))
//				if (GxsChannelPostItem *channelPostItem = dynamic_cast<GxsChannelPostItem*>(feedItem))
//					channelPostItem->setReadStatus(false,!channelPostItem->isUnread());
//					//channelPostItem->setReadStatus(false,e->Don't get read status. Will be more easier and accurate);
//		break;
		default:
		break;
	}
}

void GxsChannelPostsWidgetWithModel::showPostDetails()
{
    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();

    if(!index.isValid())
    {
        ui->postDetails_TE->clear();
        ui->postLogo_LB->clear();
		mChannelPostFilesModel->clear();
        return;
    }
    if(index.row()==0 && index.column()==0)
        std::cerr << "here" << std::endl;

	RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;

    mChannelPostFilesModel->setFiles(post.mFiles);

    auto all_msgs_versions(post.mOlderVersions);
    all_msgs_versions.insert(post.mMeta.mMsgId);

    ui->commentsDialog->commentLoad(post.mMeta.mGroupId, all_msgs_versions, post.mMeta.mMsgId);

    std::cerr << "Showing details about selected index : "<< index.row() << "," << index.column() << std::endl;

    ui->postDetails_TE->setText(RsHtml().formatText(NULL, QString::fromUtf8(post.mMsg.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));

	QPixmap postImage;

	if (post.mThumbnail.mData != NULL)
		GxsIdDetails::loadPixmapFromData(post.mThumbnail.mData, post.mThumbnail.mSize, postImage,GxsIdDetails::ORIGINAL);
	else
		postImage = QPixmap(CHAN_DEFAULT_IMAGE);

    int W = QFontMetricsF(font()).height() * 8;

    // Using fixed width so that the post will not displace the text when we browse.

	ui->postLogo_LB->setPixmap(postImage);
	ui->postName_LB->setText(QString::fromUtf8(post.mMeta.mMsgName.c_str()));

	ui->postLogo_LB->setFixedSize(W,postImage.height()/(float)postImage.width()*W);
	ui->postName_LB->setFixedWidth(W);

    ui->channelPostFiles_TV->resizeColumnToContents(RsGxsChannelPostFilesModel::COLUMN_FILES_FILE);
    ui->channelPostFiles_TV->resizeColumnToContents(RsGxsChannelPostFilesModel::COLUMN_FILES_SIZE);
    ui->channelPostFiles_TV->setAutoSelect(true);

}

void GxsChannelPostsWidgetWithModel::updateChannelFiles()
{
    std::list<RsGxsFile> files;

    mChannelPostsModel->getFilesList(files);
    mChannelFilesModel->setFiles(files);

    ui->channelFiles_TV->resizeColumnToContents(RsGxsChannelPostFilesModel::COLUMN_FILES_FILE);
    ui->channelFiles_TV->resizeColumnToContents(RsGxsChannelPostFilesModel::COLUMN_FILES_SIZE);
    ui->channelFiles_TV->setAutoSelect(true);

    mChannelPostFilesProxyModel->sort(0, Qt::AscendingOrder);
}

void GxsChannelPostsWidgetWithModel::updateGroupData()
{
	if(groupId().isNull())
		return;

	RsThread::async([this]()
	{
		std::vector<RsGxsChannelGroup> groups;

		if(!rsGxsChannels->getChannelsInfo(std::list<RsGxsGroupId>{ groupId() }, groups))
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to get autodownload value for channel: " << groupId() << std::endl;
			return;
		}

		if(groups.size() != 1)
		{
			RsErr() << __PRETTY_FUNCTION__ << " cannot retrieve channel data for group ID " << groupId() << ": ERROR." << std::endl;
			return;
		}

		RsQThreadUtils::postToObject( [this,groups]()
        {
            mGroup = groups[0];
			mChannelPostsModel->updateChannel(groupId());

            insertChannelDetails(mGroup);
        } );
	});
}

void GxsChannelPostsWidgetWithModel::updateDisplay(bool complete)
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
#warning todo
		//saveExpandedItems(mSavedExpandedMessages);

        //if(mGroupId != mChannelPostsModel->currentGroupId())
        //    mThreadId.clear();

		updateGroupData();

		return;
	}
}
GxsChannelPostsWidgetWithModel::~GxsChannelPostsWidgetWithModel()
{
	rsEvents->unregisterEventsHandler(mEventHandlerId);
	// save settings
	processSettings(false);

	delete(mAutoDownloadAction);
	delete ui;
}

void GxsChannelPostsWidgetWithModel::processSettings(bool load)
{
	Settings->beginGroup(QString("ChannelPostsWidget"));

	if (load) {
#ifdef TO_REMOVE
		// load settings

		/* Filter */
		//ui->filterLineEdit->setCurrentFilter(Settings->value("filter", FILTER_TITLE).toInt());

		/* View mode */
		//setViewMode(Settings->value("viewMode", VIEW_MODE_FEEDS).toInt());
#endif
	} else {
#ifdef TO_REMOVE
		// save settings

		/* Filter */
		//Settings->setValue("filter", ui->filterLineEdit->currentFilter());

		/* View mode */
		//Settings->setValue("viewMode", viewMode());
#endif
	}

	Settings->endGroup();
}

void GxsChannelPostsWidgetWithModel::settingsChanged()
{
	mUseThread = Settings->getChannelLoadThread();

	//mStateHelper->setWidgetVisible(ui->progressBar, mUseThread);
}

QString GxsChannelPostsWidgetWithModel::groupName(bool)
{
    return "Group name" ;
}

void GxsChannelPostsWidgetWithModel::groupNameChanged(const QString &name)
{
//	if (groupId().isNull()) {
//		ui->nameLabel->setText(tr("No Channel Selected"));
//		ui->logoLabel->setPixmap(QPixmap(":/icons/png/channels.png"));
//	} else {
//		ui->nameLabel->setText(name);
//	}
}

QIcon GxsChannelPostsWidgetWithModel::groupIcon()
{
//	if (mStateHelper->isLoading(mTokenTypeGroupData) || mStateHelper->isLoading(mTokenTypeAllPosts)) {
//		return QIcon(":/images/kalarm.png");
//	}

//	if (mNewCount) {
//		return QIcon(":/images/message-state-new.png");
//	}

	return QIcon();
}

/*************************************************************************************/
/*************************************************************************************/
/*************************************************************************************/

// Callback from Widget->FeedHolder->ServiceDialog->CommentContainer->CommentDialog,
void GxsChannelPostsWidgetWithModel::openComments(uint32_t /*type*/, const RsGxsGroupId &groupId, const QVector<RsGxsMessageId>& msg_versions,const RsGxsMessageId &msgId, const QString &title)
{
	emit loadComment(groupId, msg_versions,msgId, title);
}

void GxsChannelPostsWidgetWithModel::createMsg()
{
	if (groupId().isNull()) {
		return;
	}

	if (!IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags)) {
		return;
	}

	CreateGxsChannelMsg *msgDialog = new CreateGxsChannelMsg(groupId());
	msgDialog->show();

	/* window will destroy itself! */
}

void GxsChannelPostsWidgetWithModel::insertChannelDetails(const RsGxsChannelGroup &group)
{
	/* IMAGE */
	QPixmap chanImage;
	if (group.mImage.mData != NULL) {
		GxsIdDetails::loadPixmapFromData(group.mImage.mData, group.mImage.mSize, chanImage,GxsIdDetails::ORIGINAL);
	} else {
		chanImage = QPixmap(CHAN_DEFAULT_IMAGE);
	}
	ui->logoLabel->setPixmap(chanImage);
    ui->logoLabel->setFixedSize(QSize(ui->logoLabel->height()*chanImage.width()/(float)chanImage.height(),ui->logoLabel->height())); // make the logo have the same aspect ratio than the original image

	ui->postButton->setEnabled(bool(group.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH));

	ui->subscribeToolButton->setSubscribed(IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags));
	ui->subscribeToolButton->setEnabled(true);


	bool autoDownload ;
	rsGxsChannels->getChannelAutoDownload(group.mMeta.mGroupId,autoDownload);
	setAutoDownload(autoDownload);

	RetroShareLink link;

	if (IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags))
    {
		ui->subscribeToolButton->setText(tr("Subscribed") + " " + QString::number(group.mMeta.mPop) );
		//ui->feedToolButton->setEnabled(true);
		//ui->fileToolButton->setEnabled(true);
        ui->channel_TW->setTabEnabled(CHANNEL_TABS_POSTS,true);
        ui->details_TW->setEnabled(true);
	}
    else
    {
        ui->details_TW->setEnabled(false);
        ui->channel_TW->setTabEnabled(CHANNEL_TABS_POSTS,false);
    }


	ui->infoPosts->setText(QString::number(group.mMeta.mVisibleMsgCount));
	if(group.mMeta.mLastPost==0)
		ui->infoLastPost->setText(tr("Never"));
	else
		ui->infoLastPost->setText(DateTime::formatLongDateTime(group.mMeta.mLastPost));
	QString formatDescription = QString::fromUtf8(group.mDescription.c_str());

	unsigned int formatFlag = RSHTML_FORMATTEXT_EMBED_LINKS;

	// embed smileys ?
	if (Settings->valueFromGroup(QString("ChannelPostsWidget"), QString::fromUtf8("Emoteicons_ChannelDecription"), true).toBool()) {
		formatFlag |= RSHTML_FORMATTEXT_EMBED_SMILEYS;
	}

	formatDescription = RsHtml().formatText(NULL, formatDescription, formatFlag);

	ui->infoDescription->setText(formatDescription);

	ui->infoAdministrator->setId(group.mMeta.mAuthorId) ;

	link = RetroShareLink::createMessage(group.mMeta.mAuthorId, "");
	ui->infoAdministrator->setText(link.toHtml());

	ui->infoCreated->setText(DateTime::formatLongDateTime(group.mMeta.mPublishTs));

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

	ui->subscribeToolButton->setText(tr("Subscribe ") + " " + QString::number(group.mMeta.mPop) );
}

#ifdef TODO
int GxsChannelPostsWidgetWithModel::viewMode()
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

void GxsChannelPostsWidgetWithModel::setViewMode(int viewMode)
{
#ifdef TODO
	switch (viewMode) {
	case VIEW_MODE_FEEDS:
		ui->feedWidget->show();
		ui->fileWidget->hide();

		ui->feedToolButton->setChecked(true);
		ui->fileToolButton->setChecked(false);

		break;
	case VIEW_MODE_FILES:
		ui->feedWidget->hide();
		ui->fileWidget->show();

		ui->feedToolButton->setChecked(false);
		ui->fileToolButton->setChecked(true);

		break;
	default:
		setViewMode(VIEW_MODE_FEEDS);
		return;
	}
#endif
}

void GxsChannelPostsWidgetWithModel::filterChanged(QString s)
{
    QStringList ql = s.split(' ',QString::SkipEmptyParts);
    uint32_t count;
    mChannelPostsModel->setFilter(ql,count);

	mChannelPostFilesProxyModel->setFilterKeyColumn(RsGxsChannelPostFilesModel::COLUMN_FILES_NAME);
	mChannelPostFilesProxyModel->setFilterList(ql);
	mChannelPostFilesProxyModel->setFilterRegExp(s) ;// triggers a re-display. s is actually not used.
}

#ifdef TODO
/*static*/ bool GxsChannelPostsWidgetWithModel::filterItem(FeedItem *feedItem, const QString &text, int filter)
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

void GxsChannelPostsWidget::createPostItemFromMetaData(const RsGxsMsgMetaData& meta,bool related)
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

void GxsChannelPostsWidget::createPostItem(const RsGxsChannelPost& post, bool related)
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

void GxsChannelPostsWidget::fillThreadCreatePost(const QVariant &post, bool related, int current, int count)
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

void GxsChannelPostsWidgetWithModel::blank()
{
	ui->postButton->setEnabled(false);
	ui->subscribeToolButton->setEnabled(false);
	
	mChannelPostsModel->clear();
    groupNameChanged(QString());

	//ui->infoWidget->hide();
}

bool GxsChannelPostsWidgetWithModel::navigate(const RsGxsMessageId &msgId)
{
#warning TODO
	//return ui->feedWidget->scrollTo(feedItem, true);
    return true;
}

void GxsChannelPostsWidgetWithModel::subscribeGroup(bool subscribe)
{
	RsGxsGroupId grpId(groupId());
	if (grpId.isNull()) return;

	RsThread::async([=]()
	{
		rsGxsChannels->subscribeToChannel(grpId, subscribe);
	} );
}

void GxsChannelPostsWidgetWithModel::setAutoDownload(bool autoDl)
{
	mAutoDownloadAction->setChecked(autoDl);
	mAutoDownloadAction->setText(autoDl ? tr("Disable Auto-Download") : tr("Enable Auto-Download"));
}

void GxsChannelPostsWidgetWithModel::toggleAutoDownload()
{
	RsGxsGroupId grpId = groupId();
	if (grpId.isNull()) {
		return;
	}

	bool autoDownload;
	if(!rsGxsChannels->getChannelAutoDownload(grpId, autoDownload))
	{
		std::cerr << __PRETTY_FUNCTION__ << " failed to get autodownload value "
		          << "for channel: " << grpId.toStdString() << std::endl;
		return;
	}

	RsThread::async([this, grpId, autoDownload]()
	{
		if(!rsGxsChannels->setChannelAutoDownload(grpId, !autoDownload))
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to set autodownload "
			          << "for channel: " << grpId.toStdString() << std::endl;
			return;
		}
	});
}

class GxsChannelPostsReadData
{
public:
	GxsChannelPostsReadData(bool read)
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

void GxsChannelPostsWidgetWithModel::setAllMessagesReadDo(bool read, uint32_t &token)
{
	if (groupId().isNull() || !IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags)) {
		return;
	}

	GxsChannelPostsReadData data(read);
	//ui->feedWidget->withAll(setAllMessagesReadCallback, &data);

	token = data.mLastToken;
}


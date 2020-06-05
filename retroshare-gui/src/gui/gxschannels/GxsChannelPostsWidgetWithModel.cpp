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

Q_DECLARE_METATYPE(RsGxsFile)

void ChannelPostDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QString byteUnits[4] = {tr("B"), tr("KB"), tr("MB"), tr("GB")};
	QStyleOptionViewItem opt = option;
	QStyleOptionProgressBarV2 newopt;
	QRect pixmapRect;
	QPixmap pixmap;
	qlonglong fileSize;
	double dlspeed, multi;
	int seconds,minutes, hours, days;
	qlonglong remaining;
	QString temp ;
	qlonglong completed;
	qlonglong downloadtime;
	qint64 qi64Value;

	// prepare
	painter->save();
	painter->setClipRect(opt.rect);

	RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;
    //const RsGxsChannelPost& post(*pinfo);

	QVariant value = index.data(Qt::TextColorRole);

	if(value.isValid() && qvariant_cast<QColor>(value).isValid())
		opt.palette.setColor(QPalette::Text, qvariant_cast<QColor>(value));

	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;

	if(option.state & QStyle::State_Selected)
		painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
	else
		painter->setPen(opt.palette.color(cg, QPalette::Text));

	//painter->drawText(option.rect, Qt::AlignRight, QString("TODO"));

	QPixmap thumbnail;
	GxsIdDetails::loadPixmapFromData(post.mThumbnail.mData, post.mThumbnail.mSize, thumbnail,GxsIdDetails::ORIGINAL);

    QFontMetricsF fm(opt.font);

	int W = IMAGE_SIZE_FACTOR_W * fm.height() * IMAGE_ZOOM_FACTOR;
	int H = IMAGE_SIZE_FACTOR_H * fm.height() * IMAGE_ZOOM_FACTOR;

	int w = fm.width("X") ;
	int h = fm.height() ;

    float img_coord_x = IMAGE_MARGIN_FACTOR*0.5*w;
    float img_coord_y = IMAGE_MARGIN_FACTOR*0.5*h;

    QPoint img_pos(img_coord_x,img_coord_y);
    QPoint img_size(W,H);

    QPoint txt_pos(0,img_coord_y + H + h);

    painter->drawPixmap(QRect(opt.rect.topLeft() + img_pos,opt.rect.topLeft()+img_pos+img_size),thumbnail.scaled(W,H,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    painter->drawText(QRect(option.rect.topLeft() + txt_pos,option.rect.bottomRight()),Qt::AlignCenter,QString::fromStdString(post.mMeta.mMsgName));
}

QSize ChannelPostDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;

	//QPixmap thumbnail;
	//GxsIdDetails::loadPixmapFromData(post.mThumbnail.mData, post.mThumbnail.mSize, thumbnail,GxsIdDetails::ORIGINAL);

    QFontMetricsF fm(option.font);

	int W = IMAGE_SIZE_FACTOR_W * fm.height() * IMAGE_ZOOM_FACTOR;
	int H = IMAGE_SIZE_FACTOR_H * fm.height() * IMAGE_ZOOM_FACTOR;

	int h = fm.height() ;
	int w = fm.width("X") ;

	return QSize(W+IMAGE_MARGIN_FACTOR*w,H + 2*h);
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

#ifdef TODO

	QString byteUnits[4] = {tr("B"), tr("KB"), tr("MB"), tr("GB")};

	QStyleOptionViewItem opt = option;
	QStyleOptionProgressBarV2 newopt;
	QRect pixmapRect;
	QPixmap pixmap;
	qlonglong fileSize;
	double dlspeed, multi;
	int seconds,minutes, hours, days;
	qlonglong remaining;
	QString temp ;
	qlonglong completed;
	qlonglong downloadtime;
	qint64 qi64Value;
#endif

	// prepare
	painter->save();
	painter->setClipRect(option.rect);

    painter->save();

    painter->fillRect( option.rect, option.backgroundBrush);
	//optionFocusRect.backgroundColor = option.palette.color(colorgroup, (option.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Background);
    painter->restore();

#ifdef TODO
	RsGxsFile file = index.data(Qt::UserRole).value<RsGxsFile>() ;
	QVariant value = index.data(Qt::TextColorRole);

	if(value.isValid() && qvariant_cast<QColor>(value).isValid())
		opt.palette.setColor(QPalette::Text, qvariant_cast<QColor>(value));

	QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;

	if(option.state & QStyle::State_Selected)
		painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
	else
		painter->setPen(option.palette.color(cg, QPalette::Text));
#endif

    switch(index.column())
    {
    case RsGxsChannelPostFilesModel::COLUMN_FILES_NAME: painter->drawText(option.rect,Qt::AlignLeft | Qt::AlignVCenter," " + QString::fromUtf8(file.mName.c_str()));
        	break;
    case RsGxsChannelPostFilesModel::COLUMN_FILES_SIZE: painter->drawText(option.rect,Qt::AlignRight | Qt::AlignVCenter,misc::friendlyUnit(qulonglong(file.mSize)));
        	break;
    case RsGxsChannelPostFilesModel::COLUMN_FILES_FILE: {

		GxsChannelFilesStatusWidget w(file);

        w.setFixedWidth(option.rect.width());

		QPixmap pixmap(w.size());
    	pixmap.fill(option.palette.color(QPalette::Background));	// choose the background
		w.render(&pixmap,QPoint(),QRegion(),QWidget::DrawChildren );// draw the widgets, not the background

        painter->drawPixmap(option.rect.topLeft(),pixmap);

#ifdef TODO
        FileInfo finfo;
        if(rsFiles->FileDetails(file.mHash,RS_FILE_HINTS_DOWNLOAD,finfo))
			painter->drawText(option.rect,Qt::AlignLeft,QString::number(finfo.transfered));
#endif
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

/** Constructor */
GxsChannelPostsWidgetWithModel::GxsChannelPostsWidgetWithModel(const RsGxsGroupId &channelId, QWidget *parent) :
	GxsMessageFrameWidget(rsGxsChannels, parent),
	ui(new Ui::GxsChannelPostsWidgetWithModel)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	ui->postsTree->setModel(mChannelPostsModel = new RsGxsChannelPostsModel());
    ui->postsTree->setItemDelegate(new ChannelPostDelegate());

    ui->channelPostFiles_TV->setModel(mChannelPostFilesModel = new RsGxsChannelPostFilesModel());
    ui->channelPostFiles_TV->setItemDelegate(new ChannelPostFilesDelegate());

    connect(ui->postsTree->selectionModel(),SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),this,SLOT(showPostDetails()));

	/* Setup UI helper */

	//mStateHelper->addWidget(mTokenTypeAllPosts, ui->progressBar, UISTATE_LOADING_VISIBLE);
	//mStateHelper->addWidget(mTokenTypeAllPosts, ui->loadingLabel, UISTATE_LOADING_VISIBLE);
	//mStateHelper->addWidget(mTokenTypeAllPosts, ui->filterLineEdit);

	//mStateHelper->addWidget(mTokenTypePosts, ui->loadingLabel, UISTATE_LOADING_VISIBLE);

	//mStateHelper->addLoadPlaceholder(mTokenTypeGroupData, ui->nameLabel);

	//mStateHelper->addWidget(mTokenTypeGroupData, ui->postButton);
	//mStateHelper->addWidget(mTokenTypeGroupData, ui->logoLabel);
	//mStateHelper->addWidget(mTokenTypeGroupData, ui->subscribeToolButton);

	/* Connect signals */
	connect(ui->postButton, SIGNAL(clicked()), this, SLOT(createMsg()));
	connect(ui->subscribeToolButton, SIGNAL(subscribe(bool)), this, SLOT(subscribeGroup(bool)));
	connect(NotifyQt::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));

	ui->postButton->setText(tr("Add new post"));
	
	/* add filter actions */
	ui->filterLineEdit->addFilter(QIcon(), tr("Title"), FILTER_TITLE, tr("Search Title"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Message"), FILTER_MSG, tr("Search Message"));
	ui->filterLineEdit->addFilter(QIcon(), tr("Filename"), FILTER_FILE_NAME, tr("Search Filename"));
#ifdef TODO
	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), ui->feedWidget, SLOT(setFilterText(QString)));
	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), ui->fileWidget, SLOT(setFilterText(QString)));
#endif
	connect(ui->filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterChanged(int)));

	/* Initialize view button */
	//setViewMode(VIEW_MODE_FEEDS); see processSettings
	//ui->infoWidget->hide();

	QSignalMapper *signalMapper = new QSignalMapper(this);
	connect(ui->feedToolButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
	connect(ui->fileToolButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
	signalMapper->setMapping(ui->feedToolButton, VIEW_MODE_FEEDS);
	signalMapper->setMapping(ui->fileToolButton, VIEW_MODE_FILES);
	connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(setViewMode(int)));

	/*************** Setup Left Hand Side (List of Channels) ****************/

	ui->loadingLabel->hide();
	ui->progressBar->hide();

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

	/* Initialize GUI */
	setAutoDownload(false);
	settingsChanged();

	mChannelPostsModel->updateChannel(channelId);

	mEventHandlerId = 0;
	// Needs to be asynced because this function is called by another thread!
	rsEvents->registerEventsHandler(
	            [this](std::shared_ptr<const RsEvent> event)
	{ RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this ); },
	            mEventHandlerId, RsEventType::GXS_CHANNELS );
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
	RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;

    mChannelPostFilesModel->setFiles(post.mFiles);

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
    ui->channelPostFiles_TV->resizeColumnToContents(RsGxsChannelPostFilesModel::COLUMN_FILES_NAME);
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
		// load settings

		/* Filter */
		ui->filterLineEdit->setCurrentFilter(Settings->value("filter", FILTER_TITLE).toInt());

		/* View mode */
		setViewMode(Settings->value("viewMode", VIEW_MODE_FEEDS).toInt());
	} else {
		// save settings

		/* Filter */
		Settings->setValue("filter", ui->filterLineEdit->currentFilter());

		/* View mode */
		Settings->setValue("viewMode", viewMode());
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
		ui->feedToolButton->setEnabled(true);
		ui->fileToolButton->setEnabled(true);
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
#endif

	ui->feedToolButton->setEnabled(false);
	ui->fileToolButton->setEnabled(false);

	ui->subscribeToolButton->setText(tr("Subscribe ") + " " + QString::number(group.mMeta.mPop) );
}

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

void GxsChannelPostsWidgetWithModel::filterChanged(int filter)
{
#ifdef TODO
	ui->feedWidget->setFilterType(filter);
	ui->fileWidget->setFilterType(filter);
#endif
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

void GxsChannelPostsWidget::insertChannelPosts(std::vector<RsGxsChannelPost>& posts, GxsMessageFramePostThread *thread, bool related)
{
	if (related && thread) {
		std::cerr << "GxsChannelPostsWidget::insertChannelPosts fill only related posts as thread is not possible" << std::endl;
		return;
	}

	int count = posts.size();
	int pos = 0;

	if (!thread) {
		ui->feedWidget->setSortingEnabled(false);
	}

    // collect new versions of posts if any

#ifdef DEBUG_CHANNEL
    std::cerr << "Inserting channel posts" << std::endl;
#endif

    std::vector<uint32_t> new_versions ;
    for (uint32_t i=0;i<posts.size();++i)
    {
		if(posts[i].mMeta.mOrigMsgId == posts[i].mMeta.mMsgId)
			posts[i].mMeta.mOrigMsgId.clear();

#ifdef DEBUG_CHANNEL
        std::cerr << "  " << i << ": msg_id=" << posts[i].mMeta.mMsgId << ": orig msg id = " << posts[i].mMeta.mOrigMsgId << std::endl;
#endif

        if(!posts[i].mMeta.mOrigMsgId.isNull())
            new_versions.push_back(i) ;
    }

#ifdef DEBUG_CHANNEL
    std::cerr << "New versions: " << new_versions.size() << std::endl;
#endif

    if(!new_versions.empty())
    {
#ifdef DEBUG_CHANNEL
        std::cerr << "  New versions present. Replacing them..." << std::endl;
        std::cerr << "  Creating search map."  << std::endl;
#endif

        // make a quick search map
        std::map<RsGxsMessageId,uint32_t> search_map ;
		for (uint32_t i=0;i<posts.size();++i)
            search_map[posts[i].mMeta.mMsgId] = i ;

        for(uint32_t i=0;i<new_versions.size();++i)
        {
#ifdef DEBUG_CHANNEL
            std::cerr << "  Taking care of new version  at index " << new_versions[i] << std::endl;
#endif

            uint32_t current_index = new_versions[i] ;
            uint32_t source_index  = new_versions[i] ;
#ifdef DEBUG_CHANNEL
            RsGxsMessageId source_msg_id = posts[source_index].mMeta.mMsgId ;
#endif

            // What we do is everytime we find a replacement post, we climb up the replacement graph until we find the original post
            // (or the most recent version of it). When we reach this post, we replace it with the data of the source post.
            // In the mean time, all other posts have their MsgId cleared, so that the posts are removed from the list.

            //std::vector<uint32_t> versions ;
            std::map<RsGxsMessageId,uint32_t>::const_iterator vit ;

            while(search_map.end() != (vit=search_map.find(posts[current_index].mMeta.mOrigMsgId)))
            {
#ifdef DEBUG_CHANNEL
                std::cerr << "    post at index " << current_index << " replaces a post at position " << vit->second ;
#endif

				// Now replace the post only if the new versionis more recent. It may happen indeed that the same post has been corrected multiple
				// times. In this case, we only need to replace the post with the newest version

				//uint32_t prev_index = current_index ;
				current_index = vit->second ;

				if(posts[current_index].mMeta.mMsgId.isNull())	// This handles the branching situation where this post has been already erased. No need to go down further.
                {
#ifdef DEBUG_CHANNEL
                    std::cerr << "  already erased. Stopping." << std::endl;
#endif
                    break ;
                }

				if(posts[current_index].mMeta.mPublishTs < posts[source_index].mMeta.mPublishTs)
				{
#ifdef DEBUG_CHANNEL
                    std::cerr << " and is more recent => following" << std::endl;
#endif
                    for(std::set<RsGxsMessageId>::const_iterator itt(posts[current_index].mOlderVersions.begin());itt!=posts[current_index].mOlderVersions.end();++itt)
						posts[source_index].mOlderVersions.insert(*itt);

					posts[source_index].mOlderVersions.insert(posts[current_index].mMeta.mMsgId);
					posts[current_index].mMeta.mMsgId.clear();	    // clear the msg Id so the post will be ignored
				}
#ifdef DEBUG_CHANNEL
                else
                    std::cerr << " but is older -> Stopping" << std::endl;
#endif
            }
        }
    }

#ifdef DEBUG_CHANNEL
    std::cerr << "Now adding posts..." << std::endl;
#endif

    for (std::vector<RsGxsChannelPost>::const_reverse_iterator it = posts.rbegin(); it != posts.rend(); ++it)
    {
#ifdef DEBUG_CHANNEL
		std::cerr << "  adding post: " << (*it).mMeta.mMsgId ;
#endif

        if(!(*it).mMeta.mMsgId.isNull())
		{
#ifdef DEBUG_CHANNEL
            std::cerr << " added" << std::endl;
#endif

			if (thread && thread->stopped())
				break;

			if (thread)
				thread->emitAddPost(QVariant::fromValue(*it), related, ++pos, count);
			else
				createPostItem(*it, related);
		}
#ifdef DEBUG_CHANNEL
        else
            std::cerr << " skipped" << std::endl;
#endif
    }

	if (!thread) {
		ui->feedWidget->setSortingEnabled(true);
	}
}

void GxsChannelPostsWidget::clearPosts()
{
	ui->feedWidget->clear();
	ui->fileWidget->clear();
}
#endif

void GxsChannelPostsWidgetWithModel::blank()
{
	mStateHelper->setWidgetEnabled(ui->postButton, false);
	mStateHelper->setWidgetEnabled(ui->subscribeToolButton, false);
	
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

//		RsQThreadUtils::postToObject( [=]()
//		{
//			/* Here it goes any code you want to be executed on the Qt Gui
//			 * thread, for example to update the data model with new information
//			 * after a blocking call to RetroShare API complete, note that
//			 * Qt::QueuedConnection is important!
//			 */
//
//			std::cerr << __PRETTY_FUNCTION__ << " Has been executed on GUI "
//			          << "thread but was scheduled by async thread" << std::endl;
//		}, this );
	});
}

#ifdef TODO
bool GxsChannelPostsWidgetWithModel::insertGroupData(const RsGxsGenericGroupData *data)
{
    const RsGxsChannelGroup *d = dynamic_cast<const RsGxsChannelGroup*>(data);

    if(!d)
    {
        RsErr() << __PRETTY_FUNCTION__ << " Cannot dynamic cast input data (" << (void*)data << " to RsGxsGenericGroupData. Something bad happenned." << std::endl;
        return false;
    }

	insertChannelDetails(*d);
	return true;
}

void GxsChannelPostsWidget::insertAllPosts(const std::vector<RsGxsGenericMsgData*>& posts, GxsMessageFramePostThread *thread)
{
    std::vector<RsGxsChannelPost> cposts;

    for(auto post: posts)	// This  is not so nice but we have somehow to convert to RsGxsChannelPost at some time, and the cposts list is being modified in the insert method.
		cposts.push_back(*static_cast<RsGxsChannelPost*>(post));

	insertChannelPosts(cposts, thread, false);
}

void GxsChannelPostsWidget::insertPosts(const std::vector<RsGxsGenericMsgData*>& posts)
{
    std::vector<RsGxsChannelPost> cposts;

    for(auto post: posts)	// This  is not so nice but we have somehow to convert to RsGxsChannelPost at some timer, and the cposts list is being modified in the insert method.
		cposts.push_back(*static_cast<RsGxsChannelPost*>(post));

	insertChannelPosts(cposts);
}
#endif

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


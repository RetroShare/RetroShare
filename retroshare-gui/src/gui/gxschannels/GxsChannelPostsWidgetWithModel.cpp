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
#include <QMenu>
#include <QSignalMapper>
#include <QPainter>
#include <QMessageBox>

#include "retroshare/rsgxscircles.h"

#include "ui_GxsChannelPostsWidgetWithModel.h"
#include "gui/feeds/GxsChannelPostItem.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxs/GxsGroupFrameDialog.h"
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
#include "gui/common/FilesDefs.h"

#include "GxsChannelPostsWidgetWithModel.h"
#include "GxsChannelPostsModel.h"
#include "GxsChannelPostFilesModel.h"
#include "GxsChannelFilesStatusWidget.h"
#include "GxsChannelPostThumbnail.h"

#include <algorithm>

#define ROLE_PUBLISH FEED_TREEWIDGET_SORTROLE

/****
 * #define DEBUG_CHANNEL
 ***/

static const int CHANNEL_TABS_DETAILS= 0;
static const int CHANNEL_TABS_POSTS  = 1;
static const int CHANNEL_TABS_FILES  = 2;

QColor SelectedColor = QRgb(0xff308dc7);

/* View mode */
#define VIEW_MODE_FEEDS  1
#define VIEW_MODE_FILES  2

// Determine the Shape and size of cells as a factor of the font height. An aspect ratio of 3/4 is what's needed
// for the image, so it's important that the height is a bit larger so as to leave some room for the text.
//
//
#define COLUMN_SIZE_FONT_FACTOR_W  6
#define COLUMN_SIZE_FONT_FACTOR_H  10

#define STAR_OVERLAY_IMAGE           ":icons/star_overlay_128.png"
#define COMMENT_OVERLAY_IMAGE        ":images/white-bubble-64.png"
#define UNREAD_COMMENT_OVERLAY_IMAGE ":images/orange-bubble-64.png"
#define IMAGE_COPYLINK               ":icons/png/copy.png"
#define IMAGE_GRID_VIEW              ":icons/png/menu.png"
#define IMAGE_DOWNLOAD               ":icons/png/download.png"
#define IMAGE_UNREAD                 ":icons/png/message.png"

Q_DECLARE_METATYPE(ChannelPostFileInfo)

// Delegate used to paint into the table of thumbnails

//===============================================================================================================================================//
//===                                                      ChannelPostDelegate                                                                ===//
//===============================================================================================================================================//

int ChannelPostDelegate::cellSize(int col,const QFont& font,uint32_t parent_width) const
{
    if(mUseGrid || col==0)
        return (int)floor(mZoom*COLUMN_SIZE_FONT_FACTOR_W*QFontMetricsF(font).height());
    else
        return (int)floor(0.8*parent_width - mZoom*COLUMN_SIZE_FONT_FACTOR_W*QFontMetricsF(font).height());
}

void ChannelPostDelegate::zoom(bool zoom_or_unzoom)
{
    if(zoom_or_unzoom)
        mZoom *= 1.02;
    else
        mZoom /= 1.02;

    if(mZoom < 0.5)
        mZoom = 0.5;
    if(mZoom > 2.0)
        mZoom = 2.0;
}

void ChannelPostDelegate::setAspectRatio(ChannelPostThumbnailView::AspectRatio r)
{
    mAspectRatio = r;
}
void ChannelPostDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    // prepare
    painter->save();
    painter->setClipRect(option.rect);

    RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;

    painter->fillRect( option.rect, option.palette.base().color());
    painter->restore();

    if(mUseGrid || index.column()==0)
    {
        // Draw a thumbnail

        uint32_t flags = (mUseGrid)?(ChannelPostThumbnailView::FLAG_SHOW_TEXT | ChannelPostThumbnailView::FLAG_SCALE_FONT):0;
        ChannelPostThumbnailView w(post,flags);
        w.setBackgroundRole(QPalette::AlternateBase);
        w.setAspectRatio(mAspectRatio);
        w.updateGeometry();
        w.adjustSize();

        QPixmap pixmap(w.size());

        if((option.state & QStyle::State_Selected) && post.mMeta.mPublishTs > 0) // check if post is selected and is not empty (end of last row)
            pixmap.fill(SelectedColor);	// I dont know how to grab the backgroud color for selected objects automatically.
        else
            pixmap.fill(option.palette.base().color());

        w.render(&pixmap,QPoint(),QRegion(),QWidget::DrawChildren );// draw the widgets, not the background

        // We extract from the pixmap the part of the widget that we want. Saddly enough, Qt adds some white space
        // below the widget and there is no way to control that.

        pixmap = pixmap.copy(QRect(0,0,w.actualSize().width(),w.actualSize().height()));

//        if(index.row()==0 && index.column()==0)
//        {
//            QFile file("yourFile.png");
//            file.open(QIODevice::WriteOnly);
//            pixmap.save(&file, "JPG");
//            file.close();
//        }

        if(mZoom != 1.0)
            pixmap = pixmap.scaled(mZoom*pixmap.size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);

        if(IS_MSG_UNREAD(post.mMeta.mMsgStatus) || IS_MSG_NEW(post.mMeta.mMsgStatus))
        {
            QPainter p(&pixmap);
            QFontMetricsF fm(option.font);

            p.drawPixmap(mZoom*QPoint(0.1*fm.height(),-3.4*fm.height()),FilesDefs::getPixmapFromQtResourcePath(STAR_OVERLAY_IMAGE).scaled(mZoom*6*fm.height(),mZoom*6*fm.height(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
        }

        if(post.mUnreadCommentCount > 0)
        {
            QPainter p(&pixmap);
            QFontMetricsF fm(option.font);

            p.drawPixmap(QPoint(pixmap.width(),0.0)+mZoom*QPoint(-2.9*fm.height(),0.4*fm.height()),
                         FilesDefs::getPixmapFromQtResourcePath(UNREAD_COMMENT_OVERLAY_IMAGE).scaled(mZoom*3*fm.height(),mZoom*3*fm.height(),
                                                                                              Qt::KeepAspectRatio,Qt::SmoothTransformation));
        }
        else if(post.mCommentCount > 0)
        {
            QPainter p(&pixmap);
            QFontMetricsF fm(option.font);

            p.drawPixmap(QPoint(pixmap.width(),0.0)+mZoom*QPoint(-2.9*fm.height(),0.4*fm.height()),
                         FilesDefs::getPixmapFromQtResourcePath(COMMENT_OVERLAY_IMAGE).scaled(mZoom*3*fm.height(),mZoom*3*fm.height(),
                                                                                              Qt::KeepAspectRatio,Qt::SmoothTransformation));
        }

        painter->drawPixmap(option.rect.topLeft(),
                            pixmap.scaled(option.rect.width(),option.rect.width()*pixmap.height()/(float)pixmap.width(),
                                          Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
    }
    else
    {
        // We're drawing the text on the second column

        uint32_t font_height = QFontMetricsF(option.font).height();
        QPoint p = option.rect.topLeft();
        float y = p.y() + font_height;

        painter->save();

        if((option.state & QStyle::State_Selected) && post.mMeta.mPublishTs > 0) // check if post is selected and is not empty (end of last row)
            painter->setPen(SelectedColor);

        if(IS_MSG_UNREAD(post.mMeta.mMsgStatus) || IS_MSG_NEW(post.mMeta.mMsgStatus))
        {
            QFont font(option.font);
            font.setBold(true);
            painter->setFont(font);
        }

        {
            painter->save();

            QFont font(painter->font());
            font.setUnderline(true);
            painter->setFont(font);
            painter->drawText(QPoint(p.x()+0.5*font_height,y),QString::fromUtf8(post.mMeta.mMsgName.c_str()));

            painter->restore();
        }

        y += font_height;
        y += font_height/2.0;

        QString info_text = QDateTime::fromMSecsSinceEpoch(qint64(1000)*post.mMeta.mPublishTs).toString(Qt::DefaultLocaleShortDate);

        if(post.mAttachmentCount > 0)
            info_text += ", " + QString::number(post.mAttachmentCount)+ " " +((post.mAttachmentCount>1)?tr("files"):tr("file")) + " (" + misc::friendlyUnit(qulonglong(post.mSize)) + ")" ;

        painter->drawText(QPoint(p.x()+0.5*font_height,y),info_text);
        //y += font_height;

        painter->restore();
    }
}

QSize ChannelPostDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // This is the only place where we actually set the size of cells

    QFontMetricsF fm(option.font);

    RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;
    uint32_t flags = (mUseGrid)?(ChannelPostThumbnailView::FLAG_SHOW_TEXT | ChannelPostThumbnailView::FLAG_SCALE_FONT):0;

    ChannelPostThumbnailView w(post,flags);
    w.setAspectRatio(mAspectRatio);
    w.updateGeometry();
    w.adjustSize();

    //std::cerr << "w.size(): " << w.width() << " x " << w.height() << ". Actual size: " << w.actualSize().width() << " x " << w.actualSize().height() << std::endl;

    float aspect_ratio = w.actualSize().height()/(float)w.actualSize().width();

    float cell_width  = mZoom*COLUMN_SIZE_FONT_FACTOR_W*fm.height();
    float cell_height = mZoom*COLUMN_SIZE_FONT_FACTOR_W*fm.height()*aspect_ratio;
#ifdef DEBUG_CHANNEL_POSTS_WIDGET
    RsDbg() << "SizeHint: mUseGrid=" << mUseGrid << " cell_width=" << cell_width << " cell_height=" << cell_height << " mZoom=" << mZoom ;
#endif

    if(mUseGrid || index.column()==0)
        return QSize(cell_width,cell_height);
    else
        return QSize(option.rect.width()-cell_width,cell_height);
}

void ChannelPostDelegate::setWidgetGrid(bool use_grid)
{
    mUseGrid = use_grid;
}

//===============================================================================================================================================//
//===                                                  ChannelPostFilesDelegate                                                               ===//
//===============================================================================================================================================//

QWidget *ChannelPostFilesDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex& index) const
{
    ChannelPostFileInfo file = index.data(Qt::UserRole).value<ChannelPostFileInfo>() ;

    if(index.column() == RsGxsChannelPostFilesModel::COLUMN_FILES_FILE)
    {
        GxsChannelFilesStatusWidget* w = new GxsChannelFilesStatusWidget(file,parent,true);
        w->setFocusPolicy(Qt::StrongFocus);
        connect(w,SIGNAL(onButtonClick()),this->parent(),SLOT(updateDAll_PB()));

        return w;
    }
    else
        return NULL;
}
void ChannelPostFilesDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

void ChannelPostFilesDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    ChannelPostFileInfo file = index.data(Qt::UserRole).value<ChannelPostFileInfo>() ;

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
    case RsGxsChannelPostFilesModel::COLUMN_FILES_DATE: painter->drawText(option.rect,Qt::AlignLeft | Qt::AlignVCenter,QDateTime::fromMSecsSinceEpoch(file.mPublishTime*1000).toString("MM/dd/yyyy, hh:mm"));
            break;
    case RsGxsChannelPostFilesModel::COLUMN_FILES_FILE: {

        GxsChannelFilesStatusWidget w(file);

        w.setFixedWidth(option.rect.width());
        w.setFixedHeight(option.rect.height());

        QPixmap pixmap(w.size());

                // apparently we need to do the alternate colors manually
        //if(index.row() & 0x01)
        //    pixmap.fill(option.palette.alternateBase().color());
        //else
            pixmap.fill(option.palette.base().color());

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
    ChannelPostFileInfo file = index.data(Qt::UserRole).value<ChannelPostFileInfo>() ;

    QFontMetricsF fm(option.font);

    switch(index.column())
    {
    case RsGxsChannelPostFilesModel::COLUMN_FILES_NAME: return QSize(1.1*fm.width(QString::fromUtf8(file.mName.c_str())),fm.height());
    case RsGxsChannelPostFilesModel::COLUMN_FILES_SIZE: return QSize(1.1*fm.width(misc::friendlyUnit(qulonglong(file.mSize))),fm.height());
    case RsGxsChannelPostFilesModel::COLUMN_FILES_DATE: return QSize(1.1*fm.width(QDateTime::fromMSecsSinceEpoch(file.mPublishTime*1000).toString("MM/dd/yyyy, hh:mm")),fm.height());
    default:
    case RsGxsChannelPostFilesModel::COLUMN_FILES_FILE: return QSize(option.rect.width(),GxsChannelFilesStatusWidget(file).height());
    }
}

//===============================================================================================================================================//
//===                                               GxsChannelPostWidgetWithModel                                                            ===//
//===============================================================================================================================================//

/** Constructor */
GxsChannelPostsWidgetWithModel::GxsChannelPostsWidgetWithModel(const RsGxsGroupId &channelId, QWidget *parent) :
    GxsMessageFrameWidget(rsGxsChannels, parent),
    ui(new Ui::GxsChannelPostsWidgetWithModel)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui->setupUi(this);

    ui->viewType_TB->setIcon(FilesDefs::getIconFromQtResourcePath(":icons/svg/gridlayout.svg"));
    ui->viewType_TB->setToolTip(tr("Click to switch to list view"));
    connect(ui->viewType_TB,SIGNAL(clicked()),this,SLOT(switchView()));

    ui->showUnread_TB->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
    whileBlocking(ui->showUnread_TB)->setChecked(false);
    ui->showUnread_TB->setToolTip(tr("Show unread posts only"));
    connect(ui->showUnread_TB,SIGNAL(toggled(bool)),this,SLOT(switchOnlyUnread(bool)));

    ui->postsTree->setAlternatingRowColors(false);
    ui->postsTree->setModel(mChannelPostsModel = new RsGxsChannelPostsModel());
    ui->postsTree->setItemDelegate(mChannelPostsDelegate = new ChannelPostDelegate());
    ui->postsTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);	// prevents bug on w10, since row size depends on widget width
    ui->postsTree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);// more beautiful if we scroll at pixel level
    ui->postsTree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    mChannelPostsDelegate->setAspectRatio(ChannelPostThumbnailView::ASPECT_RATIO_16_9);

    connect(ui->postsTree,SIGNAL(zoomRequested(bool)),this,SLOT(onUpdateZoomFactor(bool)));
    connect(ui->commentsDialog,SIGNAL(commentsLoaded(int)),this,SLOT(updateCommentsCount(int)));

    ui->channelPostFiles_TV->setModel(mChannelPostFilesModel = new RsGxsChannelPostFilesModel(this));
    ui->channelPostFiles_TV->setItemDelegate(new ChannelPostFilesDelegate(this));
    ui->channelPostFiles_TV->setPlaceholderText(tr("No files in this post, or no post selected"));
    ui->channelPostFiles_TV->setSortingEnabled(true);
    ui->channelPostFiles_TV->sortByColumn(RsGxsChannelPostFilesModel::COLUMN_FILES_DATE, Qt::DescendingOrder);	// sort by time
    ui->channelPostFiles_TV->setAlternatingRowColors(false);

    ui->channelFiles_TV->setModel(mChannelFilesModel = new RsGxsChannelPostFilesModel());
    ui->channelFiles_TV->setItemDelegate(mFilesDelegate = new ChannelPostFilesDelegate(this));
    ui->channelFiles_TV->setPlaceholderText(tr("No files in the channel, or no channel selected"));
    ui->channelFiles_TV->setSortingEnabled(true);
    ui->channelFiles_TV->sortByColumn(RsGxsChannelPostFilesModel::COLUMN_FILES_DATE, Qt::DescendingOrder);	// sort by time

    connect(ui->channelPostFiles_TV->header(),SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(sortColumnPostFiles(int,Qt::SortOrder)));
    connect(ui->channelFiles_TV->header(),SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(sortColumnFiles(int,Qt::SortOrder)));

    connect(ui->channelPostFiles_TV,SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showChannelFilesContextMenu(QPoint)));
    connect(ui->channelFiles_TV,SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showChannelFilesContextMenu(QPoint)));

    connect(ui->postsTree->selectionModel(),SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),this,SLOT(showPostDetails()));
    connect(ui->postsTree,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(postContextMenu(const QPoint&)));

    connect(ui->channel_TW,SIGNAL(currentChanged(int)),this,SLOT(currentTabChanged(int)));

    connect(mChannelPostsModel,SIGNAL(channelPostsLoaded()),this,SLOT(postChannelPostLoad()));

    ui->postName_LB->hide();
    ui->postTime_LB->hide();
    ui->postLogo_LB->hide();
    ui->postDAll_PB->hide();

    connect(ui->postDAll_PB,SIGNAL(clicked()),this,SLOT(download()));

    ui->postDetails_TE->setPlaceholderText(tr("No text to display"));

    // Set initial size of the splitter
    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 0);

    QFontMetricsF fm(font());

    if(mChannelPostsModel->getMode() == RsGxsChannelPostsModel::TREE_MODE_GRID)
        for(int i=0;i<mChannelPostsModel->columnCount();++i)
            ui->postsTree->setColumnWidth(i,mChannelPostsDelegate->cellSize(i,font(),ui->postsTree->width()));

    /* Setup UI helper */

    /* Connect signals */
    connect(ui->postButton, SIGNAL(clicked()), this, SLOT(createMsg()));
    connect(ui->subscribeToolButton, SIGNAL(subscribe(bool)), this, SLOT(subscribeGroup(bool)));
    connect(NotifyQt::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));

    ui->postButton->setText(tr("Add new post"));

    /* add filter actions */
    ui->filterLineEdit->setPlaceholderText(tr("Search..."));
    connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));

    ui->postsTree->setPlaceholderText(tr("No posts available in this channel"));
    ui->postsTree->setMinimumWidth(COLUMN_SIZE_FONT_FACTOR_W*QFontMetricsF(font()).height()+1);

    connect(ui->postsTree,SIGNAL(sizeChanged(QSize)),this,SLOT(handlePostsTreeSizeChange(QSize)));

    /* Set initial section sizes */
    QHeaderView * channelpostfilesheader = ui->channelPostFiles_TV->header () ;
    QHeaderView * channelfilesheader = ui->channelFiles_TV->header () ;

    channelpostfilesheader->resizeSection (RsGxsChannelPostFilesModel::COLUMN_FILES_NAME, fm.width("RetroShare-v0.6.5-1487-g6714648e5-Windows-x64-portable-20200518-Qt-5.14.2.7z"));
    channelfilesheader->resizeSection (RsGxsChannelPostFilesModel::COLUMN_FILES_NAME, fm.width("RetroShare-v0.6.5-1487-g6714648e5-Windows-x64-portable-20200518-Qt-5.14.2.7z"));

    /* Initialize feed widget */
    //ui->feedWidget->setSortRole(ROLE_PUBLISH, Qt::DescendingOrder);
    //ui->feedWidget->setFilterCallback(filterItem);

    /* load settings */
    processSettings(true);

    ui->channelPostFiles_TV->setColumnHidden(RsGxsChannelPostFilesModel::COLUMN_FILES_DATE, true);	// no need to show this here.

    /* Initialize subscribe button */
    QIcon icon;
    icon.addPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/redled.png"), QIcon::Normal, QIcon::On);
    icon.addPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/start.png"), QIcon::Normal, QIcon::Off);

#ifdef TO_REMOVE
    mAutoDownloadAction = new QAction(icon, "", this);
    mAutoDownloadAction->setCheckable(true);
    connect(mAutoDownloadAction, SIGNAL(triggered()), this, SLOT(toggleAutoDownload()));

    ui->subscribeToolButton->addSubscribedAction(mAutoDownloadAction);
    setAutoDownload(false);
#endif

    ui->commentsDialog->setGxsService(rsGxsChannels);

    /* Initialize GUI */
    settingsChanged();

    setGroupId(channelId);
    mChannelPostsModel->updateChannel(channelId);

    mEventHandlerId = 0;
    // Needs to be asynced because this function is called by another thread!
    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
    {
        RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this );
    }, mEventHandlerId, RsEventType::GXS_CHANNELS );
}

void GxsChannelPostsWidgetWithModel::resizeEvent(QResizeEvent *e)
{
    GxsMessageFrameWidget::resizeEvent(e);
    updateZoomFactor(0);
}
void GxsChannelPostsWidgetWithModel::currentTabChanged(int t)
{
    switch(t)
    {
    case CHANNEL_TABS_DETAILS:
    case CHANNEL_TABS_FILES:
        ui->showUnread_TB->setHidden(true);
        ui->viewType_TB->setHidden(true);
        break;

    case CHANNEL_TABS_POSTS:
        ui->showUnread_TB->setHidden(false);
        ui->viewType_TB->setHidden(false);
        updateZoomFactor(0); // fixes a bug due to the widget now knowing its size when not displayed.
        break;
    }
}
void GxsChannelPostsWidgetWithModel::onUpdateZoomFactor(bool zoom_or_unzoom)
{
    if(zoom_or_unzoom)
        updateZoomFactor(1);
    else
        updateZoomFactor(-1);
}
void GxsChannelPostsWidgetWithModel::updateZoomFactor(int what_to_do)
{
    if(what_to_do != 0)
        mChannelPostsDelegate->zoom(what_to_do > 0);

    QSize s = ui->postsTree->size();
    int n_columns = std::max(1,(int)floor(s.width() / (float)(mChannelPostsDelegate->cellSize(0,font(),s.width()))));
    mChannelPostsModel->setNumColumns(n_columns);	// forces the update

    for(int i=0;i<mChannelPostsModel->columnCount();++i)
        ui->postsTree->setColumnWidth(i,mChannelPostsDelegate->cellSize(i,font(),ui->postsTree->width()));

    if(what_to_do)
        mChannelPostsModel->triggerViewUpdate(true,false);
    else
        mChannelPostsModel->triggerViewUpdate(false,true);
}

void GxsChannelPostsWidgetWithModel::sortColumnPostFiles(int col,Qt::SortOrder so)
{
    std::cerr << "Sorting post files according to col " << col << std::endl;
    mChannelPostFilesModel->sort(col,so);
}
void GxsChannelPostsWidgetWithModel::sortColumnFiles(int col,Qt::SortOrder so)
{
    std::cerr << "Sorting channel files according to col " << col << std::endl;
    mChannelFilesModel->sort(col,so);
}

void GxsChannelPostsWidgetWithModel::postContextMenu(const QPoint&)
{
    QMenu menu(this);

#ifdef TO_REMOVE
    if(mChannelPostsModel->getMode() == RsGxsChannelPostsModel::TREE_MODE_GRID)
        menu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_GRID_VIEW), tr("Switch to list view"), this, SLOT(switchView()));
    else
        menu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_GRID_VIEW), tr("Switch to grid view"), this, SLOT(switchView()));

    menu.addSeparator();
#endif

    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();

    if(index.isValid())
    {
        RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;

        if(!post.mFiles.empty())
            menu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_DOWNLOAD), tr("Download files"), this, SLOT(download()));

        if(!IS_MSG_UNREAD(post.mMeta.mMsgStatus) && !IS_MSG_NEW(post.mMeta.mMsgStatus))
            menu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_UNREAD), tr("Mark as unread"), this, SLOT(markMessageUnread()));
    }
    menu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyMessageLink()));

    if(IS_GROUP_PUBLISHER(mGroup.mMeta.mSubscribeFlags))
        menu.addAction(FilesDefs::getIconFromQtResourcePath(":/icons/png/pencil-edit-button.png"), tr("Edit"), this, SLOT(editPost()));

    menu.exec(QCursor::pos());
}

void GxsChannelPostsWidgetWithModel::markMessageUnread()
{
    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();

    mChannelPostsModel->setMsgReadStatus(index,false);
}

RsGxsMessageId GxsChannelPostsWidgetWithModel::getCurrentItemId() const
{
    RsGxsMessageId selected_msg_id ;
    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();

    if(index.isValid())
        selected_msg_id = index.data(Qt::UserRole).value<RsGxsChannelPost>().mMeta.mMsgId ;

    return selected_msg_id;
}

void GxsChannelPostsWidgetWithModel::selectItem(const RsGxsMessageId& msg_id)
{
    auto index = mChannelPostsModel->getIndexOfMessage(msg_id);

    ui->postsTree->selectionModel()->setCurrentIndex(index,QItemSelectionModel::ClearAndSelect);
    ui->postsTree->scrollTo(index);//May change if model reloaded
}

void GxsChannelPostsWidgetWithModel::switchView()
{
   auto msg_id = getCurrentItemId();

    if(mChannelPostsModel->getMode() == RsGxsChannelPostsModel::TREE_MODE_GRID)
    {
        ui->viewType_TB->setIcon(FilesDefs::getIconFromQtResourcePath(":icons/svg/listlayout.svg"));
        ui->viewType_TB->setToolTip(tr("Click to switch to grid view"));

        ui->postsTree->setSelectionBehavior(QAbstractItemView::SelectRows);

        mChannelPostsDelegate->setWidgetGrid(false);
        mChannelPostsModel->setMode(RsGxsChannelPostsModel::TREE_MODE_LIST);
    }
    else
    {
        ui->viewType_TB->setIcon(FilesDefs::getIconFromQtResourcePath(":icons/svg/gridlayout.svg"));
        ui->viewType_TB->setToolTip(tr("Click to switch to list view"));

        ui->postsTree->setSelectionBehavior(QAbstractItemView::SelectItems);

        mChannelPostsDelegate->setWidgetGrid(true);
        mChannelPostsModel->setMode(RsGxsChannelPostsModel::TREE_MODE_GRID);

        handlePostsTreeSizeChange(ui->postsTree->size(),true);
    }

    for(int i=0;i<mChannelPostsModel->columnCount();++i)
        ui->postsTree->setColumnWidth(i,mChannelPostsDelegate->cellSize(i,font(),ui->postsTree->width()));

    selectItem(msg_id);
    ui->postsTree->setFocus();

    mChannelPostsModel->triggerViewUpdate(false,true);	// This is already called by setMode(), but the model cannot know how many
                                                // columns is actually has until we call handlePostsTreeSizeChange(), so
                                                // we have to call it again here.
}

void GxsChannelPostsWidgetWithModel::copyMessageLink()
{
    try
    {
        if (groupId().isNull())
            throw std::runtime_error("No channel currently selected!");

        QModelIndex index = ui->postsTree->selectionModel()->currentIndex();

        if(!index.isValid())
            throw std::runtime_error("No post under mouse!");

        RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;

        if(post.mMeta.mMsgId.isNull())
            throw std::runtime_error("Post has empty MsgId!");

        RetroShareLink link = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_CHANNEL, groupId(), post.mMeta.mMsgId, QString::fromUtf8(post.mMeta.mMsgName.c_str()));

        if (!link.valid())
            throw std::runtime_error("Link is not valid");

        QList<RetroShareLink> urls;
        urls.push_back(link);
        RSLinkClipboard::copyLinks(urls);
    }
    catch(std::exception& e)
    {
        QMessageBox::critical(NULL,tr("Link creation error"),tr("Link could not be created: ")+e.what());
    }
}
void GxsChannelPostsWidgetWithModel::download()
{
    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();
    RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;

    std::string destination;
    rsGxsChannels->getChannelDownloadDirectory(mGroup.mMeta.mGroupId,destination);

    for(auto& file:post.mFiles)
    {
        std::list<RsPeerId> sources;

        // Add possible direct sources.
        FileInfo fileInfo;
        rsFiles->FileDetails(file.mHash, RS_FILE_HINTS_REMOTE, fileInfo);

        for(std::vector<TransferInfo>::const_iterator it = fileInfo.peers.begin(); it != fileInfo.peers.end(); ++it) {
            sources.push_back((*it).peerId);
        }

        rsFiles->FileRequest(file.mName, file.mHash, file.mSize, destination, RS_FILE_REQ_ANONYMOUS_ROUTING, sources);
    }

    ui->postDAll_PB->hide();
}

void GxsChannelPostsWidgetWithModel::updateDAll_PB()
{
    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();
    RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;

    size_t newFileToDl = 0;
    uint64_t newFileTotalSize = 0;
    QString newFilesDetails;

    for(auto& file:post.mFiles)
    {
        FileInfo fileInfo;
        if (!rsFiles->FileDetails(file.mHash, RS_FILE_HINTS_DOWNLOAD | RS_FILE_HINTS_SPEC_ONLY, fileInfo)) {
            ++newFileToDl;
            newFileTotalSize += file.mSize;
            newFilesDetails += QString::fromUtf8(file.mName.c_str()) + " " + misc::friendlyUnit(file.mSize) + "\n";
        }
    }

    ui->postDAll_PB->setHidden(newFileToDl == 0);
    ui->postDAll_PB->setToolTip((newFileToDl == 1 ? tr("Download this file:") : tr("Download All these %1 files:").arg(newFileToDl) ) + "\n"
                                + newFilesDetails
                                + tr("Totaling: %1").arg(misc::friendlyUnit(newFileTotalSize)));

}

void GxsChannelPostsWidgetWithModel::editPost()
{
    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();
    RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;

    CreateGxsChannelMsg *msgDialog = new CreateGxsChannelMsg(post.mMeta.mGroupId,post.mMeta.mMsgId);
    msgDialog->show();
}

void GxsChannelPostsWidgetWithModel::handlePostsTreeSizeChange(QSize s,bool force)
{
    if(mChannelPostsModel->getMode() != RsGxsChannelPostsModel::TREE_MODE_GRID)
        return;

    int n_columns = std::max(1,(int)floor(s.width() / (mChannelPostsDelegate->cellSize(0,font(),ui->postsTree->width()))));
    RsDbg() << "nb columns: " << n_columns << " current count=" << mChannelPostsModel->columnCount() ;

    // save current post. The setNumColumns() indeed loses selection

    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();
    RsGxsMessageId current_mid;

    if(index.isValid())
        current_mid = index.data(Qt::UserRole).value<RsGxsChannelPost>().mMeta.mMsgId ;

    if((force || (n_columns != mChannelPostsModel->columnCount())) && mChannelPostsModel->setNumColumns(n_columns))
    {
        // Restore current post. The setNumColumns() indeed loses selection
        ui->postsTree->selectionModel()->setCurrentIndex(mChannelPostsModel->getIndexOfMessage(current_mid),QItemSelectionModel::ClearAndSelect);
    }
}

void GxsChannelPostsWidgetWithModel::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    const RsGxsChannelEvent *e = dynamic_cast<const RsGxsChannelEvent*>(event.get());

    if(!e)
        return;

    switch(e->mChannelEventCode)
    {
        case RsChannelEventCode::NEW_CHANNEL:     // [[fallthrough]];
        case RsChannelEventCode::DELETED_CHANNEL: // [[fallthrough]];
        case RsChannelEventCode::UPDATED_CHANNEL: // [[fallthrough]];
        case RsChannelEventCode::RECEIVED_PUBLISH_KEY:
        case RsChannelEventCode::SYNC_PARAMETERS_UPDATED:
        {
            if(e->mChannelGroupId == groupId())
                updateDisplay(true,false);
        }
        break;

        case RsChannelEventCode::READ_STATUS_CHANGED: // This is already handled by setMsgReadStatus() that has been called and issued this event.
        break;

        case RsChannelEventCode::NEW_MESSAGE:
        {
            if(e->mChannelGroupId == groupId())
            {
                RsDbg() << "Received new message in current channel, msgId=" << e->mChannelMsgId ;

                RsThread::async([this,E=*e]()	// dereferencing to make a copy that will survive while e is deleted by the parent thread.
                {
                    // 1 - get message data from p3GxsChannels. No need for pointers here, because we send only a single post to postToObject()
                    //     At this point we dont know what kind of msg id we have. It can be a vote, a comment or an actual message.

                    std::vector<RsGxsChannelPost> posts;
                    std::vector<RsGxsComment>     comments;
                    std::vector<RsGxsVote>        votes;

                    const auto& msg_id(E.mChannelMsgId);
                    const auto& grp_id(E.mChannelGroupId);

                    if(!rsGxsChannels->getChannelContent(grp_id, { msg_id }, posts,comments,votes) || posts.size() != 1)
                    {
                        RsErr() << " failed to retrieve channel message data for channel/msg " << grp_id << "/" << msg_id;
                        return;
                    }

                    // 2 - update the model in the UI thread.

                    RsQThreadUtils::postToObject( [post=posts[0],this]()
                    {
                        std::set<RsGxsFile> added_files,removed_files;

                        mChannelPostsModel->updateSinglePost(post,added_files,removed_files);

                        std::set<ChannelPostFileInfo> added_filesi,removed_filesi;

                        for(auto f:added_files) added_filesi.insert(ChannelPostFileInfo(f,post.mMeta.mPublishTs));
                        for(auto f:removed_files) removed_filesi.insert(ChannelPostFileInfo(f,post.mMeta.mPublishTs));

                        mChannelFilesModel->update_files(added_filesi,removed_filesi);

                    },this);
                });
            }
        }
        break;

        case RsChannelEventCode::NEW_COMMENT:

            if(e->mChannelGroupId == groupId() && e->mChannelThreadId != ui->commentsDialog->messageId())
                mChannelPostsModel->updatePostWithNewComment(e->mChannelThreadId); [[fallthrough]];

        case RsChannelEventCode::NEW_VOTE:

            if(e->mChannelGroupId == groupId() && e->mChannelThreadId == ui->commentsDialog->messageId())
                ui->commentsDialog->refresh();
        break;

        default:
        break;

    }
}

void GxsChannelPostsWidgetWithModel::showPostDetails()
{
    QModelIndex index = ui->postsTree->selectionModel()->currentIndex();
    RsGxsChannelPost post = index.data(Qt::UserRole).value<RsGxsChannelPost>() ;
#ifdef DEBUG_CHANNEL_POSTS_WIDGET
    RsDbg() << "showPostDetails: current index is " << index.row() << "," << index.column() ;
#endif

    //QTextDocument doc;
    //doc.setHtml(post.mMsg.c_str());

    if(post.mMeta.mPublishTs == 0)
    {
        ui->postDetails_TE->clear();
        ui->postLogo_LB->hide();
        ui->postName_LB->hide();
        ui->postTime_LB->hide();
        mChannelPostFilesModel->clear();
        ui->details_TW->setEnabled(false);
        //mLastSelectedPosts[groupId()].clear();

        return;
    }
    ui->details_TW->setEnabled(true);

    ui->postLogo_LB->show();
    ui->postName_LB->show();
    ui->postTime_LB->show();

#ifdef DEBUG_CHANNEL_POSTS_WIDGET
    RsDbg() << "showPostDetails: setting mLastSelectedPosts[groupId()] to current post Id " << post.mMeta.mMsgId << ". Previous value: " << mLastSelectedPosts[groupId()] ;
#endif
    mLastSelectedPosts[groupId()] = post.mMeta.mMsgId;

    std::list<ChannelPostFileInfo> files;
    for(auto& file:post.mFiles)
        files.push_back(ChannelPostFileInfo(file,post.mMeta.mPublishTs));

    mChannelPostFilesModel->setFiles(files);

    auto all_msgs_versions(post.mOlderVersions);
    all_msgs_versions.insert(post.mMeta.mMsgId);

    ui->commentsDialog->commentLoad(post.mMeta.mGroupId, all_msgs_versions, post.mMeta.mMsgId,true);

#ifdef DEBUG_CHANNEL_POSTS_WIDGET
    RsDbg() << "Showing details about selected index : "<< index.row() << "," << index.column() ;
#endif

    ui->postDetails_TE->setText(RsHtml().formatText(NULL, QString::fromUtf8(post.mMsg.c_str()), /* RSHTML_FORMATTEXT_EMBED_SMILEYS |*/ RSHTML_FORMATTEXT_EMBED_LINKS));

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

    ui->postTime_LB->setText(QDateTime::fromMSecsSinceEpoch(post.mMeta.mPublishTs*1000).toString("MM/dd/yyyy, hh:mm"));
    ui->postTime_LB->setFixedWidth(W);

    //ui->channelPostFiles_TV->resizeColumnToContents(RsGxsChannelPostFilesModel::COLUMN_FILES_FILE);
    //ui->channelPostFiles_TV->resizeColumnToContents(RsGxsChannelPostFilesModel::COLUMN_FILES_SIZE);
    ui->channelPostFiles_TV->setAutoSelect(true);

    // Now also set the post as read

    if(IS_MSG_UNREAD(post.mMeta.mMsgStatus) || IS_MSG_NEW(post.mMeta.mMsgStatus) || post.mUnreadCommentCount > 0)
    {
        mChannelPostsModel->setMsgReadStatus(index,true);

        //RsGxsGrpMsgIdPair postId;
        //postId.second = post.mMeta.mMsgId;
        //postId.first  = post.mMeta.mGroupId;

        //RsThread::async([postId]()
        //{
            //rsGxsChannels->setMessageReadStatus(postId, true) ;
        //} );
    }

    updateDAll_PB();
}

void GxsChannelPostsWidgetWithModel::updateCommentsCount(int n)
{
    if(n > 0)
        ui->details_TW->setTabText(2,tr("Comments (%1)").arg(n));
    else
        ui->details_TW->setTabText(2,tr("Comments"));
}
void GxsChannelPostsWidgetWithModel::updateData(bool update_group_data, bool update_posts)
{
    if(groupId().isNull())
    {
        // clear post, files and comment widgets

        showPostDetails();
        return;
    }

    RsThread::async([this,update_group_data,update_posts]()
    {
        std::vector<RsGxsChannelGroup> groups;
        RsGxsChannelGroup group;

        if(rsGxsChannels->getChannelsInfo(std::list<RsGxsGroupId>{ groupId() }, groups) && groups.size()==1)
            group = groups[0];
        else if(!rsGxsChannels->getDistantSearchResultGroupData(groupId(),group))
        {
            std::cerr << __PRETTY_FUNCTION__ << " failed to get group data for channel: " << groupId() << std::endl;
            return;
        }

        RsQThreadUtils::postToObject( [this,update_group_data,update_posts,group]()
        {
            if(mGroup.mMeta.mGroupId != group.mMeta.mGroupId) // this prevents any attempt to display the wrong index. Navigate() if needed will use mNavigatePendingMsgId
            {
#ifdef DEBUG_CHANNEL_POSTS_WIDGET
                std::cerr << "Old group: " << mGroup.mMeta.mGroupId << ", new group: " << group.mMeta.mGroupId << ". Celaring selection" << std::endl;
#endif
                whileBlocking(ui->postsTree->selectionModel())->clear();
                whileBlocking(ui->commentsDialog)->commentClear();
                updateCommentsCount(0);
            }

            if(update_group_data)
            {
                mGroup = group;
                insertChannelDetails(mGroup);
            }

            if(update_posts)
            {
                ui->postsTree->setPlaceholderText(tr("Loading..."));

                mChannelPostsModel->updateChannel(groupId());

                whileBlocking(ui->filterLineEdit)->clear();
                whileBlocking(ui->showUnread_TB)->setChecked(false);
            }

            emit groupDataLoaded();
            emit groupChanged(this);		// signals the parent widget to e.g. update the group tab name
        } );
    });
}

void GxsChannelPostsWidgetWithModel::postChannelPostLoad()
{
    std::cerr << "Post channel load..." << std::endl;

    if (!mNavigatePendingMsgId.isNull())
        navigate(mNavigatePendingMsgId);

    else if( (mLastSelectedPosts.count(groupId()) > 0)
             && !mLastSelectedPosts[groupId()].isNull())
    {
        QModelIndex index = mChannelPostsModel->getIndexOfMessage(mLastSelectedPosts[groupId()]);

        std::cerr << "Setting current index to " << index.row() << ","<< index.column() << " for current post "
                  << mLastSelectedPosts[groupId()].toStdString() << std::endl;

        ui->postsTree->selectionModel()->setCurrentIndex(index,QItemSelectionModel::ClearAndSelect);
        ui->postsTree->scrollTo(index);//May change if model reloaded
        ui->postsTree->setFocus();
    }
    else
        std::cerr << "No pre-selected channel post." << std::endl;

    std::list<ChannelPostFileInfo> files;

    mChannelPostsModel->getFilesList(files);
    mChannelFilesModel->setFiles(files);

    ui->channelFiles_TV->setAutoSelect(true);
    ui->channelFiles_TV->sortByColumn(ui->channelFiles_TV->header()->sortIndicatorSection()
                                      ,ui->channelFiles_TV->header()->sortIndicatorOrder());

    // if there's no posts, this is what's going to be displayed.
    ui->postsTree->setPlaceholderText(tr("No posts available in this channel."));

    ui->infoPosts->setText(QString::number(mChannelPostsModel->getNumberOfPosts()) + " / " + QString::number(mGroup.mMeta.mVisibleMsgCount));

    // now compute aspect ratio for posts. We do that by looking at the 5 latest posts and compute the best aspect ratio for them.

    std::vector<uint32_t> ar_votes(4,0);

    for(uint32_t i=0;i<std::min(mChannelPostsModel->getNumberOfPosts(),5u);++i)
    {
        const RsGxsChannelPost& post = mChannelPostsModel->post(i);
        ChannelPostThumbnailView v(post,ChannelPostThumbnailView::FLAG_SHOW_TEXT | ChannelPostThumbnailView::FLAG_SCALE_FONT);

        ++ar_votes[ static_cast<uint32_t>( v.bestAspectRatio() )];
    }
    int best=0;
    for(uint32_t i=0;i<4;++i)
        if(ar_votes[i] > ar_votes[best])
            best = i;

    mChannelPostsDelegate->setAspectRatio(static_cast<ChannelPostThumbnailView::AspectRatio>(best));
    handlePostsTreeSizeChange(ui->postsTree->size(),true); // force the update

    updateZoomFactor(0);
}

void GxsChannelPostsWidgetWithModel::updateDisplay(bool update_group_data,bool update_posts)
{
    // First, clear all widget

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

    if(mGroup.mMeta.mGroupId != groupId())
    {
#ifdef DEBUG_FORUMS
        std::cerr << "  inconsistent group data. Reloading!"<< std::endl;
#endif
        update_group_data = true;
        update_posts = true;
    }

    updateData(update_group_data,update_posts);
}
GxsChannelPostsWidgetWithModel::~GxsChannelPostsWidgetWithModel()
{
    rsEvents->unregisterEventsHandler(mEventHandlerId);
    // save settings
    processSettings(false);

    //delete(mAutoDownloadAction);
    delete mFilesDelegate;
    delete ui;
}

void GxsChannelPostsWidgetWithModel::processSettings(bool load)
{
    QHeaderView *channelpostfilesheader = ui->channelPostFiles_TV->header () ;
    QHeaderView *channelfilesheader = ui->channelFiles_TV->header () ;

    Settings->beginGroup(QString("ChannelPostsWidget"));

    if (load) {

        // state of files tree
        channelpostfilesheader->restoreState(Settings->value("PostFilesTree").toByteArray());
        channelfilesheader->restoreState(Settings->value("FilesTree").toByteArray());

        // state of splitter
        ui->splitter->restoreState(Settings->value("SplitterChannelPosts").toByteArray());
    } else {
        // state of files tree
        Settings->setValue("PostFilesTree", channelpostfilesheader->saveState());
        Settings->setValue("FilesTree", channelfilesheader->saveState());

        // state of splitter
        Settings->setValue("SplitterChannelPosts", ui->splitter->saveState());
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
    return QString::fromUtf8(mGroup.mMeta.mGroupName.c_str());
}

void GxsChannelPostsWidgetWithModel::groupNameChanged(const QString &/*name*/)
{
//	if (groupId().isNull()) {
//		ui->nameLabel->setText(tr("No Channel Selected"));
//		ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/channels.png"));
//	} else {
//		ui->nameLabel->setText(name);
//	}
}

QIcon GxsChannelPostsWidgetWithModel::groupIcon()
{
    /* CHANNEL IMAGE */
    QPixmap chanImage;
    if (mGroup.mImage.mData != NULL) {
        GxsIdDetails::loadPixmapFromData(mGroup.mImage.mData, mGroup.mImage.mSize, chanImage,GxsIdDetails::ORIGINAL);
    } else {
        chanImage = FilesDefs::getPixmapFromQtResourcePath(ChannelPostThumbnailView::CHAN_DEFAULT_IMAGE);
    }

    return QIcon(chanImage);
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
    // save selection if needed

    /* IMAGE */
    QPixmap chanImage;
    if (group.mImage.mData != NULL) {
        GxsIdDetails::loadPixmapFromData(group.mImage.mData, group.mImage.mSize, chanImage,GxsIdDetails::ORIGINAL);
    } else {
        chanImage = FilesDefs::getPixmapFromQtResourcePath(ChannelPostThumbnailView::CHAN_DEFAULT_IMAGE);
    }
    if(group.mMeta.mGroupName.empty())
        ui->channelName_LB->setText(tr("[No name]"));
    else
        ui->channelName_LB->setText(QString::fromUtf8(group.mMeta.mGroupName.c_str()));

    ui->logoLabel->setPixmap(chanImage);
    ui->logoLabel->setFixedSize(QSize(ui->logoLabel->height()*chanImage.width()/(float)chanImage.height(),ui->logoLabel->height())); // make the logo have the same aspect ratio than the original image

    ui->postButton->setEnabled(bool(group.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH));

#ifdef TO_REMOVE
    bool autoDownload ;
    rsGxsChannels->getChannelAutoDownload(group.mMeta.mGroupId,autoDownload);
    setAutoDownload(autoDownload);
#endif

    setSubscribeButtonText(group.mMeta.mGroupId,group.mMeta.mSubscribeFlags, group.mMeta.mPop);

    if (IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags))
    {
        ui->channel_TW->setTabEnabled(CHANNEL_TABS_POSTS,true);
        ui->channel_TW->setTabEnabled(CHANNEL_TABS_FILES,true);
        ui->details_TW->setEnabled(true);

        ui->infoSyncTimeLabel->show();
        ui->syncPeriodTitleLabel->show();
    }
    else
    {
        ui->details_TW->setEnabled(false);
        ui->channel_TW->setTabEnabled(CHANNEL_TABS_POSTS,false);
        ui->channel_TW->setTabEnabled(CHANNEL_TABS_FILES,false);

        ui->infoSyncTimeLabel->hide();
        ui->syncPeriodTitleLabel->hide();
    }


    ui->infoPosts->setText(QString::number(group.mMeta.mVisibleMsgCount));

    if(group.mMeta.mLastPost==0)
        ui->infoLastPost->setText(tr("Never"));
    else
        ui->infoLastPost->setText(DateTime::formatLongDateTime(group.mMeta.mLastPost));

    uint32_t current_sync_time  = GxsGroupFrameDialog::checkDelay(rsGxsChannels->getSyncPeriod(group.mMeta.mGroupId))/86400 ;

    QString sync_string;
    switch(current_sync_time)
    {
    case 5: sync_string = tr("5 days");  break;
    case 15: sync_string = tr("2 weeks");  break;
    case 30: sync_string = tr("1 month");  break;
    case 90: sync_string = tr("3 months");  break;
    case 180: sync_string = tr("6 months");  break;
    case 365: sync_string = tr("1 year");  break;
    case   0: sync_string = tr("indefinitly");  break;
    default:
        sync_string = tr("Unknown");
    }

    auto sync_period = rsGxsChannels->getSyncPeriod(group.mMeta.mGroupId) ;

    if(sync_period > 0 && group.mMeta.mLastPost > 0 && group.mMeta.mLastPost + rsGxsChannels->getSyncPeriod(group.mMeta.mGroupId) < time(NULL) && IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags))
        sync_string += " (Warning: will not allow posts to sync)";

    ui->infoSyncTimeLabel->setText(sync_string);


    QString formatDescription = QString::fromUtf8(group.mDescription.c_str());

    unsigned int formatFlag = RSHTML_FORMATTEXT_EMBED_LINKS;

    // embed smileys ?
    if (Settings->valueFromGroup(QString("ChannelPostsWidget"), QString::fromUtf8("Emoteicons_ChannelDecription"), true).toBool()) {
        formatFlag |= RSHTML_FORMATTEXT_EMBED_SMILEYS;
    }

    formatDescription = RsHtml().formatText(NULL, formatDescription, formatFlag);

    ui->infoDescription->setText(formatDescription);

    ui->infoAdministrator->setId(group.mMeta.mAuthorId) ;

    if(!group.mMeta.mAuthorId.isNull())
    {
        RetroShareLink link = RetroShareLink::createMessage(group.mMeta.mAuthorId, "");
        ui->infoAdministrator->setText(link.toHtml());
    }
    else
        ui->infoAdministrator->setText("[No contact author]");

    ui->infoCreated->setText(DateTime::formatLongDateTime(group.mMeta.mPublishTs));

    QString distrib_string ( "[unknown]" );

    switch((RsGxsCircleType)group.mMeta.mCircleType)
    {
    case RsGxsCircleType::PUBLIC: distrib_string = tr("Public") ;
        break ;
    case RsGxsCircleType::EXTERNAL:
    {
        RsGxsCircleDetails det ;

        // !! What we need here is some sort of CircleLabel, which loads the circle and updates the label when done.

        if(rsGxsCircles->getCircleDetails(group.mMeta.mCircleId,det))
            distrib_string = tr("Restricted to members of circle \"")+QString::fromUtf8(det.mCircleName.c_str()) +"\"";
        else
            distrib_string = tr("Restricted to members of circle ")+QString::fromStdString(group.mMeta.mCircleId.toStdString()) ;
    }
        break ;
    case RsGxsCircleType::YOUR_EYES_ONLY: distrib_string = tr("Your eyes only");
        break ;
    case RsGxsCircleType::LOCAL: distrib_string = tr("You and your friend nodes");
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

    setSubscribeButtonText(group.mMeta.mGroupId,group.mMeta.mSubscribeFlags, group.mMeta.mPop);

    showPostDetails();
}

void GxsChannelPostsWidgetWithModel::showChannelFilesContextMenu(QPoint /*p*/)
{
    QMenu contextMnu(this) ;

    QAction *action = contextMnu.addAction(QIcon(), tr("Copy Retroshare link"), this, SLOT(copyChannelFilesLink()));
    action->setData(QVariant::fromValue(sender()));
    contextMnu.exec(QCursor::pos());
}

void GxsChannelPostsWidgetWithModel::copyChannelFilesLink()
{
    // Block the popup if no results available
    QAction *action = dynamic_cast<QAction*>(sender());
    RSTreeView *tree = dynamic_cast<RSTreeView*>(action->data().value<QWidget*>());

    QModelIndexList sel = tree->selectionModel()->selection().indexes();

    if(sel.empty())
        return;

    ChannelPostFileInfo file;

    if(!static_cast<RsGxsChannelPostFilesModel*>(tree->model())->getFileData(sel.front(),file))
        return;

    RetroShareLink link = RetroShareLink::createFile(QString::fromUtf8(file.mName.c_str()), file.mSize, QString::fromStdString(file.mHash.toStdString()));
    RSLinkClipboard::copyLinks(QList<RetroShareLink>{ link });
}

void GxsChannelPostsWidgetWithModel::setSubscribeButtonText(const RsGxsGroupId& group_id,uint32_t flags, uint32_t mPop)
{
    if(IS_GROUP_SUBSCRIBED(flags))
    {
        ui->subscribeToolButton->setText(tr("Subscribed")+ " " + QString::number(mPop));
        ui->subscribeToolButton->setSubscribed(true);
        ui->subscribeToolButton->setEnabled(true);
    }
    else
    {
        switch(rsGxsChannels->getDistantSearchStatus(group_id))
        {
        case DistantSearchGroupStatus::UNKNOWN:  	// means no search ongoing. This is not a distant search
        case DistantSearchGroupStatus::HAVE_GROUP_DATA:	// fallthrough
            ui->subscribeToolButton->setText(tr("Subscribe"));
            ui->subscribeToolButton->setToolTip("");
            ui->subscribeToolButton->setSubscribed(false);
            ui->subscribeToolButton->setEnabled(true);
            break;
        case DistantSearchGroupStatus::CAN_BE_REQUESTED:  	// means no search ongoing. This is not a distant search
            ui->subscribeToolButton->setText(tr("Channel info missing"));
            ui->subscribeToolButton->setToolTip(tr("To subscribe, first request the channel information by right-clicking Request Data in the search results.") );
            ui->subscribeToolButton->setSubscribed(false);
            ui->subscribeToolButton->setEnabled(false);
            break;
        case DistantSearchGroupStatus::ONGOING_REQUEST:
            ui->subscribeToolButton->setText(tr("Channel info requested..."));
            ui->subscribeToolButton->setToolTip("");
            ui->subscribeToolButton->setSubscribed(true);
            ui->subscribeToolButton->setEnabled(false);
            break;
        }
    }
}

void GxsChannelPostsWidgetWithModel::switchOnlyUnread(bool)
{
    filterChanged(ui->filterLineEdit->text());
    std::cerr << "Switched to unread/read"<< std::endl;
}
void GxsChannelPostsWidgetWithModel::filterChanged(QString s)
{
    QStringList ql = s.split(' ',QString::SkipEmptyParts);
    uint32_t count;
    mChannelPostsModel->setFilter(ql,ui->showUnread_TB->isChecked(),count);
    mChannelFilesModel->setFilter(ql,count);
}

void GxsChannelPostsWidgetWithModel::blank()
{
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
    ui->postDAll_PB->hide();
    groupNameChanged(QString());

}

bool GxsChannelPostsWidgetWithModel::navigate(const RsGxsMessageId& msgId)
{
    QModelIndex index = mChannelPostsModel->getIndexOfMessage(msgId);

    if(!index.isValid())
    {
        std::cerr << "(EE) Cannot navigate to msg " << msgId << " in channel " << mGroup.mMeta.mGroupId << ": index unknown. Setting mNavigatePendingMsgId." << std::endl;

        mNavigatePendingMsgId = msgId;    // not found. That means the forum may not be loaded yet. So we keep that post in mind, for after loading.
        return true;                      // we have to return true here, otherwise the caller will intepret the async loading as an error.
    }

    ui->postsTree->selectionModel()->setCurrentIndex(index,QItemSelectionModel::ClearAndSelect);
    ui->postsTree->scrollTo(index);//May change if model reloaded
    ui->postsTree->setFocus();

    ui->channel_TW->setCurrentIndex(CHANNEL_TABS_POSTS);
    ui->details_TW->setCurrentIndex(CHANNEL_TABS_DETAILS);

    mNavigatePendingMsgId.clear();

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

#ifdef TO_REMOVE
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
#endif

class GxsChannelPostsReadData
{
public:
    explicit GxsChannelPostsReadData(bool read)
    {
        mRead = read;
    }

public:
    bool mRead;
};

void GxsChannelPostsWidgetWithModel::setAllMessagesReadDo(bool read)
{
    if (groupId().isNull() || !IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags))
        return;

    //QModelIndex src_index;

    mChannelPostsModel->setAllMsgReadStatus(read);
}


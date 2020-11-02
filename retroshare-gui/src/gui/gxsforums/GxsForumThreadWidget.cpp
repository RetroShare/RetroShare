/*******************************************************************************
 * retroshare-gui/src/gui/gxsforums/GxsForumsThreadWidget.cpp                  *
 *                                                                             *
 * Copyright 2012 Retroshare Team      <retroshare.project@gmail.com>          *
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
#include <QMessageBox>
#include <QKeyEvent>
#include <QScrollBar>
#include <QPainter>

#include "util/qtthreadsutils.h"
#include "util/misc.h"
#include "GxsForumThreadWidget.h"
#include "ui_GxsForumThreadWidget.h"
#include "GxsForumModel.h"
#include "GxsForumsDialog.h"
#include "gui/RetroShareLink.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "gui/settings/rsharesettings.h"
#include "gui/common/RSElidedItemDelegate.h"
#include "gui/settings/rsharesettings.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "gui/Identity/IdDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/HandleRichText.h"
#include "CreateGxsForumMsg.h"
#include "gui/MainWindow.h"
#include "gui/msgs/MessageComposer.h"
#include "util/DateTime.h"
#include "gui/common/UIStateHelper.h"
#include "util/QtVersion.h"
#include "util/imageutil.h"

#include <retroshare/rsgxsforums.h>
#include <retroshare/rsgrouter.h>
#include <retroshare/rspeers.h>
// These should be in retroshare/ folder.
#include "retroshare/rsgxsflags.h"

#include <iostream>
#include <algorithm>

//#define DEBUG_FORUMS

/* Images for context menu icons */
#define IMAGE_MESSAGE          ":/icons/mail/compose.png"
#define IMAGE_REPLY            ":/icons/mail/reply.png"
#define IMAGE_MESSAGEREPLY     ":/icons/mail/write-mail.png"
#define IMAGE_MESSAGEEDIT      ":/icons/png/pencil-edit-button.png"
#define IMAGE_MESSAGEREMOVE    ":/images/mail_delete.png"
#define IMAGE_DOWNLOAD         ":/images/start.png"
#define IMAGE_DOWNLOADALL      ":/images/startall.png"
#define IMAGE_COPYLINK         ":/images/copyrslink.png"
#define IMAGE_BIOHAZARD        ":/icons/biohazard_red.png"
#define IMAGE_WARNING_YELLOW   ":/icons/warning_yellow_128.png"
#define IMAGE_WARNING_RED      ":/icons/warning_red_128.png"
#define IMAGE_WARNING_UNKNOWN  ":/icons/bullet_grey_128.png"
#define IMAGE_VOID             ":/icons/void_128.png"
#define IMAGE_PINPOST          ":/images/pin32.png"
#define IMAGE_POSITIVE_OPINION ":/icons/png/thumbs-up.png"
#define IMAGE_NEUTRAL_OPINION  ":/icons/png/thumbs-neutral.png"
#define IMAGE_NEGATIVE_OPINION ":/icons/png/thumbs-down.png"

#define VIEW_LAST_POST	0
#define VIEW_THREADED	1
#define VIEW_FLAT       2

/* Thread constants */

// We need consts for that!! Defined in multiple places.

#ifdef DEBUG_FORUMS
static std::ostream& operator<<(std::ostream& o,const QModelIndex& q)
{
    return o << "(" << q.row() << "," << q.column() << "," << (void*)q.internalPointer() << ")" ;
}
#endif

class DistributionItemDelegate: public QStyledItemDelegate
{
public:
    DistributionItemDelegate() {}

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if(!index.isValid())
        {
            std::cerr << "(EE) attempt to draw an invalid index." << std::endl;
            return ;
        }

        QStyleOptionViewItemV4 opt = option;
        initStyleOption(&opt, index);
        // disable default icon
        opt.icon = QIcon();
        // draw default item
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, 0);

        const QRect r = option.rect;

        QIcon icon ;

        // get pixmap
        unsigned int warning_level = qvariant_cast<unsigned int>(index.data(Qt::DecorationRole));

        switch(warning_level)
        {
        default:
        case 3:
        case 0: icon = FilesDefs::getIconFromQtResourcePath(IMAGE_VOID); break;
        case 1: icon = FilesDefs::getIconFromQtResourcePath(IMAGE_WARNING_YELLOW); break;
        case 2: icon = FilesDefs::getIconFromQtResourcePath(IMAGE_WARNING_RED); break;
        }

        QPixmap pix = icon.pixmap(r.size());

        // draw pixmap at center of item
        const QPoint p = QPoint((r.width() - pix.width())/2, (r.height() - pix.height())/2);
        painter->drawPixmap(r.topLeft() + p, pix);
    }

    virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        static auto img(FilesDefs::getPixmapFromQtResourcePath(IMAGE_WARNING_YELLOW));

        return QSize(img.width()*1.2,option.rect.height());
    }
};

class ReadStatusItemDelegate: public QStyledItemDelegate
{
public:
    ReadStatusItemDelegate() {}

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if(!index.isValid())
        {
            std::cerr << "(EE) attempt to draw an invalid index." << std::endl;
            return ;
        }

        QStyleOptionViewItemV4 opt = option;
        initStyleOption(&opt, index);
        // disable default icon
        opt.icon = QIcon();
        // draw default item
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, 0);

        const QRect r = option.rect;

        QIcon icon ;

        // get pixmap
        unsigned int read_status = qvariant_cast<uint32_t>(index.data(Qt::DecorationRole));

        bool pinned = index.data(RsGxsForumModel::ThreadPinnedRole).toBool();
        bool unread = IS_MSG_UNREAD(read_status);
        bool missing = index.sibling(index.row(),RsGxsForumModel::COLUMN_THREAD_DATA).data(RsGxsForumModel::MissingRole).toBool();

        // set icon
        if (missing)
            icon = QIcon();
        else if(pinned)
            icon = FilesDefs::getIconFromQtResourcePath(IMAGE_PINPOST);
        else
        {
            if (unread)
                icon = FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png");
            else
                icon = FilesDefs::getIconFromQtResourcePath(":/images/message-state-read.png");
        }

        QPixmap pix = icon.pixmap(r.size());

        // draw pixmap at center of item
        const QPoint p = QPoint((r.width() - pix.width())/2, (r.height() - pix.height())/2);
        painter->drawPixmap(r.topLeft() + p, pix);
    }

    virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        static auto img(FilesDefs::getPixmapFromQtResourcePath(":/images/message-state-unread.png"));

        return QSize(img.width()*1.2,option.rect.height());
    }
};

class ForumPostSortFilterProxyModel: public QSortFilterProxyModel
{
public:
    ForumPostSortFilterProxyModel(const QHeaderView *header,QObject *parent = NULL): QSortFilterProxyModel(parent),m_header(header) {}

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
    {
        bool  left_is_not_pinned  = ! left.data(RsGxsForumModel::ThreadPinnedRole).toBool();
        bool right_is_not_pinned  = !right.data(RsGxsForumModel::ThreadPinnedRole).toBool();

        if(left_is_not_pinned ^ right_is_not_pinned)
            return (m_header->sortIndicatorOrder()==Qt::AscendingOrder)?right_is_not_pinned:left_is_not_pinned ;	// always put pinned posts on top

        return left.data(RsGxsForumModel::SortRole) < right.data(RsGxsForumModel::SortRole) ;
    }

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        return sourceModel()->index(source_row,0,source_parent).data(RsGxsForumModel::FilterRole).toString() == RsGxsForumModel::FilterString ;
    }

private:
    const QHeaderView *m_header ;
};


void GxsForumThreadWidget::setTextColorRead          (QColor color) { mTextColorRead           = color; mThreadModel->setTextColorRead          (color);}
void GxsForumThreadWidget::setTextColorUnread        (QColor color) { mTextColorUnread         = color; mThreadModel->setTextColorUnread        (color);}
void GxsForumThreadWidget::setTextColorUnreadChildren(QColor color) { mTextColorUnreadChildren = color; mThreadModel->setTextColorUnreadChildren(color);}
void GxsForumThreadWidget::setTextColorNotSubscribed (QColor color) { mTextColorNotSubscribed  = color; mThreadModel->setTextColorNotSubscribed (color);}
void GxsForumThreadWidget::setTextColorMissing       (QColor color) { mTextColorMissing        = color; mThreadModel->setTextColorMissing       (color);}
// Suppose to be different from unread one
void GxsForumThreadWidget::setTextColorPinned        (QColor color) { mTextColorPinned         = color; mThreadModel->setTextColorPinned        (color);}

void GxsForumThreadWidget::setBackgroundColorPinned  (QColor color) { mBackgroundColorPinned   = color; mThreadModel->setBackgroundColorPinned   (color);}
void GxsForumThreadWidget::setBackgroundColorFiltered(QColor color) { mBackgroundColorFiltered = color; mThreadModel->setBackgroundColorFiltered (color);}

GxsForumThreadWidget::GxsForumThreadWidget(const RsGxsGroupId &forumId, QWidget *parent) :
    GxsMessageFrameWidget(rsGxsForums, parent),
    ui(new Ui::GxsForumThreadWidget)
{
    ui->setupUi(this);

    //setUpdateWhenInvisible(true);

    //mUpdating = false;
    mUnreadCount = 0;
    mNewCount = 0;

    mInMsgAsReadUnread = false;

    mThreadModel = new RsGxsForumModel(this);
    mThreadProxyModel = new ForumPostSortFilterProxyModel(ui->threadTreeWidget->header(),this);
    mThreadProxyModel->setSourceModel(mThreadModel);
    mThreadProxyModel->setSortRole(RsGxsForumModel::SortRole);
    ui->threadTreeWidget->setModel(mThreadProxyModel);

    mThreadProxyModel->setFilterRole(RsGxsForumModel::FilterRole);
    mThreadProxyModel->setFilterRegExp(QRegExp(QString(RsGxsForumModel::FilterString))) ;

    ui->threadTreeWidget->setSortingEnabled(true);

    ui->threadTreeWidget->setItemDelegateForColumn(RsGxsForumModel::COLUMN_THREAD_DISTRIBUTION,new DistributionItemDelegate()) ;
    ui->threadTreeWidget->setItemDelegateForColumn(RsGxsForumModel::COLUMN_THREAD_AUTHOR,new GxsIdTreeItemDelegate()) ;
    ui->threadTreeWidget->setItemDelegateForColumn(RsGxsForumModel::COLUMN_THREAD_READ,new ReadStatusItemDelegate()) ;

    ui->threadTreeWidget->header()->setSortIndicatorShown(true);

    connect(ui->versions_CB, SIGNAL(currentIndexChanged(int)), this, SLOT(changedVersion()));
    connect(ui->threadTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(threadListCustomPopupMenu(QPoint)));
    connect(ui->postText, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuTextBrowser(QPoint)));

    ui->subscribeToolButton->hide() ;
    connect(ui->subscribeToolButton, SIGNAL(subscribe(bool)), this, SLOT(subscribeGroup(bool)));
    connect(ui->newmessageButton, SIGNAL(clicked()), this, SLOT(replytoforummessage()));
    connect(ui->newthreadButton, SIGNAL(clicked()), this, SLOT(createthread()));

    connect(mThreadModel,SIGNAL(forumLoaded()),this,SLOT(postForumLoading()));

    ui->newmessageButton->setText(tr("Reply"));
    ui->newthreadButton->setText(tr("New thread"));

    connect(ui->threadTreeWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(clickedThread(QModelIndex)));
    connect(ui->threadTreeWidget->selectionModel(), SIGNAL(currentChanged(const QModelIndex&,const QModelIndex&)), this, SLOT(changedSelection(const QModelIndex&,const QModelIndex&)));
    connect(ui->viewBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changedViewBox()));

    //connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(togglethreadview()));
    ui->expandButton->hide();

    connect(ui->previousButton, SIGNAL(clicked()), this, SLOT(previousMessage()));
    connect(ui->nextButton, SIGNAL(clicked()), this, SLOT(nextMessage()));
    connect(ui->nextUnreadButton, SIGNAL(clicked()), this, SLOT(nextUnreadMessage()));
    connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(downloadAllFiles()));

    connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems(QString)));
    connect(ui->filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));

    connect(ui->actionSave_image, SIGNAL(triggered()), this, SLOT(saveImage()));

    /* Set own item delegate */
    RSElidedItemDelegate *itemDelegate = new RSElidedItemDelegate(this);
    itemDelegate->setSpacing(QSize(0, 2));
    itemDelegate->setOnlyPlainText(true);
    ui->threadTreeWidget->setItemDelegate(itemDelegate);

    /* add filter actions */
    ui->filterLineEdit->addFilter(QIcon(), tr("Title"), RsGxsForumModel::COLUMN_THREAD_TITLE, tr("Search Title"));
    ui->filterLineEdit->addFilter(QIcon(), tr("Date"), RsGxsForumModel::COLUMN_THREAD_DATE, tr("Search Date"));
    ui->filterLineEdit->addFilter(QIcon(), tr("Author"), RsGxsForumModel::COLUMN_THREAD_AUTHOR, tr("Search Author"));

    mLastViewType = -1;

    float f = QFontMetricsF(font()).height()/14.0f ;

    /* Set header resize modes and initial section sizes */

    QHeaderView * ttheader = ui->threadTreeWidget->header () ;
    ttheader->resizeSection (RsGxsForumModel::COLUMN_THREAD_DATE,  140*f);
    ttheader->resizeSection (RsGxsForumModel::COLUMN_THREAD_TITLE, 440*f);
    ttheader->resizeSection (RsGxsForumModel::COLUMN_THREAD_AUTHOR, 150*f);

    ui->threadTreeWidget->resizeColumnToContents(RsGxsForumModel::COLUMN_THREAD_DISTRIBUTION);
    ui->threadTreeWidget->resizeColumnToContents(RsGxsForumModel::COLUMN_THREAD_READ);

    QHeaderView_setSectionResizeModeColumn(ttheader, RsGxsForumModel::COLUMN_THREAD_TITLE,        QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(ttheader, RsGxsForumModel::COLUMN_THREAD_DATE,         QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(ttheader, RsGxsForumModel::COLUMN_THREAD_AUTHOR,       QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(ttheader, RsGxsForumModel::COLUMN_THREAD_READ,         QHeaderView::Fixed);
    QHeaderView_setSectionResizeModeColumn(ttheader, RsGxsForumModel::COLUMN_THREAD_DISTRIBUTION, QHeaderView::Fixed);

    ttheader->setCascadingSectionResizes(true);

    /* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
    ttheader->hideSection (RsGxsForumModel::COLUMN_THREAD_CONTENT);
    ttheader->hideSection (RsGxsForumModel::COLUMN_THREAD_MSGID);
    ttheader->hideSection (RsGxsForumModel::COLUMN_THREAD_DATA);

    ttheader->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ttheader, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(headerContextMenuRequested(QPoint)));

    ui->progressBar->hide();
    ui->progressText->hide();

    mFillThread = NULL;

    setGroupId(forumId);

    //ui->threadTreeWidget->installEventFilter(this) ;

    // load settings
    processSettings(true);

    mDisplayBannedText = false;

    blankPost();

    ui->subscribeToolButton->setToolTip(tr( "<p>Subscribing to the forum will gather \
                                            available posts from your subscribed friends, and make the \
                                            forum visible to all other friends.</p><p>Afterwards you can unsubscribe from the context menu of the forum list at left.</p>"));
#ifdef SUSPENDED_CODE
    ui->threadTreeWidget->enableColumnCustomize(true);
#endif

    mEventHandlerId = 0;
    // Needs to be asynced because this function is called by another thread!
    rsEvents->registerEventsHandler(
                [this](std::shared_ptr<const RsEvent> event)
    { RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this ); },
                mEventHandlerId, RsEventType::GXS_FORUMS );
}

void GxsForumThreadWidget::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    if(event->mType == RsEventType::GXS_FORUMS)
    {
        const RsGxsForumEvent *e = dynamic_cast<const RsGxsForumEvent*>(event.get());
        if(!e) return;

        switch(e->mForumEventCode)
        {
        case RsForumEventCode::UPDATED_FORUM:   // [[fallthrough]];
        case RsForumEventCode::NEW_FORUM:       // [[fallthrough]];
        case RsForumEventCode::UPDATED_MESSAGE: // [[fallthrough]];
        case RsForumEventCode::NEW_MESSAGE:
        case RsForumEventCode::SYNC_PARAMETERS_UPDATED:
            if(e->mForumGroupId == mForumGroup.mMeta.mGroupId)
                updateDisplay(true);
            break;
        default: break;
        }
    }
}

void GxsForumThreadWidget::blank()
{
    ui->subscribeToolButton->hide();
    ui->newthreadButton->hide();
    ui->forumName->setText("");
    ui->progressText->hide();
    ui->progressBar->hide();
    ui->viewBox->setEnabled(false);
    ui->filterLineEdit->setEnabled(false);

    mThreadModel->clear();

    blankPost();
}

GxsForumThreadWidget::~GxsForumThreadWidget()
{
    rsEvents->unregisterEventsHandler(mEventHandlerId);
    // save settings
    processSettings(false);

    delete ui;
}

void GxsForumThreadWidget::processSettings(bool load)
{
    QHeaderView *header = ui->threadTreeWidget->header();

    Settings->beginGroup(QString("ForumThreadWidget"));

    if (load) {
        // load settings

        // expandFiles
        bool bValue = Settings->value("expandButton", true).toBool();
        ui->expandButton->setChecked(bValue);
        togglethreadview_internal();

        // filterColumn
        ui->filterLineEdit->setCurrentFilter(Settings->value("filterColumn", RsGxsForumModel::COLUMN_THREAD_TITLE).toInt());

        // index of viewBox
        ui->viewBox->setCurrentIndex(Settings->value("viewBox", VIEW_THREADED).toInt());

        // state of thread tree
        header->restoreState(Settings->value("ThreadTree").toByteArray());

        // state of splitter
        ui->threadSplitter->restoreState(Settings->value("threadSplitter").toByteArray());
    } else {
        // save settings

        // state of thread tree
        Settings->setValue("ThreadTree", header->saveState());

        // state of splitter
        Settings->setValue("threadSplitter", ui->threadSplitter->saveState());
    }

    Settings->endGroup();
}

void GxsForumThreadWidget::changedSelection(const QModelIndex& current,const QModelIndex& last)
{
    if (current!=last
        && ( ( last.row()>=0 && last.column()>=0) //Double call when retrieve focus.
            || mThreadId.isNull() //For first click
           )
       )
        changedThread(current);
}

void GxsForumThreadWidget::groupIdChanged()
{
    ui->forumName->setText(groupId().isNull () ? "" : tr("Loading..."));

    mNewCount = 0;
    mUnreadCount = 0;

    updateDisplay(true);
}

QString GxsForumThreadWidget::groupName(bool withUnreadCount)
{
    QString name = groupId().isNull () ? tr("No name") : ui->forumName->text();

    if (withUnreadCount && mUnreadCount) {
        name += QString(" (%1)").arg(mUnreadCount);
    }

    return name;
}

QIcon GxsForumThreadWidget::groupIcon()
{
    if (mNewCount) {
        return FilesDefs::getIconFromQtResourcePath(":/images/message-state-new.png");
    }

    return QIcon();
}

void GxsForumThreadWidget::saveExpandedItems(QList<RsGxsMessageId>& expanded_items) const
{
    expanded_items.clear();

    for(int row = 0; row < mThreadProxyModel->rowCount(); ++row)
    {
        std::string path = mThreadProxyModel->index(row,0).data(Qt::DisplayRole).toString().toStdString();

        recursSaveExpandedItems(mThreadProxyModel->index(row,0),expanded_items);
    }
}

void GxsForumThreadWidget::recursSaveExpandedItems(const QModelIndex& index, QList<RsGxsMessageId>& expanded_items) const
{
    if(ui->threadTreeWidget->isExpanded(index))
    {
        for(int row=0;row<mThreadProxyModel->rowCount(index);++row)
            recursSaveExpandedItems(index.child(row,0),expanded_items) ;

        RsGxsMessageId message_id(index.sibling(index.row(),RsGxsForumModel::COLUMN_THREAD_MSGID).data(Qt::UserRole).toString().toStdString());
        expanded_items.push_back(message_id);
    }
}

void GxsForumThreadWidget::recursRestoreExpandedItems(const QModelIndex& /*index*/, const QList<RsGxsMessageId>& expanded_items)
{
    for(auto it(expanded_items.begin());it!=expanded_items.end();++it)
        ui->threadTreeWidget->setExpanded( mThreadProxyModel->mapFromSource(mThreadModel->getIndexOfMessage(*it)) ,true) ;
}


void GxsForumThreadWidget::updateDisplay(bool complete)
{
#ifdef DEBUG_FORUMS
    std::cerr << "udateDisplay: groupId()=" << groupId()<< std::endl;
#endif
    if(groupId().isNull())
    {
#ifdef DEBUG_FORUMS
        std::cerr << "  group_id=0. Return!"<< std::endl;
#endif
        ui->nextUnreadButton->setEnabled(false);
        return;
    }

    if(mForumGroup.mMeta.mGroupId.isNull() && !groupId().isNull())
    {
#ifdef DEBUG_FORUMS
        std::cerr << "  inconsistent group data. Reloading!"<< std::endl;
#endif
        complete = true;
    }
    if(complete) 	// need to update the group data, reload the messages etc.
    {
        saveExpandedItems(mSavedExpandedMessages);

        if(groupId() != mThreadModel->currentGroupId())
            mThreadId.clear();

        updateGroupData();
        mThreadModel->updateForum(groupId());

        return;
    }
}

QModelIndex GxsForumThreadWidget::GxsForumThreadWidget::getCurrentIndex() const
{
    QModelIndexList selectedIndexes = ui->threadTreeWidget->selectionModel()->selectedIndexes();

    if(selectedIndexes.size() != RsGxsForumModel::COLUMN_THREAD_NB_COLUMNS)	// check that a single row is selected
        return QModelIndex();

    return *selectedIndexes.begin();
}
bool GxsForumThreadWidget::getCurrentPost(ForumModelPostEntry& fmpe) const
{
    QModelIndex indx = getCurrentIndex() ;

    if(!indx.isValid())
        return false ;

    return mThreadModel->getPostData(mThreadProxyModel->mapToSource(indx),fmpe);
}

void GxsForumThreadWidget::threadListCustomPopupMenu(QPoint /*point*/)
{
    QMenu contextMnu(this);

    ForumModelPostEntry current_post ;
    bool has_current_post = getCurrentPost(current_post);
#ifdef DEBUG_FORUMS
    std::cerr << "Clicked on msg " << current_post.mMsgId << std::endl;
#endif
    QAction *editAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MESSAGEEDIT), tr("Edit"), &contextMnu);
    connect(editAct, SIGNAL(triggered()), this, SLOT(editforummessage()));

    bool is_pinned = mForumGroup.mPinnedPosts.ids.find(mThreadId) != mForumGroup.mPinnedPosts.ids.end();
    QAction *pinUpPostAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_PINPOST), (is_pinned?tr("Un-pin this post"):tr("Pin this post up")), &contextMnu);
    connect(pinUpPostAct , SIGNAL(triggered()), this, SLOT(togglePinUpPost()));

    QAction *replyAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_REPLY), tr("Reply"), &contextMnu);
    connect(replyAct, SIGNAL(triggered()), this, SLOT(replytoforummessage()));

    QAction *replyauthorAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MESSAGEREPLY), tr("Reply to author with private message"), &contextMnu);
    connect(replyauthorAct, SIGNAL(triggered()), this, SLOT(reply_with_private_message()));

    QAction *flagaspositiveAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_POSITIVE_OPINION), tr("Give positive opinion"), &contextMnu);
    flagaspositiveAct->setToolTip(tr("This will block/hide messages from this person, and notify friend nodes.")) ;
    flagaspositiveAct->setData(static_cast<uint32_t>(RsOpinion::POSITIVE));
    connect(flagaspositiveAct, SIGNAL(triggered()), this, SLOT(flagperson()));

    QAction *flagasneutralAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_NEUTRAL_OPINION), tr("Give neutral opinion"), &contextMnu);
    flagasneutralAct->setToolTip(tr("Doing this, you trust your friends to decide to forward this message or not.")) ;
    flagasneutralAct->setData(static_cast<uint32_t>(RsOpinion::NEUTRAL));
    connect(flagasneutralAct, SIGNAL(triggered()), this, SLOT(flagperson()));

    QAction *flagasnegativeAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_NEGATIVE_OPINION), tr("Give negative opinion"), &contextMnu);
    flagasnegativeAct->setToolTip(tr("This will block/hide messages from this person, and notify friend nodes.")) ;
    flagasnegativeAct->setData(static_cast<uint32_t>(RsOpinion::NEGATIVE));
    connect(flagasnegativeAct, SIGNAL(triggered()), this, SLOT(flagperson()));

    QAction *newthreadAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MESSAGE), tr("Start New Thread"), &contextMnu);
    newthreadAct->setEnabled (IS_GROUP_SUBSCRIBED(mForumGroup.mMeta.mSubscribeFlags));
    connect(newthreadAct , SIGNAL(triggered()), this, SLOT(createthread()));

    QAction* expandAll = new QAction(tr("Expand all"), &contextMnu);
    connect(expandAll, SIGNAL(triggered()), ui->threadTreeWidget, SLOT(expandAll()));

#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    QAction* expandSubtree = new QAction(tr("Expand subtree"), &contextMnu);
    connect(expandSubtree, SIGNAL(triggered()), this, SLOT(expandSubtree()));
#endif

    QAction* collapseAll = new QAction(tr( "Collapse all"), &contextMnu);
    connect(collapseAll, SIGNAL(triggered()), ui->threadTreeWidget, SLOT(collapseAll()));

    QAction *markMsgAsRead = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/message-mail-read.png"), tr("Mark as read"), &contextMnu);
    connect(markMsgAsRead, SIGNAL(triggered()), this, SLOT(markMsgAsRead()));

    QAction *markMsgAsReadChildren = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/message-mail-read.png"), tr("Mark as read") + " (" + tr ("with children") + ")", &contextMnu);
    connect(markMsgAsReadChildren, SIGNAL(triggered()), this, SLOT(markMsgAsReadChildren()));

    QAction *markMsgAsUnread = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/message-mail.png"), tr("Mark as unread"), &contextMnu);
    connect(markMsgAsUnread, SIGNAL(triggered()), this, SLOT(markMsgAsUnread()));

    QAction *markMsgAsUnreadChildren = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/message-mail.png"), tr("Mark as unread") + " (" + tr ("with children") + ")", &contextMnu);
    connect(markMsgAsUnreadChildren, SIGNAL(triggered()), this, SLOT(markMsgAsUnreadChildren()));

    QAction *showinpeopleAct = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/info16.png"), tr("Show author in people tab"), &contextMnu);
    connect(showinpeopleAct, SIGNAL(triggered()), this, SLOT(showInPeopleTab()));

    bool has_children = false;
    if (has_current_post) {
        has_children = !current_post.mChildren.empty();
    }

    if (IS_GROUP_SUBSCRIBED(mForumGroup.mMeta.mSubscribeFlags))
    {
        markMsgAsReadChildren->setEnabled(current_post.mPostFlags & ForumModelPostEntry::FLAG_POST_HAS_UNREAD_CHILDREN);
        markMsgAsUnreadChildren->setEnabled(current_post.mPostFlags & ForumModelPostEntry::FLAG_POST_HAS_READ_CHILDREN);

#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        expandSubtree->setEnabled(has_children);
#endif
        replyAct->setEnabled (true);
        replyauthorAct->setEnabled (true);
    }
    else
    {
        markMsgAsRead->setDisabled(true);
        markMsgAsReadChildren->setDisabled(true);
        markMsgAsUnread->setDisabled(true);
        markMsgAsUnreadChildren->setDisabled(true);
        replyAct->setDisabled (true);
        replyauthorAct->setDisabled (true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        expandSubtree->setDisabled(true);
        expandSubtree->setVisible(false);
#endif
    }

    // disable visibility for childless
    if (has_current_post) {
        // still no setEnabled
        markMsgAsRead->setVisible(IS_MSG_UNREAD(current_post.mMsgStatus));
        markMsgAsUnread->setVisible(!IS_MSG_UNREAD(current_post.mMsgStatus));
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        expandSubtree->setVisible(has_children);
#endif
        markMsgAsReadChildren->setVisible(has_children);
        markMsgAsUnreadChildren->setVisible(has_children);
    }

    if(has_current_post)
    {
        bool is_pinned = mForumGroup.mPinnedPosts.ids.find( current_post.mMsgId ) != mForumGroup.mPinnedPosts.ids.end();

        if(!is_pinned)
        {
            RsGxsId author_id;
            if(rsIdentity->isOwnId(current_post.mAuthorId))
                contextMnu.addAction(editAct);
            else
            {
                // Go through the list of own ids and see if one of them is a moderator
                // TODO: offer to select which moderator ID to use if multiple IDs fit the conditions of the forum

                std::list<RsGxsId> own_ids ;
                rsIdentity->getOwnIds(own_ids) ;

                for(auto it(own_ids.begin());it!=own_ids.end();++it)
                    if(mForumGroup.canEditPosts(*it))
                    {
                        contextMnu.addAction(editAct);
                        break ;
                    }
            }
        }

        if(IS_GROUP_ADMIN(mForumGroup.mMeta.mSubscribeFlags) && (current_post.mParent == 0))
            contextMnu.addAction(pinUpPostAct);
    }

    contextMnu.addAction(replyAct);
    contextMnu.addAction(newthreadAct);
    QAction* action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyMessageLink()));
    action->setEnabled(!groupId().isNull() && !mThreadId.isNull());
    contextMnu.addSeparator();
    contextMnu.addAction(markMsgAsRead);
    contextMnu.addAction(markMsgAsReadChildren);
    contextMnu.addAction(markMsgAsUnread);
    contextMnu.addAction(markMsgAsUnreadChildren);
    contextMnu.addSeparator();
    contextMnu.addAction(expandAll);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    contextMnu.addAction(expandSubtree);
#endif
    contextMnu.addAction(collapseAll);

    if(has_current_post)
    {
#ifdef DEBUG_FORUMS
        std::cerr << "Author is: " << current_post.mAuthorId << std::endl;
#endif
        contextMnu.addSeparator();

        RsOpinion op;

        if(!rsIdentity->isOwnId(current_post.mAuthorId) && rsReputations->getOwnOpinion(current_post.mAuthorId,op))
        {
            QMenu *submenu1 = contextMnu.addMenu(tr("Author's reputation")) ;

            if(op != RsOpinion::POSITIVE)
                submenu1->addAction(flagaspositiveAct);

            if(op != RsOpinion::NEUTRAL)
                submenu1->addAction(flagasneutralAct);

            if(op != RsOpinion::NEGATIVE)
                submenu1->addAction(flagasnegativeAct);
        }

        contextMnu.addAction(showinpeopleAct);
        contextMnu.addAction(replyauthorAct);
    }

    contextMnu.exec(QCursor::pos());
}

void GxsForumThreadWidget::contextMenuTextBrowser(QPoint point)
{
    QMatrix matrix;
    matrix.translate(ui->postText->horizontalScrollBar()->value(), ui->postText->verticalScrollBar()->value());

    QMenu *contextMnu = ui->postText->createStandardContextMenu(matrix.map(point));

    contextMnu->addSeparator();

    if(ui->postText->checkImage(point))
    {
        ui->actionSave_image->setData(point);
        contextMnu->addAction(ui->actionSave_image);
    }

    contextMnu->exec(ui->postText->viewport()->mapToGlobal(point));
    delete(contextMnu);
}

void GxsForumThreadWidget::headerContextMenuRequested(const QPoint &pos)
{
    QMenu* header_context_menu = new QMenu(tr("Show column"), this);

    QAction* title = header_context_menu->addAction(QIcon(), tr("Title"));
    title->setCheckable(true);
    title->setChecked(!ui->threadTreeWidget->isColumnHidden(RsGxsForumModel::COLUMN_THREAD_TITLE));
    title->setData(RsGxsForumModel::COLUMN_THREAD_TITLE);
    connect(title, SIGNAL(toggled(bool)), this, SLOT(changeHeaderColumnVisibility(bool)));

    QAction* read = header_context_menu->addAction(QIcon(), tr("Read"));
    read->setCheckable(true);
    read->setChecked(!ui->threadTreeWidget->isColumnHidden(RsGxsForumModel::COLUMN_THREAD_READ));
    read->setData(RsGxsForumModel::COLUMN_THREAD_READ);
    connect(read, SIGNAL(toggled(bool)), this, SLOT(changeHeaderColumnVisibility(bool)));

    QAction* date = header_context_menu->addAction(QIcon(), tr("Date"));
    date->setCheckable(true);
    date->setChecked(!ui->threadTreeWidget->isColumnHidden(RsGxsForumModel::COLUMN_THREAD_DATE));
    date->setData(RsGxsForumModel::COLUMN_THREAD_DATE);
    connect(date, SIGNAL(toggled(bool)), this, SLOT(changeHeaderColumnVisibility(bool)));

    QAction* distribution = header_context_menu->addAction(QIcon(), tr("Distribution"));
    distribution->setCheckable(true);
    distribution->setChecked(!ui->threadTreeWidget->isColumnHidden(RsGxsForumModel::COLUMN_THREAD_DISTRIBUTION));
    distribution->setData(RsGxsForumModel::COLUMN_THREAD_DISTRIBUTION);
    connect(distribution, SIGNAL(toggled(bool)), this, SLOT(changeHeaderColumnVisibility(bool)));

    // QAction* author = header_context_menu->addAction(QIcon(), tr("Author"));
    // author->setCheckable(true);
    // author->setChecked(!ui->threadTreeWidget->isColumnHidden(RsGxsForumModel::COLUMN_THREAD_AUTHOR));
    // author->setData(RsGxsForumModel::COLUMN_THREAD_AUTHOR);
    // connect(author, SIGNAL(toggled(bool)), this, SLOT(changeHeaderColumnVisibility(bool)));

    QAction* show_text_from_banned = header_context_menu->addAction(QIcon(), tr("Show text from banned persons"));
    show_text_from_banned->setCheckable(true);
    show_text_from_banned->setChecked(mDisplayBannedText);
    connect(show_text_from_banned, SIGNAL(toggled(bool)), this, SLOT(showBannedText(bool)));

    header_context_menu->exec(mapToGlobal(pos));
    delete(header_context_menu);
}

void GxsForumThreadWidget::changeHeaderColumnVisibility(bool visibility) {
    QAction* the_action = qobject_cast<QAction*>(sender());
    if ( !the_action ) {
        return;
    }
    ui->threadTreeWidget->setColumnHidden(the_action->data().toInt(), !visibility);
}

void GxsForumThreadWidget::showBannedText(bool display) {
    mDisplayBannedText = display;
    if (!mThreadId.isNull()) {
        updateMessageData(mThreadId);
    }
}

#ifdef TODO
bool GxsForumThreadWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->threadTreeWidget) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent && keyEvent->key() == Qt::Key_Space) {
                // Space pressed
                QTreeWidgetItem *item = ui->threadTreeWidget->currentItem();
                clickedThread (item, RsGxsForumModel::COLUMN_THREAD_READ);
                return true; // eat event
            }
        }
    }
    // pass the event on to the parent class
    return RsGxsUpdateBroadcastWidget::eventFilter(obj, event);
    return RsGxsUpdateBroadcastWidget::eventFilter(obj, event);
}
#endif

void GxsForumThreadWidget::togglethreadview()
{
    // save state of button
    Settings->setValueToGroup("ForumThreadWidget", "expandButton", ui->expandButton->isChecked());

    togglethreadview_internal();
}

void GxsForumThreadWidget::togglethreadview_internal()
{
//	if (ui->expandButton->isChecked()) {
        ui->postText->setVisible(true);
        ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/images/edit_remove24.png")));
        ui->expandButton->setToolTip(tr("Hide"));
//	} else  {
//		ui->postText->setVisible(false);
//		ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/images/edit_add24.png")));
//		ui->expandButton->setToolTip(tr("Expand"));
//	}
}

void GxsForumThreadWidget::changedVersion()
{
    //if(mUpdating)
    //    return;

    mThreadId = RsGxsMessageId(ui->versions_CB->itemData(ui->versions_CB->currentIndex()).toString().toStdString()) ;

    ui->postText->resetImagesStatus(Settings->getForumLoadEmbeddedImages()) ;
    insertMessage();
}

void GxsForumThreadWidget::changedThread(QModelIndex index)
{
    if(!index.isValid())
        return;

    RsGxsMessageId new_id(index.sibling(index.row(),RsGxsForumModel::COLUMN_THREAD_MSGID).data(Qt::UserRole).toString().toStdString());

    if(new_id == mThreadId)
        return;

    mThreadId = mOrigThreadId = new_id;

#ifdef DEBUG_FORUMS
    std::cerr << "Switched to new thread ID " << mThreadId << std::endl;
#endif

    insertMessage();

    if(Settings->getForumMsgSetToReadOnActivate())
    {
#ifdef DEBUG_FORUMS
        std::cerr << "Setting message read status to true" << std::endl;
#endif
        markMsgAsReadUnread(true, false, false);
    }
}

void GxsForumThreadWidget::clickedThread(QModelIndex index)
{
#ifdef DEBUG_FORUMS
    std::cerr << "Clicked on message ID " << mThreadId << ", index=" << index << std::endl;
#endif

    if(!index.isValid())
    {
#ifdef DEBUG_FORUMS
        std::cerr << "  early return because index is invalid" << std::endl;
#endif
        return;
    }


    if (index.column() == RsGxsForumModel::COLUMN_THREAD_READ)
    {
        QModelIndex src_index = mThreadProxyModel->mapToSource(index);

        ForumModelPostEntry fmpe;
        mThreadModel->getPostData(src_index,fmpe);
#ifdef DEBUG_FORUMS
        std::cerr << "Setting message read status to false" << std::endl;
#endif
        // First Load Message (may change read status) to not recall it after index change.
        changedThread(index);
        // Now index is invalid as model was reloaded, Selection isn't updated.
        markMsgAsReadUnread(IS_MSG_UNREAD(static_cast<uint32_t>(fmpe.mMsgStatus)), false, false, mThreadId);
    }
#ifdef DEBUG_FORUMS
    else
        std::cerr << "  doing nothing" << std::endl;
#endif
}

static QString getDurationString(uint32_t days)
{
    switch(days)
    {
        case 0: return QObject::tr("Indefinitely") ;
        case 5: return QObject::tr("5 days") ;
        case 15: return QObject::tr("2 weeks") ;
        case 30: return QObject::tr("1 month") ;
        case 60: return QObject::tr("2 month") ;
        case 180: return QObject::tr("6 month") ;
        case 365: return QObject::tr("1 year") ;
    default:
        return QString::number(days)+" " + QObject::tr("days") ;
    }
}

void GxsForumThreadWidget::setForumDescriptionLoading()
{
    ui->postText->setText(tr("<b>Loading...<b>"));
}

void GxsForumThreadWidget::clearForumDescription()
{
    ui->postText->clear();
}

void GxsForumThreadWidget::blankPost()
{
    ui->newmessageButton->setEnabled(false);
    ui->previousButton->setEnabled(false);
    ui->nextButton->setEnabled(false);
    ui->downloadButton->setEnabled(false);
    ui->lineLeft->hide();
    ui->time_label->clear();
    ui->versions_CB->hide();
    ui->lineRight->hide();
    ui->by_text_label->hide();
    ui->by_label->setId(RsGxsId()) ;
    ui->by_label->hide();
    ui->expandButton->hide();

    ui->postText->clear() ;
    ui->postText->setImageBlockWidget(ui->imageBlockWidget) ;
    ui->postText->resetImagesStatus(Settings->getForumLoadEmbeddedImages());

}

void GxsForumThreadWidget::updateForumDescription(bool success)
{
    if(!success)
    {
        blank();
        QString forum_description = QString("<b>ERROR:</b> Forum could not be loaded. Database might be in heavy use. Please try later.");
        ui->postText->setText(forum_description);
        ui->newthreadButton->setEnabled(false);
        return;
    }

    std::cerr << "Updating forum description" << std::endl;
    if (!mThreadId.isNull())
        return;

    // still call it to not left leftovers from previous post if any
    blankPost();

    RsIdentityDetails details;

    rsIdentity->getIdDetails(mForumGroup.mMeta.mAuthorId,details);

    QString author = GxsIdDetails::getName(details);

    const RsGxsForumGroup& group = mForumGroup;

    ui->newthreadButton->show();
    ui->forumName->setText(QString::fromUtf8(group.mMeta.mGroupName.c_str()));
    ui->viewBox->setEnabled(true);
    ui->filterLineEdit->setEnabled(true);

    QString anti_spam_features1 ;
    QString forum_description;

    if(IS_GROUP_PGP_KNOWN_AUTHED(mForumGroup.mMeta.mSignFlags)) anti_spam_features1 = tr("Anonymous/unknown posts forwarded if reputation is positive");
    else if(IS_GROUP_PGP_AUTHED(mForumGroup.mMeta.mSignFlags)) anti_spam_features1 = tr("Anonymous posts forwarded if reputation is positive");

    forum_description = QString("<b>%1: \t</b>%2<br/>").arg(tr("Forum name"), QString::fromUtf8( group.mMeta.mGroupName.c_str()));
    forum_description += QString("<b>%1: </b>%2<br/>").arg(tr("Description"), group.mDescription.empty()? tr("[None]<br/>") :(QString::fromUtf8(group.mDescription.c_str())+"<br/>"));
    forum_description += QString("<b>%1: \t</b>%2<br/>").arg(tr("Subscribers")).arg(group.mMeta.mPop);
    forum_description += QString("<b>%1: \t</b>%2<br/>").arg(tr("Posts (at neighbor nodes)")).arg(group.mMeta.mVisibleMsgCount);

    if(group.mMeta.mLastPost==0)
        forum_description += QString("<b>%1: \t</b>%2<br/>").arg(tr("Last post")).arg(tr("Never"));
    else
        forum_description += QString("<b>%1: \t</b>%2<br/>").arg(tr("Last post")).arg(DateTime::formatLongDateTime(group.mMeta.mLastPost));

    if(IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags))
    {
        forum_description += QString("<b>%1: \t</b>%2<br/>").arg(tr("Synchronization")).arg(getDurationString( rsGxsForums->getSyncPeriod(group.mMeta.mGroupId)/86400 )) ;
        forum_description += QString("<b>%1: \t</b>%2<br/>").arg(tr("Storage")).arg(getDurationString( rsGxsForums->getStoragePeriod(group.mMeta.mGroupId)/86400));
    }

    QString distrib_string = tr("[unknown]");
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
    case GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY:
    {
        distrib_string = tr("Only friends nodes in group ") ;

        RsGroupInfo ginfo ;
        rsPeers->getGroupInfo(RsNodeGroupId(group.mMeta.mInternalCircle),ginfo) ;

        QString desc;
        GroupChooser::makeNodeGroupDesc(ginfo, desc);
        distrib_string += desc ;
    }
        break ;

    case GXS_CIRCLE_TYPE_LOCAL: distrib_string = tr("Your eyes only");	// this is not yet supported. If you see this, it is a bug!
        break ;
    default:
        std::cerr << "(EE) badly initialised group distribution ID = " << group.mMeta.mCircleType << std::endl;
    }

    forum_description += QString("<b>%1: \t</b>%2<br/>").arg(tr("Distribution"), distrib_string);
    forum_description += QString("<b>%1: \t</b>%2<br/>").arg(tr("Owner"), author);

       if(!anti_spam_features1.isNull())
            forum_description += QString("<b>%1: \t</b>%2<br/>").arg(tr("Anti-spam")).arg(anti_spam_features1);

    ui->subscribeToolButton->setSubscribed(IS_GROUP_SUBSCRIBED(mForumGroup.mMeta.mSubscribeFlags));
    ui->newthreadButton->setEnabled(IS_GROUP_SUBSCRIBED(mForumGroup.mMeta.mSubscribeFlags));

    if(!group.mAdminList.ids.empty())
    {
        QString admin_list_str ;

        for(auto it(group.mAdminList.ids.begin());it!=group.mAdminList.ids.end();++it)
        {
            RsIdentityDetails det ;

            rsIdentity->getIdDetails(*it,det);
            admin_list_str += (admin_list_str.isNull()?"":", ") + QString::fromUtf8(det.mNickname.c_str()) ;
        }

        forum_description += QString("<b>%1: </b>%2").arg(tr("Moderators"), admin_list_str);
    }

    ui->postText->setText(forum_description);
}

void GxsForumThreadWidget::insertMessage()
{
#ifdef DEBUG_FORUMS
    std::cerr << "Inserting message, threadId=" << mThreadId <<std::endl;
#endif
    if (groupId().isNull())
    {
#ifdef DEBUG_FORUMS
        std::cerr << "  groupId()=NULL !! That's a bug." << std::endl;
#endif
        ui->versions_CB->hide();
        ui->time_label->show();

        ui->postText->clear();
        return;
    }

    if (mThreadId.isNull())
    {
#ifdef DEBUG_FORUMS
        std::cerr << "  mThreadId=NULL !! That's a bug." << std::endl;
#endif
        ui->versions_CB->hide();
        ui->time_label->show();

        ui->postText->setText(QString::fromUtf8(mForumGroup.mDescription.c_str()));
        return;
    }

    /* blank text, incase we get nothing */
    blankPost();
    ui->nextUnreadButton->setEnabled(true);

    // We use this instead of getCurrentIndex() because right here the currentIndex() is not set yet.

    QModelIndex index = mThreadProxyModel->mapFromSource(mThreadModel->getIndexOfMessage(mOrigThreadId));

    if (index.isValid())
    {
        QModelIndex parentIndex = index.parent();
        int curr_index = index.row();
        int count = mThreadProxyModel->rowCount(parentIndex);

        ui->previousButton->setEnabled(curr_index > 0);
        ui->nextButton->setEnabled(curr_index < count - 1);
    } else {
#ifdef DEBUG_FORUMS
        std::cerr << "  current index invalid! That's a bug." << std::endl;
#endif
        // there is something wrong
        ui->previousButton->setEnabled(false);
        ui->nextButton->setEnabled(false);
        ui->versions_CB->hide();
        ui->time_label->show();
        return;
    }

    ui->newmessageButton->setEnabled(IS_GROUP_SUBSCRIBED(mForumGroup.mMeta.mSubscribeFlags) && mThreadId.isNull() == false);

    // add/show combobox for versions, if applicable, and enable it. If no older versions of the post available, hide the combobox.

    std::vector<std::pair<time_t,RsGxsMessageId> > post_versions = mThreadModel->getPostVersions(mOrigThreadId);

#ifdef DEBUG_FORUMS
    std::cerr << "Looking into existing versions  for post " << mOrigThreadId << ", thread history: " << post_versions.size() << std::endl;
#endif
    ui->versions_CB->blockSignals(true) ;

    while(ui->versions_CB->count() > 0)
        ui->versions_CB->removeItem(0);

    if(!post_versions.empty())
    {
#ifdef DEBUG_FORUMS
        std::cerr << post_versions.size() << " versions found " << std::endl;
#endif

        ui->versions_CB->setVisible(true) ;
        ui->time_label->hide();

        int current_index = 0 ;

        for(int i=0;i<post_versions.size();++i)
        {
            ui->versions_CB->insertItem(i, ((i==0)?tr("(Latest) "):tr("(Old) "))+" "+DateTime::formatLongDateTime( post_versions[i].first));
            ui->versions_CB->setItemData(i,QString::fromStdString(post_versions[i].second.toStdString()));

#ifdef DEBUG_FORUMS
            std::cerr << "  added new post version " << post_versions[i].first << " " << post_versions[i].second << std::endl;
#endif

            if(mThreadId == post_versions[i].second)
                current_index = i ;
        }

        ui->versions_CB->setCurrentIndex(current_index) ;
    }
    else
    {
        ui->versions_CB->hide();
        ui->time_label->show();
    }

    ui->versions_CB->blockSignals(false) ;

    /* request Post */
    updateMessageData(mThreadId);

//    markMsgAsRead();
}

void GxsForumThreadWidget::setMessageLoadingError(const QString& error)
{
    ui->time_label->setText(QString(""));
    ui->by_label->setId(RsGxsId());
    ui->lineRight->show();
    ui->lineLeft->show();
    ui->by_text_label->show();
    ui->by_label->show();
    ui->threadTreeWidget->setFocus();

    ui->postText->setText(error);
}

void GxsForumThreadWidget::insertMessageData(const RsGxsForumMsg &msg)
{
    /* As some time has elapsed since request - check that this is still the current msg.
     * otherwise, another request will fill the data
     */

    if ((msg.mMeta.mGroupId != groupId()) || (msg.mMeta.mMsgId != mThreadId))
    {
        std::cerr << "GxsForumThreadWidget::insertPostData() Ignoring Invalid Data....";
        std::cerr << std::endl;
        std::cerr << "\t CurrForumId: " << groupId() << " != msg.GroupId: " << msg.mMeta.mGroupId;
        std::cerr << std::endl;
        std::cerr << "\t or CurrThdId: " << mThreadId << " != msg.MsgId: " << msg.mMeta.mMsgId;
        std::cerr << std::endl;
        std::cerr << std::endl;

        return;
    }

    RsReputationLevel overall_reputation =
            rsReputations->overallReputationLevel(msg.mMeta.mAuthorId);
    bool redacted =
            (overall_reputation == RsReputationLevel::LOCALLY_NEGATIVE);

    // TODO enabled even when there are no new message
    ui->nextUnreadButton->setEnabled(true);
    ui->lineLeft->show();
    ui->time_label->setText(DateTime::formatLongDateTime(msg.mMeta.mPublishTs));
    ui->lineRight->show();
    ui->by_text_label->show();
    ui->by_label->setId(msg.mMeta.mAuthorId);
    ui->by_label->show();
    ui->threadTreeWidget->setFocus();

    QString banned_text_info = "";
    if(redacted) {
        ui->downloadButton->setDisabled(true);
        if (!mDisplayBannedText) {
            QString extraTxt = tr( "<p><font color=\"#ff0000\"><b>The author of this message (with ID %1) is banned.</b>").arg(QString::fromStdString(msg.mMeta.mAuthorId.toStdString())) ;
            extraTxt +=        tr( "<UL><li><b><font color=\"#ff0000\">Messages from this author are not forwarded. </font></b></li>") ;
            extraTxt +=        tr( "<li><b><font color=\"#ff0000\">Messages from this author are replaced by this text. </font></b></li></ul>") ;
            extraTxt +=        tr( "<p><b><font color=\"#ff0000\">You can force the visibility and forwarding of messages by setting a different opinion for that Id in People's tab.</font></b></p>") ;

            ui->postText->setHtml(extraTxt) ;
            return;
        }
        else {
            RsIdentityDetails details;
            rsIdentity->getIdDetails(msg.mMeta.mAuthorId, details);
            QString name = GxsIdDetails::getName(details);

            banned_text_info += "<p><font color=\"#e00000\"><b>" + tr( "The author of this message (with ID %1) is banned. And named by name ( %2 )").arg(QString::fromStdString(msg.mMeta.mAuthorId.toStdString()), name) + "</b>";
            banned_text_info += "<ul><li><b><font color=\"#e00000\">" + tr( "Messages from this author are not forwarded.") + "</font></b></li></ul>";
            banned_text_info += "<p><b><font color=\"#e00000\">" + tr( "You can force the visibility and forwarding of messages by setting a different opinion for that Id in People's tab.") + "</font></b></p><hr>";
        }
    }

    uint32_t flags = RSHTML_FORMATTEXT_EMBED_LINKS;
    if(Settings->getForumLoadEmoticons())
        flags |= RSHTML_FORMATTEXT_EMBED_SMILEYS ;
    flags |= RSHTML_OPTIMIZEHTML_MASK;

    QColor backgroundColor = ui->postText->palette().base().color();
    qreal desiredContrast = Settings->valueFromGroup("Forum",
        "MinimumContrast", 4.5).toDouble();
    int desiredMinimumFontSize = Settings->valueFromGroup("Forum",
        "MinimumFontSize", 10).toInt();

    QString extraTxt = banned_text_info + RsHtml().formatText(ui->postText->document(),
        QString::fromUtf8(msg.mMsg.c_str()), flags
            , backgroundColor, desiredContrast, desiredMinimumFontSize
        );
    ui->postText->setHtml(extraTxt);

    QStringList urls;
    RsHtml::findAnchors(ui->postText->toHtml(), urls);
    ui->downloadButton->setEnabled(urls.count() > 0);
}

void GxsForumThreadWidget::previousMessage()
{
    QModelIndex current_index = getCurrentIndex();

    if (!current_index.isValid())
        return;

    QModelIndex parentIndex = current_index.parent();

    int index = current_index.row();
    int count = mThreadModel->rowCount(parentIndex) ;

    if (index > 0)
    {
        QModelIndex prevItem = mThreadProxyModel->index(index - 1,0,parentIndex) ;

        if (prevItem.isValid()) {
            ui->threadTreeWidget->setCurrentIndex(prevItem);
            ui->threadTreeWidget->scrollTo(ui->threadTreeWidget->currentIndex());//May change if model reloaded
            ui->threadTreeWidget->setFocus();
        }
    }
    ui->previousButton->setEnabled(index-1 > 0);
    ui->nextButton->setEnabled(true);

}

void GxsForumThreadWidget::nextMessage()
{
    QModelIndex current_index = getCurrentIndex();

    if (!current_index.isValid())
        return;

    QModelIndex parentIndex = current_index.parent();

    int index = current_index.row();
    int count = mThreadProxyModel->rowCount(parentIndex);

    if (index < count - 1)
    {
        QModelIndex nextItem = mThreadProxyModel->index(index + 1,0,parentIndex) ;

        if (nextItem.isValid()) {
            ui->threadTreeWidget->setCurrentIndex(nextItem);
            ui->threadTreeWidget->scrollTo(ui->threadTreeWidget->currentIndex()); //May change if model reloaded
            ui->threadTreeWidget->setFocus();
        }
    }
    ui->previousButton->setEnabled(true);
    ui->nextButton->setEnabled(index+1 < count - 1);
}

void GxsForumThreadWidget::downloadAllFiles()
{
    QStringList urls;
    if (RsHtml::findAnchors(ui->postText->toHtml(), urls) == false) {
        return;
    }

    if (urls.count() == 0) {
        return;
    }

    RetroShareLink::process(urls, RetroShareLink::TYPE_FILE/*, true*/);
}

void GxsForumThreadWidget::nextUnreadMessage()
{
    QModelIndex index = getCurrentIndex();

    if(!index.isValid())
        index = mThreadProxyModel->index(0,0);
    else
    {
        if(index.data(RsGxsForumModel::UnreadChildrenRole).toBool())
            ui->threadTreeWidget->expand(index);

        index = ui->threadTreeWidget->indexBelow(index);
    }

    while(index.isValid() && !IS_MSG_UNREAD(index.sibling(index.row(),RsGxsForumModel::COLUMN_THREAD_DATA).data(RsGxsForumModel::StatusRole).toUInt()))
    {
        if(index.data(RsGxsForumModel::UnreadChildrenRole).toBool())
            ui->threadTreeWidget->expand(index);

        index = ui->threadTreeWidget->indexBelow(index);
    }

    ui->threadTreeWidget->setCurrentIndex(index);
    ui->threadTreeWidget->scrollTo(ui->threadTreeWidget->currentIndex());//May change if model reloaded
}

void GxsForumThreadWidget::markMsgAsReadUnread (bool read, bool children, bool forum, RsGxsMessageId msgId /*= RsGxsMessageId()*/)
{
    if (groupId().isNull() || !IS_GROUP_SUBSCRIBED(mForumGroup.mMeta.mSubscribeFlags)) {
        return;
    }
    saveExpandedItems(mSavedExpandedMessages);

    QModelIndex src_index;
    if(forum)
        src_index = mThreadModel->root();
    else
    {
        if (!msgId.isNull())
            src_index = mThreadProxyModel->mapToSource(getCurrentIndex());
        else
            src_index = mThreadModel->getIndexOfMessage(mThreadId);
    }
    mThreadModel->setMsgReadStatus(src_index,read,children);

    //Restore Selection
    whileBlocking(ui->threadTreeWidget)->setCurrentIndex(mThreadProxyModel->mapFromSource(mThreadModel->getIndexOfMessage(mThreadId)));
    recursRestoreExpandedItems(QModelIndex(),mSavedExpandedMessages);
}

void GxsForumThreadWidget::markMsgAsRead()
{
    markMsgAsReadUnread(true, false, false);
}

void GxsForumThreadWidget::markMsgAsReadChildren()
{
    markMsgAsReadUnread(true, true, false);
}

void GxsForumThreadWidget::markMsgAsUnread()
{
    markMsgAsReadUnread(false, false, false);
}

void GxsForumThreadWidget::markMsgAsUnreadChildren()
{
    markMsgAsReadUnread(false, true, false);
}

void GxsForumThreadWidget::setAllMessagesReadDo(bool read, uint32_t &/*token*/)
{
    markMsgAsReadUnread(read, true, true);
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
void GxsForumThreadWidget::expandSubtree() {
    QAction* the_action = qobject_cast<QAction*>(sender());
    if (!the_action) {
        return;
    }
    const QModelIndex current_index = ui->threadTreeWidget->currentIndex();
    if (!current_index.isValid()) {
        return;
    }
    ui->threadTreeWidget->expandRecursively(current_index);
}
#endif

bool GxsForumThreadWidget::navigate(const RsGxsMessageId &msgId)
{
    QModelIndex source_index = mThreadModel->getIndexOfMessage(msgId);

    if(!source_index.isValid())
    {
        std::cerr << "(EE) Cannot navigate to msg " << msgId << " in forum " << mForumGroup.mMeta.mGroupId << ": index unknown. Setting mNavigatePendingMsgId." << std::endl;

        mNavigatePendingMsgId = msgId;		// not found. That means the forum may not be loaded yet. So we keep that post in mind, for after loading.
        return true;						// we have to return true here, otherwise the caller will intepret the async loading as an error.
    }

    QModelIndex indx = mThreadProxyModel->mapFromSource(source_index);

    ui->threadTreeWidget->setCurrentIndex(indx);
    ui->threadTreeWidget->scrollTo(ui->threadTreeWidget->currentIndex());//May change if model reloaded
    ui->threadTreeWidget->setFocus();
    return true;
}

void GxsForumThreadWidget::copyMessageLink()
{
    if (groupId().isNull() || mThreadId.isNull()) {
        return;
    }

    ForumModelPostEntry fmpe ;
    getCurrentPost(fmpe);

    QString thread_title = QString::fromUtf8(fmpe.mTitle.c_str());

    RetroShareLink link = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_FORUM, groupId(), mThreadId, thread_title);

    if (link.valid()) {
        QList<RetroShareLink> urls;
        urls.push_back(link);
        RSLinkClipboard::copyLinks(urls);
    }
}

void GxsForumThreadWidget::subscribeGroup(bool subscribe)
{
    if (groupId().isNull()) {
        return;
    }

    uint32_t token;
    rsGxsForums->subscribeToGroup(token, groupId(), subscribe);
}

void GxsForumThreadWidget::createmessage()
{
    if (groupId().isNull () || !IS_GROUP_SUBSCRIBED(mForumGroup.mMeta.mSubscribeFlags)) {
        return;
    }

    CreateGxsForumMsg *cfm = new CreateGxsForumMsg(groupId(), mThreadId,RsGxsMessageId());
    cfm->show();

    /* window will destroy itself! */
}

void GxsForumThreadWidget::togglePinUpPost()
{
    if (groupId().isNull() || mOrigThreadId.isNull())
        return;

    QModelIndex index = getCurrentIndex();

    // normally this method is only called on top level items. We still check it just in case...

    if(mThreadProxyModel->mapToSource(index).parent().isValid())
    {
        std::cerr << "(EE) togglePinUpPost() called on non top level post. This is inconsistent." << std::endl;
        return ;
    }

    QString thread_title = index.sibling(index.row(),RsGxsForumModel::COLUMN_THREAD_TITLE).data(Qt::DisplayRole).toString();

#ifdef DEBUG_FORUMS
    std::cerr << "Toggling Pin-up state of post " << mThreadId.toStdString() << ": \"" << thread_title.toStdString() << "\"" << std::endl;
#endif

    if(mForumGroup.mPinnedPosts.ids.find(mThreadId) == mForumGroup.mPinnedPosts.ids.end())
        mForumGroup.mPinnedPosts.ids.insert(mThreadId) ;
    else
        mForumGroup.mPinnedPosts.ids.erase(mThreadId) ;

    uint32_t token;
    rsGxsForums->updateGroup(token,mForumGroup);

    groupIdChanged();		// reloads all posts. We could also update the model directly, but the cost is so small now ;-)
    updateDisplay(true) ;
}

void GxsForumThreadWidget::createthread()
{
    if (groupId().isNull ()) {
        QMessageBox::information(this, tr("RetroShare"), tr("No Forum Selected!"));
        return;
    }

    CreateGxsForumMsg *cfm = new CreateGxsForumMsg(groupId(), RsGxsMessageId(),RsGxsMessageId());
    cfm->show();

    /* window will destroy itself! */
}

static QString buildReplyHeader(const RsMsgMetaData &meta)
{
    RetroShareLink link = RetroShareLink::createMessage(meta.mAuthorId, "");
    QString from = link.toHtml();

    QString header = QString("<span>-----%1-----").arg(QApplication::translate("GxsForumThreadWidget", "Original Message"));
    header += QString("<br><font size='3'><strong>%1: </strong>%2</font><br>").arg(QApplication::translate("GxsForumThreadWidget", "From"), from);

    header += QString("<br><font size='3'><strong>%1: </strong>%2</font><br>").arg(QApplication::translate("GxsForumThreadWidget", "Sent"), DateTime::formatLongDateTime(meta.mPublishTs));
    header += QString("<font size='3'><strong>%1: </strong>%2</font></span><br>").arg(QApplication::translate("GxsForumThreadWidget", "Subject"), QString::fromUtf8(meta.mMsgName.c_str()));
    header += "<br>";

    header += QApplication::translate("GxsForumThreadWidget", "On %1, %2 wrote:").arg(DateTime::formatDateTime(meta.mPublishTs), from);

    return header;
}

void GxsForumThreadWidget::flagperson()
{
    // no need to use the token system for that, since we just need to find out the author's name, which is in the item.

    if (groupId().isNull() || mThreadId.isNull()) {
        QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to a non-existant Message"));
        return;
    }

    RsOpinion opinion =
            static_cast<RsOpinion>(
                qobject_cast<QAction*>(sender())->data().toUInt() );

    mThreadModel->setAuthorOpinion(
                mThreadProxyModel->mapToSource(getCurrentIndex()), opinion );
}

void GxsForumThreadWidget::replytoforummessage()        { async_msg_action( &GxsForumThreadWidget::replyForumMessageData ); }
void GxsForumThreadWidget::editforummessage()           { async_msg_action( &GxsForumThreadWidget::editForumMessageData  ); }
void GxsForumThreadWidget::reply_with_private_message() { async_msg_action( &GxsForumThreadWidget::replyMessageData      ); }
void GxsForumThreadWidget::showInPeopleTab()            { async_msg_action( &GxsForumThreadWidget::showAuthorInPeople    ); }

void GxsForumThreadWidget::async_msg_action(const MsgMethod &action)
{
    if (groupId().isNull() || mThreadId.isNull()) {
        QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to a non-existant Message"));
        return;
    }

    RsThread::async([this,action]()
    {
        // 1 - get message data from p3GxsForums

#ifdef DEBUG_FORUMS
        std::cerr << "Retrieving post data for post " << mThreadId << std::endl;
#endif

        std::set<RsGxsMessageId> msgs_to_request ;
        std::vector<RsGxsForumMsg> msgs;

        msgs_to_request.insert(mThreadId);

        if(!rsGxsForums->getForumContent(groupId(),msgs_to_request,msgs))
        {
            std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve forum message info for forum " << groupId() << " and thread " << mThreadId << std::endl;
            return;
        }

        if(msgs.size() != 1)
        {
            std::cerr << __PRETTY_FUNCTION__ << " more than 1 or no msgs selected in forum " << groupId() << std::endl;
            return;
        }

        // 2 - sort the messages into a proper hierarchy

        RsGxsForumMsg msg = msgs[0];

        // 3 - update the model in the UI thread.

        RsQThreadUtils::postToObject( [msg,action,this]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete */

            (this->*action)(msg);

        }, this );

    });
}

void GxsForumThreadWidget::replyMessageData(const RsGxsForumMsg &msg)
{
    if ((msg.mMeta.mGroupId != groupId()) || (msg.mMeta.mMsgId != mThreadId))
    {
        std::cerr << "(EE) GxsForumThreadWidget::replyMessageData() ERROR Message Ids have changed!";
        std::cerr << std::endl;
        return;
    }

    if (!msg.mMeta.mAuthorId.isNull())
    {
        MessageComposer *msgDialog = MessageComposer::newMsg();
        msgDialog->setTitleText(QString::fromUtf8(msg.mMeta.mMsgName.c_str()), MessageComposer::REPLY);

        msgDialog->setQuotedMsg(QString::fromUtf8(msg.mMsg.c_str()), buildReplyHeader(msg.mMeta));

        msgDialog->addRecipient(MessageComposer::TO, RsGxsId(msg.mMeta.mAuthorId));
        msgDialog->show();
        msgDialog->activateWindow();

        /* window will destroy itself! */
    }
    else
    {
        QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to an Anonymous Author"));
    }
}

void GxsForumThreadWidget::editForumMessageData(const RsGxsForumMsg& msg)
{
    if ((msg.mMeta.mGroupId != groupId()) || (msg.mMeta.mMsgId != mThreadId))
    {
        std::cerr << "(EE) GxsForumThreadWidget::replyMessageData() ERROR Message Ids have changed!";
        std::cerr << std::endl;
        return;
    }

    // Go through the list of own ids and see if one of them is a moderator
    // TODO: offer to select which moderator ID to use if multiple IDs fit the conditions of the forum

    RsGxsId moderator_id ;

    std::list<RsGxsId> own_ids ;
    rsIdentity->getOwnIds(own_ids) ;

    for(auto it(own_ids.begin());it!=own_ids.end();++it)
        if(mForumGroup.mAdminList.ids.find(*it) != mForumGroup.mAdminList.ids.end())
        {
            moderator_id = *it;
            break;
        }

    // Check that author is in own ids, if not use the moderator id that was collected among own ids.
    bool is_own = false ;
    for(auto it(own_ids.begin());it!=own_ids.end() && !is_own;++it)
        if(*it == msg.mMeta.mAuthorId)
            is_own = true ;

    if (!msg.mMeta.mAuthorId.isNull())
    {
        CreateGxsForumMsg *cfm = new CreateGxsForumMsg(groupId(), msg.mMeta.mParentId, msg.mMeta.mMsgId, is_own?(msg.mMeta.mAuthorId):moderator_id,!is_own);

        cfm->insertPastedText(QString::fromUtf8(msg.mMsg.c_str())) ;
        cfm->show();

        /* window will destroy itself! */
    }
    else
    {
        QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to an Anonymous Author"));
    }
}
void GxsForumThreadWidget::replyForumMessageData(const RsGxsForumMsg &msg)
{
    if ((msg.mMeta.mGroupId != groupId()) || (msg.mMeta.mMsgId != mThreadId))
    {
        std::cerr << "(EE) GxsForumThreadWidget::replyMessageData() ERROR Message Ids have changed!";
        std::cerr << std::endl;
        return;
    }

    if (!msg.mMeta.mAuthorId.isNull())
    {
    CreateGxsForumMsg *cfm = new CreateGxsForumMsg(groupId(), mThreadId,RsGxsMessageId());

    RsHtml::makeQuotedText(ui->postText);

    cfm->insertPastedText(RsHtml::makeQuotedText(ui->postText)) ;
    cfm->show();

        /* window will destroy itself! */
    }
    else
    {
        QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to an Anonymous Author"));
    }
}

void GxsForumThreadWidget::saveImage()
{
    QPoint point = ui->actionSave_image->data().toPoint();
    QTextCursor cursor = ui->postText->cursorForPosition(point);
    ImageUtil::extractImage(window(), cursor);
}

void GxsForumThreadWidget::changedViewBox()
{
    ui->threadTreeWidget->selectionModel()->clear();
    ui->threadTreeWidget->selectionModel()->reset();
    mThreadId.clear();

    // save index
    Settings->setValueToGroup("ForumThreadWidget", "viewBox", ui->viewBox->currentIndex());

    if(ui->viewBox->currentIndex() == VIEW_FLAT)
        mThreadModel->setTreeMode(RsGxsForumModel::TREE_MODE_FLAT);
    else
        mThreadModel->setTreeMode(RsGxsForumModel::TREE_MODE_TREE);

    if(ui->viewBox->currentIndex() == VIEW_LAST_POST)
        mThreadModel->setSortMode(RsGxsForumModel::SORT_MODE_CHILDREN_PUBLISH_TS);
    else
        mThreadModel->setSortMode(RsGxsForumModel::SORT_MODE_PUBLISH_TS);
}

void GxsForumThreadWidget::filterColumnChanged(int column)
{
    filterItems(ui->filterLineEdit->text());

    // save index
    Settings->setValueToGroup("ForumThreadWidget", "filterColumn", column);
}

void GxsForumThreadWidget::filterItems(const QString& text)
{
    QStringList lst = text.split(" ",QString::SkipEmptyParts) ;

    int filterColumn = ui->filterLineEdit->currentFilter();

    uint32_t count;
    mThreadModel->setFilter(filterColumn,lst,count) ;

    // We do this in order to trigger a new filtering action in the proxy model.
    mThreadProxyModel->setFilterRegExp(QRegExp(QString(RsGxsForumModel::FilterString))) ;

    if(!lst.empty())
        ui->threadTreeWidget->expandAll();
    else {
        // currentIndex() not on the clicked message, so not this way
        // if (!mThreadId.isNull()) {
        // 	an_index = mThreadProxyModel->mapToSource(ui->threadTreeWidget->currentIndex());
        // }
        ui->threadTreeWidget->collapseAll();
        if (!mThreadId.isNull()) {
            // ...but this one
            QModelIndex an_index = mThreadModel->getIndexOfMessage(mThreadId);
            if (an_index.isValid()) {
                QModelIndex the_index = mThreadProxyModel->mapFromSource(an_index);
                ui->threadTreeWidget->setCurrentIndex(the_index);
                ui->threadTreeWidget->scrollTo(the_index);
                // don't change focus
                // ui->threadTreeWidget->setFocus();
            }
        }
    }

    if(count > 0)
        ui->filterLineEdit->setToolTip(tr("No result.")) ;
    else
        ui->filterLineEdit->setToolTip(tr("Found %1 results.").arg(count)) ;
}

/*********************** **** **** **** ***********************/
/** Request / Response of Data ********************************/
/*********************** **** **** **** ***********************/

void GxsForumThreadWidget::postForumLoading()
{
    if(groupId().isNull())
    {
        ui->nextUnreadButton->setEnabled(false);
        return;
    }

#ifdef DEBUG_FORUMS
    std::cerr << "Post forum loading..." << std::endl;
#endif
    if(!mNavigatePendingMsgId.isNull() && mThreadModel->getIndexOfMessage(mNavigatePendingMsgId).isValid())
    {
#ifdef DEBUG_FORUMS
        std::cerr << "Pending msg navigation: " << mNavigatePendingMsgId << ". Using it as new thread Id" << std::endl;
#endif

        QModelIndex source_index = mThreadModel->getIndexOfMessage(mNavigatePendingMsgId);
        QModelIndex index = mThreadProxyModel->mapFromSource(source_index);

        ui->threadTreeWidget->selectionModel()->setCurrentIndex(index,QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        ui->threadTreeWidget->scrollTo(ui->threadTreeWidget->currentIndex());//May change if model reloaded

        mNavigatePendingMsgId.clear();
    }
    else
    {

        QModelIndex source_index = mThreadModel->getIndexOfMessage(mThreadId);

        if(!mThreadId.isNull() && source_index.isValid())
        {
            QModelIndex index = mThreadProxyModel->mapFromSource(source_index);
            ui->threadTreeWidget->selectionModel()->setCurrentIndex(index,QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
            ui->threadTreeWidget->scrollTo(ui->threadTreeWidget->currentIndex());//May change if model reloaded
#ifdef DEBUG_FORUMS
            std::cerr << "  re-selecting index of message " << mThreadId << " to " << source_index.row() << "," << source_index.column() << " " << (void*)source_index.internalPointer() << std::endl;
#endif
        }
        else
        {
#ifdef DEBUG_FORUMS
            std::cerr << "  previously message " << mThreadId << " not visible anymore -> de-selecting" << std::endl;
#endif
            ui->threadTreeWidget->selectionModel()->clear();
            ui->threadTreeWidget->selectionModel()->reset();
            mThreadId.clear();
            //blank();
        }
        // we also need to restore expanded threads
    }

    ui->newthreadButton->show();
    ui->forumName->setText(QString::fromUtf8(mForumGroup.mMeta.mGroupName.c_str()));
    ui->threadTreeWidget->sortByColumn(RsGxsForumModel::COLUMN_THREAD_DATE, Qt::DescendingOrder);
    ui->threadTreeWidget->update();
    ui->viewBox->setEnabled(true);
    ui->filterLineEdit->setEnabled(true);

    recursRestoreExpandedItems(mThreadProxyModel->mapFromSource(mThreadModel->root()),mSavedExpandedMessages);
    //mUpdating = false;

    ui->nextUnreadButton->setEnabled(true);
}

void GxsForumThreadWidget::updateGroupData()
{
    if(groupId().isNull())
        return;

    // ui->threadTreeWidget->selectionModel()->clear();
    // ui->threadTreeWidget->selectionModel()->reset();
    // mThreadProxyModel->clear();

    setForumDescriptionLoading();

    RsThread::async([this]()
    {
        // 1 - get message data from p3GxsForums

        std::list<RsGxsGroupId> forumIds;
        std::vector<RsGxsForumGroup> groups;

        forumIds.push_back(groupId());
        bool success = false;

        if(!rsGxsForums->getForumsInfo(forumIds,groups))
            std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve forum group info for forum " << groupId() << std::endl;
        else if(groups.size() != 1)
            std::cerr << __PRETTY_FUNCTION__ << " obtained more than one group info for forum " << groupId() << std::endl;
        else
            success = true;

        if(success)
        {
            // 2 - sort the messages into a proper hierarchy

            RsGxsForumGroup group(groups[0]);	// we use a copy to share the object in order to avoid group deletion while we're in the thread.

            // 3 - update the model in the UI thread.

            RsQThreadUtils::postToObject( [group,this]()
            {
                /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete */

                mForumGroup = group;
                mThreadId.clear();

                ui->threadTreeWidget->setColumnHidden(RsGxsForumModel::COLUMN_THREAD_DISTRIBUTION, !IS_GROUP_PGP_KNOWN_AUTHED(mForumGroup.mMeta.mSignFlags) && !(IS_GROUP_PGP_AUTHED(mForumGroup.mMeta.mSignFlags)));
                ui->subscribeToolButton->setHidden(IS_GROUP_SUBSCRIBED(mForumGroup.mMeta.mSubscribeFlags)) ;

                updateForumDescription(true);

                emit groupChanged(this);		// signals the parent widget to e.g. update the group tab name

            }, this );
        }
        else
            RsQThreadUtils::postToObject( [this]() { updateForumDescription(false); },this);
    });
}

void GxsForumThreadWidget::updateMessageData(const RsGxsMessageId& msgId)
{
    RsThread::async([msgId,this]()
    {
        // 1 - get message data from p3GxsForums

#ifdef DEBUG_FORUMS
        std::cerr << "Retrieving post data for post " << msgId << std::endl;
#endif

        std::set<RsGxsMessageId> msgs_to_request ;
        std::vector<RsGxsForumMsg> msgs;

        msgs_to_request.insert(msgId);
        QString error_string;

        if(!rsGxsForums->getForumContent(groupId(),msgs_to_request,msgs))
        {
            std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve message info for forum " << groupId() << " and MsgId " << msgId << std::endl;
            error_string = tr("Failed to retrieve this message. Is the database currently overloaded?");
        }

        if(msgs.empty())
        {
            std::cerr << __PRETTY_FUNCTION__ << " no posts for msgId " << msgId << ". Database corruption?" << std::endl;
            error_string = tr("No data for this message. Is the database corrupted?");
        }
        if(msgs.size() > 1)
        {
            std::cerr << __PRETTY_FUNCTION__ << " obtained more than one msg info for msgId " << msgId << ". This could be a bug. Only showing the first msg in the list." << std::endl;
            std::cerr << "Messages are:" << std::endl;
            for(auto it(msgs.begin());it!=msgs.end();++it)
                std::cerr << (*it).mMeta << std::endl;

            error_string = tr("More than one entry for this message. Is the database corrupted?");
        }

        if(error_string.isNull())
        {
            // 2 - sort the messages into a proper hierarchy

            RsGxsForumMsg msg(msgs[0]);

            // 3 - update the model in the UI thread.

            RsQThreadUtils::postToObject( [msg,this]()
            {
                /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete */

                insertMessageData(msg);

                ui->threadTreeWidget->setColumnHidden(RsGxsForumModel::COLUMN_THREAD_DISTRIBUTION, !IS_GROUP_PGP_KNOWN_AUTHED(mForumGroup.mMeta.mSignFlags) && !(IS_GROUP_PGP_AUTHED(mForumGroup.mMeta.mSignFlags)));
                ui->subscribeToolButton->setHidden(IS_GROUP_SUBSCRIBED(mForumGroup.mMeta.mSubscribeFlags)) ;
            }, this );
        }
        else
            RsQThreadUtils::postToObject( [error_string,this](){ setMessageLoadingError(error_string); } );
    });
}

void GxsForumThreadWidget::showAuthorInPeople(const RsGxsForumMsg& msg)
{
    if(msg.mMeta.mAuthorId.isNull())
    {
        std::cerr << "(EE) GxsForumThreadWidget::loadMsgData_showAuthorInPeople() ERROR Missing Message Data...";
        std::cerr << std::endl;
    }

    /* window will destroy itself! */
    IdDialog *idDialog = dynamic_cast<IdDialog*>(MainWindow::getPage(MainWindow::People));

    if (!idDialog)
        return ;

    MainWindow::showWindow(MainWindow::People);
    idDialog->navigate(RsGxsId(msg.mMeta.mAuthorId));
}

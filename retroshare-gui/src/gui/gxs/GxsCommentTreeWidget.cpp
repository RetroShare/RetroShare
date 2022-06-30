/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsCommentTreeWidget.cpp                         *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie   <retroshare.project@gmail.com>       *
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

#include "GxsCommentTreeWidget.h"

#include "gui/common/FilesDefs.h"
#include "gui/common/RSElidedItemDelegate.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "gui/gxs/GxsCreateCommentDialog.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "util/qtthreadsutils.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QTextEdit>
#include <QHeaderView>
#include <QClipboard>
#include <QMessageBox>
#include <QDateTime>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QTextDocument>

#include <iostream>

#define PCITEM_COLUMN_COMMENT		0
#define PCITEM_COLUMN_AUTHOR		1
#define PCITEM_COLUMN_DATE		2
#define PCITEM_COLUMN_SCORE		3
#define PCITEM_COLUMN_UPVOTES		4
#define PCITEM_COLUMN_DOWNVOTES		5
#define PCITEM_COLUMN_OWNVOTE		6
#define PCITEM_COLUMN_MSGID		7
#define PCITEM_COLUMN_PARENTID		8
#define PCITEM_COLUMN_AUTHORID		9

#define ROLE_SORT           Qt::UserRole + 1

#define GXSCOMMENTS_LOADTHREAD		1

#define COMMENT_VOTE_ACK	0x001234

#define POST_CELL_SIZE_ROLE (Qt::UserRole+1)
#define POST_COLOR_ROLE     (Qt::UserRole+2)

/* Images for context menu icons */
#define IMAGE_MESSAGE       ":/icons/mail/compose.png"
#define IMAGE_REPLY         ":/icons/mail/reply.png"
#define IMAGE_COPY          ":/images/copy.png"
#define IMAGE_VOTEUP        ":/images/vote_up.png"
#define IMAGE_VOTEDOWN      ":/images/vote_down.png"

std::map<RsGxsMessageId, std::vector<RsGxsComment> > GxsCommentTreeWidget::mCommentsCache;
QMutex GxsCommentTreeWidget::mCacheMutex;

//#define USE_NEW_DELEGATE 1

// This class allows to draw the item using an appropriate size

class MultiLinesCommentDelegate: public QStyledItemDelegate
{
public:
    MultiLinesCommentDelegate(QFontMetricsF f) : qf(f){}

    QSize sizeHint(const QStyleOptionViewItem &/*option*/, const QModelIndex &index) const
    {
        return index.data(POST_CELL_SIZE_ROLE).toSize() ;
    }

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        Q_ASSERT(index.isValid());

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        // disable default icon
        opt.icon = QIcon();
        opt.text = QString();

        // draw default item background
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
        } else {
            const QWidget *widget = opt.widget;
            QStyle *style = widget ? widget->style() : QApplication::style();
            style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, widget);
        }

        const QRect r = option.rect.adjusted(0,0,-option.decorationSize.width(),0);

        QTextDocument td ;
        td.setHtml("<html>"+index.data(Qt::DisplayRole).toString()+"</html>");
        td.setTextWidth(r.width());
        QSizeF s = td.documentLayout()->documentSize();

        int m = QFontMetricsF(QFont()).height();
        QSize full_area(std::min(r.width(),(int)s.width())+m,std::min(r.height(),(int)s.height())+m);

        QPixmap px(full_area.width(),full_area.height());
        px.fill(QColor(0,0,0,0));//Transparent background as item background is already paint.
        QPainter p(&px) ;
        p.setRenderHint(QPainter::Antialiasing);


        QPainterPath path ;
        path.addRoundedRect(QRectF(m/4.0,m/4.0,s.width()+m/2.0,s.height()+m/2.0),m,m) ;
        QPen pen(Qt::gray,m/7.0f);
        p.setPen(pen);
        p.fillPath(path,QColor::fromHsv( index.data(POST_COLOR_ROLE).toInt()/255.0*360,40,220));	// varies the color according to the post author
        p.drawPath(path);

        QAbstractTextDocumentLayout::PaintContext ctx;
        ctx.clip = QRectF(0,0,s.width(),s.height());
        p.translate(QPointF(m/2.0,m/2.0));
        td.documentLayout()->draw( &p, ctx );


        painter->drawPixmap(r.topLeft(),px);

        const_cast<QAbstractItemModel*>(index.model())->setData(index,px.size(),POST_CELL_SIZE_ROLE);
    }

private:
	QFontMetricsF qf;
};

#ifdef USE_NEW_DELEGATE
class NoEditDelegate: public QStyledItemDelegate
{
public:
    NoEditDelegate(QObject* parent=0): QStyledItemDelegate(parent) {}
    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return nullptr;
    }
};

class GxsCommentDelegate: public QStyledItemDelegate
{
public:
    GxsCommentDelegate(QFontMetricsF f) : qf(f){}

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex &index) const override
    {
        return index.data(POST_CELL_SIZE_ROLE).toSize() ;
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex& index) const override
    {
        if(index.column() == PCITEM_COLUMN_COMMENT)
            editor->setGeometry(option.rect);
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if(index.column() == PCITEM_COLUMN_COMMENT)
        {
            QTextEdit *b = new QTextEdit(parent);

            b->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            b->setFixedSize(option.rect.size());
            b->setAcceptRichText(true);
            b->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::LinksAccessibleByMouse);
            b->document()->setHtml("<html>"+index.data(Qt::DisplayRole).toString()+"</html>");
            b->adjustSize();

            return b;
        }
        else
            return nullptr;
    }

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if((option.state & QStyle::State_Selected)) // Avoids double display. The selected widget is never exactly the size of the rendered one,
            return;                                 // so when selected, we only draw the selected one.

        Q_ASSERT(index.isValid());

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        // disable default icon
        opt.icon = QIcon();
        opt.text = QString();

        const QRect r = option.rect.adjusted(0,0,-option.decorationSize.width(),0);

        QTextDocument td ;
        td.setHtml("<html>"+index.data(Qt::DisplayRole).toString()+"</html>");
        td.setTextWidth(r.width());
        QSizeF s = td.documentLayout()->documentSize();

        int m = QFontMetricsF(QFont()).height() ;
        //int m = 2;

        QSize full_area(std::min(r.width(),(int)s.width())+m,std::min(r.height(),(int)s.height())+m);

        QPixmap px(full_area.width(),full_area.height());
        px.fill(QColor(0,0,0,0));//Transparent background as item background is already paint.
        QPainter p(&px) ;
        p.setRenderHint(QPainter::Antialiasing);

        QTextEdit b;
        b.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        b.setFixedSize(full_area);
        b.setAcceptRichText(true);
        b.setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::LinksAccessibleByMouse);
        b.document()->setHtml("<html>"+index.data(Qt::DisplayRole).toString()+"</html>");
        b.adjustSize();
        b.render(&p,QPoint(),QRegion(),QWidget::DrawChildren );// draw the widgets, not the background

        painter->drawPixmap(opt.rect.topLeft(),px);

        const_cast<QAbstractItemModel*>(index.model())->setData(index,px.size(),POST_CELL_SIZE_ROLE);
    }

private:
    QFontMetricsF qf;
};
#endif

void GxsCommentTreeWidget::mouseMoveEvent(QMouseEvent *e)
{
    QModelIndex idx = indexAt(e->pos());

    if(idx != selectionModel()->currentIndex())
        selectionModel()->setCurrentIndex(idx,QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    QTreeView::mouseMoveEvent(e);
}

GxsCommentTreeWidget::GxsCommentTreeWidget(QWidget *parent)
    :QTreeWidget(parent), mTokenQueue(NULL), mRsTokenService(NULL), mCommentService(NULL)
{
    setVerticalScrollMode(ScrollPerPixel);
	setContextMenuPolicy(Qt::CustomContextMenu);
	RSElidedItemDelegate *itemDelegate = new RSElidedItemDelegate(this);
	itemDelegate->setSpacing(QSize(0, 2));
	setItemDelegate(itemDelegate);
	setWordWrap(true);

    setSelectionBehavior(QAbstractItemView::SelectRows);

#ifdef USE_NEW_DELEGATE
    setMouseTracking(true); // for auto selection
    setItemDelegateForColumn(PCITEM_COLUMN_COMMENT,new GxsCommentDelegate(QFontMetricsF(font()))) ;

    // Apparently the following below is needed, since there is no way to set item flags for a single column
    // so after setting flags Qt will believe that all columns are editable.

    setItemDelegateForColumn(PCITEM_COLUMN_AUTHOR,   new NoEditDelegate(this));
    setItemDelegateForColumn(PCITEM_COLUMN_DATE,     new NoEditDelegate(this));
    setItemDelegateForColumn(PCITEM_COLUMN_SCORE,    new NoEditDelegate(this));
    setItemDelegateForColumn(PCITEM_COLUMN_UPVOTES,  new NoEditDelegate(this));
    setItemDelegateForColumn(PCITEM_COLUMN_DOWNVOTES,new NoEditDelegate(this));
    setItemDelegateForColumn(PCITEM_COLUMN_OWNVOTE,  new NoEditDelegate(this));

    QObject::connect(header(),SIGNAL(geometriesChanged()),this,SLOT(updateContent()));
    QObject::connect(header(),SIGNAL(sectionResized(int,int,int)),this,SLOT(updateContent()));

    setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked);
#else
    setItemDelegateForColumn(PCITEM_COLUMN_COMMENT,new MultiLinesCommentDelegate(QFontMetricsF(font()))) ;
#endif

	commentsRole = new RSTreeWidgetItemCompareRole;
    commentsRole->setRole(PCITEM_COLUMN_DATE, ROLE_SORT);

    mUseCache = false;

    //header()->setSectionResizeMode(PCITEM_COLUMN_COMMENT,QHeaderView::ResizeToContents);
    return;
}

void GxsCommentTreeWidget::updateContent()
{
    model()->dataChanged(QModelIndex(),QModelIndex());

    std::cerr << "Updating content" << std::endl;
}
GxsCommentTreeWidget::~GxsCommentTreeWidget()
{
	if (mTokenQueue) {
		delete(mTokenQueue);
	}
}

void GxsCommentTreeWidget::setCurrentCommentMsgId(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{

	Q_UNUSED(previous);

	if(current)
	{
		mCurrentCommentMsgId = RsGxsMessageId(current->text(PCITEM_COLUMN_MSGID).toStdString());
		mCurrentCommentText = current->text(PCITEM_COLUMN_COMMENT);
		mCurrentCommentAuthor = current->text(PCITEM_COLUMN_AUTHOR);
		mCurrentCommentAuthorId = RsGxsId(current->text(PCITEM_COLUMN_AUTHORID).toStdString());
	} else {
		mCurrentCommentMsgId.clear();
		mCurrentCommentText.clear();
		mCurrentCommentAuthor.clear();
		mCurrentCommentAuthorId.clear();
	}
}

void GxsCommentTreeWidget::customPopUpMenu(const QPoint& point)
{
    QTreeWidgetItem *item = itemAt(point);

	QMenu contextMnu( this );
	QAction* action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_REPLY), tr("Reply to Comment"), this, SLOT(replyToComment()));
	action->setDisabled(!item || mCurrentCommentMsgId.isNull());

	action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MESSAGE), tr("Submit Comment"), this, SLOT(makeComment()));
	action->setDisabled(mMsgVersions.empty());

	action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COPY), tr("Copy Comment"), this, SLOT(copyComment()));
    action->setData( item ? item->data(PCITEM_COLUMN_COMMENT,Qt::DisplayRole) : "" );
    action->setDisabled(!item || mCurrentCommentMsgId.isNull());

	contextMnu.addSeparator();

	action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_VOTEUP), tr("Vote Up"), this, SLOT(voteUp()));
	action->setDisabled(!item || mCurrentCommentMsgId.isNull() || mVoterId.isNull());

	action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_VOTEDOWN), tr("Vote Down"), this, SLOT(voteDown()));
	action->setDisabled(!item || mCurrentCommentMsgId.isNull() || mVoterId.isNull());


	if (!mCurrentCommentMsgId.isNull())
	{
        // not implemented yet
        /*
		contextMnu.addSeparator();
		QMenu *rep_menu = contextMnu.addMenu(tr("Reputation"));

        action = rep_menu->addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MESSAGE), tr("Show Reputation"), this, SLOT(showReputation()));
		contextMnu.addSeparator();

        action = rep_menu->addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MESSAGE), tr("Interesting User"), this, SLOT(markInteresting()));
		contextMnu.addSeparator();

        action = rep_menu->addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MESSAGE), tr("Mark Spammy"), this, SLOT(markSpammer()));
        action = rep_menu->addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MESSAGE), tr("Ban User"), this, SLOT(banUser()));
        */
	}

	contextMnu.exec(QCursor::pos());
}


void GxsCommentTreeWidget::voteUp()
{
#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
    std::cerr << "GxsCommentTreeWidget::voteUp()";
	std::cerr << std::endl;
#endif

	vote(mGroupId, mLatestMsgId, mCurrentCommentMsgId, mVoterId, true);
}


void GxsCommentTreeWidget::voteDown()
{
#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
    std::cerr << "GxsCommentTreeWidget::voteDown()";
	std::cerr << std::endl;
#endif

	vote(mGroupId, mLatestMsgId, mCurrentCommentMsgId, mVoterId, false);
}

void GxsCommentTreeWidget::setVoteId(const RsGxsId &voterId)
{
	mVoterId = voterId;
#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
    std::cerr << "GxsCommentTreeWidget::setVoterId(" << mVoterId << ")";
	std::cerr << std::endl;
#endif
}


void GxsCommentTreeWidget::vote(const RsGxsGroupId &groupId, const RsGxsMessageId &threadId, 
					const RsGxsMessageId &parentId, const RsGxsId &authorId, bool up)
{
    RsThread::async([this,groupId,threadId,parentId,authorId,up]()
    {
        std::string error_string;
        RsGxsMessageId vote_id;
        RsGxsVoteType tvote = up?(RsGxsVoteType::UP):(RsGxsVoteType::DOWN);

        bool res = mCommentService->voteForComment(groupId, threadId, parentId, authorId,tvote,vote_id, error_string);

        RsQThreadUtils::postToObject( [this,res,error_string]()
        {

            if(res)
                service_requestComments(mGroupId,mMsgVersions);
            else
                QMessageBox::critical(nullptr,tr("Cannot vote"),tr("Error while voting: ")+QString::fromStdString(error_string));
        });
    });

    //	uint32_t token;
    //  mCommentService->createNewVote(token, vote);
    //  mTokenQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, COMMENT_VOTE_ACK);
}


void GxsCommentTreeWidget::showReputation()
{
	std::cerr << "GxsCommentTreeWidget::showReputation() TODO";
	std::cerr << std::endl;
}

void GxsCommentTreeWidget::markInteresting()
{
	std::cerr << "GxsCommentTreeWidget::markInteresting() TODO";
	std::cerr << std::endl;
}

void GxsCommentTreeWidget::markSpammer()
{
	std::cerr << "GxsCommentTreeWidget::markSpammer() TODO";
	std::cerr << std::endl;
}

void GxsCommentTreeWidget::banUser()
{
	std::cerr << "GxsCommentTreeWidget::banUser() TODO";
	std::cerr << std::endl;
}

void GxsCommentTreeWidget::makeComment()
{
    GxsCreateCommentDialog pcc(mCommentService, std::make_pair(mGroupId,mLatestMsgId), mLatestMsgId, mVoterId,this);
	pcc.exec();
}

void GxsCommentTreeWidget::replyToComment()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mGroupId;
	msgId.second = mCurrentCommentMsgId;
    GxsCreateCommentDialog pcc(mCommentService, msgId, mLatestMsgId, mVoterId,this);

	pcc.loadComment(mCurrentCommentText, mCurrentCommentAuthor, mCurrentCommentAuthorId);
	pcc.exec();
}

void GxsCommentTreeWidget::copyComment()
{
    QString txt = dynamic_cast<QAction*>(sender())->data().toString();

	QMimeData *mimeData = new QMimeData();
    mimeData->setHtml("<html>"+txt+"</html>");
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setMimeData(mimeData, QClipboard::Clipboard);
}

void GxsCommentTreeWidget::setup(RsTokenService *token_service, RsGxsCommentService *comment_service)
{
	mRsTokenService = token_service;
	mCommentService = comment_service;
	mTokenQueue = new TokenQueue(token_service, this);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customPopUpMenu(QPoint)));
	connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(setCurrentCommentMsgId(QTreeWidgetItem*, QTreeWidgetItem*)));

	return;
}


/* Load Comments */
void GxsCommentTreeWidget::requestComments(const RsGxsGroupId& group, const std::set<RsGxsMessageId>& message_versions,const RsGxsMessageId& most_recent_message)
{
	/* request comments */

    mGroupId = group ;
	mMsgVersions = message_versions ;
    mLatestMsgId = most_recent_message;

    if(mUseCache)
    {
        QMutexLocker lock(&mCacheMutex);

        auto it = mCommentsCache.find(most_recent_message);

        if(it != mCommentsCache.end())
        {
            std::cerr << "Got " << it->second.size() << " comments from cache." << std::endl;
            insertComments(it->second);
            completeItems();
        }
    }
	service_requestComments(group,message_versions);
}


void GxsCommentTreeWidget::service_requestComments(const RsGxsGroupId& group_id,const std::set<RsGxsMessageId> & msgIds)
{
	/* request comments */
#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
    std::cerr << "GxsCommentTreeWidget::service_requestComments for group " << group_id << std::endl;
#endif

   RsThread::async([this,group_id,msgIds]()
    {
        std::vector<RsGxsComment> comments;

        if(!mCommentService->getRelatedComments(group_id,msgIds,comments))
        {
            std::cerr << __PRETTY_FUNCTION__ << " failed to get comments" << std::endl;
            return;
        }

        RsQThreadUtils::postToObject( [this,comments]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete, note that
             * Qt::QueuedConnection is important!
             */

            clearItems();

            service_loadThread(comments);

            completeItems();

            emit commentsLoaded(treeCount(this));

        }, this );
    });
}

#ifdef TODO
void GxsCommentTreeWidget::async_msg_action(const CmtMethod &action)
{
    RsThread::async([this,action]()
    {
        // 1 - get message data from p3GxsForums

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
#endif


/* Generic Handling */
void GxsCommentTreeWidget::clearItems()
{
	mPendingInsertMap.clear();
	mLoadingMap.clear();
}


void GxsCommentTreeWidget::completeItems()
{
	/* handle pending items */
	RsGxsMessageId parentId;
	QTreeWidgetItem *parent = NULL;
	QList<QTreeWidgetItem *> topLevelItems;

	std::map<RsGxsMessageId, QTreeWidgetItem *>::iterator lit;
	std::multimap<RsGxsMessageId, QTreeWidgetItem *>::iterator pit;

#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
    std::cerr << "GxsCommentTreeWidget::completeItems() " << mPendingInsertMap.size();
	std::cerr << " PendingItems";
	std::cerr << std::endl;
#endif

	for(pit = mPendingInsertMap.begin(); pit != mPendingInsertMap.end(); ++pit)
	{
#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
        std::cerr << "GxsCommentTreeWidget::completeItems() item->parent: " << pit->first;
		std::cerr << std::endl;
#endif

		if (pit->first != parentId)
		{
			/* find parent */
			parentId = pit->first;
			lit = mLoadingMap.find(pit->first);
			if (lit != mLoadingMap.end())
			{
					parent = lit->second;
			}
			else
			{
					parent = NULL;
			}
		}

		if (parent)
		{
#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
            std::cerr << "GxsCommentTreeWidget::completeItems() Added to Parent";
			std::cerr << std::endl;
#endif

			parent->addChild(pit->second);
		}
		else if (mMsgVersions.find(parentId) != mMsgVersions.end())
		{
#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
            std::cerr << "GxsCommentTreeWidget::completeItems() Added to topLevelItems";
			std::cerr << std::endl;
#endif

			topLevelItems.append(pit->second);
		}
		else
		{

			/* missing parent -> insert At Top Level */
			QTreeWidgetItem *missingItem = service_createMissingItem(pit->first);

#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
            std::cerr << "GxsCommentTreeWidget::completeItems() Added MissingItem";
			std::cerr << std::endl;
#endif

			parent = missingItem;
			parent->addChild(pit->second);
			topLevelItems.append(parent);
		}
	}

	/* now push final tree into Tree */
	clear();
	insertTopLevelItems(0, topLevelItems);

	/* cleanup temp stuff */
	mLoadingMap.clear();
	mPendingInsertMap.clear();
}


void GxsCommentTreeWidget::addItem(RsGxsMessageId itemId, RsGxsMessageId parentId, QTreeWidgetItem *item)
{
#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
    std::cerr << "GxsCommentTreeWidget::addItem() Id: " << itemId;
	std::cerr << " ParentId: " << parentId;
	std::cerr << std::endl;
#endif

	/* store in map -> for children */
	mLoadingMap[itemId] = item;

	std::map<RsGxsMessageId, QTreeWidgetItem *>::iterator it;
	it = mLoadingMap.find(parentId);
	if (it != mLoadingMap.end())
	{
#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
        std::cerr << "GxsCommentTreeWidget::addItem() Added to Parent";
		std::cerr << std::endl;
#endif

		it->second->addChild(item);
	}
	else
	{
#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
        std::cerr << "GxsCommentTreeWidget::addItem() Added to Pending List";
		std::cerr << std::endl;
#endif

		mPendingInsertMap.insert(std::make_pair(parentId, item));
	}
}

int GxsCommentTreeWidget::treeCount(QTreeWidget *tree, QTreeWidgetItem *parent)
{
    int count = 0;
    if (parent == 0)
    {
        int topCount = tree->topLevelItemCount();
        for (int i = 0; i < topCount; i++)
            count += treeCount(tree, tree->topLevelItem(i));

        count += topCount;
    }
    else
    {
        int childCount = parent->childCount();
        for (int i = 0; i < childCount; i++)
            count += treeCount(tree, parent->child(i));

        count += childCount;
    }
    return count;
}
// void GxsCommentTreeWidget::loadThread(const uint32_t &token)
// {
// 	clearItems();
//
// 	service_loadThread(token);
//
// 	completeItems();
//
//     emit commentsLoaded(treeCount(this));
// }

void GxsCommentTreeWidget::acknowledgeComment(const uint32_t &token)
{
	RsGxsGrpMsgIdPair msgId;
	mCommentService->acknowledgeComment(token, msgId);

	// simply reload data
	service_requestComments(mGroupId,mMsgVersions);
}


void GxsCommentTreeWidget::acknowledgeVote(const uint32_t &token)
{
	RsGxsGrpMsgIdPair msgId;
	if (mCommentService->acknowledgeVote(token, msgId))
	{
		// reload data if vote was added.
		service_requestComments(mGroupId,mMsgVersions);
	}
}

void GxsCommentTreeWidget::service_loadThread(const std::vector<RsGxsComment>& comments)
{
    std::cerr << "GxsCommentTreeWidget::service_loadThread() ERROR must be overloaded!";
    std::cerr << std::endl;

    // This is inconsistent since we cannot know here that all comments are for the same thread. However they are only
    // requested in requestComments() where a single MsgId is used.

    if(mUseCache)
    {
        QMutexLocker lock(&mCacheMutex);

        if(!comments.empty())
        {
            std::cerr << "Updating cache with " << comments.size() << " for thread " << comments[0].mMeta.mThreadId << std::endl;
            mCommentsCache[comments[0].mMeta.mThreadId] = comments;
        }
    }

    insertComments(comments);
}


// void GxsCommentTreeWidget::service_loadThread(const uint32_t &token)
// {
// 	std::cerr << "GxsCommentTreeWidget::service_loadThread() ERROR must be overloaded!";
// 	std::cerr << std::endl;
//
// 	std::vector<RsGxsComment> comments;
// 	mCommentService->getRelatedComments(token, comments);
//
//     // This is inconsistent since we cannot know here that all comments are for the same thread. However they are only
//     // requested in requestComments() where a single MsgId is used.
//
//     if(mUseCache)
//     {
//         QMutexLocker lock(&mCacheMutex);
//
//         if(!comments.empty())
//         {
//             std::cerr << "Updating cache with " << comments.size() << " for thread " << comments[0].mMeta.mThreadId << std::endl;
//             mCommentsCache[comments[0].mMeta.mThreadId] = comments;
//         }
//     }
//
//     insertComments(comments);
// }

void GxsCommentTreeWidget::insertComments(const std::vector<RsGxsComment>& comments)
{
    std::list<RsGxsMessageId> new_comments;

    for(auto vit = comments.begin(); vit != comments.end(); ++vit)
	{
        const RsGxsComment &comment = *vit;

        if(IS_MSG_NEW(comment.mMeta.mMsgStatus))
            new_comments.push_back(comment.mMeta.mMsgId);

		/* convert to a QTreeWidgetItem */
		std::cerr << "GxsCommentTreeWidget::service_loadThread() Got Comment: " << comment.mMeta.mMsgId;
		std::cerr << std::endl;

		GxsIdRSTreeWidgetItem *item = new GxsIdRSTreeWidgetItem(NULL,GxsIdDetails::ICON_TYPE_AVATAR) ;
		QString text;

		{
			QDateTime qtime ;
			qtime.setTime_t(comment.mMeta.mPublishTs) ;

			text = qtime.toString("yyyy-MM-dd hh:mm:ss") ;
			item->setText(PCITEM_COLUMN_DATE, text) ;
			item->setToolTip(PCITEM_COLUMN_DATE, text) ;
			item->setData(PCITEM_COLUMN_DATE, ROLE_SORT, QVariant(qlonglong(comment.mMeta.mPublishTs)));

		}

		text = QString::fromUtf8(comment.mComment.c_str());
        item->setText(PCITEM_COLUMN_COMMENT, text);
        item->setToolTip(PCITEM_COLUMN_COMMENT, text);

        RsGxsId authorId = comment.mMeta.mAuthorId;
		item->setId(authorId, PCITEM_COLUMN_AUTHOR, false);
        item->setData(PCITEM_COLUMN_COMMENT,POST_COLOR_ROLE,QVariant(authorId.toByteArray()[1]));

        text = QString::number(comment.mScore);
		item->setText(PCITEM_COLUMN_SCORE, text);

		text = QString::number(comment.mUpVotes);
		item->setText(PCITEM_COLUMN_UPVOTES, text);

		text = QString::number(comment.mDownVotes);
		item->setText(PCITEM_COLUMN_DOWNVOTES, text);

		text = QString::number(comment.mOwnVote);
		item->setText(PCITEM_COLUMN_OWNVOTE, text);

		text = QString::fromUtf8(comment.mMeta.mMsgId.toStdString().c_str());
		item->setText(PCITEM_COLUMN_MSGID, text);

		text = QString::fromUtf8(comment.mMeta.mParentId.toStdString().c_str());
		item->setText(PCITEM_COLUMN_PARENTID, text);
		
		text = QString::fromUtf8(comment.mMeta.mAuthorId.toStdString().c_str());
		item->setText(PCITEM_COLUMN_AUTHORID, text);

#ifdef USE_NEW_DELEGATE
        // Allows to call createEditor() in the delegate. Without this, createEditor() is never called in
        // the styled item delegate.
        item->setFlags(Qt::ItemIsEditable | item->flags());
#endif

		addItem(comment.mMeta.mMsgId, comment.mMeta.mParentId, item);
	}

    // now set all loaded comments as not new, since they have been loaded.

    for(auto cid:new_comments)
    {
        uint32_t token=0;
        mCommentService->setCommentAsRead(token,mGroupId,cid);
    }
}

QTreeWidgetItem *GxsCommentTreeWidget::service_createMissingItem(const RsGxsMessageId& parent)
{
	std::cerr << "GxsCommentTreeWidget::service_createMissingItem()";
	std::cerr << std::endl;
		
	QTreeWidgetItem *item = new QTreeWidgetItem();
	QString text("Unknown");

	item->setText(PCITEM_COLUMN_DATE, text);
		
	item->setText(PCITEM_COLUMN_COMMENT, text);
		
	item->setText(PCITEM_COLUMN_AUTHOR, text);

	item->setText(PCITEM_COLUMN_MSGID, text);
	
	item->setText(PCITEM_COLUMN_AUTHORID, text);


		text = QString::fromUtf8(parent.toStdString().c_str());
	item->setText(PCITEM_COLUMN_PARENTID, text);

	return item;
}	



void GxsCommentTreeWidget::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
#ifdef DEBUG_GXSCOMMENT_TREEWIDGET
	std::cerr << "GxsCommentTreeWidget::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif
		
	if (queue != mTokenQueue)
	{
		std::cerr << "GxsCommentTreeWidget::loadRequest() Queue ERROR";
		std::cerr << std::endl;
		return;
	}
		
	/* now switch on req */
		switch(req.mType)
	{
		
			case TOKENREQ_MSGINFO:
			{
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_ACK:
						if (req.mUserType == COMMENT_VOTE_ACK)
						{
							acknowledgeVote(req.mToken);
						}
						else
						{
							acknowledgeComment(req.mToken);
						}
						break;
//					case RS_TOKREQ_ANSTYPE_DATA:
//						loadThread(req.mToken);
//						break;
				}
			}
			break;
			default:
					std::cerr << "GxsCommentTreeWidget::loadRequest() UNKNOWN UserType ";
					std::cerr << std::endl;
					break;

	}
}

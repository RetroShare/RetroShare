/****************************************************************
*  RetroShare is distributed under the following license:
*
*  Copyright (C) 2008 Robert Fernie
*
*  This program is free software; you can redistribute it and/or
*  modify it under the terms of the GNU General Public License
*  as published by the Free Software Foundation; either version 2
*  of the License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
*  Boston, MA  02110-1301, USA.
****************************************************************/

#ifndef _GXS_COMMENT_TREE_WIDGET_H
#define _GXS_COMMENT_TREE_WIDGET_H

#include <QTreeWidget>

#include "util/TokenQueue.h"

/* indicies for search results item columns SR_ = Search Result */
#define SR_NAME_COL         0
#define SR_SIZE_COL         1
#define SR_ID_COL           2
#define SR_TYPE_COL         3
#define SR_AGE_COL          4
#define SR_HASH_COL         5
#define SR_SEARCH_ID_COL    6
#define SR_UID_COL          7
#define SR_DATA_COL         SR_NAME_COL

#define SR_ROLE_LOCAL       Qt::UserRole

class GxsCommentTreeWidget : public QTreeWidget, public TokenResponse
{
    Q_OBJECT
        
public:
    GxsCommentTreeWidget(QWidget *parent = 0);
    void setup(RsTokenService *service);

    void requestComments(const RsGxsGrpMsgIdPair& threadId);
    void getCurrentMsgId(RsGxsMessageId& parentId);
    void setCurrentMsgId(QTreeWidgetItem* current, QTreeWidgetItem* previous);

    void loadRequest(const TokenQueue *queue, const TokenRequest &req);

protected:

    /* to be overloaded */
    virtual void service_requestComments(const RsGxsGrpMsgIdPair& threadId);
    virtual void service_loadThread(const uint32_t &token);
    virtual QTreeWidgetItem *service_createMissingItem(const RsGxsMessageId& parent);

    void clearItems();
    void completeItems();

    void acknowledgeComment(const uint32_t& token);
    void loadThread(const uint32_t &token);

    void addItem(std::string itemId, std::string parentId, QTreeWidgetItem *item);

public slots:
    void customPopUpMenu(const QPoint& point);
private slots:


    void makeComment();

protected:

    /* Data */
    RsGxsGrpMsgIdPair mThreadId;
    RsGxsMessageId mCurrentMsgId;

    std::map<std::string, QTreeWidgetItem *> mLoadingMap;
    std::multimap<std::string, QTreeWidgetItem *> mPendingInsertMap;

    TokenQueue *mTokenQueue;
    RsTokenService *mRsService;

};


#endif


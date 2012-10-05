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

#include "util/TokenQueueVEG.h"

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

class GxsCommentTreeWidget : public QTreeWidget, public TokenResponseVEG
{
    Q_OBJECT
        
public:
	GxsCommentTreeWidget(QWidget *parent = 0);
        void setup(RsTokenServiceVEG *service);

	void requestComments(std::string threadId);

        void loadRequest(const TokenQueueVEG *queue, const TokenRequestVEG &req);

protected:

	/* to be overloaded */
	virtual void service_requestComments(std::string threadId);
	virtual void service_loadThread(const uint32_t &token);
	virtual QTreeWidgetItem *service_createMissingItem(std::string parent);

	void clearItems();
	void completeItems();

	void loadThread(const uint32_t &token);
	
	void addItem(std::string itemId, std::string parentId, QTreeWidgetItem *item);



	/* Data */
	std::string mThreadId;

	std::map<std::string, QTreeWidgetItem *> mLoadingMap;
	std::multimap<std::string, QTreeWidgetItem *> mPendingInsertMap;

        TokenQueueVEG *mTokenQueue;
        RsTokenServiceVEG *mRsService;

	protected:
//virtual QMimeData * mimeData ( const QList<QTreeWidgetItem *> items ) const;
//virtual QStringList mimeTypes () const; 
//virtual Qt::DropActions supportedDropActions () const;

};


#endif


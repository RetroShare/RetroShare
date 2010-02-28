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

#ifndef _BLOGSDIALOG_H
#define _BLOGSDIALOG_H

#include "gui/mainpage.h"
#include "ui_BlogsDialog.h"

#include <QStandardItemModel>

#include "gui/feeds/FeedHolder.h"

#define OWN	0
#define SUBSCRIBED	1
#define POPULAR	2
#define OTHER	3

class BlogsMsgItem;


class BlogsDialog : public MainPage, public FeedHolder, private Ui::BlogsDialog
{
  Q_OBJECT

public:
  /** Default Constructor */
  BlogsDialog(QWidget *parent = 0);
  /** Default Destructor */


virtual void deleteFeedItem(QWidget *item, uint32_t type);
virtual void openChat(std::string peerId);
virtual void openMsg(uint32_t type, std::string grpId, std::string inReplyTo);

public slots:

	void selectBlog( std::string );
	void selectBlog(const QModelIndex &);
	void toggleSelection(const QModelIndex &);

private slots:

  void blogListCustomPopupMenu( QPoint point );

	void checkUpdate();

	void createBlog();
	//void sendMsg();

	void blogSelection();

	void subscribeBlog();
	void unsubscribeBlog();
	
	void createMsg();

  void showBlogDetails();

private:

	void updateBlogList();
	void updateBlogListOwn(std::list<std::string> &ids);
	void updateBlogListSub(std::list<std::string> &ids);
	void updateBlogListPop(std::list<std::string> &ids);
	void updateBlogListOther(std::list<std::string> &ids);

	void updateBlogMsgs();

	QStandardItemModel *model;

	std::string mBlogId; /* current Blog */
	std::string mPeerId;

	/* Layout Pointers */
	QBoxLayout *mMsgLayout;



	std::list<BlogsMsgItem *> mBlogMsgItems;

	QFont mBlogFont;
	QFont itemFont;

};



#endif


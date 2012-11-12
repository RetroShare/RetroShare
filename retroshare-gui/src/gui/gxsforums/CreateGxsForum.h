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


#ifndef _CREATE_GXSFORUM_DIALOG_H
#define _CREATE_GXSFORUM_DIALOG_H

#include "ui_CreateGxsForum.h"

#include "util/TokenQueue.h"

class CreateGxsForum : public QDialog, public TokenResponse
{
	Q_OBJECT

public:
	CreateGxsForum(QWidget *parent = 0);

	void newForum(); /* cleanup */

        // Callback for all Loads.
virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:
	/* actions to take.... */
	void createForum();
	void cancelForum();

	// set private forum key share list
	void setShareList();

private:
	void sendShareList(std::string forumId);
	void completeCreateNewForum(const RsGroupMetaData &newForumMeta);
	void loadNewForumId(const uint32_t &token);


	std::list<std::string> mShareList;

	QPixmap picture;

	TokenQueue *mForumQueue;

	/** Qt Designer generated object */
	Ui::CreateGxsForum ui;
};

#endif


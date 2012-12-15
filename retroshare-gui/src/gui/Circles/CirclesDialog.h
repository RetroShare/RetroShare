/*
 * Retroshare Circle.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifndef MRK_CIRCLE_DIALOG_H
#define MRK_CIRCLE_DIALOG_H

#include "retroshare-gui/mainpage.h"
#include "ui_CirclesDialog.h"

//#include <retroshare/rscircles.h>

#include "util/TokenQueue.h"

#include <map>

class CirclesDialog : public MainPage, public TokenResponse
{
  Q_OBJECT

public:
	CirclesDialog(QWidget *parent = 0);

void 	loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:

#if 0
	void checkUpdate();
	void OpenOrShowAddPageDialog();
	void OpenOrShowAddGroupDialog();
	void OpenOrShowEditDialog();
	void OpenOrShowRepublishDialog();

	void groupTreeChanged();

	void newGroup();
	void showGroupDetails();
	void editGroupDetails();

	void insertWikiGroups();
#endif

private:

#if 0
void 	clearWikiPage();
void    clearGroupTree();

void 	updateWikiPage(const RsWikiSnapshot &page);

bool 	getSelectedPage(std::string &groupId, std::string &pageId, std::string &origPageId);	
std::string getSelectedPage();
std::string getSelectedGroup();

void 	requestGroupList();
void 	loadGroupData(const uint32_t &token);

void 	requestPages(const std::list<RsGxsGroupId> &groupIds);
void 	loadPages(const uint32_t &token);

void 	requestWikiPage(const  RsGxsGrpMsgIdPair &msgId);
void 	loadWikiPage(const uint32_t &token);
#endif

	TokenQueue *mCircleQueue;

	/* UI - from Designer */
	Ui::CirclesDialog ui;

};

#endif


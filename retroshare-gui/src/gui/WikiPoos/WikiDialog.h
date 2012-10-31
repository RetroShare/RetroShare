/*
 * Retroshare Wiki Plugin.
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

#ifndef MRK_WIKI_DIALOG_H
#define MRK_WIKI_DIALOG_H

#include "retroshare-gui/mainpage.h"
#include "ui_WikiDialog.h"

#include <retroshare/rswiki.h>

#include "util/TokenQueue.h"

#include <map>

class WikiAddDialog;
class WikiEditDialog;

class WikiDialog : public MainPage, public TokenResponse
{
  Q_OBJECT

public:
	WikiDialog(QWidget *parent = 0);

void 	loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:

	void checkUpdate();
	void OpenOrShowAddPageDialog();
	void OpenOrShowAddGroupDialog();
	void OpenOrShowEditDialog();

	void groupTreeChanged();
	void modTreeChanged();

	void newGroup();
	void showGroupDetails();
	void editGroupDetails();

private:

void 	clearWikiPage();
void    clearGroupTree();
void    clearModsTree();

void 	insertWikiGroups();
void 	insertModsForPage(const std::string &origPageId);

void 	updateWikiPage(const RsWikiSnapshot &page);

bool 	getSelectedPage(std::string &pageId, std::string &origPageId);	
std::string getSelectedPage();
std::string getSelectedGroup();
std::string getSelectedMod();



void 	requestGroupList();
void 	loadGroupList(const uint32_t &token);
void 	requestGroupData(const std::list<std::string> &groupIds);
void 	loadGroupData(const uint32_t &token);
void 	requestOriginalPages(const std::list<std::string> &groupIds);
void 	loadOriginalPages(const uint32_t &token);
void 	requestLatestPages(const std::list<std::string> &msgIds);
void 	loadLatestPages(const uint32_t &token);
void 	requestPages(const std::list<std::string> &msgIds);
void 	loadPages(const uint32_t &token);


void 	requestModPageList(const std::string &origMsgId);
void 	loadModPageList(const uint32_t &token);
void 	requestModPages(const std::list<std::string> &msgIds);
void 	loadModPages(const uint32_t &token);

void 	requestWikiPage(const std::string &msgId);
void 	loadWikiPage(const uint32_t &token);


	TokenQueue *mWikiQueue;


	WikiAddDialog *mAddPageDialog;
	WikiAddDialog *mAddGroupDialog;
	WikiEditDialog *mEditDialog;

	std::string mGroupSelected;
	std::string mPageSelected;
	std::string mModSelected;

	/* UI - from Designer */
	Ui::WikiDialog ui;

};

#endif


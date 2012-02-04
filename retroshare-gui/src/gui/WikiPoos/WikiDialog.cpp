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

#include <QFile>
#include <QFileInfo>

#include "WikiDialog.h"
#include "gui/WikiPoos/WikiAddDialog.h"
#include "gui/WikiPoos/WikiEditDialog.h"

#include <retroshare/rswiki.h>

#include <iostream>
#include <sstream>

#include <QTimer>

/******
 * #define WIKI_DEBUG 1
 *****/

#define WIKI_DEBUG 1


/** Constructor */
WikiDialog::WikiDialog(QWidget *parent)
: MainPage(parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	mAddPageDialog = NULL;
	mAddGroupDialog = NULL;
	mEditDialog = NULL;

	connect( ui.toolButton_NewGroup, SIGNAL(clicked()), this, SLOT(OpenOrShowAddGroupDialog()));
	connect( ui.toolButton_NewPage, SIGNAL(clicked()), this, SLOT(OpenOrShowAddPageDialog()));
	connect( ui.toolButton_Edit, SIGNAL(clicked()), this, SLOT(OpenOrShowEditDialog()));

	connect( ui.treeWidget_Pages, SIGNAL(itemSelectionChanged()), this, SLOT(groupTreeChanged()));
	connect( ui.treeWidget_Mods, SIGNAL(itemSelectionChanged()), this, SLOT(modTreeChanged()));

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);


}

void WikiDialog::checkUpdate()
{
	/* update */
	if (!rsWiki)
		return;

	if (rsWiki->updated())
	{
		insertWikiGroups();
	}

	return;
}


void WikiDialog::OpenOrShowAddPageDialog()
{
	std::string groupId = getSelectedGroup();
	if (groupId == "")
	{
		std::cerr << "WikiDialog::OpenOrShowAddPageDialog() No Group selected";
		std::cerr << std::endl;
		return;
	}

	if (!mEditDialog)
	{
		mEditDialog = new WikiEditDialog(NULL);
	}

	std::cerr << "WikiDialog::OpenOrShowAddPageDialog() GroupId: " << groupId;
	std::cerr << std::endl;

	RsWikiGroup group;
	if (!rsWiki->getGroup(groupId, group))
	{
		std::cerr << "WikiDialog::OpenOrShowAddPageDialog() Failed to Get Group";
		std::cerr << std::endl;
	}


	mEditDialog->setGroup(group);
	mEditDialog->setNewPage();

	mEditDialog->show();
}


void WikiDialog::OpenOrShowAddGroupDialog()
{
	if (mAddGroupDialog)
	{
		mAddGroupDialog->show();
	}
	else
	{
		mAddGroupDialog = new WikiAddDialog(NULL);
		mAddGroupDialog->show();
	}
}


void WikiDialog::OpenOrShowEditDialog()
{
	std::string groupId = getSelectedGroup();
	if (groupId == "")
	{
		std::cerr << "WikiDialog::OpenOrShowAddPageDialog() No Group selected";
		std::cerr << std::endl;
		return;
	}

	std::string pageId = getSelectedPage();
	std::string modId = getSelectedMod();
	std::string realPageId;
	if (pageId == "")
	{
		std::cerr << "WikiDialog::OpenOrShowAddPageDialog() No PageId selected";
		std::cerr << std::endl;
		return;
	}

	if (modId == "")
	{
		realPageId = pageId;
	}
	else
	{
		realPageId = modId;
	}


	if (!mEditDialog)
	{
		mEditDialog = new WikiEditDialog(NULL);
	}

	RsWikiGroup group;
	rsWiki->getGroup(groupId, group);
	mEditDialog->setGroup(group);

	RsWikiPage page;
	rsWiki->getPage(realPageId, page);
	mEditDialog->setPreviousPage(page);

	mEditDialog->show();
}



void WikiDialog::groupTreeChanged()
{
	/* */
	std::string pageId = getSelectedPage();
	if (pageId == mPageSelected)
	{
		return; /* nothing changed */
	}

	if (pageId == "")
	{
		/* clear Mods */
		clearGroupTree();
		clearModsTree();

		return;
	}

	clearModsTree();
	RsWikiPage page;

	if (rsWiki->getPage(pageId, page))
	{
		insertModsForPage(page.mOrigPageId);
		updateWikiPage(page);
	}
}

void WikiDialog::modTreeChanged()
{
	/* */
	std::string pageId = getSelectedMod();
	if (pageId == mModSelected)
	{
		return; /* nothing changed */
	}

	if (pageId == "")
	{
		clearWikiPage();
		return;
	}

	RsWikiPage page;
	if (rsWiki->getPage(pageId, page))
	{
		updateWikiPage(page);
	}
}


void WikiDialog::updateWikiPage(const RsWikiPage &page)
{
	ui.textBrowser->setPlainText(QString::fromStdString(page.mPage));
}


void WikiDialog::clearWikiPage()
{
	ui.textBrowser->setPlainText("");
}


void 	WikiDialog::clearGroupTree()
{
	ui.treeWidget_Pages->clear();
}

void 	WikiDialog::clearModsTree()
{
	ui.treeWidget_Mods->clear();
}



#define WIKI_GROUP_COL_GROUPNAME	0
#define WIKI_GROUP_COL_GROUPID		1

#define WIKI_GROUP_COL_PAGENAME		0
#define WIKI_GROUP_COL_PAGEID		1
#define WIKI_GROUP_COL_ORIGPAGEID	2


void WikiDialog::insertWikiGroups()
{

	std::cerr << "WikiDialog::insertWikiGroups()";
	std::cerr << std::endl;

	/* clear it all */
	clearGroupTree();

	std::list<std::string> groupIds;
	std::list<std::string>::iterator it;

	rsWiki->getGroupList(groupIds);

	for(it = groupIds.begin(); it != groupIds.end(); it++)
	{
		/* add a group Item */
		RsWikiGroup group;
		rsWiki->getGroup(*it, group);

		QTreeWidgetItem *groupItem = new QTreeWidgetItem(NULL);
		groupItem->setText(WIKI_GROUP_COL_GROUPNAME, QString::fromStdString(group.mName));
		groupItem->setText(WIKI_GROUP_COL_GROUPID, QString::fromStdString(group.mGroupId));
		ui.treeWidget_Pages->addTopLevelItem(groupItem);

		std::cerr << "Group: " << group.mName;
		std::cerr << std::endl;

		std::list<std::string> pageIds;
		std::list<std::string>::iterator pit;

		rsWiki->getOrigPageList(*it, pageIds);

		for(pit = pageIds.begin(); pit != pageIds.end(); pit++)
		{
			std::cerr << "\tOrigPageId: " << *pit;
			std::cerr << std::endl;

			/* get newest page */
			RsWikiPage page;
			std::string latestPageId;
			if (!rsWiki->getLatestPage(*pit, latestPageId))
			{
				std::cerr << "\tgetLatestPage() Failed";
				std::cerr << std::endl;
			}

			if (!rsWiki->getPage(latestPageId, page))
			{
				std::cerr << "\tgetPage() Failed";
				std::cerr << std::endl;
			}

			std::cerr << "\tLatestPageId: " << latestPageId;
			std::cerr << std::endl;
			std::cerr << "\tExtracted OrigPageId: " << page.mOrigPageId;
			std::cerr << std::endl;
			std::cerr << "\tExtracted PageId: " << page.mPageId;
			std::cerr << std::endl;

			QTreeWidgetItem *pageItem = new QTreeWidgetItem(NULL);
			pageItem->setText(WIKI_GROUP_COL_PAGENAME, QString::fromStdString(page.mName));
			pageItem->setText(WIKI_GROUP_COL_PAGEID, QString::fromStdString(page.mPageId));
			pageItem->setText(WIKI_GROUP_COL_ORIGPAGEID, QString::fromStdString(page.mOrigPageId));

			groupItem->addChild(pageItem);

			std::cerr << "\tPage: " << page.mName;
			std::cerr << std::endl;
		}
	}
}


std::string WikiDialog::getSelectedPage()
{
	std::string pageId;
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedPage()" << std::endl;
#endif

	/* get current item */
	QTreeWidgetItem *item = ui.treeWidget_Pages->currentItem();

	if (!item)
	{
		/* leave current list */
#ifdef WIKI_DEBUG 
		std::cerr << "WikiDialog::getSelectedPage() Nothing selected" << std::endl;
#endif
		return pageId;
	}

	QTreeWidgetItem *parent = item->parent();

	if (!parent)
	{
#ifdef WIKI_DEBUG 
		std::cerr << "WikiDialog::getSelectedPage() No Parent -> Group Selected" << std::endl;
#endif
		return pageId;
	}


	/* check if it has changed */
	pageId = item->text(WIKI_GROUP_COL_PAGEID).toStdString();
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedPage() PageId: " << pageId << std::endl;
#endif
	return pageId;
}


std::string WikiDialog::getSelectedGroup()
{
	std::string groupId;
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedGroup()" << std::endl;
#endif

	/* get current item */
	QTreeWidgetItem *item = ui.treeWidget_Pages->currentItem();

	if (!item)
	{
		/* leave current list */
#ifdef WIKI_DEBUG 
		std::cerr << "WikiDialog::getSelectedGroup() Nothing selected" << std::endl;
#endif
		return groupId;
	}

	QTreeWidgetItem *parent = item->parent();

	if (parent)
	{
		groupId = parent->text(WIKI_GROUP_COL_GROUPID).toStdString();
#ifdef WIKI_DEBUG 
		std::cerr << "WikiDialog::getSelectedGroup() Page -> GroupId: " << groupId << std::endl;
#endif
		return groupId;
	}

	/* otherwise, we are on the group already */
	groupId = item->text(WIKI_GROUP_COL_GROUPID).toStdString();
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedGroup() GroupId: " << groupId << std::endl;
#endif
	return groupId;
}

#define WIKI_MODS_COL_ORIGPAGEID	0
#define WIKI_MODS_COL_PAGEID		1

void WikiDialog::insertModsForPage(std::string &origPageId)
{
	/* clear it all */
	//clearPhotos();
	//ui.photoLayout->clear();

	/* create a list of albums */

	std::list<std::string> pageIds;
	std::list<std::string>::const_iterator it;

	rsWiki->getPageVersions(origPageId, pageIds);


	for(it = pageIds.begin(); it != pageIds.end(); it++)
	{
		RsWikiPage page;
		rsWiki->getPage(*it, page);

		QTreeWidgetItem *modItem = new QTreeWidgetItem(NULL);
		modItem->setText(WIKI_MODS_COL_ORIGPAGEID, QString::fromStdString(page.mOrigPageId));
		modItem->setText(WIKI_MODS_COL_PAGEID, QString::fromStdString(page.mPageId));
		ui.treeWidget_Mods->addTopLevelItem(modItem);
	}
}



std::string WikiDialog::getSelectedMod()
{
	std::string pageId;
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedMod()" << std::endl;
#endif

	/* get current item */
	QTreeWidgetItem *item = ui.treeWidget_Mods->currentItem();

	if (!item)
	{
		/* leave current list */
#ifdef WIKI_DEBUG 
		std::cerr << "WikiDialog::getSelectedMod() Nothing selected" << std::endl;
#endif
		return pageId;
	}

	pageId = item->text(WIKI_MODS_COL_PAGEID).toStdString();
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedMod() PageId: " << pageId << std::endl;
#endif
	return pageId;
}



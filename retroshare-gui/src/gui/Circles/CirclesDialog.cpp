/*
 * Retroshare Circle Plugin.
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

#include "gui/Circles/CirclesDialog.h"
#include "gui/Circles/CreateCircleDialog.h"

#include <retroshare/rsgxscircles.h>
#include <retroshare/rspeers.h>

#include <iostream>
#include <sstream>

#include <QTimer>

/******
 * #define CIRCLE_DEBUG 1
 *****/


#define CIRCLESDIALOG_GROUPMETA			1

/** Constructor */
CirclesDialog::CirclesDialog(QWidget *parent)
: MainPage(parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	connect( ui.pushButton_refresh, SIGNAL(clicked()), this, SLOT(reloadAll()));
	connect( ui.pushButton_extCircle, SIGNAL(clicked()), this, SLOT(createExternalCircle()));
	connect( ui.pushButton_localCircle, SIGNAL(clicked()), this, SLOT(createPersonalCircle()));
	connect( ui.pushButton_editCircle, SIGNAL(clicked()), this, SLOT(editExistingCircle()));

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);

	/* setup TokenQueue */
        mCircleQueue = new TokenQueue(rsGxsCircles->getTokenService(), this);

	connect( ui.treeWidget_membership, SIGNAL(itemSelectionChanged()), this, SLOT(circle_selected()));
	connect( ui.treeWidget_friends, SIGNAL(itemSelectionChanged()), this, SLOT(friend_selected()));
	connect( ui.treeWidget_category, SIGNAL(itemSelectionChanged()), this, SLOT(category_selected()));
}


void CirclesDialog::checkUpdate()
{

	/* update */
	if (!rsGxsCircles)
		return;

	if (rsGxsCircles->updated())
	{
		reloadAll();
	}

	return;
}

#define CIRCLEGROUP_CIRCLE_COL_GROUPNAME	0
#define CIRCLEGROUP_CIRCLE_COL_GROUPID		1

#define CIRCLEGROUP_FRIEND_COL_NAME	0
#define CIRCLEGROUP_FRIEND_COL_ID	1

void CirclesDialog::createExternalCircle()
{
	CreateCircleDialog *createDialog = new CreateCircleDialog();
	createDialog->editNewId(true);
	createDialog->show();

}


void CirclesDialog::createPersonalCircle()
{
	CreateCircleDialog *createDialog = new CreateCircleDialog();
	createDialog->editNewId(false);
	createDialog->show();
}


void CirclesDialog::editExistingCircle()
{
#if 0
	std::string id;
	CreateCircleDialog *createDialog = new CreateCircleDialog();
	createDialog->editExistingId(id);
	createDialog->show();
#endif
}


void CirclesDialog::reloadAll()
{
	requestGroupMeta();
	/* grab all ids */
	//std::list<std::string>

	std::list<std::string> friend_pgpIds;
	std::list<std::string> all_pgpIds;
	std::list<std::string>::iterator it;

	std::set<std::string> friend_set;

	rsPeers->getGPGAcceptedList(friend_pgpIds);
	rsPeers->getGPGAllList(all_pgpIds);

	/* clear tree */
	ui.treeWidget_friends->clear();

	/* add the top level item */
	QTreeWidgetItem *friendsItem = new QTreeWidgetItem();
	friendsItem->setText(0, "Friends");
	ui.treeWidget_friends->addTopLevelItem(friendsItem);

	QTreeWidgetItem *fofItem = new QTreeWidgetItem();
	fofItem->setText(0, "Friends Of Friends");
	ui.treeWidget_friends->addTopLevelItem(fofItem);

	for(it = friend_pgpIds.begin(); it != friend_pgpIds.end(); it++)
	{
		RsPeerDetails details;
		if (rsPeers->getGPGDetails(*it, details))
		{
			friend_set.insert(*it);
			QTreeWidgetItem *item = new QTreeWidgetItem();

			item->setText(CIRCLEGROUP_FRIEND_COL_NAME, QString::fromStdString(details.name));
			item->setText(CIRCLEGROUP_FRIEND_COL_ID, QString::fromStdString(*it));
			friendsItem->addChild(item);
		}
        }


	for(it = all_pgpIds.begin(); it != all_pgpIds.end(); it++)
	{
		if (friend_set.end() != friend_set.find(*it))
		{
			// already added as a friend.
			continue;
		}

		RsPeerDetails details;
		if (rsPeers->getGPGDetails(*it, details))
		{
			QTreeWidgetItem *item = new QTreeWidgetItem();

			item->setText(CIRCLEGROUP_FRIEND_COL_NAME, QString::fromStdString(details.name));
			item->setText(CIRCLEGROUP_FRIEND_COL_ID, QString::fromStdString(*it));
			fofItem->addChild(item);
		}
        }
}
#define CLEAR_BACKGROUND	0
#define GREEN_BACKGROUND	1
#define BLUE_BACKGROUND		2
#define RED_BACKGROUND		3
#define GRAY_BACKGROUND		4


void set_item_background(QTreeWidgetItem *item, uint32_t type)
{
	QBrush brush;
	switch(type)
	{
		default:
		case CLEAR_BACKGROUND:
			brush = QBrush(Qt::white);
			break;
		case GREEN_BACKGROUND:
			brush = QBrush(Qt::green);
			break;
		case BLUE_BACKGROUND:
			brush = QBrush(Qt::blue);
			break;
		case RED_BACKGROUND:
			brush = QBrush(Qt::red);
			break;
		case GRAY_BACKGROUND:
			brush = QBrush(Qt::gray);
			break;
	}
	item->setBackground (0, brush);
}

void update_children_background(QTreeWidgetItem *item, uint32_t type)
{
        int count = item->childCount();
        for(int i = 0; i < count; i++)
        {
                QTreeWidgetItem *child = item->child(i);

                if (child->childCount() > 0)
                {
                        update_children_background(child, type);
                }
                set_item_background(child, type);
        }
}


void set_tree_background(QTreeWidget *tree, uint32_t type)
{
        std::cerr << "CirclesDialog set_tree_background()";
        std::cerr << std::endl;

	/* grab all toplevel */
        int count = tree->topLevelItemCount();
        for(int i = 0; i < count; i++)
        {
                QTreeWidgetItem *item = tree->topLevelItem(i);
		/* resursively clear child backgrounds */
                update_children_background(item, type);
                set_item_background(item, type);
        }
}


void check_mark_item(QTreeWidgetItem *item, const std::set<std::string> &names, uint32_t col, uint32_t type)
{
	QString coltext = item->text(col);
	std::string colstr = coltext.toStdString();
	if (names.end() != names.find(colstr))
	{
                set_item_background(item, type);
		std::cerr << "CirclesDialog check_mark_item: found match: " << colstr;
		std::cerr << std::endl;
	}
}

void update_mark_children(QTreeWidgetItem *item, const std::set<std::string> &names, uint32_t col, uint32_t type)
{
        int count = item->childCount();
        for(int i = 0; i < count; i++)
        {
                QTreeWidgetItem *child = item->child(i);

                if (child->childCount() > 0)
                {
                        update_mark_children(child, names, col, type);
                }
                check_mark_item(child, names, col, type);
        }
}

void mark_matching_tree(QTreeWidget *tree, const std::set<std::string> &names, uint32_t col, uint32_t type)
{
        std::cerr << "CirclesDialog mark_matching_tree()";
        std::cerr << std::endl;

	/* grab all toplevel */
        int count = tree->topLevelItemCount();
        for(int i = 0; i < count; i++)
        {
                QTreeWidgetItem *item = tree->topLevelItem(i);
		/* resursively clear child backgrounds */
                update_mark_children(item, names, col, type);
                check_mark_item(item, names, col, type);
        }
}



/**** Circles checks - v expensive ***/

void mark_circle_item(QTreeWidgetItem *item, const std::set<std::string> &names)
{
	std::string id = item->text(CIRCLEGROUP_CIRCLE_COL_GROUPID).toStdString();
	RsGxsCircleDetails details;
	if (rsGxsCircles->getCircleDetails(id, details))
	{
		std::set<std::string>::iterator it;
		for(it = names.begin(); it != names.end(); it++)
		{
			if (details.mAllowedPeers.end() != details.mAllowedPeers.find(*it))
			{
               			set_item_background(item, GREEN_BACKGROUND);
				std::cerr << "CirclesDialog mark_circle_item: found match: " << id;
				std::cerr << std::endl;
			}
		}
	}
	else
	{
                set_item_background(item, GRAY_BACKGROUND);
		std::cerr << "CirclesDialog mark_circle_item: no details: " << id;
		std::cerr << std::endl;
	}
}

void mark_circle_children(QTreeWidgetItem *item, const std::set<std::string> &names)
{
        int count = item->childCount();
        for(int i = 0; i < count; i++)
        {
                QTreeWidgetItem *child = item->child(i);

                if (child->childCount() > 0)
                {
                        mark_circle_children(child, names);
                }
                mark_circle_item(child, names);
        }
}


void mark_circle_tree(QTreeWidget *tree, const std::set<std::string> &names)
{
        std::cerr << "CirclesDialog mark_circle_tree()";
        std::cerr << std::endl;

	/* grab all toplevel */
        int count = tree->topLevelItemCount();
        for(int i = 0; i < count; i++)
        {
                QTreeWidgetItem *item = tree->topLevelItem(i);
                mark_circle_children(item, names);
        }
}



void CirclesDialog::circle_selected()
{
	QTreeWidgetItem *item = ui.treeWidget_membership->currentItem();

	std::cerr << "CirclesDialog::circle_selected() valid circle chosen";
	std::cerr << std::endl;

	set_tree_background(ui.treeWidget_membership, CLEAR_BACKGROUND);
	set_tree_background(ui.treeWidget_friends, CLEAR_BACKGROUND);
	set_tree_background(ui.treeWidget_category, CLEAR_BACKGROUND);

	if ((!item) || (!item->parent()))
	{
		return;
	}

	set_item_background(item, BLUE_BACKGROUND);

	QString coltext = item->text(CIRCLEGROUP_CIRCLE_COL_GROUPID);
	std::string id = coltext.toStdString();

	/* update friend lists */
	RsGxsCircleDetails details;
	if (rsGxsCircles->getCircleDetails(id, details))
	{
		/* now mark all the members */
		std::set<std::string> members;
	        std::map<RsPgpId, std::list<RsGxsId> >::iterator it;
		for(it = details.mAllowedPeers.begin(); it != details.mAllowedPeers.end(); it++)
		{
			members.insert(it->first);
			std::cerr << "Circle member: " << it->first;
			std::cerr << std::endl;
		}

		mark_matching_tree(ui.treeWidget_friends, members, 
			CIRCLEGROUP_FRIEND_COL_ID, GREEN_BACKGROUND);
	}
	else
	{
		set_tree_background(ui.treeWidget_friends, GRAY_BACKGROUND);
	}

}


void CirclesDialog::friend_selected()
{
	/* update circle lists */
	QTreeWidgetItem *item = ui.treeWidget_friends->currentItem();

	if ((!item) || (!item->parent()))
	{
		return;
	}


	set_tree_background(ui.treeWidget_membership, CLEAR_BACKGROUND);
	set_tree_background(ui.treeWidget_friends, CLEAR_BACKGROUND);
	set_tree_background(ui.treeWidget_category, CLEAR_BACKGROUND);

	set_item_background(item, BLUE_BACKGROUND);

	std::string id = item->text(CIRCLEGROUP_FRIEND_COL_ID).toStdString();

	/* update permission lists */
	std::set<std::string> names;
	names.insert(id);
	mark_circle_tree(ui.treeWidget_membership, names);
}

void CirclesDialog::category_selected()
{



}




#if 0
void CirclesDialog::groupTreeChanged()
{
	/* */
	std::string groupId;
	std::string pageId;
	std::string origPageId;

	getSelectedPage(groupId, pageId, origPageId);
	if (pageId == mPageSelected)
	{
		return; /* nothing changed */
	}

	if (pageId == "")
	{
		/* clear Mods */
		clearGroupTree();
		return;
	}

        RsGxsGrpMsgIdPair origPagePair = std::make_pair(groupId, origPageId);
        RsGxsGrpMsgIdPair pagepair = std::make_pair(groupId, pageId);
	requestWikiPage(pagepair);
}

void CirclesDialog::updateWikiPage(const RsWikiSnapshot &page)
{
#ifdef USE_PEGMMD_RENDERER
	/* render as HTML */
	int extensions = 0;
	char *answer = markdown_to_string((char *) page.mPage.c_str(), extensions, HTML_FORMAT);

	QString renderedText = QString::fromUtf8(answer);
	ui.textBrowser->setHtml(renderedText);

	// free answer.
	free(answer);
#else
	/* render as HTML */
	QString renderedText = "IN (dummy) RENDERED TEXT MODE:\n";
	renderedText += QString::fromStdString(page.mPage);
	ui.textBrowser->setPlainText(renderedText);
#endif
}


void CirclesDialog::clearWikiPage()
{
	ui.textBrowser->setPlainText("");
}


void 	CirclesDialog::clearGroupTree()
{
	ui.treeWidget_Pages->clear();
}


#define WIKI_GROUP_COL_GROUPNAME	0
#define WIKI_GROUP_COL_GROUPID		1

#define WIKI_GROUP_COL_PAGENAME		0
#define WIKI_GROUP_COL_PAGEID		1
#define WIKI_GROUP_COL_ORIGPAGEID	2


bool CirclesDialog::getSelectedPage(std::string &groupId, std::string &pageId, std::string &origPageId)
{
#ifdef WIKI_DEBUG 
	std::cerr << "CirclesDialog::getSelectedPage()" << std::endl;
#endif

	/* get current item */
	QTreeWidgetItem *item = ui.treeWidget_Pages->currentItem();

	if (!item)
	{
		/* leave current list */
#ifdef WIKI_DEBUG 
		std::cerr << "CirclesDialog::getSelectedPage() Nothing selected" << std::endl;
#endif
		return false;
	}

	QTreeWidgetItem *parent = item->parent();

	if (!parent)
	{
#ifdef WIKI_DEBUG 
		std::cerr << "CirclesDialog::getSelectedPage() No Parent -> Group Selected" << std::endl;
#endif
		return false;
	}


	/* check if it has changed */
	groupId = parent->text(WIKI_GROUP_COL_GROUPID).toStdString();
	pageId = item->text(WIKI_GROUP_COL_PAGEID).toStdString();
	origPageId = item->text(WIKI_GROUP_COL_ORIGPAGEID).toStdString();

#ifdef WIKI_DEBUG 
	std::cerr << "CirclesDialog::getSelectedPage() PageId: " << pageId << std::endl;
#endif
	return true;
}


std::string CirclesDialog::getSelectedGroup()
{
	std::string groupId;
#ifdef WIKI_DEBUG 
	std::cerr << "CirclesDialog::getSelectedGroup()" << std::endl;
#endif

	/* get current item */
	QTreeWidgetItem *item = ui.treeWidget_Pages->currentItem();

	if (!item)
	{
		/* leave current list */
#ifdef WIKI_DEBUG 
		std::cerr << "CirclesDialog::getSelectedGroup() Nothing selected" << std::endl;
#endif
		return groupId;
	}

	QTreeWidgetItem *parent = item->parent();

	if (parent)
	{
		groupId = parent->text(WIKI_GROUP_COL_GROUPID).toStdString();
#ifdef WIKI_DEBUG 
		std::cerr << "CirclesDialog::getSelectedGroup() Page -> GroupId: " << groupId << std::endl;
#endif
		return groupId;
	}

	/* otherwise, we are on the group already */
	groupId = item->text(WIKI_GROUP_COL_GROUPID).toStdString();
#ifdef WIKI_DEBUG 
	std::cerr << "CirclesDialog::getSelectedGroup() GroupId: " << groupId << std::endl;
#endif
	return groupId;
}

#endif

/************************** Request / Response *************************/
/*** Loading Main Index ***/

void CirclesDialog::requestGroupMeta()
{
	std::cerr << "CirclesDialog::requestGroupMeta()";
	std::cerr << std::endl;

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token;
	mCircleQueue->requestGroupInfo(token,  RS_TOKREQ_ANSTYPE_SUMMARY, opts, CIRCLESDIALOG_GROUPMETA);
}


void CirclesDialog::loadGroupMeta(const uint32_t &token)
{
	std::cerr << "CirclesDialog::loadGroupMeta()";
	std::cerr << std::endl;

	ui.treeWidget_membership->clear();

	std::list<RsGroupMetaData> groupInfo;
	std::list<RsGroupMetaData>::iterator vit;

	if (!rsGxsCircles->getGroupSummary(token,groupInfo))
	{
                std::cerr << "CirclesDialog::loadGroupMeta() Error getting GroupMeta";
                std::cerr << std::endl;
                return;
        }

	/* add the top level item */
	QTreeWidgetItem *personalCirclesItem = new QTreeWidgetItem();
	personalCirclesItem->setText(0, "Personal Circles");
	ui.treeWidget_membership->addTopLevelItem(personalCirclesItem);

	QTreeWidgetItem *externalAdminCirclesItem = new QTreeWidgetItem();
	externalAdminCirclesItem->setText(0, "External Circles (Admin)");
	ui.treeWidget_membership->addTopLevelItem(externalAdminCirclesItem);

	QTreeWidgetItem *externalSubCirclesItem = new QTreeWidgetItem();
	externalSubCirclesItem->setText(0, "External Circles (Subscribed)");
	ui.treeWidget_membership->addTopLevelItem(externalSubCirclesItem);

	QTreeWidgetItem *externalOtherCirclesItem = new QTreeWidgetItem();
	externalOtherCirclesItem->setText(0, "External Circles (Other)");
	ui.treeWidget_membership->addTopLevelItem(externalOtherCirclesItem);

        for(vit = groupInfo.begin(); vit != groupInfo.end(); vit++)
        {
		/* Add Widget, and request Pages */
		std::cerr << "CirclesDialog::loadGroupMeta() GroupId: " << vit->mGroupId;
		std::cerr << " Group: " << vit->mGroupName;
		std::cerr << std::endl;

		QTreeWidgetItem *groupItem = new QTreeWidgetItem();
		groupItem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, QString::fromStdString(vit->mGroupName));
		groupItem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPID, QString::fromStdString(vit->mGroupId));

		if (vit->mCircleType == GXS_CIRCLE_TYPE_LOCAL)
		{
			personalCirclesItem->addChild(groupItem);
		}
		else
		{
			if (vit->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
			{
				externalAdminCirclesItem->addChild(groupItem);
			}
			else if (vit->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
			{
				externalSubCirclesItem->addChild(groupItem);
			}
			else
			{
				externalOtherCirclesItem->addChild(groupItem);
			}
		}
        }
}



void CirclesDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{

	std::cerr << "CirclesDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
	
	if (queue == mCircleQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case CIRCLESDIALOG_GROUPMETA:
				loadGroupMeta(req.mToken);
				break;

			default:
				std::cerr << "CirclesDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}
}
	
	
	
	



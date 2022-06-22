/*******************************************************************************
 * gui/Circles/CirclesDialog.cpp                                               *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2012, robert Fernie <retroshare.project@gmail.com>            *
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

#include <QMessageBox>

#include "gui/Circles/CirclesDialog.h"
#include "gui/Circles/CreateCircleDialog.h"
#include "gui/common/UIStateHelper.h"
#include "util/qtthreadsutils.h"

#include <retroshare/rsgxscircles.h>
#include <retroshare/rspeers.h>

/******
 * #define CIRCLE_DEBUG 1
 *****/

#define CIRCLEGROUP_CIRCLE_COL_GROUPNAME 0
#define CIRCLEGROUP_CIRCLE_COL_GROUPID   1
#define CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS   2

#define CIRCLEGROUP_FRIEND_COL_NAME 0
#define CIRCLEGROUP_FRIEND_COL_ID   1

#define CLEAR_BACKGROUND 0
#define GREEN_BACKGROUND 1
#define BLUE_BACKGROUND  2
#define RED_BACKGROUND   3
#define GRAY_BACKGROUND  4

#define CIRCLESDIALOG_GROUPMETA			1

/** Constructor */
CirclesDialog::CirclesDialog(QWidget *parent)
	: MainPage(parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);
	mStateHelper->addWidget(CIRCLESDIALOG_GROUPMETA, ui.pushButton_extCircle);
	mStateHelper->addWidget(CIRCLESDIALOG_GROUPMETA, ui.pushButton_localCircle);
	mStateHelper->addWidget(CIRCLESDIALOG_GROUPMETA, ui.pushButton_editCircle);

	mStateHelper->addWidget(CIRCLESDIALOG_GROUPMETA, ui.treeWidget_membership, UISTATE_ACTIVE_ENABLED);
	mStateHelper->setWidgetEnabled(ui.pushButton_editCircle, false);

	/* Connect signals */
	connect(ui.pushButton_extCircle, SIGNAL(clicked()), this, SLOT(createExternalCircle()));
	connect(ui.pushButton_localCircle, SIGNAL(clicked()), this, SLOT(createPersonalCircle()));
	connect(ui.pushButton_editCircle, SIGNAL(clicked()), this, SLOT(editExistingCircle()));
	connect(ui.todoPushButton, SIGNAL(clicked()), this, SLOT(todo()));

	connect(ui.treeWidget_membership, SIGNAL(itemSelectionChanged()), this, SLOT(circle_selected()));

	/* Set header resize modes and initial section sizes */
	QHeaderView * membership_header = ui.treeWidget_membership->header () ;
	membership_header->resizeSection ( CIRCLEGROUP_CIRCLE_COL_GROUPNAME, 200 );
}

CirclesDialog::~CirclesDialog()
{
    delete mCircleQueue;
}

void CirclesDialog::todo()
{
	QMessageBox::information(this, "Todo",
							 "<b>Open points:</b><ul>"
							 "<li>Improve create dialog"
							 "<li>Edit circles"
							 "<li>Categories"
							 "<li>Don't refill complete trees"
							 "</ul>");
}

void CirclesDialog::updateDisplay(bool /*complete*/)
{
	reloadAll();
}

void CirclesDialog::createExternalCircle()
{
	CreateCircleDialog dlg;
	dlg.editNewId(true);
	dlg.exec();
}

void CirclesDialog::createPersonalCircle()
{
	CreateCircleDialog dlg;
	dlg.editNewId(false);
	dlg.exec();
}

void CirclesDialog::editExistingCircle()
{
	QTreeWidgetItem *item = ui.treeWidget_membership->currentItem();
	if ((!item) || (!item->parent()))
	{
		return;
	}

	QString coltext = item->text(CIRCLEGROUP_CIRCLE_COL_GROUPID);
    RsGxsGroupId id ( coltext.toStdString());

	uint32_t subscribe_flags = item->data(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS, Qt::UserRole).toUInt();
    
	CreateCircleDialog dlg;
	dlg.editExistingId(id,true,!!(subscribe_flags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)) ;
	dlg.exec();
}

void CirclesDialog::reloadAll()
{
	requestGroupMeta();

	/* grab all ids */
    std::list<RsPgpId> friend_pgpIds;
    std::list<RsPgpId> all_pgpIds;
    std::list<RsPgpId>::iterator it;

    std::set<RsPgpId> friend_set;

	rsPeers->getGPGAcceptedList(friend_pgpIds);
	rsPeers->getGPGAllList(all_pgpIds);

#ifdef SUSPENDED_CODE
	/* clear tree */
	ui.treeWidget_friends->clear();

	/* add the top level item */
	QTreeWidgetItem *friendsItem = new QTreeWidgetItem();
	friendsItem->setText(0, tr("Friends"));
	ui.treeWidget_friends->addTopLevelItem(friendsItem);

	QTreeWidgetItem *fofItem = new QTreeWidgetItem();
	fofItem->setText(0, tr("Friends Of Friends"));
	ui.treeWidget_friends->addTopLevelItem(fofItem);

	for(it = friend_pgpIds.begin(); it != friend_pgpIds.end(); ++it)
	{
		RsPeerDetails details;
		if (rsPeers->getGPGDetails(*it, details))
		{
			friend_set.insert(*it);
			QTreeWidgetItem *item = new QTreeWidgetItem();

			item->setText(CIRCLEGROUP_FRIEND_COL_NAME, QString::fromUtf8(details.name.c_str()));
            item->setText(CIRCLEGROUP_FRIEND_COL_ID, QString::fromStdString((*it).toStdString()));
			friendsItem->addChild(item);
		}
	}

	for(it = all_pgpIds.begin(); it != all_pgpIds.end(); ++it)
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

			item->setText(CIRCLEGROUP_FRIEND_COL_NAME, QString::fromUtf8(details.name.c_str()));
            item->setText(CIRCLEGROUP_FRIEND_COL_ID, QString::fromStdString((*it).toStdString()));
			fofItem->addChild(item);
		}
	}
#endif
}

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
	for(int i = 0; i < count; ++i)
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
	for(int i = 0; i < count; ++i)
	{
		QTreeWidgetItem *item = tree->topLevelItem(i);
		/* resursively clear child backgrounds */
		update_children_background(item, type);
		set_item_background(item, type);
	}
}

void check_mark_item(QTreeWidgetItem *item, const std::set<RsPgpId> &names, uint32_t col, uint32_t type)
{
	QString coltext = item->text(col);
    RsPgpId colstr ( coltext.toStdString());
	if (names.end() != names.find(colstr))
	{
		set_item_background(item, type);
		std::cerr << "CirclesDialog check_mark_item: found match: " << colstr;
		std::cerr << std::endl;
	}
}

void update_mark_children(QTreeWidgetItem *item, const std::set<RsPgpId> &names, uint32_t col, uint32_t type)
{
	int count = item->childCount();
	for(int i = 0; i < count; ++i)
	{
		QTreeWidgetItem *child = item->child(i);

		if (child->childCount() > 0)
		{
			update_mark_children(child, names, col, type);
		}
		check_mark_item(child, names, col, type);
	}
}

void mark_matching_tree(QTreeWidget *tree, const std::set<RsPgpId> &names, uint32_t col, uint32_t type)
{
	std::cerr << "CirclesDialog mark_matching_tree()";
	std::cerr << std::endl;

	/* grab all toplevel */
	int count = tree->topLevelItemCount();
	for(int i = 0; i < count; ++i)
	{
		QTreeWidgetItem *item = tree->topLevelItem(i);
		/* resursively clear child backgrounds */
		update_mark_children(item, names, col, type);
		check_mark_item(item, names, col, type);
	}
}

/**** Circles checks - v expensive ***/

void mark_circle_item(QTreeWidgetItem *item, const std::set<RsPgpId> &names)
{
    RsGxsCircleId id ( item->text(CIRCLEGROUP_CIRCLE_COL_GROUPID).toStdString());
	RsGxsCircleDetails details;
	if (rsGxsCircles->getCircleDetails(id, details))
	{
        std::set<RsPgpId>::iterator it;
		for(it = names.begin(); it != names.end(); ++it)
		{
            if (details.mAllowedNodes.end() != details.mAllowedNodes.find(*it))
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

void mark_circle_children(QTreeWidgetItem *item, const std::set<RsPgpId> &names)
{
	int count = item->childCount();
	for(int i = 0; i < count; ++i)
	{
		QTreeWidgetItem *child = item->child(i);

		if (child->childCount() > 0)
		{
			mark_circle_children(child, names);
		}
		mark_circle_item(child, names);
	}
}

void mark_circle_tree(QTreeWidget *tree, const std::set<RsPgpId> &names)
{
	std::cerr << "CirclesDialog mark_circle_tree()";
	std::cerr << std::endl;

	/* grab all toplevel */
	int count = tree->topLevelItemCount();
	for(int i = 0; i < count; ++i)
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
		mStateHelper->setWidgetEnabled(ui.pushButton_editCircle, false);
		return;
	}

	set_item_background(item, BLUE_BACKGROUND);

	QString coltext = item->text(CIRCLEGROUP_CIRCLE_COL_GROUPID);
    RsGxsCircleId id ( coltext.toStdString()) ;

	/* update friend lists */
	RsGxsCircleDetails details;
	if (rsGxsCircles->getCircleDetails(id, details))
	{
		/* now mark all the members */
		mark_matching_tree(ui.treeWidget_friends, details.mAllowedNodes, CIRCLEGROUP_FRIEND_COL_ID, GREEN_BACKGROUND);
	}
	else
	{
		set_tree_background(ui.treeWidget_friends, GRAY_BACKGROUND);
	}
	mStateHelper->setWidgetEnabled(ui.pushButton_editCircle, true);
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

    RsPgpId id ( item->text(CIRCLEGROUP_FRIEND_COL_ID).toStdString());

	/* update permission lists */
    std::set<RsPgpId> names;
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


void CirclesDialog::clearGroupTree()
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
	mStateHelper->setLoading(CIRCLESDIALOG_GROUPMETA, true);

    RsThread::async([this]()
    {
        std::list<RsGroupMetaData> circles;

        if(!rsGxsCircles->getCirclesSummaries(circles))
        {
            std::cerr << __PRETTY_FUNCTION__ << " failed to get circles summaries " << std::endl;
            return;
        }

        RsQThreadUtils::postToObject( [this,circles]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete, note that
             * Qt::QueuedConnection is important!
             */

            loadGroupMeta(circles);

        }, this );
    });
}

void CirclesDialog::loadGroupMeta(const std::list<RsGroupMetaData>& groupInfo)
{
	mStateHelper->setLoading(CIRCLESDIALOG_GROUPMETA, false);

	ui.treeWidget_membership->clear();

	mStateHelper->setActive(CIRCLESDIALOG_GROUPMETA, true);

	/* add the top level item */
	QTreeWidgetItem *personalCirclesItem = new QTreeWidgetItem();
	personalCirclesItem->setText(0, tr("Personal Circles"));
	ui.treeWidget_membership->addTopLevelItem(personalCirclesItem);

	QTreeWidgetItem *externalAdminCirclesItem = new QTreeWidgetItem();
	externalAdminCirclesItem->setText(0, tr("External Circles (Admin)"));
	ui.treeWidget_membership->addTopLevelItem(externalAdminCirclesItem);

	QTreeWidgetItem *externalSubCirclesItem = new QTreeWidgetItem();
	externalSubCirclesItem->setText(0, tr("External Circles (Subscribed)"));
	ui.treeWidget_membership->addTopLevelItem(externalSubCirclesItem);

	QTreeWidgetItem *externalOtherCirclesItem = new QTreeWidgetItem();
	externalOtherCirclesItem->setText(0, tr("External Circles (Other)"));
	ui.treeWidget_membership->addTopLevelItem(externalOtherCirclesItem);

    for(auto vit = groupInfo.begin(); vit != groupInfo.end(); ++vit)
	{
		/* Add Widget, and request Pages */
		std::cerr << "CirclesDialog::loadGroupMeta() GroupId: " << vit->mGroupId;
		std::cerr << " Group: " << vit->mGroupName;
		std::cerr << std::endl;

		QTreeWidgetItem *groupItem = new QTreeWidgetItem();
		groupItem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, QString::fromUtf8(vit->mGroupName.c_str()));
        groupItem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPID, QString::fromStdString(vit->mGroupId.toStdString()));

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

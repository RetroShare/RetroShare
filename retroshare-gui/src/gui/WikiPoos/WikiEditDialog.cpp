/*******************************************************************************
 * gui/WikiPoos/WikiEditDialog.cpp                                             *
 *                                                                             *
 * Copyright (C) 2012 Robert Fernie   <retroshare.project@gmail.com>           *
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

#include <QDateTime>

#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "gui/WikiPoos/WikiEditDialog.h"

#include <iostream>

#define USE_PEGMMD_RENDERER	1

#ifdef USE_PEGMMD_RENDERER
#include "markdown_lib.h"
#endif


#define 	WIKIEDITDIALOG_GROUP		0x0001
#define 	WIKIEDITDIALOG_PAGE		0x0002
#define 	WIKIEDITDIALOG_BASEHISTORY	0x0003
#define 	WIKIEDITDIALOG_EDITTREE		0x0004


#define WET_COL_DATE		0
#define WET_COL_AUTHORID	1
#define WET_COL_PAGEID		2

#define WET_DATA_COLUMN		0

#define WET_ROLE_ORIGPAGEID	Qt::UserRole
#define WET_ROLE_PAGEID		Qt::UserRole + 1
#define WET_ROLE_PARENTID	Qt::UserRole + 2
#define WET_ROLE_SORT		Qt::UserRole + 3


/** Constructor */
WikiEditDialog::WikiEditDialog(QWidget *parent)
: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.pushButton_Cancel, SIGNAL( clicked() ), this, SLOT( cancelEdit() ) );
	connect(ui.pushButton_Revert, SIGNAL( clicked() ), this, SLOT( revertEdit() ) );
	connect(ui.postButton, SIGNAL( clicked() ), this, SLOT( submitEdit() ) );
	connect(ui.pushButton_Preview, SIGNAL( clicked() ), this, SLOT( previewToggle() ) );
	connect(ui.pushButton_History, SIGNAL( clicked() ), this, SLOT( historyToggle() ) );
	connect(ui.toolButton_Show, SIGNAL( clicked() ), this, SLOT( detailsToggle() ) );
	connect(ui.toolButton_Hide, SIGNAL( clicked() ), this, SLOT( detailsToggle() ) );
	connect(ui.textEdit, SIGNAL( textChanged() ), this, SLOT( textChanged() ) );
	connect(ui.checkBox_OldHistory, SIGNAL( clicked() ), this, SLOT( oldHistoryChanged() ) );
	connect(ui.checkBox_Merge, SIGNAL( clicked() ), this, SLOT( mergeModeToggle() ) );
	connect(ui.pushButton_Merge, SIGNAL( clicked() ), this, SLOT( generateMerge() ) );
	connect(ui.treeWidget_History, SIGNAL( itemSelectionChanged() ), this, SLOT( historySelected() ) );

	mWikiQueue = new TokenQueue(rsWiki->getTokenService(), this);

	mThreadCompareRole = new RSTreeWidgetItemCompareRole;
	mThreadCompareRole->setRole(WET_COL_DATE, WET_ROLE_SORT);

	mRepublishMode = false;
	mPreviewMode = false;
	mPageLoading = false;

	mIgnoreTextChange = false;
	mTextChanged = false;
	mCurrentText = "";

	mHistoryLoaded = false;
	mHistoryMergeMode = false;

	ui.toolButton_Show->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/down-arrow.png")));
	ui.toolButton_Hide->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/up-arrow.png")));
	ui.pushButton_Preview->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/search.png")));
	ui.pushButton_History->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/history-clock-white.png")));

	ui.checkBox_OldHistory->setChecked(false);
	mOldHistoryEnabled = false;
	ui.groupBox_History->hide();
	detailsToggle();
}

WikiEditDialog::~WikiEditDialog()
{
	delete (mThreadCompareRole);
	delete(mWikiQueue);
}

void WikiEditDialog::mergeModeToggle()
{
	mHistoryMergeMode = ui.checkBox_Merge->isChecked();
	updateHistoryStatus();
}

void WikiEditDialog::generateMerge()
{
	std::cerr << "WikiEditDialog::generateMerge() TODO" << std::endl;

}

void WikiEditDialog::textChanged()
{
	if (mIgnoreTextChange)
	{
		std::cerr << "WikiEditDialog::textChanged() Ignored" << std::endl;
		return;
	}
	std::cerr << "WikiEditDialog::textChanged()" << std::endl;

	mTextChanged = true;
	ui.pushButton_Revert->setEnabled(true);
	ui.postButton->setEnabled(true);
	ui.label_Status->setText("Modified");

	// Disable Selection in Edit History.
	ui.treeWidget_History->setSelectionMode(QAbstractItemView::NoSelection);
	updateHistoryStatus();

	// unselect anything.
}


void WikiEditDialog::textReset()
{
	std::cerr << "WikiEditDialog::textReset()" << std::endl;

	mTextChanged = false;
	ui.pushButton_Revert->setEnabled(false);
	ui.postButton->setEnabled(false);
	ui.label_Status->setText("Original");

	// Enable Selection in Edit History.
	ui.treeWidget_History->setSelectionMode(QAbstractItemView::SingleSelection);
	updateHistoryStatus();
}

void WikiEditDialog::historySelected()
{
	std::cerr << "WikiEditDialog::historySelected()" << std::endl;

	QList<QTreeWidgetItem *> selected = ui.treeWidget_History->selectedItems();
	if (selected.empty())
	{
		std::cerr << "WikiEditDialog::historySelected() ERROR Nothing selected" << std::endl;
		return;
	}
	QTreeWidgetItem *item = *(selected.begin());
	
	RsGxsGrpMsgIdPair newSnapshot = mThreadMsgIdPair;
	std::string pageId = item->data(WET_DATA_COLUMN, WET_ROLE_PAGEID).toString().toStdString();
	newSnapshot.second = RsGxsMessageId(pageId);

	std::cerr << "WikiEditDialog::historySelected() New PageId: " << pageId;
	std::cerr << std::endl;

	requestPage(newSnapshot);
}


void WikiEditDialog::oldHistoryChanged()
{
	mOldHistoryEnabled = ui.checkBox_OldHistory->isChecked();
	updateHistoryStatus();
}


void WikiEditDialog::updateHistoryStatus()
{
	std::cerr << "WikiEditDialog::updateHistoryStatus()";
	std::cerr << std::endl;

	/* iterate through every History Item */
	int count = ui.treeWidget_History->topLevelItemCount();
	for(int i = 0; i < count; ++i)
	{
		QTreeWidgetItem *item = ui.treeWidget_History->topLevelItem(i);
		bool isLatest = (i==count-1);
		updateHistoryChildren(item, isLatest);
		updateHistoryItem(item, isLatest);
	}
}

void WikiEditDialog::updateHistoryChildren(QTreeWidgetItem *item, bool isLatest)
{
	int count = item->childCount();
	for(int i = 0; i < count; ++i)
	{
		QTreeWidgetItem *child = item->child(i);

		if (child->childCount() > 0)
		{
			updateHistoryChildren(child, isLatest);
		}
		updateHistoryItem(child, isLatest);
	}
}


void WikiEditDialog::updateHistoryItem(QTreeWidgetItem *item, bool isLatest)
{
	bool isSelectable = true;
	if (mTextChanged)
	{
		isSelectable = false;
	}
	else if ((!mOldHistoryEnabled) && (!isLatest))
	{
		isSelectable = false;
	}	

	if (isSelectable)
	{
		std::cerr << "WikiEditDialog::updateHistoryItem() isSelectable";
		std::cerr << std::endl;

		item->setFlags(Qt::ItemIsSelectable | 
			Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

		if (mHistoryMergeMode)
		{
			QVariant qvar = item->data(WET_COL_PAGEID, Qt::CheckStateRole);
			std::cerr << "WikiEditDialog::CheckStateRole:: VariantType: " << (int) qvar.type();
			std::cerr << std::endl;
			if (!qvar.isValid())
			{
				item->setData(WET_COL_PAGEID, Qt::CheckStateRole, Qt::Unchecked);
			}
			
		}
		else
		{
			item->setData(WET_COL_PAGEID, Qt::CheckStateRole, QVariant());
		}
	}
	else
	{
		std::cerr << "WikiEditDialog::updateHistoryItem() NOT isSelectable";
		std::cerr << std::endl;

		item->setData(WET_COL_PAGEID, Qt::CheckStateRole, QVariant());
		item->setFlags(Qt::ItemIsUserCheckable);
	}
}

void WikiEditDialog::detailsToggle()
{
	std::cerr << "WikiEditDialog::detailsToggle()";
	std::cerr << std::endl;
	if (ui.toolButton_Hide->isHidden())
	{
		ui.toolButton_Hide->show();
		ui.toolButton_Show->hide();

		ui.label_PrevVersion->show();
		ui.label_Group->show();
		ui.label_Tags->show();
		ui.lineEdit_PrevVersion->show();
		ui.lineEdit_Group->show();
		ui.lineEdit_Tags->show();
	}
	else
	{
		ui.toolButton_Hide->hide();
		ui.toolButton_Show->show();

		ui.label_PrevVersion->hide();
		ui.label_Group->hide();
		ui.label_Tags->hide();
		ui.lineEdit_PrevVersion->hide();
		ui.lineEdit_Group->hide();
		ui.lineEdit_Tags->hide();
	}
}

void WikiEditDialog::historyToggle()
{
	std::cerr << "WikiEditDialog::historyToggle()";
	std::cerr << std::endl;
	if (ui.groupBox_History->isHidden())
	{
		ui.groupBox_History->show();
		ui.pushButton_History->setToolTip(tr("Hide Edit History"));
	}
	else
	{
		ui.groupBox_History->hide();
		ui.pushButton_History->setToolTip(tr("Show Edit History"));
	}
}


void WikiEditDialog::previewToggle()
{
	std::cerr << "WikiEditDialog::previewToggle()";
	std::cerr << std::endl;

	if (mPreviewMode)
	{
		mPreviewMode = false;
		ui.pushButton_Preview->setText(tr("Preview"));
		ui.pushButton_Preview->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/search.png")));
	}
	else
	{
		// Save existing Text into buffer.
		mCurrentText = ui.textEdit->toPlainText();
		mPreviewMode = true;
		ui.pushButton_Preview->setText(tr("Edit Page"));
		ui.pushButton_Preview->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/pencil-edit-button.png")));
	}


	mIgnoreTextChange = true;
	redrawPage();
	mIgnoreTextChange = false;

	std::cerr << "WikiEditDialog::previewToggle() END";
	std::cerr << std::endl;
}


void WikiEditDialog::redrawPage()
{
	std::cerr << "WikiEditDialog::redrawPage()";
	std::cerr << std::endl;

	if (mPreviewMode)
	{
#ifdef USE_PEGMMD_RENDERER
		/* render as HTML */
		QByteArray byte_array = mCurrentText.toUtf8();

		int extensions = 0;
		char *answer = markdown_to_string(byte_array.data(), extensions, HTML_FORMAT);

		QString renderedText = QString::fromUtf8(answer);
		ui.textEdit->setHtml(renderedText);

		// free answer.
		free(answer);
#else
		/* render as HTML */
		QString renderedText = "IN (dummy) RENDERED TEXT MODE:\n";
		renderedText += mCurrentText;
		ui.textEdit->setPlainText(renderedText);
#endif


		/* disable edit */
		ui.textEdit->setReadOnly(true);
	}
	else
	{
		/* plain text - for editing */
		ui.textEdit->setPlainText(mCurrentText);

		/* enable edit */
		ui.textEdit->setReadOnly(false);
	}
}


void WikiEditDialog::setGroup(RsWikiCollection &group)
{
	std::cerr << "WikiEditDialog::setGroup(): " << group;
	std::cerr << std::endl;

	mWikiCollection = group;

	ui.lineEdit_Group->setText(QString::fromStdString(mWikiCollection.mMeta.mGroupName));
}


void WikiEditDialog::setPreviousPage(RsWikiSnapshot &page)
{
	std::cerr << "WikiEditDialog::setPreviousPage(): " << page;
	std::cerr << std::endl;

	mNewPage = false;
	mWikiSnapshot = page;

	ui.lineEdit_Page->setText(QString::fromStdString(mWikiSnapshot.mMeta.mMsgName));
    ui.lineEdit_PrevVersion->setText(QString::fromStdString(mWikiSnapshot.mMeta.mMsgId.toStdString()));
	mCurrentText = QString::fromUtf8(mWikiSnapshot.mPage.c_str());

	mIgnoreTextChange = true;
	redrawPage();
	mIgnoreTextChange = false;

	textReset();
}


void WikiEditDialog::setNewPage()
{
	mNewPage = true;
	mRepublishMode = false;
	mHistoryLoaded = false;
	ui.lineEdit_Page->setText("");
	ui.lineEdit_PrevVersion->setText("");

	mCurrentText = "";
	redrawPage();
	ui.treeWidget_History->clear();
	ui.groupBox_History->hide();
	ui.pushButton_History->setToolTip(tr("Show Edit History"));

	ui.headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/images/addpage.png"));
	ui.headerFrame->setHeaderText(tr("Create New Wiki Page"));
	setWindowTitle(tr("Create New Wiki Page"));

	/* No need for for REQUIRED ID */
	ui.comboBox_IdChooser->loadIds(0, RsGxsId());

	textReset();
}


void WikiEditDialog::setRepublishMode(RsGxsMessageId &origMsgId)
{
	mRepublishMode = true;
	mRepublishOrigId = origMsgId;
	ui.postButton->setText(tr("Republish"));
	/* No need for for REQUIRED ID */
	ui.comboBox_IdChooser->loadIds(0, RsGxsId());
}



void WikiEditDialog::cancelEdit()
{
	hide();
}


void WikiEditDialog::revertEdit()
{
	if (mNewPage) {
		mCurrentText = "";
	} else {//if (mNewPage
		ui.textEdit->setPlainText(QString::fromStdString(mWikiSnapshot.mPage));
		mCurrentText = QString::fromUtf8(mWikiSnapshot.mPage.c_str());
	}//if (mNewPage
        redrawPage();
	textReset();
}


void WikiEditDialog::submitEdit()
{
	std::cerr << "WikiEditDialog::submitEdit()";
	std::cerr << std::endl;

	RsGxsId authorId;
	switch (ui.comboBox_IdChooser->getChosenId(authorId)) {
		case GxsIdChooser::KnowId:
		case GxsIdChooser::UnKnowId:
			mWikiSnapshot.mMeta.mAuthorId = authorId;
			std::cerr << "WikiEditDialog::submitEdit() AuthorId: " << authorId;
			std::cerr << std::endl;

		break;
		case GxsIdChooser::NoId:
		case GxsIdChooser::None:
		default:
			std::cerr << "WikiEditDialog::submitEdit() ERROR GETTING AuthorId!";
			std::cerr << std::endl;
	}//switch (ui.comboBox_IdChooser->getChosenId(authorId))

	if (mNewPage) {
		mWikiSnapshot.mMeta.mGroupId = mWikiCollection.mMeta.mGroupId;
		mWikiSnapshot.mMeta.mOrigMsgId.clear() ;
		mWikiSnapshot.mMeta.mMsgId.clear() ;
		mWikiSnapshot.mMeta.mParentId.clear() ;
		mWikiSnapshot.mMeta.mThreadId.clear() ;

		std::cerr << "WikiEditDialog::submitEdit() Is New Page";
		std::cerr << std::endl;
	} else if (mRepublishMode) {
		std::cerr << "WikiEditDialog::submitEdit() In Republish Mode";
		std::cerr << std::endl;
		// A New Version of the ThreadHead.
		mWikiSnapshot.mMeta.mGroupId = mWikiCollection.mMeta.mGroupId;
		mWikiSnapshot.mMeta.mOrigMsgId = mRepublishOrigId;
		mWikiSnapshot.mMeta.mParentId.clear() ;
		mWikiSnapshot.mMeta.mThreadId.clear() ;
		mWikiSnapshot.mMeta.mMsgId.clear() ;
	} else {
		std::cerr << "WikiEditDialog::submitEdit() In Child Edit Mode";
		std::cerr << std::endl;

		// A Child of the current message.
		bool isFirstChild = false;
		if (mWikiSnapshot.mMeta.mParentId.isNull()) {
			isFirstChild = true;
		}//if (mWikiSnapshot.mMeta.mParentId.isNull())

		mWikiSnapshot.mMeta.mGroupId = mWikiCollection.mMeta.mGroupId;

		if (isFirstChild){
			mWikiSnapshot.mMeta.mThreadId = mWikiSnapshot.mMeta.mOrigMsgId;
			// Special HACK here... parentId points to specific Msg, rather than OrigMsgId.
			// This allows versioning to work well.
			mWikiSnapshot.mMeta.mParentId = mWikiSnapshot.mMeta.mMsgId;
		} else {//if (isFirstChild)
			// ThreadId is the same.
			mWikiSnapshot.mMeta.mParentId = mWikiSnapshot.mMeta.mOrigMsgId;
		}//if (isFirstChild)

        mWikiSnapshot.mMeta.mMsgId.clear() ;
        mWikiSnapshot.mMeta.mOrigMsgId.clear() ;
	}//if (mNewPage)


	mWikiSnapshot.mMeta.mMsgName = ui.lineEdit_Page->text().toStdString();

	if (!mPreviewMode) {
		/* can just use the current text */
		mCurrentText = ui.textEdit->toPlainText();
	}//if (!mPreviewMode)

	{// complicated way of preserving Utf8 text */
		QByteArray byte_array = mCurrentText.toUtf8();
		mWikiSnapshot.mPage = std::string(byte_array.data());
	}// complicated way of preserving Utf8 text */

	std::cerr << "WikiEditDialog::submitEdit() PageTitle: " << mWikiSnapshot.mMeta.mMsgName;
	std::cerr << std::endl;
	std::cerr << "WikiEditDialog::submitEdit() GroupId: " << mWikiSnapshot.mMeta.mGroupId;
	std::cerr << std::endl;
	std::cerr << "WikiEditDialog::submitEdit() OrigMsgId: " << mWikiSnapshot.mMeta.mOrigMsgId;
	std::cerr << std::endl;
	std::cerr << "WikiEditDialog::submitEdit() MsgId: " << mWikiSnapshot.mMeta.mMsgId;
	std::cerr << std::endl;
	std::cerr << "WikiEditDialog::submitEdit() ThreadId: " << mWikiSnapshot.mMeta.mThreadId;
	std::cerr << std::endl;
	std::cerr << "WikiEditDialog::submitEdit() ParentId: " << mWikiSnapshot.mMeta.mParentId;
	std::cerr << "WikiEditDialog::submitEdit() AuthorId: " << mWikiSnapshot.mMeta.mAuthorId;
	std::cerr << std::endl;

	uint32_t token;
	//bool  isNew = mNewPage;
	//rsWiki->createPage(token, mWikiSnapshot, isNew);
	rsWiki->submitSnapshot(token, mWikiSnapshot);
	hide();
}

void WikiEditDialog::setupData(const RsGxsGroupId &groupId, const RsGxsMessageId &pageId)
{
        mRepublishMode = false;
	mHistoryLoaded = false;
    if (!groupId.isNull())
	{
		requestGroup(groupId);
	}

    if (!pageId.isNull())
	{
        	RsGxsGrpMsgIdPair msgId = std::make_pair(groupId, pageId);
		requestPage(msgId);
	}

    ui.headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/images/editpage.png"));
	ui.headerFrame->setHeaderText(tr("Edit Wiki Page"));
	setWindowTitle(tr("Edit Wiki Page"));

        /* fill in the available OwnIds for signing */
        ui.comboBox_IdChooser->loadIds(IDCHOOSER_ID_REQUIRED, RsGxsId());

}


/***** Request / Response for Data **********/

void WikiEditDialog::requestGroup(const RsGxsGroupId &groupId)
{
        std::cerr << "WikiEditDialog::requestGroup()";
        std::cerr << std::endl;

        std::list<RsGxsGroupId> ids;
    ids.push_back(groupId);

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	uint32_t token;
        mWikiQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, ids, WIKIEDITDIALOG_GROUP);
}

void WikiEditDialog::loadGroup(const uint32_t &token)
{
        std::cerr << "WikiEditDialog::loadGroup()";
        std::cerr << std::endl;

	std::vector<RsWikiCollection> groups;
	if (rsWiki->getCollections(token, groups))
	{
		if (groups.size() != 1)
		{
        		std::cerr << "WikiEditDialog::loadGroup() ERROR No group data";
        		std::cerr << std::endl;
			return;
		}
		setGroup(groups[0]);
	}
}

void WikiEditDialog::requestPage(const RsGxsGrpMsgIdPair &msgId)
{
        std::cerr << "WikiEditDialog::requestPage()";
        std::cerr << std::endl;

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

        GxsMsgReq msgIds;
        std::set<RsGxsMessageId> &set_msgIds = msgIds[msgId.first];
        set_msgIds.insert(msgId.second);

	uint32_t token;
        mWikiQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, WIKIEDITDIALOG_PAGE);
	mPageLoading = true;
}

void WikiEditDialog::loadPage(const uint32_t &token)
{
        std::cerr << "WikiEditDialog::loadPage()";
        std::cerr << std::endl;

	std::vector<RsWikiSnapshot> snapshots;

	if (rsWiki->getSnapshots(token, snapshots))
	{
		if (snapshots.size() != 1)
		{
        		std::cerr << "WikiEditDialog::loadGroup() ERROR No group data";
        		std::cerr << std::endl;
			return;
		}

		RsWikiSnapshot &page = snapshots[0];
		setPreviousPage(page);

		/* request the history now */
		mThreadMsgIdPair.first = page.mMeta.mGroupId;
        if (page.mMeta.mThreadId.isNull())
		{
			mThreadMsgIdPair.second = page.mMeta.mOrigMsgId;
		}
		else
		{
			mThreadMsgIdPair.second = page.mMeta.mThreadId;
		}
		if (!mHistoryLoaded)
		{
			requestBaseHistory(mThreadMsgIdPair);
		}
	}
	mPageLoading = false;
}


/*********************** LOAD EDIT HISTORY **********************/

void WikiEditDialog::requestBaseHistory(const RsGxsGrpMsgIdPair &origMsgId)
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_VERSIONS;
        std::vector<RsGxsGrpMsgIdPair> msgIds;
        msgIds.push_back(origMsgId);
	uint32_t token;
        mWikiQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, WIKIEDITDIALOG_BASEHISTORY);
	ui.treeWidget_History->clear();
}

void WikiEditDialog::loadBaseHistory(const uint32_t &token)
{
	std::cerr << "WikiEditDialog::loadBaseHistory()";
	std::cerr << std::endl;


	std::vector<RsWikiSnapshot> snapshots;
	std::vector<RsWikiSnapshot>::iterator vit;
        if (!rsWiki->getRelatedSnapshots(token, snapshots))
	{
		// ERROR
		std::cerr << "WikiEditDialog::loadBaseHistory() ERROR";
		std::cerr << std::endl;
		return;
	}

	for(vit = snapshots.begin(); vit != snapshots.end(); ++vit)
	{
                RsWikiSnapshot &page = *vit;

	        std::cerr << "WikiEditDialog::loadBaseHistory() TopLevel Result: PageTitle: " << page.mMeta.mMsgName;
	        std::cerr << " GroupId: " << page.mMeta.mGroupId;
	        std::cerr << std::endl;
	        std::cerr << "\tOrigMsgId: " << page.mMeta.mOrigMsgId;
	        std::cerr << " MsgId: " << page.mMeta.mMsgId;
	        std::cerr << std::endl;
	        std::cerr << "\tThreadId: " << page.mMeta.mThreadId;
	        std::cerr << " ParentId: " << page.mMeta.mParentId;
	        std::cerr << std::endl;
		
		GxsIdRSTreeWidgetItem *modItem = new GxsIdRSTreeWidgetItem(mThreadCompareRole, GxsIdDetails::ICON_TYPE_AVATAR);
        modItem->setData(WET_DATA_COLUMN, WET_ROLE_ORIGPAGEID, QString::fromStdString(page.mMeta.mOrigMsgId.toStdString()));
        modItem->setData(WET_DATA_COLUMN, WET_ROLE_PAGEID, QString::fromStdString(page.mMeta.mMsgId.toStdString()));

        modItem->setData(WET_DATA_COLUMN, WET_ROLE_PARENTID, QString::fromStdString(page.mMeta.mParentId.toStdString()));

		{
			// From Forum stuff.
			QDateTime qtime;
			QString text;
			QString sort;
			qtime.setTime_t(page.mMeta.mPublishTs);	
			sort = qtime.toString("yyyyMMdd_hhmmss");
			text = qtime.toString("dd/MM/yy hh:mm");
			
			modItem->setText(WET_COL_DATE, text);
			modItem->setData(WET_COL_DATE, WET_ROLE_SORT, sort);
		}
		modItem->setId(page.mMeta.mAuthorId, WET_COL_AUTHORID, false);
        modItem->setText(WET_COL_PAGEID, QString::fromStdString(page.mMeta.mMsgId.toStdString()));

		ui.treeWidget_History->addTopLevelItem(modItem);
        }

	/* then we need to request all pages from this thread */
	requestEditTreeData();
}


void WikiEditDialog::requestEditTreeData() //const RsGxsGroupId &groupId)
{
	// SWITCH THIS TO A THREAD REQUEST - WHEN WE CAN!

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
        opts.mOptions = RS_TOKREQOPT_MSG_LATEST;

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(mThreadMsgIdPair.first);

	uint32_t token;
	mWikiQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, WIKIEDITDIALOG_EDITTREE);
}


void WikiEditDialog::loadEditTreeData(const uint32_t &token)
{
	std::cerr << "WikiEditDialog::loadEditTreeData()";
	std::cerr << std::endl;


	std::vector<RsWikiSnapshot> snapshots;
	std::vector<RsWikiSnapshot>::iterator vit;
        if (!rsWiki->getSnapshots(token, snapshots))
	{
		// ERROR
		std::cerr << "WikiEditDialog::loadEditTreeData() ERROR";
		std::cerr << std::endl;
		return;
	}

	std::cerr << "WikiEditDialog::loadEditTreeData() Loaded " << snapshots.size();
	std::cerr << std::endl;
	std::cerr << "WikiEditDialog::loadEditTreeData() Using ThreadId: " << mThreadMsgIdPair.second;
	std::cerr << std::endl;


	std::map<RsGxsMessageId, QTreeWidgetItem *> items;
	std::map<RsGxsMessageId, QTreeWidgetItem *>::iterator iit;
	std::list<QTreeWidgetItem *> unparented;
	std::list<QTreeWidgetItem *>::iterator uit;

	// Grab the existing TopLevelItems, and insert into map.
       	int itemCount = ui.treeWidget_History->topLevelItemCount();
       	for (int nIndex = 0; nIndex < itemCount; ++nIndex)
       	{
		QTreeWidgetItem *item = ui.treeWidget_History->topLevelItem(nIndex);

		/* index by MsgId --> ONLY For Wiki Thread Head Items... SPECIAL HACK FOR HERE! */	
        RsGxsMessageId msgId ( item->data(WET_DATA_COLUMN, WET_ROLE_PAGEID).toString().toStdString() );
		items[msgId] = item;
	}


	for(vit = snapshots.begin(); vit != snapshots.end(); ++vit)
	{
                RsWikiSnapshot &snapshot = *vit;
	
	        std::cerr << "Result: PageTitle: " << snapshot.mMeta.mMsgName;
	        std::cerr << " GroupId: " << snapshot.mMeta.mGroupId;
	        std::cerr << std::endl;
	        std::cerr << "\tOrigMsgId: " << snapshot.mMeta.mOrigMsgId;
	        std::cerr << " MsgId: " << snapshot.mMeta.mMsgId;
	        std::cerr << std::endl;
	        std::cerr << "\tThreadId: " << snapshot.mMeta.mThreadId;
	        std::cerr << " ParentId: " << snapshot.mMeta.mParentId;
	        std::cerr << std::endl;

        if (snapshot.mMeta.mParentId.isNull())
		{
			/* Ignore! */
	        	std::cerr << "Ignoring ThreadHead Item";
	        	std::cerr << std::endl;
			continue;
		}

		if (snapshot.mMeta.mThreadId != mThreadMsgIdPair.second)
		{
			/* Ignore! */
	        	std::cerr << "Ignoring Different Thread Item";
	        	std::cerr << std::endl;
			continue;
		}

		/* create an Entry */
		GxsIdRSTreeWidgetItem *modItem = new GxsIdRSTreeWidgetItem(mThreadCompareRole, GxsIdDetails::ICON_TYPE_AVATAR);
        modItem->setData(WET_DATA_COLUMN, WET_ROLE_ORIGPAGEID, QString::fromStdString(snapshot.mMeta.mOrigMsgId.toStdString()));
        modItem->setData(WET_DATA_COLUMN, WET_ROLE_PAGEID, QString::fromStdString(snapshot.mMeta.mMsgId.toStdString()));
        modItem->setData(WET_DATA_COLUMN, WET_ROLE_PARENTID, QString::fromStdString(snapshot.mMeta.mParentId.toStdString()));

		{
			// From Forum stuff.
			QDateTime qtime;
			QString text;
			QString sort;
			qtime.setTime_t(snapshot.mMeta.mPublishTs);	
			sort = qtime.toString("yyyyMMdd_hhmmss");
			text = qtime.toString("dd/MM/yy hh:mm");
			
			modItem->setText(WET_COL_DATE, text);
			modItem->setData(WET_COL_DATE, WET_ROLE_SORT, sort);
		}
		modItem->setId(snapshot.mMeta.mAuthorId, WET_COL_AUTHORID, false);
        modItem->setText(WET_COL_PAGEID, QString::fromStdString(snapshot.mMeta.mMsgId.toStdString()));

		/* find the parent */
		iit = items.find(snapshot.mMeta.mParentId);
		if (iit != items.end())
		{
			(iit->second)->addChild(modItem);
		}
		else
		{
			unparented.push_back(modItem);
		}
		items[snapshot.mMeta.mOrigMsgId] = modItem;
	}

	for(uit = unparented.begin(); uit != unparented.end(); ++uit)
	{
        RsGxsMessageId parentId ( (*uit)->data(WET_DATA_COLUMN, WET_ROLE_PARENTID).toString().toStdString());


		iit = items.find(parentId);
		if (iit != items.end())
		{
			(iit->second)->addChild(*uit);
		}
		else
		{
			/* ERROR */
			std::cerr << "Unparented!!!";
			std::cerr << std::endl;
		}
	}

	// Enable / Disable Items.
	mHistoryLoaded = true;
	updateHistoryStatus();
}



void WikiEditDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "WikiEditDialog::loadRequest()";
	std::cerr << std::endl;
		
	if (queue == mWikiQueue)
	{
		switch(req.mUserType)
		{
			case WIKIEDITDIALOG_GROUP:
				loadGroup(req.mToken);
				break;

			case WIKIEDITDIALOG_PAGE:
				loadPage(req.mToken);
				break;

			case WIKIEDITDIALOG_BASEHISTORY:
				loadBaseHistory(req.mToken);
				break;

			case WIKIEDITDIALOG_EDITTREE:
				loadEditTreeData(req.mToken);
				break;
			default:
				std::cerr << "WikiEditDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
			break;
		}
	}
}









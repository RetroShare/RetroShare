/*******************************************************************************
 * gui/WikiPoos/WikiEditDialog.h                                               *
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

#ifndef MRK_WIKI_EDIT_DIALOG_H
#define MRK_WIKI_EDIT_DIALOG_H

#include "ui_WikiEditDialog.h"

#include <retroshare/rswiki.h>
#include "util/TokenQueue.h"

class RSTreeWidgetItemCompareRole;

class WikiEditDialog : public QWidget, public TokenResponse
{
  Q_OBJECT

public:
	WikiEditDialog(QWidget *parent = 0);
	~WikiEditDialog();

void 	setNewPage();

void 	setupData(const RsGxsGroupId &groupId, const RsGxsMessageId &pageId);
void 	loadRequest(const TokenQueue *queue, const TokenRequest &req);

void 	setRepublishMode(RsGxsMessageId &origMsgId);

private slots:

void 	cancelEdit();
void 	revertEdit();
void 	submitEdit();
void 	previewToggle();
void 	historyToggle();
void  	detailsToggle();
void 	textChanged();
void    textReset();

void 	historySelected();
void 	oldHistoryChanged();
void  	mergeModeToggle();
void  	generateMerge();

private:

void 	updateHistoryStatus();
void 	updateHistoryChildren(QTreeWidgetItem *item, bool isLatest);
void 	updateHistoryItem(QTreeWidgetItem *item, bool isLatest);

void    redrawPage();

void 	setGroup(RsWikiCollection &group);
void 	setPreviousPage(RsWikiSnapshot &page);

void 	requestPage(const RsGxsGrpMsgIdPair &msgId);
void 	loadPage(const uint32_t &token);
void 	requestGroup(const RsGxsGroupId &groupId);
void 	loadGroup(const uint32_t &token);

void 	requestBaseHistory(const RsGxsGrpMsgIdPair &origMsgId);
void 	loadBaseHistory(const uint32_t &token);
void 	requestEditTreeData();
void 	loadEditTreeData(const uint32_t &token);



        bool mNewPage;

	bool mPreviewMode;
	bool mPageLoading;
        bool mHistoryLoaded;
        bool mHistoryMergeMode;
        bool mOldHistoryEnabled;

        bool mRepublishMode;
	bool mIgnoreTextChange; // when we do it programmatically.
        bool mTextChanged;

	QString mCurrentText;

	RsGxsGrpMsgIdPair mThreadMsgIdPair;
	RsGxsMessageId mRepublishOrigId;

	RsWikiCollection mWikiCollection;
	RsWikiSnapshot mWikiSnapshot;

	Ui::WikiEditDialog ui;

	RSTreeWidgetItemCompareRole *mThreadCompareRole;

	TokenQueue *mWikiQueue;
};

#endif


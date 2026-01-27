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

#include <map>
#include <vector>

#include <retroshare/rswiki.h>

class RSTreeWidgetItemCompareRole;
class QTreeWidgetItem;

class WikiEditDialog : public QWidget
{
  Q_OBJECT

public:
	WikiEditDialog(QWidget *parent = 0);
	~WikiEditDialog();

void 	setNewPage();

void 	setupData(const RsGxsGroupId &groupId, const RsGxsMessageId &pageId);

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

void 	setGroup(const RsWikiCollection &group);
void 	setPreviousPage(const RsWikiSnapshot &page);

void 	requestPage(const RsGxsGrpMsgIdPair &msgId);
void 	loadPage(const RsWikiSnapshot &page);
void 	requestGroup(const RsGxsGroupId &groupId);
void 	loadGroup(const RsWikiCollection &group);

void 	requestBaseHistory(const RsGxsGrpMsgIdPair &origMsgId);
void 	loadBaseHistory(const std::vector<RsWikiSnapshot> &snapshots);
void 	requestEditTreeData();
void 	loadEditTreeData(const std::vector<RsWikiSnapshot> &snapshots);

void 	performMerge(
		const std::vector<RsGxsMessageId> &editIds,
		const std::map<RsGxsMessageId, std::string> &contents);
rstime_t getEditTimestamp(const RsGxsMessageId &msgId) const;
QString getAuthorName(const RsGxsMessageId &msgId) const;
QTreeWidgetItem *findHistoryItem(const RsGxsMessageId &msgId) const;


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
};

#endif

/*
 * Retroshare Circles.
 *
 * Copyright 2012-2013 by Robert Fernie.
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

#ifndef MRK_CREATE_CIRCLE_DIALOG_H
#define MRK_CREATE_CIRCLE_DIALOG_H

#include "ui_CreateCircleDialog.h"

#include "util/TokenQueue.h"

#include <retroshare/rsgxscircles.h>
#include <QDialog>

class CreateCircleDialog : public QDialog, public TokenResponse
{
	Q_OBJECT

public:
	CreateCircleDialog();
	~CreateCircleDialog();

#if 0
	void newMsg(); /* cleanup */
#endif

	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:

	void addMember();
	void removeMember();

	void selectedId(QTreeWidgetItem*, QTreeWidgetItem*);
	void selectedMember(QTreeWidgetItem*, QTreeWidgetItem*);

#if 0
	/** Create the context popup menu and it's submenus */
	void forumMessageCostumPopupMenu( QPoint point );

	void fileHashingFinished(QList<HashedFile> hashedFiles);
	/* actions to take.... */
	void createMsg();
	void pasteLink();
	void pasteLinkFull();
	void pasteOwnCertificateLink();

	void smileyWidgetForums();
	void addSmileys();
	void addFile();
#endif

protected:

#if 0
	void closeEvent (QCloseEvent * event);
#endif

private:

#if 0
	void saveForumInfo(const RsGroupMetaData &meta);
	void saveParentMsg(const RsGxsForumMsg &msg);
	void loadFormInformation();

	void loadForumInfo(const uint32_t &token);
	void loadParentMsg(const uint32_t &token);

	std::string mForumId;
	std::string mParentId;

	bool mParentMsgLoaded;
	bool mForumMetaLoaded;
	RsGxsForumMsg mParentMsg;
	RsGroupMetaData mForumMeta;
#endif

	void  createCircle();

	void loadCircle(uint32_t token);
	void loadIdentities(uint32_t token);

	void requestCircle(const RsGxsGroupId &groupId);
	void requestIdentities();

	TokenQueue *mCircleQueue;
	TokenQueue *mIdQueue;

	/** Qt Designer generated object */
	Ui::CreateCircleDialog ui;
};

#endif

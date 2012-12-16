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


#ifndef _CREATE_GXSFORUM_MSG_DIALOG_H
#define _CREATE_GXSFORUM_MSG_DIALOG_H

#include "ui_CreateGxsForumMsg.h"

#include "util/TokenQueue.h"

#include <retroshare/rsgxsforums.h>

class CreateGxsForumMsg : public QDialog, public TokenResponse
{
  Q_OBJECT

public:
    CreateGxsForumMsg(const std::string &fId, const std::string &pId);

    void newMsg(); /* cleanup */
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:
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

protected:
    void closeEvent (QCloseEvent * event);

private:
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

	TokenQueue *mForumQueue;

    /** Qt Designer generated object */
    Ui::CreateGxsForumMsg ui;
};

#endif


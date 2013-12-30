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


#ifndef _CREATE_FORUM_MSG_DIALOG_H
#define _CREATE_FORUM_MSG_DIALOG_H

#include "ui_CreateForumMsg.h"

class CreateForumMsg : public QDialog
{
  Q_OBJECT

public:
    CreateForumMsg(const std::string &fId, const std::string &pId);

    void newMsg(); /* cleanup */

private slots:
    void fileHashingFinished(QList<HashedFile> hashedFiles);
    /* actions to take.... */
    void createMsg();

    void smileyWidgetForums();
    void addSmileys();
    void addFile();
    void addPicture();

protected:
    void closeEvent (QCloseEvent * event);

private:
    std::string mForumId;
    std::string mParentId;

    /** Qt Designer generated object */
    Ui::CreateForumMsg ui;
};

#endif


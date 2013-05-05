/*
 * Retroshare Posted
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

#include <QMessageBox>
#include "PostedCreatePostDialog.h"
#include "ui_PostedCreatePostDialog.h"

#include <iostream>

PostedCreatePostDialog::PostedCreatePostDialog(TokenQueue* tokenQ, RsPosted *posted, const RsGxsGroupId& grpId, QWidget *parent):
    QDialog(parent), mTokenQueue(tokenQ), mPosted(posted), mGrpId(grpId),
    ui(new Ui::PostedCreatePostDialog)
{
    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(createPost()));

    /* fill in the available OwnIds for signing */
    ui->idChooser->loadIds(IDCHOOSER_ID_REQUIRED, "");

}

void PostedCreatePostDialog::createPost()
{
    RsGxsId authorId;
    if (!ui->idChooser->getChosenId(authorId))
    {
        std::cerr << "PostedCreatePostDialog::createPost() ERROR GETTING AuthorId!, Post Failed";
        std::cerr << std::endl;

        QMessageBox::warning(this, tr("RetroShare"),tr("Please create or choose a Signing Id first"),
            QMessageBox::Ok, QMessageBox::Ok);

	return;
    }


    RsPostedPost post;
    post.mMeta.mGroupId = mGrpId;
    post.mLink = std::string(ui->linkEdit->text().toUtf8());
    post.mNotes = std::string(ui->notesTextEdit->toPlainText().toUtf8());
    post.mMeta.mMsgName = std::string(ui->titleEdit->text().toUtf8());
    post.mMeta.mAuthorId = authorId;

    uint32_t token;
    mPosted->createPost(token, post);
    mTokenQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, TOKEN_USER_TYPE_POST);
    accept();
}

PostedCreatePostDialog::~PostedCreatePostDialog()
{
    delete ui;
}

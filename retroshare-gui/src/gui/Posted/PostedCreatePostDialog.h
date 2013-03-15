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

#ifndef POSTEDCREATEPOSTDIALOG_H
#define POSTEDCREATEPOSTDIALOG_H

#include <QDialog>
#include "retroshare/rsposted.h"
#include "PostedUserTypes.h"

#include "util/TokenQueue.h"

namespace Ui {
    class PostedCreatePostDialog;
}

class PostedCreatePostDialog : public QDialog
{
    Q_OBJECT

public:

    /*!
     * @param tokenQ parent callee token
     * @param posted
     */
    explicit PostedCreatePostDialog(TokenQueue* tokenQ, RsPosted* posted, const RsGxsGroupId& grpId, QWidget *parent = 0);
    ~PostedCreatePostDialog();

private slots:

    void createPost();

private:
    Ui::PostedCreatePostDialog *ui;

    QString mLink;
    QString mNotes;
    RsPosted* mPosted;
    RsGxsGroupId mGrpId;
    TokenQueue* mTokenQueue;
};

#endif // POSTEDCREATEPOSTDIALOG_H

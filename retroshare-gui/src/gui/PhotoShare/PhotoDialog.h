/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/PhotoDialog.h                             *
 *                                                                             *
 * Copyright (C) 2018 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#ifndef PHOTODIALOG_H
#define PHOTODIALOG_H

#include <QDialog>
#include <QSet>
#include "retroshare/rsphoto.h"
#include "util/TokenQueue.h"
#include "PhotoCommentItem.h"

namespace Ui {
    class PhotoDialog;
}

class PhotoDialog : public QDialog, public TokenResponse
{
    Q_OBJECT

public:
    explicit PhotoDialog(RsPhoto* rs_photo, const RsPhotoPhoto& photo, QWidget *parent = 0);
    ~PhotoDialog();

private slots:

    void addComment();
    void createComment();
	void setFullScreen();

public:
    void loadRequest(const TokenQueue *queue, const TokenRequest &req);
private:
    void setUp();

    /*!
     * clears comments
     * and places them back in dialog
     */
    void resetComments();

    /*!
     * Request comments
     */
    void requestComments();

    /*!
     * Simply removes comments but doesn't place them back in dialog
     */
    void clearComments();

    void acknowledgeComment(uint32_t token);
    void loadComment(uint32_t token);
    void loadList(uint32_t token);
    void addComment(const RsPhotoComment& comment);
private:
    Ui::PhotoDialog *ui;

    RsPhoto* mRsPhoto;
    TokenQueue* mPhotoQueue;
    RsPhotoPhoto mPhotoDetails;
    QSet<PhotoCommentItem*> mComments;
};

#endif // PHOTODIALOG_H

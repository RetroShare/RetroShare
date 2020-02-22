/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/AlbumCreateDialog.h                       *
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

#ifndef ALBUMCREATEDIALOG_H
#define ALBUMCREATEDIALOG_H

#include <QDialog>
#include "util/TokenQueue.h"
#include "retroshare/rsphoto.h"
#include "retroshare/rsphoto.h"
#include "PhotoShareItemHolder.h"
#include "PhotoItem.h"
#include "PhotoDrop.h"

namespace Ui {
    class AlbumCreateDialog;
}


class AlbumCreateDialog : public QDialog, public PhotoShareItemHolder
{
    Q_OBJECT

public:
    explicit AlbumCreateDialog(TokenQueue* photoQueue, RsPhoto* rs_photo, QWidget *parent = 0);
    ~AlbumCreateDialog();
    
    void notifySelection(PhotoShareItem* selection);
    

private slots:
    void publishAlbum();
    void publishPhotos();
    void addAlbumThumbnail();
	void changePage();
	void backPage();    
    

private:

    bool getAlbumThumbnail(RsGxsImage &image);
private:
    Ui::AlbumCreateDialog *ui;

    TokenQueue* mPhotoQueue;
    RsPhoto* mRsPhoto;
    QPixmap mThumbNail;
    PhotoDrop* mPhotoDrop;
    PhotoItem* mPhotoSelected;
};



#endif // ALBUMCREATEDIALOG_H

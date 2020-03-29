/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/AlbumDialog.cpp                           *
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

#include <QPixmap>

#include "AlbumDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "ui_AlbumDialog.h"
#include "retroshare/rsgxsflags.h"

AlbumDialog::AlbumDialog(const RsPhotoAlbum& album, TokenQueue* photoQueue, RsPhoto* rs_Photo, QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
    ui(new Ui::AlbumDialog), mRsPhoto(rs_Photo), mPhotoShareQueue(photoQueue), mAlbum(album), mPhotoSelected(NULL)
{
    ui->setupUi(this);
    
    ui->headerFrame->setHeaderImage(QPixmap(":/images/kview_64.png"));
    ui->headerFrame->setHeaderText(tr("Album"));

    connect(ui->pushButton_PublishPhotos, SIGNAL(clicked()), this, SLOT(updateAlbumPhotos()));
    connect(ui->pushButton_DeletePhoto, SIGNAL(clicked()), this, SLOT(deletePhoto()));

    mPhotoDrop = ui->scrollAreaWidgetContents;

    if (!(mAlbum.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN))
    {
        ui->scrollAreaPhotos->setEnabled(false);
        ui->pushButton_DeletePhoto->setEnabled(false);
    }
    mPhotoDrop->setPhotoItemHolder(this);

    // setup PhotoQueue for internal loading.
    mPhotoQueue = new TokenQueue(rsPhoto->getTokenService(), this);
    requestPhotoList(mAlbum.mMeta.mGroupId);

    // setup gui.
    setUp();
}


void AlbumDialog::setUp()
{
    ui->lineEdit_Title->setText(QString::fromStdString(mAlbum.mMeta.mGroupName));
    ui->lineEdit_Caption->setText(QString::fromStdString(mAlbum.mCaption));

    if (mAlbum.mThumbnail.mSize != 0)
    {
        QPixmap qtn;
        GxsIdDetails::loadPixmapFromData(mAlbum.mThumbnail.mData, mAlbum.mThumbnail.mSize,qtn, GxsIdDetails::ORIGINAL);
        ui->label_thumbNail->setPixmap(qtn);
    }
    else
    {
        // display a default Album icon when album has no Thumbnail
        ui->label_thumbNail->setPixmap(QPixmap(":/images/album_default_128.png"));
    }
}

void AlbumDialog::addPhoto(const RsPhotoPhoto &photo)
{
    mPhotoDrop->addPhotoItem(new PhotoItem(this, photo));
}

void AlbumDialog::updateAlbumPhotos()
{

    QSet<PhotoItem*> photos;

    mPhotoDrop->getPhotos(photos);

    QSetIterator<PhotoItem*> sit(photos);

    while (sit.hasNext())
    {
        PhotoItem* item = sit.next();
        uint32_t token;
        RsPhotoPhoto photo = item->getPhotoDetails();

        // ensure GroupId, AuthorId match Album.
        photo.mMeta.mGroupId = mAlbum.mMeta.mGroupId;
        photo.mMeta.mAuthorId = mAlbum.mMeta.mAuthorId;

        bool publish = true;
        switch(item->getState())
        {
            case PhotoItem::New:
            {
                // new photo will be published.
            }
            break;
            case PhotoItem::Existing:
            {
                publish = false;
            }
            break;
            case PhotoItem::Modified:
            {
                // update will be published.
            }
            break;
            case PhotoItem::Deleted:
            {
                // flag image as Deleted.
                // clear image.
                // clear file URL (todo)
                photo.mLowResImage.clear();
            }
            break;
        }

        if (publish) {
            mRsPhoto->submitPhoto(token, photo);
            mPhotoShareQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, 0);
        }
    }
    close();
}

void AlbumDialog::deletePhoto(){

    if (mPhotoSelected)
    {
        mPhotoSelected->setSelected(false);
        mPhotoSelected->markForDeletion();
    }
}

void AlbumDialog::editPhoto()
{

}

AlbumDialog::~AlbumDialog()
{
    delete ui;
}

void AlbumDialog::notifySelection(PhotoShareItem *selection)
{
    PhotoItem* pItem = dynamic_cast<PhotoItem*>(selection);

    if (mPhotoSelected  == NULL)
    {
        mPhotoSelected = pItem;
    }
    else
    {
        mPhotoSelected->setSelected(false);
        mPhotoSelected = pItem;
    }

    mPhotoSelected->setSelected(true);
}


/**************************** Request / Response Filling of Data ************************/

void AlbumDialog::requestPhotoList(GxsMsgReq& req)
{
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;
    uint32_t token;
    mPhotoQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts, req, 0);
    return;
}

void AlbumDialog::requestPhotoList(const RsGxsGroupId &albumId)
{
    std::list<RsGxsGroupId> grpIds;
    grpIds.push_back(albumId);
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;
    opts.mOptions = RS_TOKREQOPT_MSG_LATEST;
    uint32_t token;
    mPhotoQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_LIST, opts, grpIds, 0);
}

void AlbumDialog::acknowledgeMessage(const uint32_t &token)
{
    std::pair<RsGxsGroupId, RsGxsMessageId> p;
    rsPhoto->acknowledgeMsg(token, p);
}

void AlbumDialog::loadPhotoList(const uint32_t &token)
{
    GxsMsgIdResult res;

    rsPhoto->getMsgList(token, res);
    requestPhotoData(res);
}

void AlbumDialog::requestPhotoData(GxsMsgReq &photoIds)
{
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
    uint32_t token;
    mPhotoQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, photoIds, 0);
}

void AlbumDialog::loadPhotoData(const uint32_t &token)
{
    PhotoResult res;
    rsPhoto->getPhoto(token, res);
    PhotoResult::iterator mit = res.begin();

    for (; mit != res.end(); ++mit)
    {
        std::vector<RsPhotoPhoto>& photoV = mit->second;
        std::vector<RsPhotoPhoto>::iterator vit = photoV.begin();

        for (; vit != photoV.end(); ++vit)
        {
            RsPhotoPhoto& photo = *vit;
            if (!photo.mLowResImage.empty()) {
                addPhoto(photo);
            }
        }
    }
}


/**************************** Request / Response Filling of Data ************************/

void AlbumDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
    if (queue == mPhotoQueue)
    {
        /* now switch on req */
        switch(req.mType)
        {
            case TOKENREQ_MSGINFO:
                switch(req.mAnsType)
                {
                    case RS_TOKREQ_ANSTYPE_LIST:
                        loadPhotoList(req.mToken);
                        break;
                    case RS_TOKREQ_ANSTYPE_ACK:
                        acknowledgeMessage(req.mToken);
                        break;
                    case RS_TOKREQ_ANSTYPE_DATA:
                        loadPhotoData(req.mToken);
                        break;
                    default:
                        std::cerr << "PhotoShare::loadRequest() ERROR: MSG: INVALID ANS TYPE";
                        std::cerr << std::endl;
                        break;
                }
                break;
            case TOKENREQ_MSGRELATEDINFO:
                switch(req.mAnsType)
                {
                    case RS_TOKREQ_ANSTYPE_DATA:
                        loadPhotoData(req.mToken);
                        break;
                    default:
                        std::cerr << "PhotoShare::loadRequest() ERROR: MSG: INVALID ANS TYPE";
                        std::cerr << std::endl;
                        break;
                }
                break;
            default:
                std::cerr << "PhotoShare::loadRequest() ERROR: INVALID TYPE";
                std::cerr << std::endl;
                break;
        }
    }
}

/**************************** Request / Response Filling of Data ************************/

/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/PhotoItem.cpp                             *
 *                                                                             *
 * Copyright (C) 2012 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#include <QMouseEvent>
#include <QBuffer>
#include <iostream>
#include <QLabel>

#include "PhotoItem.h"
#include "ui_PhotoItem.h"

PhotoItem::PhotoItem(PhotoShareItemHolder *holder, const RsPhotoPhoto &photo, QWidget *parent):
    QWidget(parent),
    ui(new Ui::PhotoItem), mHolder(holder), mPhotoDetails(photo)
{

    ui->setupUi(this);
    setSelected(false);

    ui->lineEdit_PhotoGrapher->setEnabled(false);
    ui->lineEdit_Title->setEnabled(false);

    ui->editLayOut->removeWidget(ui->lineEdit_Title);
    ui->editLayOut->removeWidget(ui->lineEdit_PhotoGrapher);

    ui->lineEdit_Title->setVisible(false);
    ui->lineEdit_PhotoGrapher->setVisible(false);

    setUp();

    ui->idChooser->setVisible(false);
}

PhotoItem::PhotoItem(PhotoShareItemHolder *holder, const QString& path, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PhotoItem), mHolder(holder)
{
    ui->setupUi(this);


    QPixmap qtn = QPixmap(path);
    mThumbNail = qtn;
    ui->label_Thumbnail->setPixmap(mThumbNail.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    setSelected(false);

    getPhotoThumbnail(mPhotoDetails.mThumbnail);

    connect(ui->lineEdit_Title, SIGNAL(editingFinished()), this, SLOT(setTitle()));
    connect(ui->lineEdit_PhotoGrapher, SIGNAL(editingFinished()), this, SLOT(setPhotoGrapher()));

    ui->idChooser->loadIds(0, RsGxsId());

}

void PhotoItem::setSelected(bool selected)
{
    mSelected = selected;
    if (mSelected)
    {
            ui->photoFrame->setStyleSheet("QFrame#photoFrame{border: 2px solid #9562B8;\nbackground: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #55EE55, stop: 1 #CCCCCC);\nborder-radius: 10px}");
    }
    else
    {
            ui->photoFrame->setStyleSheet("QFrame#photoFrame{border: 2px solid #CCCCCC;\nbackground: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #EEEEEE, stop: 1 #CCCCCC);\nborder-radius: 10px}");
    }
    update();
}

bool PhotoItem::getPhotoThumbnail(RsPhotoThumbnail &nail)
{
        const QPixmap *tmppix = &mThumbNail;

        QByteArray ba;
        QBuffer buffer(&ba);

        if(!tmppix->isNull())
        {
                // send chan image

                buffer.open(QIODevice::WriteOnly);
                tmppix->save(&buffer, "PNG"); // writes image into ba in PNG format

                RsPhotoThumbnail tmpnail;
                tmpnail.data = (uint8_t *) ba.data();
                tmpnail.size = ba.size();

                nail.copyFrom(tmpnail);

                return true;
        }

        nail.data = NULL;
        nail.size = 0;
        return false;
}



void PhotoItem::setTitle(){

    mPhotoDetails.mMeta.mMsgName = ui->lineEdit_Title->text().toStdString();
}

void PhotoItem::setPhotoGrapher()
{
    mPhotoDetails.mPhotographer = ui->lineEdit_PhotoGrapher->text().toStdString();
}

const RsPhotoPhoto& PhotoItem::getPhotoDetails()
{


	if (ui->idChooser->isVisible()) {
        RsGxsId id;
		switch (ui->idChooser->getChosenId(id)) {
			case GxsIdChooser::KnowId:
			case GxsIdChooser::NoId:
			case GxsIdChooser::UnKnowId:
        mPhotoDetails.mMeta.mAuthorId = id;

			break;
			case GxsIdChooser::None:
			default:
			break;
		}//switch (ui->idChooser->getChosenId(id))
	}//if (ui->idChooser->isVisible())

    return mPhotoDetails;
}

PhotoItem::~PhotoItem()
{
    delete ui;
}

void PhotoItem::setUp()
{

    mTitleLabel = new QLabel();
    mPhotoGrapherLabel = new QLabel();

    mTitleLabel->setText(QString::fromStdString(mPhotoDetails.mMeta.mMsgName));
    mPhotoGrapherLabel->setText(QString::fromStdString(mPhotoDetails.mPhotographer));


    ui->editLayOut->addWidget(mPhotoGrapherLabel);
    ui->editLayOut->addWidget(mTitleLabel);

    updateImage(mPhotoDetails.mThumbnail);
}

void PhotoItem::updateImage(const RsPhotoThumbnail &thumbnail)
{
    if (thumbnail.data != NULL)
    {
            QPixmap qtn;
            qtn.loadFromData(thumbnail.data, thumbnail.size, thumbnail.type.c_str());
            ui->label_Thumbnail->setPixmap(qtn.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            mThumbNail = qtn;
    }
}

void PhotoItem::mousePressEvent(QMouseEvent *event)
{

        QPoint pos = event->pos();

        std::cerr << "PhotoItem::mousePressEvent(" << pos.x() << ", " << pos.y() << ")";
        std::cerr << std::endl;

        if(mHolder)
            mHolder->notifySelection(this);
        else
            setSelected(true);

        QWidget::mousePressEvent(event);
}

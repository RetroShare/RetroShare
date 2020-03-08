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
    mState = State::Existing;

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

PhotoItem::PhotoItem(PhotoShareItemHolder *holder, const QString& path, uint32_t order, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PhotoItem), mHolder(holder)
{
    mState = State::New;

    ui->setupUi(this);

    mPhotoDetails.mOrder = order;

    QPixmap qtn = QPixmap(path);
    mLowResImage = qtn.scaled(512,512, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    ui->label_Thumbnail->setPixmap(qtn.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    setSelected(false);

    getLowResImage(mPhotoDetails.mLowResImage);

    connect(ui->lineEdit_Title, SIGNAL(editingFinished()), this, SLOT(setTitle()));
    connect(ui->lineEdit_PhotoGrapher, SIGNAL(editingFinished()), this, SLOT(setPhotoGrapher()));

    ui->idChooser->loadIds(0, RsGxsId());
}

void PhotoItem::markForDeletion()
{
    mState = State::Deleted;
    setSelected(true); // to repaint with deleted scheme.
}

void PhotoItem::setSelected(bool selected)
{
    mSelected = selected;

	QString bottomColor;
	switch(mState)
	{
		case State::New:
			bottomColor = "#FFFFFF";
			break;
		case State::Existing:
			bottomColor = "#888888";
			break;
		default:
		case State::Deleted:
			bottomColor = "#EE5555";
			break;
	}

    if (mSelected)
    {
            ui->photoFrame->setStyleSheet("QFrame#photoFrame{border: 2px solid #9562B8;\nbackground: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #55EE55, stop: 1 " + bottomColor + ");\nborder-radius: 10px}");
    }
    else
    {
            ui->photoFrame->setStyleSheet("QFrame#photoFrame{border: 2px solid #CCCCCC;\nbackground: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #EEEEEE, stop: 1 " + bottomColor + ");\nborder-radius: 10px}");
    }
    update();
}

void PhotoItem::setTitle(){

    mPhotoDetails.mMeta.mMsgName = ui->lineEdit_Title->text().toStdString();
}

void PhotoItem::setPhotoGrapher()
{
    // mPhotoDetails.mPhotographer = ui->lineEdit_PhotoGrapher->text().toStdString();
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

    ui->editLayOut->addWidget(mPhotoGrapherLabel);
    ui->editLayOut->addWidget(mTitleLabel);

    updateImage(mPhotoDetails.mLowResImage);
}

bool PhotoItem::getLowResImage(RsGxsImage &image)
{
        const QPixmap *tmppix = &mLowResImage;

        QByteArray ba;
        QBuffer buffer(&ba);

        if(!tmppix->isNull())
        {
                buffer.open(QIODevice::WriteOnly);
                tmppix->save(&buffer, "JPG");
                image.copy((uint8_t *) ba.data(), ba.size());
                return true;
        }

        image.clear();
        return false;
}

void PhotoItem::updateImage(const RsGxsImage &image)
{
    if (image.mData != NULL)
    {
            QPixmap qtn;
            qtn.loadFromData(image.mData, image.mSize);
            ui->label_Thumbnail->setPixmap(qtn.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            mLowResImage = qtn;
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

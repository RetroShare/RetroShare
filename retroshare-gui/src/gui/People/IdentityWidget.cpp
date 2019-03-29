/*******************************************************************************
 * retroshare-gui/src/gui/People/IdentityWidget.cpp                            *
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

#include "retroshare/rsreputations.h"
#include "gui/People/IdentityWidget.h"
#include "ui_IdentityWidget.h"

#include "gui/common/AvatarDefs.h"
#include <gui/gxs/GxsIdDetails.h>

#include <QDrag>
#include <QMimeData>
#include <QGraphicsProxyWidget>

IdentityWidget::IdentityWidget(QString name/*=QString()*/, QWidget *parent/*=0*/) :
  FlowLayoutItem(name, parent),
  ui(new Ui::IdentityWidget)
{
	ui->setupUi(this);
	_haveGXSId = false;
	_havePGPDetail = false;

	m_myName = name;
	ui->labelName->setText(m_myName);
	ui->labelName->setToolTip(m_myName);
	QFont font = ui->labelName->font();
	font.setItalic(false);
	ui->labelName->setFont(font);

	_keyId="";
	ui->labelKeyId->setText(_keyId);
	ui->labelKeyId->setToolTip(_keyId);
	ui->labelKeyId->setVisible(false);

	ui->labelGXSId->setText(_keyId);
	ui->labelGXSId->setToolTip(_keyId);
	ui->labelGXSId->setVisible(false);
	
	ui->labelPositive->setVisible(false);
	ui->labelNegative->setVisible(false);
	ui->label_PosIcon_2->setVisible(false);
	ui->label_NegIcon_2->setVisible(false);

	ui->pbAdd->setVisible(false);
	QObject::connect(ui->pbAdd, SIGNAL(clicked()), this, SLOT(pbAdd_clicked()));

	_scene = new QGraphicsScene(this);
	ui->graphicsView->setScene(_scene);

	//To grab events
	ui->graphicsView->setEnabled(false);

	ui->graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);

	setIsCurrent(false);
	setIsSelected(false);
	setAcceptDrops(true);
}

IdentityWidget::~IdentityWidget()
{
	delete _scene;
	delete ui;
}

void IdentityWidget::updateData(const RsGxsIdGroup &gxs_group_info)
{
	//if (_group_info != gxs_group_info) {
		_group_info = gxs_group_info;
		_haveGXSId = true;

		RsReputationInfo info ;
		rsReputations->getReputationInfo(RsGxsId(_group_info.mMeta.mGroupId),_group_info.mPgpId,info) ;
		
		m_myName = QString::fromUtf8(_group_info.mMeta.mGroupName.c_str());
		//ui->labelName->setId(RsGxsId(_group_info.mMeta.mGroupId));
		ui->labelName->setText(m_myName);

		_gxsId = QString::fromStdString(_group_info.mMeta.mGroupId.toStdString());
		ui->labelGXSId->setId(RsGxsId(_group_info.mMeta.mGroupId));
		ui->labelGXSId->setText(_gxsId);

		ui->labelPositive->setText(QString::number(info.mFriendsPositiveVotes));
		ui->labelNegative->setText(QString::number(info.mFriendsNegativeVotes));

		if (!_havePGPDetail) {
			QFont font = ui->labelName->font();
			font.setItalic(false);
			ui->labelName->setFont(font);

			_keyId=QString::fromStdString(_group_info.mMeta.mGroupId.toStdString());
			ui->labelKeyId->setId(RsGxsId(_group_info.mMeta.mGroupId));
			ui->labelKeyId->setText(_keyId);
			ui->labelKeyId->setVisible(false);

    /// (TODO) Get real ident icon
		QImage image;
		
		if(_group_info.mImage.mSize > 0 && image.loadFromData(_group_info.mImage.mData, _group_info.mImage.mSize, "PNG"))
			image = image;
		else
			 image = GxsIdDetails::makeDefaultIcon(RsGxsId(_group_info.mMeta.mGroupId));
			
			if (_avatar != image) {
				_avatar = image;
				_scene->clear();
				_scene->addPixmap(QPixmap::fromImage(image.scaled(ui->graphicsView->width(),ui->graphicsView->height())));
				emit imageUpdated();
			}//if (_avatar != image)
		}//if (!_havePGPDetail)

	//}//if (_group_info != gxs_group_info)
}

void IdentityWidget::updateData(const RsPeerDetails &pgp_details)
{
	//if (_details != pgp_details) {
		_details = pgp_details;
		_havePGPDetail = true;

		_nickname = QString::fromUtf8(_details.name.c_str());
		if (!_haveGXSId) m_myName = _nickname;
		ui->labelName->setText(m_myName);
		if (_haveGXSId) {
			ui->labelName->setToolTip(tr("GXS name:") + (" "+m_myName) + ("\n")
			                          +(tr("PGP name:")+(" "+_nickname)));
		} else {//if (m_myName != _nickname)
			ui->labelName->setToolTip(tr("PGP name:")+(" "+_nickname));
		}//else (m_myName != _nickname)

		QFont font = ui->labelName->font();
		font.setItalic(true);
		ui->labelName->setFont(font);

		_keyId = QString::fromStdString(_details.gpg_id.toStdString());
		ui->labelKeyId->setText(_keyId);
		ui->labelKeyId->setToolTip(tr("PGP id:")+(" "+_keyId));

		if (!_haveGXSId) {
			QPixmap avatar;
			AvatarDefs::getAvatarFromGpgId(_details.gpg_id, avatar);
			if (_avatar != avatar.toImage()) {
				_avatar = avatar.toImage();
				_scene->clear();
				_scene->addPixmap(avatar.scaled(ui->graphicsView->width(),ui->graphicsView->height()));
				emit imageUpdated();
			}
		}
		

	//}//if (_details != gpg_details)
}

void IdentityWidget::updateData(const RsGxsIdGroup &gxs_group_info, const RsPeerDetails &pgp_details)
{
	updateData(gxs_group_info);
	updateData(pgp_details);
}

QSize IdentityWidget::sizeHint() const
{
	QSize size;
	size.setHeight(ui->graphicsView->size().height() + ui->labelName->size().height());
	size.setWidth(ui->graphicsView->size().width() > ui->labelName->size().width()
	              ?ui->graphicsView->size().width() : ui->labelName->size().width());
	return size;
}

const QPixmap IdentityWidget::getImage()
{
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
	return ui->graphicsView->grab();
	//return this->grab(); //QT5
#else
	return QPixmap::grabWidget(ui->graphicsView);
	//return QPixmap::grabWidget(this);
#endif
}

const QPixmap IdentityWidget::getDragImage()
{
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
	return ui->graphicsView->grab();
	//return this->grab(); //QT5
#else
	return QPixmap::grabWidget(ui->graphicsView);
	//return QPixmap::grabWidget(this);
#endif
}

void IdentityWidget::setIsSelected(bool value)
{
	m_isSelected=value;
	QFont font=ui->labelName->font();
	font.setBold(value);
	ui->labelName->setFont(font);
}
/*
bool IdentityWidget::isSelected()
{
	return m_isSelected;
}*/

void IdentityWidget::setIsCurrent(bool value)
{
	m_isCurrent=value;
	ui->labelKeyId->setVisible(value);
	ui->labelGXSId->setVisible(value && (_haveGXSId && _havePGPDetail));
	ui->labelPositive->setVisible(value);
	ui->labelNegative->setVisible(value);
	ui->label_PosIcon_2->setVisible(value);
	ui->label_NegIcon_2->setVisible(value);
	ui->pbAdd->setVisible(value);
}
/*
bool IdentityWidget::isCurrent()
{
	return m_isCurrent;
}*/

void IdentityWidget::pbAdd_clicked()
{
	emit addButtonClicked();
}


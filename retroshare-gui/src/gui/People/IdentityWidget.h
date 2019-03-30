/*******************************************************************************
 * retroshare-gui/src/gui/People/IdentityWidget.h                              *
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

#ifndef IDENTITYWIDGET_H
#define IDENTITYWIDGET_H

#include "gui/common/FlowLayout.h"
#include <QImage>
#include <QGraphicsScene>
#include <QWidget>
#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>

namespace Ui {
class IdentityWidget;
}

class IdentityWidget : public FlowLayoutItem
{
	Q_OBJECT

public:
	explicit IdentityWidget(QString name = QString()
	                        , QWidget *parent = 0);
	~IdentityWidget();
	void updateData(const RsGxsIdGroup& gxs_group_info);
	void updateData(const RsPeerDetails& pgp_details);
	void updateData(const RsGxsIdGroup& gxs_group_info
	          , const RsPeerDetails& pgp_details);

	//Start QWidget Properties
	QSize sizeHint() const;
	//Start FlowLayoutItem Properties
	virtual const QPixmap getImage();
	virtual const QPixmap getDragImage();
	virtual void setIsSelected(bool value);
	virtual void setIsCurrent(bool value);
	//End Properties

	bool haveGXSId() { return _haveGXSId; }
	bool havePGPDetail() { return _havePGPDetail; }
	const RsGxsIdGroup& groupInfo() const { return _group_info; }
	const RsPeerDetails& details() const { return _details; }
	const QString keyId() const { return _keyId; }
	const QString idtype() const { return _idtype; }
	const QString nickname() const { return _nickname; }
	const QString gxsId() const { return _gxsId; }
	const QImage avatar() const { return _avatar; }

signals:
	void addButtonClicked();

private slots:
	void pbAdd_clicked();

private:
	bool _haveGXSId;
	bool _havePGPDetail;
	RsGxsIdGroup _group_info;
	RsPeerDetails _details;
	QGraphicsScene* _scene;
	QImage _avatar;

	QString _keyId;
	QString _idtype;
	QString _nickname;
	QString _gxsId;
	QString _postiveVotes;
	QString _negativeVotes;

	Ui::IdentityWidget *ui;
};

#endif // IDENTITYWIDGET_H

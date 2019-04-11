/*******************************************************************************
 * retroshare-gui/src/gui/People/CircleWidget.h                                *
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

#ifndef CIRCLEWIDGET_H
#define CIRCLEWIDGET_H

#include "gui/common/FlowLayout.h"
#include "gui/People/IdentityWidget.h"
#include <QWidget>
#include <QGraphicsScene>

#include <retroshare/rsgxscircles.h>

namespace Ui {
class CircleWidget;
}

class CircleWidget : public FlowLayoutItem
{
	Q_OBJECT

public:
	explicit CircleWidget(QString name = QString()
	                    , QWidget *parent = 0);
	~CircleWidget();
	void updateData(const RsGroupMetaData& gxs_group_info
	          , const RsGxsCircleDetails& details);

	//Start QWidget Properties
	QSize sizeHint() const;
	//Start FlowLayoutItem Properties
	virtual const QPixmap getImage();
	virtual const QPixmap getDragImage();
	//End Properties

	void addIdent(IdentityWidget* item);
	const QMap<std::string, IdentityWidget*> idents() const {return _list;}

	const RsGroupMetaData& groupInfo() const { return _group_info;}
	const RsGxsCircleDetails& circleDetails() const {return _circle_details;}

signals:
	void askForGXSIdentityWidget(RsGxsId gxs_id);
	void askForPGPIdentityWidget(RsPgpId pgp_id);

private slots:
	void updateIdImage();

private:
	void updateScene();

	QGraphicsScene* _scene;
	QMap<std::string, IdentityWidget*> _list;
	RsGroupMetaData _group_info ;
	RsGxsCircleDetails _circle_details ;

	Ui::CircleWidget *ui;
};

#endif // CIRCLEWIDGET_H

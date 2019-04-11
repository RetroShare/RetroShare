/*******************************************************************************
 * retroshare-gui/src/gui/People/CircleWidget.cpp                              *
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

#include "gui/gxs/GxsIdDetails.h"
#include "gui/People/CircleWidget.h"
#include "ui_CircleWidget.h"
#include <retroshare/rspeers.h>
#include <QDrag>
#include <QMimeData>
#include <QGraphicsProxyWidget>
#include <QtCore/qmath.h>

CircleWidget::CircleWidget(QString name/*=QString()*/
                         , QWidget *parent/*=0*/) :
  FlowLayoutItem(name, parent),
  ui(new Ui::CircleWidget)
{
	ui->setupUi(this);
	m_myName = name;
	ui->label->setText(m_myName);
	ui->label->setToolTip(m_myName);
	_scene = new QGraphicsScene(this);
	_scene->addText(tr("Empty Circle"));
	ui->graphicsView->setScene(_scene);

	//To grab events
	ui->graphicsView->setEnabled(false);

	ui->graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	
	ui->graphicsView->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );

	setIsCurrent(false);
	setIsSelected(false);
	setAcceptDrops(true);
}

CircleWidget::~CircleWidget()
{
	delete _scene;
	delete ui;
}

static bool same_RsGxsCircleDetails(const RsGxsCircleDetails& d1,const RsGxsCircleDetails& d2)
{
		return (  d1.mCircleId     == d2.mCircleId
		       && d1.mCircleName   == d2.mCircleName
		       && d1.mCircleType   == d2.mCircleType
		       && d1.mSubscriptionFlags   == d2.mSubscriptionFlags
		       && d1.mAllowedGxsIds== d2.mAllowedGxsIds
		       && d1.mAllowedNodes == d2.mAllowedNodes
		      );
}

void CircleWidget::updateData(const RsGroupMetaData& gxs_group_info
                            , const RsGxsCircleDetails& details)
{
	//if (_group_info != gxs_group_info) {
		_group_info=gxs_group_info;
		std::string desc_string = _group_info.mGroupName ;
		QString cirName = QString::fromUtf8(desc_string.c_str());
		m_myName = cirName;
		ui->label->setText(m_myName);
		ui->label->setToolTip(m_myName);
		update();
	//}

	if(!same_RsGxsCircleDetails(_circle_details , details)) 
    {
		_circle_details=details;
		typedef std::set<RsGxsId>::iterator itUnknownPeers;
		for (itUnknownPeers it = _circle_details.mAllowedGxsIds.begin()
		     ; it != _circle_details.mAllowedGxsIds.end()
		     ; ++it) {
			RsGxsId gxs_id = *it;
			if(!gxs_id.isNull()) {
				emit askForGXSIdentityWidget(gxs_id);
			}
		}

		typedef std::set<RsPgpId>::const_iterator itAllowedPeers;
		for (itAllowedPeers it = _circle_details.mAllowedNodes.begin() ; it != _circle_details.mAllowedNodes.end() ; ++it ) 
        {
			RsPgpId pgp_id = *it;
			emit askForPGPIdentityWidget(pgp_id);

//			std::set<RsGxsId> gxs_id_list = it->second;
//			typedef std::set<RsGxsId>::const_iterator itGxsId;
//			for (itGxsId curs=gxs_id_list.begin(); curs != gxs_id_list.end(); ++curs) 
//            		{
//				RsGxsId gxs_id = *curs;
//				if(!gxs_id.isNull()) 
//					emit askForGXSIdentityWidget(gxs_id);
//				}
//			}
		}

		update();
	}
}

QSize CircleWidget::sizeHint() const
{
	QSize size;
	size.setHeight(ui->graphicsView->size().height() + ui->label->size().height());
	size.setWidth(ui->graphicsView->size().width() > ui->label->size().width()
	              ?ui->graphicsView->size().width() : ui->label->size().width());
	return size;
}

const QPixmap CircleWidget::getImage()
{
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
	//return ui->graphicsView->grab(); //QT5
	return this->grab(); //QT5
#else
	//return QPixmap::grabWidget(ui->graphicsView);
	return QPixmap::grabWidget(this);
#endif
}

const QPixmap CircleWidget::getDragImage()
{
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
	return ui->graphicsView->grab();
	//return this->grab(); //QT5
#else
	return QPixmap::grabWidget(ui->graphicsView);
	//return QPixmap::grabWidget(this);
#endif
}

void CircleWidget::addIdent(IdentityWidget *item)
{
	if (item){
		std::string id;
		if (item->haveGXSId()) {
			id =item->groupInfo().mMeta.mGroupId.toStdString();
		} else {//if (item->haveGXSId())
			id =item->keyId().toStdString();
		}//else (item->haveGXSId())
		if (!_list.contains(id)){
			_list[id] = item;
			connect(item,SIGNAL(imageUpdated()), this, SLOT(updateIdImage()));
			updateScene();
		}//if (!_list.contains(id))
	}//if (item)
}

void CircleWidget::updateIdImage()
{
	updateScene();
}

void CircleWidget::updateScene()
{
	const qreal PI = qAtan(1.0)*4;
	int count=_list.size();
	qreal pitch = (2*PI) / count;

	_scene->clear();

	QRect r = ui->graphicsView->geometry();
	QBrush b = QBrush(QColor(Qt::black));
	QPen p = QPen(b, 4.0);
	qreal topleftX = r.width()/8;
	qreal topleftY = r.height()/8;
	qreal radiusX = 3*topleftX;
	qreal radiusY = 3*topleftY;
	QGraphicsEllipseItem* ellipse = _scene->addEllipse(0, 0, radiusX*2, radiusY*2, p);
	ellipse->setPos(topleftX, topleftY);
	qreal sizeX = topleftX*2;
	qreal sizeY = topleftY*2;

	int curs = 0;
	typedef QMap<std::string, IdentityWidget*>::const_iterator itList;
	for (itList it=_list.constBegin(); it!=_list.constEnd(); ++it){
		QPixmap pix = it.value()->getImage();
		pix = pix.scaled(sizeX, sizeY);
		QGraphicsPixmapItem* item = _scene->addPixmap(pix);
		qreal x = (qCos(curs*pitch)*radiusX)-(sizeX/2)+topleftX+radiusX;
		qreal y = (qSin(curs*pitch)*radiusY)-(sizeY/2)+topleftY+radiusY;
		item->setPos(QPointF(x, y));

		QString name = it.value()->getName();
		QString idKey = it.value()->keyId();
		QString gxsId = it.value()->gxsId();
		if (idKey == gxsId) gxsId.clear();
		item->setToolTip(name.append("\n")
		                 .append(idKey).append(gxsId.isEmpty()?"":"\n")
		                 .append(gxsId));
		++curs;
	}//for (itList it=_list.constBegin(); it!=_list.constEnd(); ++it)
	emit imageUpdated();
}

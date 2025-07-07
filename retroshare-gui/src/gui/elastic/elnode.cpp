/*******************************************************************************
 * gui/elastic/elnode.cpp                                                      *
 *                                                                             *
 * Copyright (c) 2012, RetroShare Team <retroshare.project@gmail.com>          *
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

// This code is inspired from http://doc.qt.io/qt-5/qtwidgets-graphicsview-elasticnodes-node-cpp.html

#include "gui/common/FilesDefs.h"
#include "gui/settings/rsharesettings.h"
#include <math.h>

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QAction>
#include <QMenu>
#include <QStyleOption>
#include <iostream>

#include <gui/connect/PGPKeyDialog.h>

#include <retroshare/rspeers.h>
#include "edge.h"
#include "elnode.h"
#include "graphwidget.h"

#define IMAGE_AUTHED         ":/images/accepted16.png"
#define IMAGE_DENIED         ":/images/denied16.png"
#define IMAGE_TRUSTED        ":/images/rs-2.png"
#define IMAGE_MAKEFRIEND     ":/images/user/add_user16.png"

Node *Node::_selected_node = NULL ;

Node::Node(const std::string& node_string,GraphWidget::NodeType type,GraphWidget::AuthType auth,GraphWidget *graphWidget,const RsPeerId& ssl_id,const RsPgpId& gpg_id)
    : graph(graphWidget),_desc_string(node_string),_type(type),_auth(auth),_ssl_id(ssl_id),_gpg_id(gpg_id)
{
#ifdef DEBUG_ELASTIC
	std::cerr << "Created node type " << type << ", string=" << node_string << std::endl ;
#endif
    setFlag(ItemIsMovable);
#if QT_VERSION >= 0x040600
    setFlag(ItemSendsGeometryChanges);
#endif
    setCacheMode(DeviceCoordinateCache);
    setZValue(1);
	 mDeterminedBB = false ;
	 mBBWidth = 0 ;
	 mNodeDrawSize = 20;

	 _speedx=_speedy=0;
	 _steps=0;

	if(_type == GraphWidget::ELASTIC_NODE_TYPE_OWN)
		_auth = GraphWidget::ELASTIC_NODE_AUTH_FULL ;
}

const float Node::MASS_FACTOR = 10 ;
const float Node::FRICTION_FACTOR = 10.8f ;
const float Node::REPULSION_FACTOR = 4;
const float Node::NODE_DISTANCE = 130.0f ;


void Node::addEdge(Edge *edge)
{
    edgeList << edge;
    edge->adjust();
}

const QList<Edge *>& Node::edges() const
{
    return edgeList;
}

//static double interpolate(const double *map,int W,int H,float x,float y)
//{
//	if(x>W-2) x=W-2 ;
//	if(y>H-2) y=H-2 ;
//	if(x<0  ) x=0   ;
//	if(y<0  ) y=0   ;
//
//	int i=(int)floor(x) ;
//	int j=(int)floor(y) ;
//	double di = x-i ;
//	double dj = y-j ;
//
//	return (1-di)*( (1-dj)*map[2*(i+W*j)] + dj*map[2*(i+W*(j+1))])
//				+di *( (1-dj)*map[2*(i+1+W*j)] + dj*map[2*(i+1+W*(j+1))]) ;
//}

void Node::calculateForces(const double *map,int width,int height,int W,int H,float x,float y,float friction_factor)
{
	if (!scene() || scene()->mouseGrabberItem() == this) 
	{
		newPos = pos();
		return;
	}


	// Sum up all forces pushing this item away
	qreal xforce = 0;
	qreal yforce = 0;

	float dei=0.0f ;
	float dej=0.0f ;

	static float *e = NULL ;
	static const int KS = 5 ;

	if(e == NULL)
	{
		e = new float[(2*KS+1)*(2*KS+1)] ;

		for(int i=-KS;i<=KS;++i)
			for(int j=-KS;j<=KS;++j)
				e[i+KS+(2*KS+1)*(j+KS)] = exp( -(i*i+j*j)/30.0 ) ;	// can be precomputed
	}

	for(int i=-KS;i<=KS;++i)
		for(int j=-KS;j<=KS;++j)
		{
			int X = std::min(W-1,std::max(0,(int)rint(x))) ;
			int Y = std::min(H-1,std::max(0,(int)rint(y))) ;

			float val = map[2*((i+X+W)%W + W*((j+Y+H)%H))] ;

			dei += i * e[i+KS+(2*KS+1)*(j+KS)] * val ;
			dej += j * e[i+KS+(2*KS+1)*(j+KS)] * val ;
		}

	xforce = REPULSION_FACTOR * dei/25.0;
	yforce = REPULSION_FACTOR * dej/25.0;

	// Now subtract all forces pulling items together
	double weight = (n_edges() + 1) ;

	foreach (Edge *edge, edgeList) {
		QPointF pos;
		double w2 ;	// This factor makes the edge length depend on connectivity, so clusters of friends tend to stay in the
						// same location.
						//
		if (edge->sourceNode() == this)
		{
			pos = mapFromItem(edge->destNode(), 0, 0);
			w2 = sqrtf(std::min(n_edges(),edge->destNode()->n_edges())) ;
		}
		else
		{
			pos = mapFromItem(edge->sourceNode(), 0, 0);
			w2 = sqrtf(std::min(n_edges(),edge->sourceNode()->n_edges())) ;
		}

		float dist = sqrtf(pos.x()*pos.x() + pos.y()*pos.y()) ;
		float val = dist - graph->edgeLength() * w2 ;

		xforce += 0.01*pos.x() * val / weight;
		yforce += 0.01*pos.y() * val / weight;
	}

	xforce -= FRICTION_FACTOR * _speedx ;
	yforce -= FRICTION_FACTOR * _speedy ;

	// This term drags nodes away from the sides.
	//
	if(x < 15) xforce += 100.0/(x+0.1) ;
	if(y < 15) yforce += 100.0/(y+0.1) ;
	if(x > width-15) xforce -= 100.0/(width-x+0.1) ;
	if(y > height-15) yforce -= 100.0/(height-y+0.1) ;

	// now time filter:

	_speedx += xforce / MASS_FACTOR ;
	_speedy += yforce / MASS_FACTOR ;

	if(_speedx > 10) _speedx = 10.0f ;
	if(_speedy > 10) _speedy = 10.0f ;
	if(_speedx <-10) _speedx =-10.0f ;
	if(_speedy <-10) _speedy =-10.0f ;

	QRectF sceneRect = scene()->sceneRect();
	newPos = pos() + QPointF(_speedx, _speedy) / friction_factor;
	newPos.setX(qMin(qMax(newPos.x(), sceneRect.left()), sceneRect.right()));
	newPos.setY(qMin(qMax(newPos.y(), sceneRect.top()) , sceneRect.bottom()));
}

bool Node::progress()
{
	if(_type == GraphWidget::ELASTIC_NODE_TYPE_OWN)
		return false;

	float f = std::max(fabs(newPos.x() - pos().x()), fabs(newPos.y() - pos().y())) ;

    setPos(newPos);
    return f > 0.5;
}

QRectF Node::boundingRect() const
{
        float m = QFontMetricsF(graph->font()).height();
        float f = m/16.0;
    qreal adjust = 2*f;
    /* add in the size of the text */
    qreal realwidth = 40*f;
    
    if (mDeterminedBB)
    {
	realwidth = mBBWidth + adjust;
    }
    if (realwidth < 23*f + adjust)
    {
    	realwidth = 23*f + adjust;
    }

    return QRectF(-10*f - adjust, -10*f - adjust, realwidth, 23*f + adjust);
}

QPainterPath Node::shape() const
{
    QPainterPath path;
    path.addEllipse(-10, -10, 20, 20);
    return path;
}

#if QT_VERSION >= 0x040700
static QColor lightdark(const QColor& col,int l,int d)
{
	int h,s,v ;

   col.getHsv(&h,&s,&v) ;

	v = (int)floor(v * l/(float)d ) ;

	return QColor::fromHsv(h,s,v) ;
}
#endif

void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
	static QColor type_color[4] = { QColor(Qt::yellow), QColor(Qt::green), QColor(Qt::cyan), QColor(Qt::black) } ;

	QColor col0 ;

	if(_selected_node == NULL)
		col0 = type_color[_type] ;
	else if(_selected_node == this)
		col0 = type_color[0] ;
	else 
	{
		bool found = false ;
		for(QList<Edge*>::const_iterator it(edgeList.begin());it!=edgeList.end();++it)
			if( (*it)->sourceNode() == _selected_node || (*it)->destNode() == _selected_node)
			{
				col0 = type_color[1] ;
				found = true ;
				break ;
			}

		if(!found)
			col0= type_color[2] ;
	}

	painter->setPen(Qt::NoPen);
	painter->setBrush(Qt::darkGray);
	int mNodeDrawSize2 = mNodeDrawSize/2;
	painter->drawEllipse(-mNodeDrawSize2+3, -mNodeDrawSize2+3, mNodeDrawSize, mNodeDrawSize);

	QRadialGradient gradient(-3, -3, 10);
	if (option->state & QStyle::State_Sunken) 
	{
		gradient.setCenter(3, 3);
		gradient.setFocalPoint(3, 3);
#if QT_VERSION >= 0x040700
		gradient.setColorAt(1, lightdark(col0,120, 100+_auth*50));
		gradient.setColorAt(0, lightdark(col0, 70, 100+_auth*50));
#else
		gradient.setColorAt(1, col0.light(120).dark(100+_auth*100));
		gradient.setColorAt(0, col0.light(70).dark(100+_auth*100));
#endif
	} 
	else 
	{
#if QT_VERSION >= 0x040700
		gradient.setColorAt(1, lightdark(col0, 50,100+_auth*50));
		gradient.setColorAt(0, lightdark(col0,100,100+_auth*50));
#else
		gradient.setColorAt(1, col0.light(50).dark(100+_auth*100));
		gradient.setColorAt(0, col0.dark(100+_auth*100));
#endif
	}
	painter->setBrush(gradient);
	if (Settings->getSheetName() == ":Standard_Dark"){
		painter->setPen(QPen(Qt::white, 0));
	} else {
		painter->setPen(QPen(Qt::black, 0));
	}
	painter->drawEllipse(-mNodeDrawSize2, -mNodeDrawSize2, mNodeDrawSize, mNodeDrawSize);
    
    	QString txt = QString::fromUtf8(_desc_string.c_str());
        float m = QFontMetricsF(graph->font()).height();
        float f = m/16.0;
        
	painter->drawText(-10, 5*f, txt) ;

	if (!mDeterminedBB)
	{
		QRect textBox = painter->boundingRect(-10, 5*f, QFontMetricsF(graph->font()).width(txt), 1.5*m, Qt::AlignVCenter, QString::fromUtf8(_desc_string.c_str()));
		mBBWidth = textBox.width()+40*f;
		mDeterminedBB = true;
	}
}

QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
    case ItemPositionHasChanged:
        foreach (Edge *edge, edgeList)
            edge->adjust();
        graph->itemMoved();
        break;
    default:
        break;
    };

    return QGraphicsItem::itemChange(change, value);
}

void Node::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if(event->button() == Qt::LeftButton)
	{
		_selected_node = this ;
		graph->forceRedraw() ;
	}

	update();
	QGraphicsItem::mousePressEvent(event);
}

void Node::peerDetails()
{
#ifdef DEBUG_ELASTIC
	std::cerr << "Calling peer details" << std::endl;
#endif
    PGPKeyDialog::showIt(_gpg_id, PGPKeyDialog::PageDetails);
}
void Node::makeFriend()
{
    PGPKeyDialog::showIt(_gpg_id, PGPKeyDialog::PageDetails);
}
void Node::denyFriend()
{
    PGPKeyDialog::showIt(_gpg_id, PGPKeyDialog::PageDetails);
}

void Node::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) 
{
	QMenu contextMnu ;

	if(_type == GraphWidget::ELASTIC_NODE_TYPE_FRIEND)
        contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_DENIED), QObject::tr( "Deny friend" ), this, SLOT(denyFriend()) );
	else if(_type != GraphWidget::ELASTIC_NODE_TYPE_OWN)
        contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MAKEFRIEND), QObject::tr( "Make friend" ), this, SLOT(makeFriend()) );

    contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MAKEFRIEND), QObject::tr( "Peer details" ), this, SLOT(peerDetails()) );

	contextMnu.exec(event->screenPos());
}

void Node::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	 _selected_node = NULL ;
	graph->forceRedraw() ;

    update();
    QGraphicsItem::mouseReleaseEvent(event);
}


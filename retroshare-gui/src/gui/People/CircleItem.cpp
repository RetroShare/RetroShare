#include <math.h>

#include <QPainter>
#include <QMenu>
#include <QStyle>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

#include "gui/gxs/GxsIdDetails.h"
#include "IdentityItem.h"
#include "CircleItem.h"

#define IMAGE_MAKEFRIEND ""

CircleItem *CircleItem::_selected_node = NULL ;

CircleItem::CircleItem(const RsGroupMetaData& group_info, const RsGxsCircleDetails& details)
	: _group_info(group_info), _circle_details(details)
{
	std::cerr << "Created group item for id=" <<group_info.mGroupId << std::endl;

	    setFlag(ItemIsMovable);
	setAcceptHoverEvents(true) ;
#if QT_VERSION >= 0x040600
    setFlag(ItemSendsGeometryChanges);
#endif
    setCacheMode(DeviceCoordinateCache);
	 _selected = false ;

	 mDeterminedBB = false ;
	 mBBWidth = 40 ;

	 /* update friend lists */

	 // update the position of all members

	 uint32_t n=details.mUnknownPeers.size() ;

	 _angles.resize(n) ;
	 for(uint32_t i=0;i<n;++i)
		 _angles[i] = 2*M_PI/n ;
}

void CircleItem::hoverEnterEvent(QGraphicsSceneHoverEvent *e)
{
	std::cerr << "Object was entered!" << std::endl;
	_selected = true ;
	_selected_node = this ;

	emit itemChanged() ;
	update() ;
}
void CircleItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e)
{
	std::cerr << "Object was left!" << std::endl;
	_selected = false ;
	_selected_node = NULL ;

	emit itemChanged() ;
	update() ;
}

QRectF CircleItem::boundingRect() const
{
	static const bool mDeterminedBB = false ;
	static const int mBBWidth = 40 ;

    return QRectF(-(int)IMG_SIZE/2 - CRC_SIZE, -(int)IMG_SIZE/2 - CRC_SIZE, IMG_SIZE+2*CRC_SIZE,IMG_SIZE+2*CRC_SIZE) ;
}

void CircleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
	if(_selected)
		painter->setOpacity(0.7) ;
	else
		painter->setOpacity(1.0) ;

	// draw a circle in the center
	//
	QRadialGradient gradient(-10, -IMG_SIZE/3.0, IMG_SIZE*1.5);
	gradient.setColorAt(0.0f,Qt::lightGray) ;
	gradient.setColorAt(1.0f,Qt::darkGray) ;
	painter->setBrush(gradient);

	painter->setPen(Qt::NoPen);
	painter->setBrush(Qt::lightGray);
	painter->drawEllipse(-7, -7, 20, 20);

	// Now draw all members of this circle into a circle
	//
	uint32_t i=0 ;
	for(std::set<RsGxsId>::const_iterator it(_circle_details.mUnknownPeers.begin());it!= _circle_details.mUnknownPeers.end();++it)
	{
		painter->drawImage(QPoint(CRC_SIZE*cos(_angles[i]) -(int)IMG_SIZE/2, CRC_SIZE*sin(_angles[i]) -(int)IMG_SIZE/2), GxsIdDetails::makeDefaultIcon(*it)) ;
		++i ;
	}

	// Draw the name of the circle
	//
	painter->setPen(QPen(Qt::black, 0));

	//painter->drawRoundedRect(QRectF(-(int)IMG_SIZE/2-10, -(int)IMG_SIZE/2-10, 20+IMG_SIZE, 20+IMG_SIZE),20,15) ;
	//painter->drawImage(QPoint(-(int)IMG_SIZE/2, -(int)IMG_SIZE/2), makeDefaultIcon(_group_info.mMeta.mGroupId)) ;
	//painter->drawRect(-(int)IMG_SIZE/2, -(int)IMG_SIZE/2, IMG_SIZE, IMG_SIZE);
	//std::string desc_string = _group_info.mMeta.mGroupId.toStdString() ;

	std::string desc_string = _group_info.mGroupName ;

	painter->drawText(-8*desc_string.size()/2, IMG_SIZE/2+24, QString::fromUtf8(desc_string.c_str()));

	if (!mDeterminedBB)
	{
		QRect textBox = painter->boundingRect(-10, 0, 400, 20, 0, QString::fromUtf8(desc_string.c_str()));
		mBBWidth = textBox.width();
		mDeterminedBB = true;
	}
}

QVariant CircleItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
//    switch (change) {
//    case ItemPositionHasChanged:
//        foreach (Edge *edge, edgeList)
//            edge->adjust();
//        graph->itemMoved();
//        break;
//    default:
//        break;
//    };

    return QGraphicsItem::itemChange(change, value);
}

void CircleItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if(event->button() == Qt::LeftButton)
	{
		//_selected_node = this ;
		//graph->forceRedraw() ;
	}

	update();
	QGraphicsItem::mousePressEvent(event);
}
void CircleItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	QMenu contextMnu ;

	//if(_type == GraphWidget::ELASTIC_NODE_TYPE_FRIEND)
	//	contextMnu.addAction(QIcon(IMAGE_DENIED), QObject::tr( "Deny friend" ), this, SLOT(denyFriend()) );
	//else if(_type != GraphWidget::ELASTIC_NODE_TYPE_OWN)
	//	contextMnu.addAction(QIcon(IMAGE_MAKEFRIEND), QObject::tr( "Make friend" ), this, SLOT(makeFriend()) );

	contextMnu.addAction(QIcon(IMAGE_MAKEFRIEND), QObject::tr( "Group details" ), this, SLOT(peerDetails()) );

	contextMnu.exec(event->screenPos());
}

void CircleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	 //_selected_node = NULL ;
	//graph->forceRedraw() ;

    update();
    QGraphicsItem::mouseReleaseEvent(event);
}



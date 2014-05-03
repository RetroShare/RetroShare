#include <math.h>

#include <QPainter>
#include <QMenu>
#include <QStyle>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

#include "IdentityItem.h"

#define IMAGE_MAKEFRIEND ""

IdentityItem *IdentityItem::_selected_node = NULL ;

IdentityItem::IdentityItem(const RsGxsIdGroup& group_info)
	: _group_info(group_info)
{
	std::cerr << "Created identity item for id=" <<group_info.mMeta.mGroupId << std::endl;

//	    setFlag(ItemIsMovable);
	setAcceptHoverEvents(true) ;
#if QT_VERSION >= 0x040600
    setFlag(ItemSendsGeometryChanges);
#endif
    setCacheMode(DeviceCoordinateCache);
	 _selected = false ;

	 mDeterminedBB = false ;
	 mBBWidth = 40 ;
}

void IdentityItem::hoverEnterEvent(QGraphicsSceneHoverEvent *e)
{
	std::cerr << "Object was entered!" << std::endl;
	_selected = true ;
	_selected_node = this ;

	emit itemChanged() ;
	update() ;
}
void IdentityItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e)
{
	std::cerr << "Object was left!" << std::endl;
	_selected = false ;
	_selected_node = NULL ;

	emit itemChanged() ;
	update() ;
}


QRectF IdentityItem::boundingRect() const
{
	static const bool mDeterminedBB = false ;
	static const int mBBWidth = 40 ;

    return QRectF(-(int)IMG_SIZE/2-10, -(int)IMG_SIZE/2-10, (int)IMG_SIZE+20,(int)IMG_SIZE+35) ;
}

//QPainterPath IdentityItem::shape() const
//{
//    QPainterPath path;
//    path.addRect(-(int)IMG_SIZE, -(int)IMG_SIZE, 2*(int)IMG_SIZE, 2*(int)IMG_SIZE);
//    return path;
//}

QImage IdentityItem::makeDefaultIcon(const RsGxsGroupId& id)
{
	int S = 128 ;
	QImage pix(S,S,QImage::Format_RGB32) ;

	uint64_t n = reinterpret_cast<const uint64_t*>(id.toByteArray())[0] ;

	uint8_t a[8] ;
	for(int i=0;i<8;++i)
	{
		a[i] = n&0xff ;
		n >>= 8 ;
	}
	QColor val[16] = {
		QColor::fromRgb( 255, 110, 180),
		QColor::fromRgb( 238,  92,  66),
		QColor::fromRgb( 255, 127,  36),
		QColor::fromRgb( 255, 193, 193),
		QColor::fromRgb( 127, 255, 212),
		QColor::fromRgb(   0, 255, 255),
		QColor::fromRgb( 224, 255, 255),
		QColor::fromRgb( 199,  21, 133),
		QColor::fromRgb(  50, 205,  50),
		QColor::fromRgb( 107, 142,  35),
		QColor::fromRgb(  30, 144, 255),
		QColor::fromRgb(  95, 158, 160),
		QColor::fromRgb( 143, 188, 143),
		QColor::fromRgb( 233, 150, 122),
		QColor::fromRgb( 151, 255, 255),
	   QColor::fromRgb( 162, 205,  90),
};

	int c1 = (a[0]^a[1]) & 0xf ;
	int c2 = (a[1]^a[2]) & 0xf ;
	int c3 = (a[2]^a[3]) & 0xf ;
	int c4 = (a[3]^a[4]) & 0xf ;

	for(int i=0;i<S/2;++i)
		for(int j=0;j<S/2;++j)
		{
			float res1 = 0.0f ;
			float res2 = 0.0f ;
			float f = 1.70;

			for(int k1=0;k1<4;++k1)
			for(int k2=0;k2<4;++k2)
			{
				res1 += cos( (2*M_PI*i/(float)S) * k1 * f) * (a[k1  ] & 0xf) + sin( (2*M_PI*j/(float)S) * k2 * f) * (a[k2  ] >> 4) + sin( (2*M_PI*i/(float)S) * k1 * f) * cos( (2*M_PI*j/(float)S) * k2 * f) * (a[k1+k2] >> 4) ;
				res2 += cos( (2*M_PI*i/(float)S) * k2 * f) * (a[k1+2] & 0xf) + sin( (2*M_PI*j/(float)S) * k1 * f) * (a[k2+1] >> 4) + sin( (2*M_PI*i/(float)S) * k2 * f) * cos( (2*M_PI*j/(float)S) * k1 * f) * (a[k1^k2] >> 4) ;
			}

			uint32_t q = 0 ;
			if(res1 >= 0.0f) q += val[c1].rgb() ; else q += val[c2].rgb() ;
			if(res2 >= 0.0f) q += val[c3].rgb() ; else q += val[c4].rgb() ;

			pix.setPixel( i, j, q) ; 
			pix.setPixel( S-1-i, j, q) ; 
			pix.setPixel( S-1-i, S-1-j, q) ; 
			pix.setPixel(     i, S-1-j, q) ; 
		}
	return pix.scaled(IMG_SIZE,IMG_SIZE,Qt::KeepAspectRatio,Qt::SmoothTransformation) ;
}


void IdentityItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
	static QColor type_color[4] = { QColor(Qt::yellow), QColor(Qt::green), QColor(Qt::cyan), QColor(Qt::black) } ;

	painter->setPen(Qt::NoPen);
	painter->setBrush(Qt::lightGray);
	//painter->drawEllipse(-7, -7, 20, 20);

	QRadialGradient gradient(-10, -IMG_SIZE/3.0, IMG_SIZE*1.5);
	gradient.setColorAt(0.0f,Qt::lightGray) ;
	gradient.setColorAt(1.0f,Qt::darkGray) ;
	painter->setBrush(gradient);
	
	if(_selected)
		painter->setOpacity(0.7) ;
	else
		painter->setOpacity(1.0) ;

	painter->setPen(QPen(Qt::black, 0));

	painter->drawRoundedRect(QRectF(-(int)IMG_SIZE/2-10, -(int)IMG_SIZE/2-10, 20+IMG_SIZE, 20+IMG_SIZE),20,15) ;
	painter->drawImage(QPoint(-(int)IMG_SIZE/2, -(int)IMG_SIZE/2), makeDefaultIcon(_group_info.mMeta.mGroupId)) ;
	//painter->drawRect(-(int)IMG_SIZE/2, -(int)IMG_SIZE/2, IMG_SIZE, IMG_SIZE);

	//std::string desc_string = _group_info.mMeta.mGroupId.toStdString() ;
	std::string desc_string = _group_info.mMeta.mGroupName ;

	painter->drawText(-8*desc_string.size()/2, IMG_SIZE/2+24, QString::fromUtf8(desc_string.c_str()));

	if (!mDeterminedBB)
	{
		QRect textBox = painter->boundingRect(-10, 0, 400, 20, 0, QString::fromUtf8(desc_string.c_str()));
		mBBWidth = textBox.width();
		mDeterminedBB = true;
	}
}

QVariant IdentityItem::itemChange(GraphicsItemChange change, const QVariant &value)
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

void IdentityItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if(event->button() == Qt::LeftButton)
	{
		//_selected_node = this ;
		//graph->forceRedraw() ;
	}

	update();
	QGraphicsItem::mousePressEvent(event);
}
void IdentityItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) 
{
	QMenu contextMnu ;

	//if(_type == GraphWidget::ELASTIC_NODE_TYPE_FRIEND)
	//	contextMnu.addAction(QIcon(IMAGE_DENIED), QObject::tr( "Deny friend" ), this, SLOT(denyFriend()) );
	//else if(_type != GraphWidget::ELASTIC_NODE_TYPE_OWN)
	//	contextMnu.addAction(QIcon(IMAGE_MAKEFRIEND), QObject::tr( "Make friend" ), this, SLOT(makeFriend()) );

	contextMnu.addAction(QIcon(IMAGE_MAKEFRIEND), QObject::tr( "Peer details" ), this, SLOT(peerDetails()) );

	contextMnu.exec(event->screenPos());
}

void IdentityItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	 //_selected_node = NULL ;
	//graph->forceRedraw() ;

    update();
    QGraphicsItem::mouseReleaseEvent(event);
}



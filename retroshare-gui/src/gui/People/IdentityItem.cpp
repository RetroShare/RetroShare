#include <math.h>

#include <retroshare/rsmsgs.h>

#include <QPainter>
#include <QMessageBox>
#include <QMenu>
#include <QStyle>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

#include <gui/chat/ChatDialog.h>
#include <gui/gxs/GxsIdDetails.h>
#include "IdentityItem.h"

#define IMAGE_MAKEFRIEND ":/images/user/add_user16.png"
#define IMAGE_CHAT       ":/icons/png/chats.png"

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

void IdentityItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
	static QColor type_color[4] = { QColor(Qt::yellow), QColor(Qt::green), QColor(Qt::cyan), QColor(Qt::black) } ;

	painter->setPen(Qt::NoPen);
	painter->setBrush(Qt::lightGray);
	//painter->drawEllipse(-7, -7, 20, 20);
	
	painter->setRenderHint(QPainter::Antialiasing);

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
	painter->drawImage(QPoint(-(int)IMG_SIZE/2, -(int)IMG_SIZE/2), GxsIdDetails::makeDefaultIcon(RsGxsId(_group_info.mMeta.mGroupId))) ;

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

	contextMnu.addAction(QIcon(IMAGE_MAKEFRIEND), QObject::tr( "Peer details" ), this, SLOT(peerDetails()) );
	contextMnu.addAction(QIcon(IMAGE_CHAT), QObject::tr( "Chat this peer" ), this, SLOT(distantChat()) );
	contextMnu.exec(event->screenPos());
}

void IdentityItem::distantChat()
{
	uint32_t error_code ;

	if(!rsMsgs->initiateDistantChatConnexion(RsGxsId(_group_info.mMeta.mGroupId), error_code))
		QMessageBox::information(NULL,"Distant cannot work","Distant chat refused with this peer. Reason: "+QString::number(error_code)) ;
}

void IdentityItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	 //_selected_node = NULL ;
	//graph->forceRedraw() ;

    update();
    QGraphicsItem::mouseReleaseEvent(event);
}


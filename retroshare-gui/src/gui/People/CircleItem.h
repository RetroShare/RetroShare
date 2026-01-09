#pragma once

#include <QGraphicsItem>

#include <retroshare/rsgxscircles.h>

class CircleItem: public QObject, public QGraphicsItem
{
	Q_OBJECT

	public:
		CircleItem(const RsGroupMetaData& gxs_group_info,const RsGxsCircleDetails& details) ;

		QRectF boundingRect() const ;
		//QPainterPath shape() const ;

		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) ;

		//static QImage makeDefaultIcon(const RsGxsGroupId& id) ;

		static const int IMG_SIZE = 32;
		static const int CRC_SIZE = 128;

		static CircleItem *_selected_node ;

		const RsGroupMetaData& groupInfo() const { return _group_info  ; }
signals:
		void itemChanged() ;

	private:
		virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *e);
		virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *e);
		virtual QVariant itemChange(GraphicsItemChange change,const QVariant& value) ;
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

		virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *) ;

		RsGroupMetaData _group_info ;
		RsGxsCircleDetails _circle_details ;

		bool mDeterminedBB;
		bool mBBWidth;
		bool _selected ;

		std::vector<float> _angles ;
};


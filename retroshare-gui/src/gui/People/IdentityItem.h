#pragma once

#include <QGraphicsItem>

#include <retroshare/rsidentity.h>

class IdentityItem: public QObject, public QGraphicsItem
{
	Q_OBJECT

	public:
		IdentityItem(const RsGxsIdGroup& gxs_group_info) ;

		QRectF boundingRect() const ;
		//QPainterPath shape() const ;

		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) ;

		static const int IMG_SIZE = 64;
		static IdentityItem *_selected_node ;

		const RsGxsIdGroup& groupInfo() const { return _group_info  ; }

	public slots:
		void distantChat() ;

signals:
		void itemChanged() ;

	private:
		virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *e);
		virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *e);
		virtual QVariant itemChange(GraphicsItemChange change,const QVariant& value) ;
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

		virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *) ;

		RsGxsIdGroup _group_info ;

		bool mDeterminedBB;
		bool mBBWidth;
		bool _selected ;
};


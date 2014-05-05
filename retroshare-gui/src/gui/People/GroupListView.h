#pragma once

#include <map>
#include <QGraphicsView>
#include <stdint.h>
#include <retroshare/rstypes.h>

#include "util/TokenQueue.h"

#include "gui/gxs/RsGxsUpdateBroadcastPage.h"

class UIStateHelper;
class IdentityItem ;
class CircleItem ;

class GroupListView : public QGraphicsView,  public TokenResponse
{
    Q_OBJECT

public:
		 static const uint32_t GLVIEW_IDLIST    ;
		 static const uint32_t GLVIEW_IDDETAILS ;
		 static const uint32_t GLVIEW_CIRCLES   ;
		 static const uint32_t GLVIEW_REFRESH   ;

    GroupListView(QWidget * = NULL);

	 void clearGraph() ;

	 // Derives from RsGxsUpdateBroadcastPage
	 virtual void updateDisplay(bool) ;

    virtual void itemMoved();

	 void setEdgeLength(uint32_t l) ;
	 uint32_t edgeLength() const { return _edge_length ; }

	 void loadRequest(const TokenQueue * /*queue*/, const TokenRequest &req) ;

	 void requestIdList() ;
	 void requestCirclesList() ;

	 void insertIdList(uint32_t token) ;
	 void insertCircles(uint32_t token) ;

	 protected slots:
		 void forceRedraw() ;
protected:
    void timerEvent(QTimerEvent *event);
    void wheelEvent(QWheelEvent *event);
    void drawBackground(QPainter *painter, const QRectF &rect);

    void scaleView(qreal scaleFactor);

    virtual void keyPressEvent(QKeyEvent *event);
	 virtual void resizeEvent(QResizeEvent *e) ;

	 static QImage makeDefaultIcon(const RsGxsId& id) ;

private:
    int timerId;
    //Node *centerNode;
	 bool mDeterminedBB ;
	 bool mIsFrozen;

	 //std::vector<Node *> _groups ;
	 //std::map<std::pair<NodeId,NodeId>,Edge *> _edges ;
	 //std::map<std::string,QPointF> _node_cached_positions ;

	 uint32_t _edge_length ;
	 float _friction_factor ;
	 //NodeId _current_node ;

	 TokenQueue *mIdQueue;
	 UIStateHelper *mStateHelper;

     std::map<RsGxsGroupId,IdentityItem *> _identity_items ;
     std::map<RsGxsGroupId,CircleItem *> _circles_items ;
};


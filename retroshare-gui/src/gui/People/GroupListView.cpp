#include <iostream>
#include <QDebug>
#include <QGraphicsScene>
#include <QWheelEvent>

#include "gui/common/UIStateHelper.h"
#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>

#include "GroupListView.h"
#include "IdentityItem.h"

#include <math.h>

const uint32_t GroupListView::GLVIEW_IDLIST    = 0x0001 ;
const uint32_t GroupListView::GLVIEW_IDDETAILS = 0x0002 ;
const uint32_t GroupListView::GLVIEW_REFRESH   = 0x0003 ;
const uint32_t GroupListView::GLVIEW_REPLIST   = 0x0004 ;

GroupListView::GroupListView(QWidget *)
    : timerId(0), mIsFrozen(false)
{
	mStateHelper = new UIStateHelper(this);
	//mStateHelper->addWidget(IDDIALOG_IDLIST, ui.treeWidget_IdList);

	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);

    QGraphicsScene *scene = new QGraphicsScene(QRectF(0,0,width(),height()),this);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
	 scene->clear() ;
    setScene(scene);
    scene->setSceneRect(0, 0, width(), height());

    setCacheMode(CacheBackground);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(AnchorUnderMouse);
    setResizeAnchor(AnchorViewCenter);
	 _friction_factor = 1.0f ;

	 setMouseTracking(true) ;
    //scale(qreal(0.8), qreal(0.8));
}

void GroupListView::resizeEvent(QResizeEvent *e)
{
    scene()->setSceneRect(0, 0, e->size().width(), e->size().height());
}

void GroupListView::clearGraph()
{
//	QGraphicsScene *scene = new QGraphicsScene(this);
//	scene->setItemIndexMethod(QGraphicsScene::NoIndex);
//	setScene(scene);

//	scene->addItem(centerNode);
//	centerNode->setPos(0, 0);

//	if (oldscene != NULL)
//	{
//		delete oldscene;
//	}

	scene()->clear();
	scene()->setSceneRect(-200, -200, 1000, 1000);
//	_edges.clear();
//	_nodes.clear();
	_friction_factor = 1.0f ;
}

// GroupListView::NodeId GroupListView::addNode(const std::string& node_short_string,const std::string& node_complete_string,NodeType type,AuthType auth,const RsPeerId& ssl_id,const RsPgpId& gpg_id)
// {
//     Node *node = new Node(node_short_string,type,auth,this,ssl_id,gpg_id);
//      node->setToolTip(QString::fromUtf8(node_complete_string.c_str())) ;
// 	 _nodes.push_back(node) ;
//     scene()->addItem(node);
// 
// 	 std::map<std::string,QPointF>::const_iterator it(_node_cached_positions.find(gpg_id.toStdString())) ;
// 	 if(_node_cached_positions.end() != it)
// 		 node->setPos(it->second) ;
// 	 else
// 	 {
// 		 qreal x1,y1,x2,y2 ;
// 		 sceneRect().getCoords(&x1,&y1,&x2,&y2) ;
// 
// 		 float f1 = (type == GroupListView::ELASTIC_NODE_TYPE_OWN)?0.5:(rand()/(float)RAND_MAX) ;
// 		 float f2 = (type == GroupListView::ELASTIC_NODE_TYPE_OWN)?0.5:(rand()/(float)RAND_MAX) ;
// 
// 		 node->setPos(x1+f1*(x2-x1),y1+f2*(y2-y1));
// 	 }
// #ifdef DEBUG_ELASTIC
// 	 std::cerr << "Added node " << _nodes.size()-1 << std::endl ;
// #endif
// 	 return _nodes.size()-1 ;
// }

void GroupListView::itemMoved()
{
    if (!timerId)
	 {
#ifdef DEBUG_ELASTIC
		 std::cout << "starting timer" << std::endl;
#endif
        timerId = startTimer(1000 / 25);	// hit timer 25 times per second.
		  _friction_factor = 1.0f ;
	 }
}

void GroupListView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
//    case Qt::Key_Up:
//        centerNode->moveBy(0, -20);
//        break;
//    case Qt::Key_Down:
//        centerNode->moveBy(0, 20);
//        break;
//    case Qt::Key_Left:
//        centerNode->moveBy(-20, 0);
//        break;
//    case Qt::Key_Right:
//        centerNode->moveBy(20, 0);
//        break;
    case Qt::Key_Plus:
        scaleView(qreal(1.2));
        break;
    case Qt::Key_Minus:
        scaleView(1 / qreal(1.2));
        break;
    case Qt::Key_Space:
    case Qt::Key_Enter:
		  requestIdList() ;
//        foreach (QGraphicsItem *item, scene()->items()) {
//            if (qgraphicsitem_cast<Node *>(item))
//                item->setPos(-150 + qrand() % 300, -150 + qrand() % 300);
//        }
        break;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}

void GroupListView::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

	 if(!isVisible())
		 return ;

	if (mIsFrozen)
	{
		update();
		return;
	}

	 QRectF R(scene()->sceneRect()) ;

//    if (!itemsMoved) {
//        killTimer(timerId);
//#ifdef DEBUG_ELASTIC
//		  std::cerr << "Killing timr" << std::endl ;
//#endif
//        timerId = 0;
//    }
	 _friction_factor *= 1.001f ;
//	 std::cerr << "Friction factor = " << _friction_factor << std::endl;
}

void GroupListView::setEdgeLength(uint32_t l)
{
	_edge_length = l ;

	if(!timerId)
	{
#ifdef DEBUG_ELASTIC
		 std::cout << "starting timer" << std::endl;
#endif
        timerId = startTimer(1000 / 25);
		  _friction_factor = 1.0f ;
	}
}
void GroupListView::insertIdList(uint32_t token)
{
	std::cerr << "**** In insertIdList() ****" << std::endl;

	mStateHelper->setLoading(GLVIEW_IDLIST, false);

//	int accept = ui.filterComboBox->itemData(ui.filterComboBox->currentIndex()).toInt();

	RsGxsIdGroup data;
	std::vector<RsGxsIdGroup> datavector;
	std::vector<RsGxsIdGroup>::iterator vit;

	if (!rsIdentity->getGroupData(token, datavector))
	{
		std::cerr << "IdDialog::insertIdList() Error getting GroupData";
		std::cerr << std::endl;

		mStateHelper->setLoading(GLVIEW_IDDETAILS, false);
		mStateHelper->setLoading(GLVIEW_REPLIST, false);
		mStateHelper->setActive(GLVIEW_IDLIST, false);
		mStateHelper->setActive(GLVIEW_IDDETAILS, false);
		mStateHelper->setActive(GLVIEW_REPLIST, false);
		mStateHelper->clear(GLVIEW_IDLIST);
		mStateHelper->clear(GLVIEW_IDDETAILS);
		mStateHelper->clear(GLVIEW_REPLIST);

		return;
	}

	mStateHelper->setActive(GLVIEW_IDLIST, true);

	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	/* Insert items */
	int i=0 ;

    for (vit = datavector.begin(); vit != datavector.end(); ++vit)
        if(_identity_items.find((*vit).mMeta.mGroupId) == _identity_items.end())
        {
            std::cerr << "Loading data vector identity ID = " << (*vit).mMeta.mGroupId << std::endl;

            IdentityItem *new_item = new IdentityItem(*vit) ;
            _identity_items[(*vit).mMeta.mGroupId] = new_item ;

				new_item->setPos(15+IdentityItem::IMG_SIZE/2,(40+IdentityItem::IMG_SIZE)*(i+0.5)) ;

				QObject::connect(new_item,SIGNAL(itemChanged()),this,SLOT(forceRedraw())) ;

				scene()->addItem(new_item) ;
				++i ;
        }
        else
            std::cerr << "Updating data vector identity ID = " << (*vit).mMeta.mGroupId << std::endl;

    //	filterIds();

	// fix up buttons.
//	updateSelection();
}

void GroupListView::requestIdList()
{
	std::cerr << "Requesting ID list..." << std::endl;

	if (!mIdQueue)
		return;

	mStateHelper->setLoading(GLVIEW_IDLIST,    true);
	//mStateHelper->setLoading(GLVIEW_IDDETAILS, true);
	//mStateHelper->setLoading(GLVIEW_REPLIST,   true);

	mIdQueue->cancelActiveRequestTokens(GLVIEW_IDLIST);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;

	mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, GLVIEW_IDLIST);
}

void GroupListView::forceRedraw()
{
	std::cerr << "force redraw" << std::endl;
	for(std::map<RsGxsGroupId,IdentityItem*>::const_iterator it(_identity_items.begin());it!=_identity_items.end();++it)
		it->second->update( it->second->boundingRect()) ;

	updateSceneRect(sceneRect()) ;
	scaleView(1.0);
}
void GroupListView::wheelEvent(QWheelEvent *event)
{
    //scaleView(pow((double)2, -event->delta() / 240.0));
}
void GroupListView::loadRequest(const TokenQueue * /*queue*/, const TokenRequest &req)
{
	std::cerr << "IdDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

	switch(req.mUserType)
	{
		case GLVIEW_IDLIST:
			insertIdList(req.mToken);
			break;

		case GLVIEW_IDDETAILS:
			//insertIdDetails(req.mToken);
			break;

		case GLVIEW_REPLIST:
			//insertRepList(req.mToken);
			break;

		case GLVIEW_REFRESH:
			updateDisplay(true);
			break;
		default:
			std::cerr << "IdDialog::loadRequest() ERROR";
			std::cerr << std::endl;
			break;
	}
}

void GroupListView::updateDisplay(bool)
{
	std::cerr << "update display" << std::endl;
}
	
void GroupListView::drawBackground(QPainter *painter, const QRectF &rect)
{
	//Q_UNUSED(rect);

	std::cerr << "drawing background..." << std::endl;
	// Shadow
	QRectF sceneRect = this->sceneRect();

	// QRectF rightShadow(sceneRect.right(), sceneRect.top() + 5, 5, sceneRect.height());
	// QRectF bottomShadow(sceneRect.left() + 5, sceneRect.bottom(), sceneRect.width(), 5);
	// if (rightShadow.intersects(rect) || rightShadow.contains(rect))
	// 	painter->fillRect(rightShadow, Qt::darkGray);
	// if (bottomShadow.intersects(rect) || bottomShadow.contains(rect))
	// 	painter->fillRect(bottomShadow, Qt::darkGray);

	// Fill
	QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
	gradient.setColorAt(0, Qt::white);
	gradient.setColorAt(1, Qt::lightGray);
	painter->fillRect(rect.intersected(sceneRect), gradient);
	//painter->setBrush(Qt::NoBrush);
	//painter->drawRect(sceneRect);

	// Text
	// QRectF textRect(sceneRect.left() + 4, sceneRect.top() + 4,
	// 		sceneRect.width() - 4, sceneRect.height() - 4);
	// QString message(tr("Click and drag the nodes around, and zoom with the mouse "
	// 			"wheel or the '+' and '-' keys"));

	// QFont font = painter->font();
	// font.setBold(true);
	// font.setPointSize(14);
	// painter->setFont(font);
	// painter->setPen(Qt::lightGray);
	// painter->drawText(textRect.translated(2, 2), message);
	// painter->setPen(Qt::black);
	// painter->drawText(textRect, message);

	// Now draw information about current Identity
	//
	if(IdentityItem::_selected_node != NULL)
	{
		std::cerr << "Drawing selected info" << std::endl;
		painter->setPen(Qt::black);
		QFont font = painter->font();
		font.setBold(true);
		font.setPointSize(14);
		painter->setFont(font);

		const RsGxsIdGroup& group_info(IdentityItem::_selected_node->groupInfo()) ;

		painter->drawText(QPointF(IdentityItem::IMG_SIZE*2.5,50), QString("Id: ")+QString::fromStdString(group_info.mMeta.mGroupId.toStdString() ));
		painter->drawText(QPointF(100,100), QString("Id: ")+QString::fromStdString(group_info.mMeta.mGroupId.toStdString() ));
	}
}

void GroupListView::scaleView(qreal scaleFactor)
{
    qreal factor = matrix().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;

    scale(scaleFactor, scaleFactor);
}

//void GroupListView::setFreeze(bool freeze)
//{
//    mIsFrozen = freeze;
//}
//
//bool GroupListView::isFrozen() const
//{
//	return mIsFrozen;
//}

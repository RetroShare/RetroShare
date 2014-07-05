#include <iostream>
#include <QDebug>
#include <QGraphicsScene>
#include <QWheelEvent>

#include "gui/common/UIStateHelper.h"
#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsgxscircles.h>

#include "GroupListView.h"
#include "IdentityItem.h"
#include "CircleItem.h"

#include <math.h>

const uint32_t GroupListView::GLVIEW_IDLIST    = 0x0001 ;
const uint32_t GroupListView::GLVIEW_IDDETAILS = 0x0002 ;
const uint32_t GroupListView::GLVIEW_REFRESH   = 0x0003 ;
const uint32_t GroupListView::GLVIEW_CIRCLES   = 0x0004 ;

GroupListView::GroupListView(QWidget *)
    : timerId(0), mIsFrozen(false)
{
	mStateHelper = new UIStateHelper(this);
	//mStateHelper->addWidget(IDDIALOG_IDLIST, ui.treeWidget_IdList);

	mIdentityQueue = new TokenQueue(rsIdentity->getTokenService(), this);
	mCirclesQueue = new TokenQueue(rsGxsCircles->getTokenService(), this);

	QGraphicsScene *scene = new QGraphicsScene(QRectF(0,0,width(),height()),this);
	scene->setItemIndexMethod(QGraphicsScene::NoIndex);
	scene->clear() ;
	setScene(scene);
	scene->setSceneRect(0, 0, width(), height());

	setCacheMode(CacheBackground);
	setViewportUpdateMode(FullViewportUpdate);
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
    switch (event->key()) 
	 {
		 case Qt::Key_Plus: scaleView(qreal(1.2));
								  break;
		 case Qt::Key_Minus:scaleView(1 / qreal(1.2));
								  break;
		 case Qt::Key_Space:
		 case Qt::Key_Enter:requestIdList() ;
								  requestCirclesList() ;
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

	 // do nothing for now.
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
		mStateHelper->setLoading(GLVIEW_CIRCLES, false);

		mStateHelper->setActive(GLVIEW_IDLIST, false);
		mStateHelper->setActive(GLVIEW_IDDETAILS, false);
		mStateHelper->setActive(GLVIEW_CIRCLES, false);

		mStateHelper->clear(GLVIEW_IDLIST);
		mStateHelper->clear(GLVIEW_IDDETAILS);
		mStateHelper->clear(GLVIEW_CIRCLES);

		return;
	}

	mStateHelper->setActive(GLVIEW_IDLIST, true);

	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	/* Insert items */
	int i=0 ;

    for (vit = datavector.begin(); vit != datavector.end(); ++vit)
        if(_identity_items.find((*vit).mMeta.mGroupId) == _identity_items.end())
        {
            std::cerr << "Loading data vector identity ID = " << (*vit).mMeta.mGroupId << ", i="<< i << std::endl;

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
void GroupListView::insertCircles(uint32_t token)
{
	mStateHelper->setLoading(GLVIEW_CIRCLES, false);

	std::cerr << "CirclesDialog::loadGroupMeta()" << std::endl;

	std::list<RsGroupMetaData> groupInfo;
	std::list<RsGroupMetaData>::iterator vit;

	if (!rsGxsCircles->getGroupSummary(token,groupInfo))
	{
		std::cerr << "CirclesDialog::loadGroupMeta() Error getting GroupMeta";
		std::cerr << std::endl;
		mStateHelper->setActive(GLVIEW_CIRCLES, false);
		return;
	}

	mStateHelper->setActive(GLVIEW_CIRCLES, true);

	/* add the top level item */
	int i=0;

	for(vit = groupInfo.begin(); vit != groupInfo.end(); vit++)
	{
		/* Add Widget, and request Pages */
		std::cerr << "CirclesDialog::loadGroupMeta() GroupId: " << vit->mGroupId;
		std::cerr << " Group: " << vit->mGroupName;
		std::cerr << std::endl;

		RsGxsCircleDetails details ;

		if(!rsGxsCircles->getCircleDetails(RsGxsCircleId(vit->mGroupId), details))
		{
			std::cerr << "(EE) Cannot get details for circle id " << vit->mGroupId << ". Circle item is not created!" << std::endl;
			continue ;
		}
		CircleItem *gitem = new CircleItem( *vit, details ) ;

		_circles_items[(*vit).mGroupId] = gitem ;

		gitem->setPos(150+IdentityItem::IMG_SIZE/2,(40+IdentityItem::IMG_SIZE)*(i+0.5)) ;

		QObject::connect(gitem,SIGNAL(itemChanged()),this,SLOT(forceRedraw())) ;

		scene()->addItem(gitem) ;
		++i ;
	}
}

void GroupListView::requestIdList()
{
	std::cerr << "Requesting ID list..." << std::endl;

	if (!mIdentityQueue)
		return;

	mStateHelper->setLoading(GLVIEW_IDLIST,    true);
	//mStateHelper->setLoading(GLVIEW_IDDETAILS, true);
	//mStateHelper->setLoading(GLVIEW_REPLIST,   true);

	mIdentityQueue->cancelActiveRequestTokens(GLVIEW_IDLIST);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;

	mIdentityQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, GLVIEW_IDLIST);
}
void GroupListView::requestCirclesList()
{
	std::cerr << "Requesting Circles list..." << std::endl;

	if (!mCirclesQueue)
		return;

	mStateHelper->setLoading(GLVIEW_CIRCLES,    true);
	//mStateHelper->setLoading(GLVIEW_IDDETAILS, true);
	//mStateHelper->setLoading(GLVIEW_REPLIST,   true);

	mCirclesQueue->cancelActiveRequestTokens(GLVIEW_CIRCLES);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token;
	mCirclesQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, GLVIEW_CIRCLES);
}
void GroupListView::forceRedraw()
{
	std::cerr << "force redraw" << std::endl;
	for(std::map<RsGxsGroupId,IdentityItem*>::const_iterator it(_identity_items.begin());it!=_identity_items.end();++it)
		it->second->update( it->second->boundingRect()) ;

	updateSceneRect(sceneRect()) ;
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

		case GLVIEW_CIRCLES:
			insertCircles(req.mToken);
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

    // Fill
    QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
    gradient.setColorAt(0, Qt::white);
    gradient.setColorAt(1, Qt::lightGray);
    painter->fillRect(rect.intersected(sceneRect), gradient);

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
    static const int font_size = 12 ;

    if(IdentityItem::_selected_node != NULL)
    {
        std::cerr << "Drawing selected info" << std::endl;
        painter->setPen(Qt::black);
        QFont font = painter->font();
        font.setBold(true);
        font.setPointSize(font_size);
        painter->setFont(font);

        const RsGxsIdGroup& group_info(IdentityItem::_selected_node->groupInfo()) ;

        std::cerr << "scene rect: " << sceneRect.x() << " " << sceneRect.y() << " " << sceneRect.width() << " " << sceneRect.height() << " " << std::endl;
        int n=1 ;
        static const int line_height = font_size+2 ;

        QString typestring = (group_info.mPgpKnown)?(QString("Signed by node ")+QString::fromStdString(rsPeers->getGPGName(group_info.mPgpId))
                                                                           + " ("
                                                                           +QString::fromStdString(group_info.mPgpId.toStdString())
                                                                           +")"):tr("Anonymous") ;

        painter->drawText(sceneRect.translated(IdentityItem::IMG_SIZE*2.5,line_height*n++), QString("Name : ")+QString::fromStdString(group_info.mMeta.mGroupName ));
        painter->drawText(sceneRect.translated(IdentityItem::IMG_SIZE*2.5+31,line_height*n++), QString("Id : ")+QString::fromStdString(group_info.mMeta.mGroupId.toStdString() ));
        painter->drawText(sceneRect.translated(IdentityItem::IMG_SIZE*2.5+10,line_height*n++), QString("Type : ")+typestring );
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

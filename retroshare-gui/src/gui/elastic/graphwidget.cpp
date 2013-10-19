/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the example classes of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "graphwidget.h"
#include "edge.h"
#include "node.h"

#include <iostream>
#include <QDebug>
#include <QGraphicsScene>
#include <QWheelEvent>

#include <math.h>

#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr

void fourn(double data[],unsigned long nn[],unsigned long ndim,int isign)
{
	int i1,i2,i3,i2rev,i3rev,ip1,ip2,ip3,ifp1,ifp2;
	int ibit,idim,k1,k2,n,nprev,nrem,ntot;
	double tempi,tempr;
	double theta,wi,wpi,wpr,wr,wtemp;

	ntot=1;
	for (idim=1;idim<=(long)ndim;idim++)
		ntot *= nn[idim];
	nprev=1;
	for (idim=ndim;idim>=1;idim--) {
		n=nn[idim];
		nrem=ntot/(n*nprev);
		ip1=nprev << 1;
		ip2=ip1*n;
		ip3=ip2*nrem;
		i2rev=1;
		for (i2=1;i2<=ip2;i2+=ip1) {
			if (i2 < i2rev) {
				for (i1=i2;i1<=i2+ip1-2;i1+=2) {
					for (i3=i1;i3<=ip3;i3+=ip2) {
						i3rev=i2rev+i3-i2;
						SWAP(data[i3],data[i3rev]);
						SWAP(data[i3+1],data[i3rev+1]);
					}
				}
			}
			ibit=ip2 >> 1;
			while (ibit >= ip1 && i2rev > ibit) {
				i2rev -= ibit;
				ibit >>= 1;
			}
			i2rev += ibit;
		}
		ifp1=ip1;
		while (ifp1 < ip2) {
			ifp2=ifp1 << 1;
			theta=isign*6.28318530717959/(ifp2/ip1);
			wtemp=sin(0.5*theta);
			wpr = -2.0*wtemp*wtemp;
			wpi=sin(theta);
			wr=1.0;
			wi=0.0;
			for (i3=1;i3<=ifp1;i3+=ip1) {
				for (i1=i3;i1<=i3+ip1-2;i1+=2) {
					for (i2=i1;i2<=ip3;i2+=ifp2) {
						k1=i2;
						k2=k1+ifp1;
						tempr=wr*data[k2]-wi*data[k2+1];
						tempi=wr*data[k2+1]+wi*data[k2];
						data[k2]=data[k1]-tempr;
						data[k2+1]=data[k1+1]-tempi;
						data[k1] += tempr;
						data[k1+1] += tempi;
					}
				}
				wr=(wtemp=wr)*wpr-wi*wpi+wr;
				wi=wi*wpr+wtemp*wpi+wi;
			}
			ifp1=ifp2;
		}
		nprev *= n;
	}
}

#undef SWAP

GraphWidget::GraphWidget(QWidget *)
    : timerId(0)
{
//    QGraphicsScene *scene = new QGraphicsScene(QRectF(0,0,500,500),this);
//    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
//	 scene->clear() ;
//    setScene(scene);
//    scene->setSceneRect(0, 0, 500, 500);

    setCacheMode(CacheBackground);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(AnchorUnderMouse);
    setResizeAnchor(AnchorViewCenter);
	 _friction_factor = 1.0f ;

    scale(qreal(0.8), qreal(0.8));
}

void GraphWidget::clearGraph()
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
	_edges.clear();
	_nodes.clear();
	_friction_factor = 1.0f ;
}

GraphWidget::NodeId GraphWidget::addNode(const std::string& node_short_string,const std::string& node_complete_string,NodeType type,AuthType auth,const std::string& ssl_id,const std::string& gpg_id)
{
    Node *node = new Node(node_short_string,type,auth,this,ssl_id,gpg_id);
     node->setToolTip(QString::fromUtf8(node_complete_string.c_str())) ;
	 _nodes.push_back(node) ;
    scene()->addItem(node);

	 std::map<std::string,QPointF>::const_iterator it(_node_cached_positions.find(gpg_id)) ;
	 if(_node_cached_positions.end() != it)
		 node->setPos(it->second) ;
	 else
	 {
		 qreal x1,y1,x2,y2 ;
		 sceneRect().getCoords(&x1,&y1,&x2,&y2) ;

		 float f1 = (type == GraphWidget::ELASTIC_NODE_TYPE_OWN)?0.5:(rand()/(float)RAND_MAX) ;
		 float f2 = (type == GraphWidget::ELASTIC_NODE_TYPE_OWN)?0.5:(rand()/(float)RAND_MAX) ;

		 node->setPos(x1+f1*(x2-x1),y1+f2*(y2-y1));
	 }
#ifdef DEBUG_ELASTIC
	 std::cerr << "Added node " << _nodes.size()-1 << std::endl ;
#endif
	 return _nodes.size()-1 ;
}
void GraphWidget::snapshotNodesPositions()
{
	for(uint32_t i=0;i<_nodes.size();++i)
		_node_cached_positions[_nodes[i]->idString()] = _nodes[i]->mapToScene(QPointF(0,0)) ;
}
void GraphWidget::clearNodesPositions()
{
	_node_cached_positions.clear() ;
}

GraphWidget::EdgeId GraphWidget::addEdge(NodeId n1,NodeId n2)
{
	std::pair<NodeId,NodeId> ed(std::min(n1,n2),std::max(n1,n2)) ;

	if( _edges.find(ed) == _edges.end() )
	{
		Edge *edge = new Edge(_nodes[n1],_nodes[n2]);
		scene()->addItem(edge);
		_edges[ed] = edge ;
#ifdef DEBUG_ELASTIC
		std::cerr << "Added edge " << n1 << " - " << n2 << std::endl ;
#endif
	}

	return 0 ;
}

void GraphWidget::itemMoved()
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

void GraphWidget::keyPressEvent(QKeyEvent *event)
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
        foreach (QGraphicsItem *item, scene()->items()) {
            if (qgraphicsitem_cast<Node *>(item))
                item->setPos(-150 + qrand() % 300, -150 + qrand() % 300);
        }
        break;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}

static void convolveWithGaussian(double *forceMap,int S,int /*s*/)
{
	static double *bf = NULL ;

	if(bf == NULL)
	{
		bf = new double[S*S*2] ;

		for(int i=0;i<S;++i)
			for(int j=0;j<S;++j)
			{
				int x = (i<S/2)?i:(S-i) ;
				int y = (j<S/2)?j:(S-j) ;
//				int l=2*(x*x+y*y);
				bf[2*(i+S*j)] = log(sqrtf(0.1 + x*x+y*y)); // linear -> derivative is constant
				bf[2*(i+S*j)+1] = 0 ;
			}

		unsigned long nn[2] = {S,S};
		fourn(&bf[-1],&nn[-1],2,1) ;
	}

	unsigned long nn[2] = {S,S};
	fourn(&forceMap[-1],&nn[-1],2,1) ;

	for(int i=0;i<S;++i)
		for(int j=0;j<S;++j)
		{
			float a = forceMap[2*(i+S*j)]*bf[2*(i+S*j)] - forceMap[2*(i+S*j)+1]*bf[2*(i+S*j)+1] ;
			float b = forceMap[2*(i+S*j)]*bf[2*(i+S*j)+1] + forceMap[2*(i+S*j)+1]*bf[2*(i+S*j)] ;

			forceMap[2*(i+S*j)]   = a ;
			forceMap[2*(i+S*j)+1] = b ;
		}

	fourn(&forceMap[-1],&nn[-1],2,-1) ;

	for(int i=0;i<S*S*2;++i)
		forceMap[i] /= S*S;
}

void GraphWidget::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

	 if(!isVisible())
		 return ;

	 static const int S = 256 ;
	 static double *forceMap = new double[2*S*S] ;

	 // Update force map only once every 8 hits.
	 //
	 static uint32_t hit = 0 ;

	 QRectF R(scene()->sceneRect()) ;

	 if( (hit++ & 7) == 0)
	 {
		 memset(forceMap,0,2*S*S*sizeof(double)) ;

		 foreach (Node *node, _nodes)
		 {
			 QPointF pos = node->mapToScene(QPointF(0,0)) ;

			 float x = S*(pos.x()-R.left())/R.width() ;
			 float y = S*(pos.y()- R.top())/R.height() ;

			 int i=(int)floor(x) ;
			 int j=(int)floor(y) ;
			 float di = x-i ;
			 float dj = y-j ;

			 if( i>=0 && i<S-1 && j>=0 && j<S-1)
			 {
				 forceMap[2*(i  +S*(j  ))] += (1-di)*(1-dj) ;
				 forceMap[2*(i+1+S*(j  ))] +=    di *(1-dj) ;
				 forceMap[2*(i  +S*(j+1))] += (1-di)*dj ;
				 forceMap[2*(i+1+S*(j+1))] +=    di *dj ;
			 }
		 }

		 // compute convolution with 1/omega kernel.
		 convolveWithGaussian(forceMap,S,20) ;
	 }

	 foreach (Node *node, _nodes)
	 {
		 QPointF pos = node->mapToScene(QPointF(0,0)) ;
		 float x = S*(pos.x()-R.left())/R.width() ;
		 float y = S*(pos.y()-R.top())/R.height() ;

		 node->calculateForces(forceMap,R.width(),R.height(),S,S,x,y,_friction_factor);
	 }

    bool itemsMoved = false;
    foreach (Node *node, _nodes) 
        if(node->advance())
            itemsMoved = true;

    if (!itemsMoved) {
        killTimer(timerId);
#ifdef DEBUG_ELASTIC
		  std::cerr << "Killing timr" << std::endl ;
#endif
        timerId = 0;
    }
	 _friction_factor *= 1.001f ;
//	 std::cerr << "Friction factor = " << _friction_factor << std::endl;
}

void GraphWidget::setEdgeLength(uint32_t l)
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

void GraphWidget::forceRedraw()
{
	for(uint32_t i=0;i<_nodes.size();++i)
		_nodes[i]->update(_nodes[i]->boundingRect()) ;
}
void GraphWidget::wheelEvent(QWheelEvent *event)
{
    scaleView(pow((double)2, -event->delta() / 240.0));
}

void GraphWidget::drawBackground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(rect);

    // Shadow
    QRectF sceneRect = this->sceneRect();
    QRectF rightShadow(sceneRect.right(), sceneRect.top() + 5, 5, sceneRect.height());
    QRectF bottomShadow(sceneRect.left() + 5, sceneRect.bottom(), sceneRect.width(), 5);
    if (rightShadow.intersects(rect) || rightShadow.contains(rect))
	painter->fillRect(rightShadow, Qt::darkGray);
    if (bottomShadow.intersects(rect) || bottomShadow.contains(rect))
	painter->fillRect(bottomShadow, Qt::darkGray);

    // Fill
    QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
    gradient.setColorAt(0, Qt::white);
    gradient.setColorAt(1, Qt::lightGray);
    painter->fillRect(rect.intersected(sceneRect), gradient);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(sceneRect);

    // Text
    QRectF textRect(sceneRect.left() + 4, sceneRect.top() + 4,
                    sceneRect.width() - 4, sceneRect.height() - 4);
    QString message(tr("Click and drag the nodes around, and zoom with the mouse "
                       "wheel or the '+' and '-' keys"));

    QFont font = painter->font();
    font.setBold(true);
    font.setPointSize(14);
    painter->setFont(font);
    painter->setPen(Qt::lightGray);
    painter->drawText(textRect.translated(2, 2), message);
    painter->setPen(Qt::black);
    painter->drawText(textRect, message);
}

void GraphWidget::scaleView(qreal scaleFactor)
{
    qreal factor = matrix().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;

    scale(scaleFactor, scaleFactor);
}

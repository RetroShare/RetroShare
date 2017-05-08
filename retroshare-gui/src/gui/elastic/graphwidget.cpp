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
#include "fft.h"

#include <iostream>
#include <QDebug>
#include <QGraphicsScene>
#include <QWheelEvent>

#include <math.h>

GraphWidget::GraphWidget(QWidget *)
    : timerId(0), mIsFrozen(false)
{
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
	scene()->clear();
	scene()->setSceneRect(0, 0, width(), height());
    
	_edges.clear();
	_nodes.clear();
	_friction_factor = 1.0f ;
}

GraphWidget::NodeId GraphWidget::addNode(const std::string& node_short_string,const std::string& node_complete_string,NodeType type,AuthType auth,const RsPeerId& ssl_id,const RsPgpId& gpg_id)
{
    Node *node = new Node(node_short_string,type,auth,this,ssl_id,gpg_id);
     node->setToolTip(QString::fromUtf8(node_complete_string.c_str())) ;
	 _nodes.push_back(node) ;
    scene()->addItem(node);

	 std::map<std::string,QPointF>::const_iterator it(_node_cached_positions.find(gpg_id.toStdString())) ;
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

static void convolveWithForce(double *forceMap,unsigned int S,int /*s*/)
{
	static double **bf = NULL ;
	static double **tmp = NULL ;
    static int *ip = NULL ;
    static double *w = NULL ;
    static uint32_t last_S = 0 ;

	if(bf == NULL)
	{
		bf  = fft::alloc_2d_double(S, 2*S);

        for(unsigned int i=0;i<S;++i)
            for(unsigned int j=0;j<S;++j)
			{
				int x = (i<S/2)?i:(S-i) ;
				int y = (j<S/2)?j:(S-j) ;

				bf[i][j*2+0] = log(sqrtf(0.1 + x*x+y*y)); // linear -> derivative is constant
				bf[i][j*2+1] = 0 ;
			}

        ip = fft::alloc_1d_int(2 + (int) sqrt(S + 0.5));
        w = fft::alloc_1d_double(S/2+S);
        ip[0] = 0;

		fft::cdft2d(S, 2*S, 1, bf, ip, w);
	}

    if(last_S != S)
    {
        if(tmp)
            fft::free_2d_double(tmp) ;

		tmp = fft::alloc_2d_double(S, 2*S);
        last_S = S ;
    }
    memcpy(tmp[0],forceMap,S*S*2*sizeof(double)) ;

	fft::cdft2d(S, 2*S, 1, tmp, ip, w);

	for (unsigned int i=0;i<S;++i)
		for (unsigned int j=0;j<S;++j)
		{
			float a = tmp[i][2*j+0]*bf[i][2*j+0] - tmp[i][2*j+1]*bf[i][2*j+1] ;
			float b = tmp[i][2*j+0]*bf[i][2*j+1] + tmp[i][2*j+1]*bf[i][2*j+0] ;

			tmp[i][2*j+0] = a ;
			tmp[i][2*j+1] = b ;
		}

	fft::cdft2d(S, 2*S,-1, tmp, ip, w);

    memcpy(forceMap,tmp[0],S*S*2*sizeof(double)) ;

    for(uint32_t i=0;i<2*S*S;++i)
        forceMap[i] /= S*S;
}

void GraphWidget::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

	 if(!isVisible())
		 return ;

	if (mIsFrozen)
	{
		update();
		return;
	}

	 static const int S = 256 ;
	 static double *forceMap = new double[2*S*S] ;

	 // Update force map only once every 8 hits.
	 //
	 static uint32_t hit = 0 ;

	 QRectF R(scene()->sceneRect()) ;

	 if( (hit++ & 3) == 0)
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
		 convolveWithForce(forceMap,S,20) ;
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
        if(node->progress())
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


void GraphWidget::setNameSearch(QString s)
{
    float f = QFontMetrics(font()).height()/16.0 ;
    
	if (s.length() == 0){
		for(uint32_t i=0;i<_nodes.size();++i)
			_nodes[i]->setNodeDrawSize(12 * f);
		forceRedraw();
		return;
	}
	std::string qs = s.toLower().toStdString();
	for(uint32_t i=0;i<_nodes.size();++i){
		Node* ni = _nodes[i];
		//std::cout << ni->descString() << std::endl;
		std::string ns = QString::fromStdString(ni->descString()).toLower().toStdString();

		if (ns.find(qs) != std::string::npos) {
			//std::cout << "found!" << '\n';
			ni->setNodeDrawSize(22 * f);
			//std::cout << ni->getNodeDrawSize() << '\n';
		} else {
			ni->setNodeDrawSize(12 * f);

		}
	}
	forceRedraw();
}

void GraphWidget::forceRedraw()
{
	for(uint32_t i=0;i<_nodes.size();++i)
		_nodes[i]->update(_nodes[i]->boundingRect()) ;
}

void GraphWidget::resizeEvent(QResizeEvent *event)
{
    scene()->setSceneRect(QRectF(QPointF(0,0),event->size()));
}

void GraphWidget::wheelEvent(QWheelEvent *event)
{
    scaleView(pow((double)2, -event->delta() / 240.0));
}

void GraphWidget::scaleView(qreal scaleFactor)
{
    qreal factor = matrix().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;

    scale(scaleFactor, scaleFactor);
}

void GraphWidget::setFreeze(bool freeze)
{
    mIsFrozen = freeze;
}

bool GraphWidget::isFrozen() const
{
	return mIsFrozen;
}

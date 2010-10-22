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

#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QtGui/QGraphicsView>

class Node;

class GraphWidget : public QGraphicsView
{
    Q_OBJECT

public:
    GraphWidget(QWidget * = NULL);

	 typedef int NodeId ;
	 typedef int EdgeId ;

	 typedef enum  {
		 	ELASTIC_NODE_FLAG_OWN				= 0x0001,
			ELASTIC_NODE_FLAG_FRIEND 			= 0x0002,
			ELASTIC_NODE_FLAG_AUTHED			= 0x0004,
			ELASTIC_NODE_FLAG_MARGINALAUTH	= 0x0008
	 } NodeFlags ;

    virtual void itemMoved();
	 NodeId addNode(const std::string& NodeText,uint32_t flags) ;
	 EdgeId addEdge(NodeId n1,NodeId n2) ;

	 void clearGraph() ;

protected:
    void keyPressEvent(QKeyEvent *event);
    void timerEvent(QTimerEvent *event);
    void wheelEvent(QWheelEvent *event);
    void drawBackground(QPainter *painter, const QRectF &rect);

    void scaleView(qreal scaleFactor);

private:
    int timerId;
    //Node *centerNode;
	 bool mDeterminedBB ;

	 std::vector<Node *> _nodes ;
	 std::vector<Node *> _edges ;
};

#endif

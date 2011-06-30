#ifndef VIEWER_H
#define VIEWER_H

#include <QGraphicsView>

class Viewer : public QGraphicsView
{
Q_OBJECT

public:
Viewer(QWidget *parent = 0);
virtual ~Viewer();

private:
virtual void resizeEvent(QResizeEvent*);
};

#endif

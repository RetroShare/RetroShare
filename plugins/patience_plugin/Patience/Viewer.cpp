#include "Viewer.h"
#include "Scene.h"
#include <QResizeEvent>

using namespace std;

Viewer::Viewer(QWidget *parent) : QGraphicsView(parent)
{
}


Viewer::~Viewer()
{
}


void Viewer::resizeEvent(QResizeEvent* event)
{
QGraphicsView::resizeEvent(event);

if (scene() != 0) ((Scene*) scene())->groessenanpassung(event);
}

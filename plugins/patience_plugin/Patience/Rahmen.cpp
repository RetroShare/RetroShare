#include "Rahmen.h"
#include "Basisstapel.h"
#include "Proportionen.h"
#include <QPainter>

using namespace std;

Rahmen::Rahmen(Basisstapel* parent) : QObject(parent), QGraphicsItem(0), Groesse(QSizeF(parent->boundingRect().size().width() + (((parent->boundingRect().size().height() / RAHMEN_DICKE_VERHAELTNIS) - 1) * 2), parent->boundingRect().size().height() + (((parent->boundingRect().size().height() / RAHMEN_DICKE_VERHAELTNIS) - 1) * 2))), meinstapel(0), dicke(parent->boundingRect().size().height() / RAHMEN_DICKE_VERHAELTNIS), eckradius(parent->boundingRect().size().height() / RAHMEN_ECKRADIUS_VERHAELTNIS)
{
setZValue(500);

setVisible(false);
}


Rahmen::~Rahmen()
{
}


void Rahmen::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
QPen stift(painter->pen());

stift.setWidth(dicke);
stift.setColor(Qt::darkRed);

painter->setPen(stift);

painter->drawRoundedRect(boundingRect().adjusted(dicke / 2, dicke / 2, -(dicke / 2), -(dicke / 2)), eckradius, eckradius);
}


QRectF Rahmen::boundingRect() const
{
return QRectF(QPointF(0, 0), Groesse);
}


void Rahmen::zeige(Basisstapel* stapel, const QPointF& position)
{
if (stapel != meinstapel || isVisible() == false)

meinstapel = stapel;

setPos(QPointF(position.x() - (dicke), position.y() - (dicke)));

setVisible(true);
}


void Rahmen::verstecke()
{
if (isVisible() == true) setVisible(false);
}


Basisstapel* Rahmen::aktueller_stapel() const
{
return meinstapel;
}


void Rahmen::passe_groesse_an(const QRectF& wert)
{
prepareGeometryChange();

Groesse = QSizeF(wert.size().width() + ((wert.size().height() / RAHMEN_DICKE_VERHAELTNIS) * 2), wert.size().height() + ((wert.size().height() / RAHMEN_DICKE_VERHAELTNIS) * 2));

dicke = wert.size().height() / RAHMEN_DICKE_VERHAELTNIS;
eckradius = wert.size().height() / RAHMEN_ECKRADIUS_VERHAELTNIS;

update();
}

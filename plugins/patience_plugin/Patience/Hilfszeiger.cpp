#include "Hilfszeiger.h"
#include "Basisstapel.h"
#include "Proportionen.h"
#include <QPainter>

using namespace std;

Hilfszeiger::Hilfszeiger(Basisstapel *parent) : QObject(parent), QGraphicsItem(parent), meinstapel(parent), breite(parent->boundingRect().width() / HILFSZEIGER_BREITE_VERHAELTNIS), hoehe(parent->boundingRect().height() / HILFSZEIGER_HOEHE_VERHAELTNIS), dicke(parent->boundingRect().height() / HILFSZEIGER_DICKE_VERHAELTNIS)
{
setZValue(1000);
}


Hilfszeiger::~Hilfszeiger()
{
}


void Hilfszeiger::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
painter->setPen(Qt::darkBlue);
painter->setBrush(Qt::darkBlue);

QPen stift(painter->pen());
stift.setWidth(dicke);
painter->setPen(stift);

painter->drawLine(breite / 2, 0 + (dicke / 2), breite / 2, hoehe - (dicke / 2));
painter->drawLine(breite / 2, 0 + (dicke / 2), dicke / 2, (hoehe / 2));
painter->drawLine(breite / 2, 0 + (dicke / 2), breite - (dicke / 2), (hoehe / 2));
}


QRectF Hilfszeiger::boundingRect() const
{
return QRectF(0, 0, breite, hoehe);
}


void Hilfszeiger::passe_groesse_an(const QRectF& wert)
{
prepareGeometryChange();

breite = wert.width() / HILFSZEIGER_BREITE_VERHAELTNIS;
hoehe =  wert.height() / HILFSZEIGER_HOEHE_VERHAELTNIS;
dicke = wert.height() / HILFSZEIGER_DICKE_VERHAELTNIS;

update();
}

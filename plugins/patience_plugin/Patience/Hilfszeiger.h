#ifndef HILFSZEIGER_H
#define HILFSZEIGER_H

#include <QObject>
#include <QGraphicsItem>

class Basisstapel;

class Hilfszeiger : public QObject, public QGraphicsItem
{
Q_OBJECT

public:
Hilfszeiger(Basisstapel *parent);
virtual ~Hilfszeiger();

virtual QRectF boundingRect() const;
void passe_groesse_an(const QRectF&);

private:
Basisstapel *meinstapel;
double breite, hoehe, dicke;

virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);
};

#endif

#ifndef RAHMEN_H
#define RAHMEN_H

#include <QObject>
#include <QGraphicsItem>

class Basisstapel;

class Rahmen : public QObject, public QGraphicsItem
{
Q_OBJECT

public:
Rahmen(Basisstapel* parent);
virtual ~Rahmen();

void zeige(Basisstapel*, const QPointF&);
Basisstapel *aktueller_stapel() const;
void passe_groesse_an(const QRectF&);

public slots:
void verstecke();

signals:

private slots:

private:
QSizeF Groesse;
Basisstapel* meinstapel;
double dicke;
double eckradius;

void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);
QRectF boundingRect() const;
};

#endif

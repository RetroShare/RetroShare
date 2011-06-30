#ifndef ABLAGESTAPEL_H
#define ABLAGESTAPEL_H

#include "Basisstapel.h"

class Ablagestapel : public Basisstapel
{
Q_OBJECT

public:
Ablagestapel(const QPixmap& pixmap, Scene* oparent, QGraphicsItem* gparent = 0);
virtual ~Ablagestapel();

virtual bool ablage_moeglich(Karte*) const;

public slots:
virtual void passe_groesse_an(double);

protected:
virtual QPointF ablageposition() const;
};

#endif

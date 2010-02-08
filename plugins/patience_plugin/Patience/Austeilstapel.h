#ifndef AUSTEILSTAPEL_H
#define AUSTEILSTAPEL_H

#include "Basisstapel.h"

class Austeilstapel : public Basisstapel
{
Q_OBJECT

public:
Austeilstapel(const QPixmap& pixmap, Scene* oparent, QGraphicsItem* gparent = 0);
virtual ~Austeilstapel();

virtual void registriere_costapel(Basisstapel*);
virtual void karte_wurde_aufgedeckt(Karte*);
virtual bool ablage_moeglich(Karte*) const;
virtual void undo_karten_ablage(Karte*);

public slots:
virtual void hilfsanfrage_start(Karte*);

private:
Basisstapel *costapel;

virtual void mousePressEvent(QGraphicsSceneMouseEvent*);
};

#endif

#ifndef ZIELSTAPEL_H
#define ZIELSTAPEL_H

#include "Basisstapel.h"
#include <QList>

class Siegkontrolle;

class Zielstapel : public Basisstapel
{
Q_OBJECT

public:
Zielstapel(const QPixmap& pixmap, Scene* oparent, QGraphicsItem* gparent = 0);
virtual ~Zielstapel();

virtual void registriere_siegkontrolle(Siegkontrolle*);
virtual void registriere_nachbar_zielstapel(Basisstapel*, Basisstapel*, Basisstapel*);

virtual void ablage_erfolgt();
virtual bool ablage_moeglich(Karte *) const;
virtual bool lege_karte_ab(Karte*);

public slots:
virtual void hilfsanfrage_start(Karte*);

private:
Siegkontrolle *siegkontrolle;
QList<Basisstapel*> nachbarn;
};

#endif

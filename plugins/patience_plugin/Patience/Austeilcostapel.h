#ifndef AUSTEILCOSTAPEL_H
#define AUSTEILCOSTAPEL_H

#include "Basisstapel.h"

class Austeilcostapel : public Basisstapel
{
Q_OBJECT

public:
Austeilcostapel(const QPixmap& pixmap, Scene* oparent, QGraphicsItem* gparent = 0);
virtual ~Austeilcostapel();

virtual void registriere_austeilstapel(Basisstapel*);
virtual bool ablage_moeglich(Karte *) const;
virtual bool lege_karte_ab(Karte*);
virtual void entferne_karte(Karte*);
virtual const QStringList speichere() const;
virtual bool lade(const QStringList&);

public slots:
virtual void resette_ablagenummer();
virtual void hilfsanfrage_start(Karte*);
virtual void passe_groesse_an(double);

private:
Basisstapel *austeilstapel;
bool nur_eine_ziehen;
virtual void alle_karten_einreihen();

protected:
int ablagenummer;

virtual QPointF ablageposition() const;
};

#endif

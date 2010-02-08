#ifndef BASISSTAPEL_H
#define BASISSTAPEL_H

#include "Zug.h"
#include <QGraphicsPixmapItem>
#include <QObject>
#include <QPixmap>
#include <QList>
#include <QPointF>
#include <QRectF>

class Karte;
class Siegkontrolle;
class Rahmen;
class Hilfszeiger;
class Scene;

class Basisstapel : public QObject, public QGraphicsPixmapItem
{
Q_OBJECT

public:
Basisstapel(const QPixmap& pixmap, Scene* oparent, QGraphicsItem* gparent = 0);
virtual ~Basisstapel();

void registriere_rahmen(Rahmen*);
void zeige_rahmen();

virtual bool beruehrungstest(Karte*);
virtual Karte* oberste_karte() const;
virtual bool ist_oberste_karte(Karte*);
virtual void lege_child_karten_ab(Karte*);
virtual void erhoehe_zwert();
virtual void normalisiere_zwert();
virtual int karten() const;
virtual void setze_kartenliste_zurueck();
virtual QRectF gesamt_rect() const;
virtual int ueberlappungs_flaeche(Karte*);
virtual bool nur_eine_wird_gezogen() const;
virtual bool ablage_moeglich(Karte *) const;
virtual bool lege_karte_ab(Karte*);
virtual void undo_karten_ablage(Karte*);
virtual void initialisiere_karte(Karte*);
virtual void karte_wurde_aufgedeckt(Karte*);
virtual void ablage_erfolgt();
virtual void entferne_karte(Karte*);
virtual void registriere_costapel(Basisstapel*);
virtual void registriere_austeilstapel(Basisstapel*);
virtual void resette_ablagenummer();
virtual void registriere_siegkontrolle(Siegkontrolle*);
virtual void registriere_nachbar_zielstapel(Basisstapel*, Basisstapel*, Basisstapel*);
virtual const QStringList speichere() const;
virtual bool lade(const QStringList&);

bool eine_wird_gezogen();

public slots:
virtual void eine_ziehen();
virtual void drei_ziehen();
virtual void hilfsanfrage_start(Karte*);
virtual void hilfsanfrage_ende();
virtual void passe_groesse_an(double);

signals:
void zug(const Zug&);
void stapel_durch();

protected:
QList<Karte*> kartenliste;

virtual QPointF ablageposition() const;
virtual int zwert() const;

private:
Scene *meine_scene;
bool nur_eine_ziehen;
Rahmen *meinrahmen;
Hilfszeiger *meinhilfszeiger;
QPixmap bild, bild_skaliert;
double stapelgroesse;

virtual void alle_karten_einreihen();
};

#endif

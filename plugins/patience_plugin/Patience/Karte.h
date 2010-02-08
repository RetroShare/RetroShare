#ifndef KARTE_H
#define KARTE_H

#include <QObject>
#include <QGraphicsPixmapItem>
#include <QGraphicsItem>
#include <QPixmap>
#include <QColor>
#include <QList>
#include <QPointF>

class Scene;
class Basisstapel;

class Karte : public QObject, public QGraphicsPixmapItem
{
Q_OBJECT

public:
Karte(const QPixmap& vorderseite, const QPixmap& hinterseite, Scene *oparent, QGraphicsItem *gparent = 0);
virtual ~Karte();

void setze_farbe(const QString&);
const QString& farbe() const;
void setze_wert(int);
int wert() const;
bool ist_vorderseite() const;
bool ist_rueckseite() const;
void registriere_stapel(const QList<Basisstapel*>& stapelliste_);
void setze_meinstapel(Basisstapel *meinstapel_);
void setze_rueckehrkoordinaten(const QPointF&);
void registriere_zielstapel(Basisstapel* kreutzstapel_, Basisstapel* pikstapel_, Basisstapel* herzstapel_, Basisstapel* karoherzstapel_);
Basisstapel* eigentuemer_stapel();
void setze_kartenbilder(const QPixmap&, const QPixmap&);
Basisstapel* suche_ziel();
QList<Karte*> kinderkarten();
QRectF gesamt_rect();
Karte* unterste_karte();
bool hat_kinderkarten() const;
const QStringList speichere() const;
bool lade(const QStringList&);
void nach_hause();
void speichere_zuhause();

public slots:
void zeige_vorderseite();
void zeige_rueckseite();
void sichtbarkeit_rahmen(bool);
void passe_groesse_an(double);

signals:
void rahmen_verstecken();
void hilfsanfrage_start(Karte*);
void hilfsanfrage_ende();

private:
QPixmap vorne, hinten, vorne_skaliert, hinten_skaliert;
QString Farbe;
int Wert;
bool vorderseite_oben, rahmen_anzeigen, double_click_sperre, gegrabbt;
Scene *scene;
QList<Basisstapel*> stapelliste;
Basisstapel *Meinstapel;
QPointF Rueckkehrkoordinaten;
Basisstapel *kreutzstapel, *pikstapel, *herzstapel, *karostapel;
double kartengroesse;

virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);
virtual void mousePressEvent(QGraphicsSceneMouseEvent*);
virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*);
};

#endif

#include "Karte.h"
#include "Basisstapel.h"
#include "Proportionen.h"
#include "Scene.h"
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

using namespace std;

Karte::Karte(const QPixmap& vorderseite, const QPixmap& hinterseite, Scene *oparent, QGraphicsItem *gparent) : QObject(oparent), QGraphicsPixmapItem(hinterseite, gparent), vorne(vorderseite), hinten(hinterseite), vorne_skaliert(vorderseite), hinten_skaliert(hinterseite), Wert(0), vorderseite_oben(false), rahmen_anzeigen(true), double_click_sperre(false), gegrabbt(false), scene(oparent), kreutzstapel(0), pikstapel(0), herzstapel(0), karostapel(0), kartengroesse(vorderseite.height())
{
setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
}


Karte::~Karte()
{
}


void Karte::setze_farbe(const QString& farbe_)
{
Farbe = farbe_;
}


const QString& Karte::farbe() const
{
return Farbe;
}


void Karte::setze_wert(int wert_)
{
Wert = wert_;
}


int Karte::wert() const
{
return Wert;
}


void Karte::zeige_vorderseite()
{
vorderseite_oben = true;

setPixmap(vorne_skaliert);

Meinstapel->karte_wurde_aufgedeckt(this);

// wenn es sich beim stapel nicht um austeilcostapel handelt die karte beweglich machen
if (Meinstapel->objectName() != BASISSTAPEL_AUSTEILCOSTAPEL) setFlag(QGraphicsItem::ItemIsMovable, true);

// ansonsten die karte unbeweglich machen. dies muss spaeter, im mouse release event wieder rueckgaengig gemacht werden
else setFlag(QGraphicsItem::ItemIsMovable, false);
}


void Karte::zeige_rueckseite()
{
vorderseite_oben = false;

setPixmap(hinten_skaliert);

setFlag(QGraphicsItem::ItemIsMovable, false);
}


bool Karte::ist_vorderseite() const
{
return vorderseite_oben;
}


bool Karte::ist_rueckseite() const
{
return !vorderseite_oben;
}


void Karte::registriere_stapel(const QList<Basisstapel*>& stapelliste_)
{
stapelliste = stapelliste_;
}


void Karte::setze_meinstapel(Basisstapel *meinstapel_)
{
Meinstapel = meinstapel_;
}


void Karte::setze_rueckehrkoordinaten(const QPointF& punkt)
{
Rueckkehrkoordinaten = punkt;
}


// diese methode ist fuer das ablegen der karte auf dem passenden zielstapel zustaendig, sofern moeglich
void Karte::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
if (event->buttons() == Qt::LeftButton && gegrabbt == false && flags() == QGraphicsItem::ItemIsMovable && event->button() == Qt::LeftButton && ist_vorderseite() == true && Meinstapel->ist_oberste_karte(this) == true && hat_kinderkarten() == false && Meinstapel->objectName().contains(BASISSTAPEL_ZIELSTAPEL) == false)
{
// nach einem double click soll die karte bis zum naechsten release gegen bewegungen geschuetzt werden
double_click_sperre = true;

// die karte soll nicht mehr über die anderen erhoeht werden
Meinstapel->normalisiere_zwert();

if (farbe() == KARTEN_KREUTZ) kreutzstapel->lege_karte_ab(this);
else if (farbe() == KARTEN_PIK) pikstapel->lege_karte_ab(this);
else if (farbe() == KARTEN_HERZ) herzstapel->lege_karte_ab(this);
else if (farbe() == KARTEN_KARO) karostapel->lege_karte_ab(this);

// sicherstellen, das die scene aktualisiert wird
scene->update();
}
}


void Karte::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
// linke maustaste
if (event->button() == Qt::LeftButton && gegrabbt == false)
{
gegrabbt = true;

// die karte soll ueber den anderen erhoeht angezeigt werden
Meinstapel->erhoehe_zwert();

if (ist_vorderseite() == true) QGraphicsPixmapItem::mousePressEvent(event);

// wenn diese karte die oberste karte ist und noch nicht die vorderseite gezeigt wird die vorderseite zeigen
else if (Meinstapel->ist_oberste_karte(this) == true && ist_vorderseite() == false) zeige_vorderseite();
}

// wenn die rechte maustaste gedrueckt wurde und die karte beweglich, jedoch nicht gegrabbt ist eine hilfsanfrage starten
else if (event->button() == Qt::RightButton && flags() == QGraphicsItem::ItemIsMovable && gegrabbt == false)
{
gegrabbt = true;

emit hilfsanfrage_start(this);
}
}


void Karte::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
// die karte soll nicht mehr über die anderen erhoeht werden
Meinstapel->normalisiere_zwert();

QGraphicsPixmapItem::mouseReleaseEvent(event);

// nur wenn keine weitere maustaste mehr gehalten wird
if (event->buttons() == Qt::NoButton)
{
// wenn double klick sperre nicht aktive ist
if (double_click_sperre == false)
{
Basisstapel *ziel = suche_ziel();

if (ziel == 0) setPos(Rueckkehrkoordinaten);

else if (ziel != 0 && ziel->lege_karte_ab(this) == false) setPos(Rueckkehrkoordinaten);

else setPos(Rueckkehrkoordinaten);

// die ist noetig, weil beim aufdecken auf dem austeilcostapel die karte unbeweglich gemacht wurde. dies ist jedoch nur zulaessig, wenn nur eine karte gezogen wird. wenn 3 gezogen werden, erfolgt die freigabe aus dem austeilstapel heraus
if (Meinstapel->objectName() == BASISSTAPEL_AUSTEILCOSTAPEL && Meinstapel->nur_eine_wird_gezogen() == true) setFlag(QGraphicsItem::ItemIsMovable, true);
}

// wenn double klick sperre doch aktive ist
else setPos(Rueckkehrkoordinaten);
}

// wenn doch noch eine weitere maustaste gehalten wird
else setPos(Rueckkehrkoordinaten);

// den rahmen wieder verstecken
emit rahmen_verstecken();

// den hilfs pfeil wieder ausblenden
emit hilfsanfrage_ende();

// die double click sperre wieder aufheben, damit die karte zukuenftig wieder beweglich ist
double_click_sperre = false;

gegrabbt = false;

// sicherstellen, das das element nicht laenger gegrabbt ist !!!
ungrabMouse();
}


Basisstapel* Karte::eigentuemer_stapel()
{
return Meinstapel;
}


void Karte::registriere_zielstapel(Basisstapel* kreutzstapel_, Basisstapel* pikstapel_, Basisstapel* herzstapel_, Basisstapel* karostapel_)
{
kreutzstapel = kreutzstapel_;
pikstapel = pikstapel_;
herzstapel = herzstapel_;
karostapel = karostapel_;
}


void Karte::setze_kartenbilder(const QPixmap& vorne_, const QPixmap& hinten_)
{
vorne = vorne_;
hinten = hinten_;

passe_groesse_an(kartengroesse);
}


Basisstapel* Karte::suche_ziel()
{
QList<Basisstapel*> moegliche_ziele(stapelliste);

Basisstapel *erg = 0;

// den eigenen stapel aus moegliche_ziele entfernen
moegliche_ziele.removeAll(Meinstapel);

// alle unmoeglichen stapel entfernen
for (register int idx = 0; idx < moegliche_ziele.size(); idx++) if (moegliche_ziele.at(idx)->ablage_moeglich(this) == false && (moegliche_ziele.at(idx)->objectName().contains("ziel") == false || (moegliche_ziele.at(idx)->objectName().contains("ziel") == true && kreutzstapel->ablage_moeglich(this) == false && pikstapel->ablage_moeglich(this) == false && herzstapel->ablage_moeglich(this) == false && karostapel->ablage_moeglich(this) == false)))
{
moegliche_ziele.removeAt(idx);

idx--;
}

// wenn das ziel noch nicht eindeutig ist die groesste ueberlappung finden
if (moegliche_ziele.size() > 1)
{
// das ziel mit der groessten ueberlappenden flaeche finden
Basisstapel *ziel = 0;
int groesste_ueberlappung = 0;

for (register int idx = 0; idx < moegliche_ziele.size(); idx++)
{
int ueberlappung = moegliche_ziele.at(idx)->ueberlappungs_flaeche(this);

if (ueberlappung > groesste_ueberlappung)
{
groesste_ueberlappung = ueberlappung;

ziel = moegliche_ziele.at(idx);
}
}

erg = ziel;
}

else if (moegliche_ziele.size() == 1) erg = moegliche_ziele.first();

// besteht mit dem sieger eine beruehrung ?
if (erg != 0 && erg->beruehrungstest(this) == false) erg = 0;

// wird ein zielstapel angepeilt?
if (erg != 0 && erg->objectName().contains("ziel") == true)
{
if (farbe() == KARTEN_KREUTZ && kreutzstapel->ablage_moeglich(this) == true) erg = kreutzstapel;
else if (farbe() == KARTEN_PIK && pikstapel->ablage_moeglich(this) == true) erg = pikstapel;
else if (farbe() == KARTEN_HERZ && herzstapel->ablage_moeglich(this) == true) erg = herzstapel;
else if (farbe() == KARTEN_KARO && karostapel->ablage_moeglich(this) == true) erg = karostapel;
else erg = 0;
}

return erg;
}


void Karte::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
if (gegrabbt == true)
{
if (double_click_sperre == false && event->buttons() == Qt::LeftButton) QGraphicsPixmapItem::mouseMoveEvent(event);

else setPos(Rueckkehrkoordinaten);

if (rahmen_anzeigen == true)
{
Basisstapel *ziel = suche_ziel();

if (ziel != 0) ziel->zeige_rahmen();

else
{
emit rahmen_verstecken();
}
}
}
}


void Karte::sichtbarkeit_rahmen(bool wert)
{
rahmen_anzeigen = wert;
}


QRectF Karte::gesamt_rect()
{
QRectF erg(sceneBoundingRect());

Karte *testkarte = unterste_karte();

erg.setBottomRight(testkarte->sceneBoundingRect().bottomRight());

return erg;
}


Karte* Karte::unterste_karte()
{
Karte *erg = this;

if (hat_kinderkarten() == true) erg = kinderkarten().last();

return erg;
}


// wenn die karte child items besitzt, die nicht vom typ karte sind muessen diese hier herausgefiltert werden !!!
bool Karte::hat_kinderkarten() const
{
bool erg = false;

if (childItems().size() > 1)
{
qDebug() << tr("The Card ") << objectName() << tr(" have more than one Child Card");

exit(1);
}

if (childItems().size() > 0) erg = true;

return erg;
}


// wenn die karte child items besitzt, die nicht vom typ karte sind muessen diese hier herausgefiltert werden !!!
QList<Karte*> Karte::kinderkarten()
{
QList<Karte*> erg;

if (childItems().size() > 1)
{
qDebug() << tr("The Card ") << objectName() << tr(" have more than one Child Card");

exit(1);
}

if (childItems().isEmpty() == false) erg.append((Karte*) childItems().first());

if (erg.isEmpty() == false) erg.append(erg.first()->kinderkarten());

return erg;
}


const QStringList Karte::speichere() const
{
QStringList erg;

// die karten id speichern
erg.append(objectName());

// die kartenseite speichern
erg.append(QString::number(ist_vorderseite()));

return erg;
}


bool Karte::lade(const QStringList& daten)
{
bool erg = false;

if (daten.size() == KARTE_ANZAHL_SPEICHERELEMENTE && daten.first() == objectName())
{
erg = true;

if (daten.at(KARTE_IST_VORNE_IDX).toInt() == 1) zeige_vorderseite();

else zeige_rueckseite();
}

return erg;
}


void Karte::passe_groesse_an(double wert)
{
kartengroesse = wert;

vorne_skaliert = vorne.scaledToHeight(kartengroesse, Qt::SmoothTransformation);
hinten_skaliert = hinten.scaledToHeight(kartengroesse, Qt::SmoothTransformation);

if (ist_vorderseite() == true) setPixmap(vorne_skaliert);
else setPixmap(hinten_skaliert);
}


void  Karte::nach_hause()
{
setPos(Rueckkehrkoordinaten);

emit rahmen_verstecken();

if (scene->mouseGrabberItem() == this) ungrabMouse();
}


void Karte::speichere_zuhause()
{
Rueckkehrkoordinaten = pos();
}

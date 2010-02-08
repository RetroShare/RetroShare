#include "Basisstapel.h"
#include "Karte.h"
#include "Rahmen.h"
#include "Hilfszeiger.h"
#include "Scene.h"
#include "Proportionen.h"

using namespace std;

Basisstapel::Basisstapel(const QPixmap& pixmap, Scene* oparent, QGraphicsItem* gparent) : QObject(oparent), QGraphicsPixmapItem(pixmap, gparent), meine_scene(oparent), nur_eine_ziehen(true), meinrahmen(0), meinhilfszeiger(0), bild(pixmap), bild_skaliert(pixmap), stapelgroesse(pixmap.height())
{
// den hilfszeiger erstellen
meinhilfszeiger = new Hilfszeiger(this);
meinhilfszeiger->hide();
}


Basisstapel::~Basisstapel()
{
}


bool Basisstapel::lege_karte_ab(Karte* karte)
{
bool erg = false;

if (ablage_moeglich(karte) == true)
{
erg = true;

Basisstapel *quelle(karte->eigentuemer_stapel());

// die oberste karte,oder den stapel, wenn keine karte vorhanden ist zum parent der karte machen
if (oberste_karte() == 0) karte->setParentItem(this);
else karte->setParentItem(oberste_karte());

// die position der karte einstellen
karte->setPos(ablageposition());

// die karte aus dem alten stapel entfernen
karte->eigentuemer_stapel()->entferne_karte(karte);

// der karten den stapel, zu der sie jetzt gehoert mitteilen
karte->setze_meinstapel(this);

// die rueckkehrposition der karte anpassen
karte->setze_rueckehrkoordinaten(karte->pos());

// den zwert der karte setzen
karte->setZValue(zwert());

// die karte in die kartenliste einfuegen
kartenliste.append(karte);

// die kinder karten der karten ebenfalls ablegen
lege_child_karten_ab(karte);

emit zug(Zug(karte, quelle, this));

ablage_erfolgt();
}

return erg;
}


bool Basisstapel::beruehrungstest(Karte* karte)
{
return gesamt_rect().intersects(karte->gesamt_rect());
}


bool Basisstapel::ablage_moeglich(Karte* karte) const
{
bool erg = false;

if ((oberste_karte() == 0 || (oberste_karte() != 0 && oberste_karte()->ist_vorderseite() == true)) && karte->eigentuemer_stapel() != this) erg = true;

return erg;
}


QPointF Basisstapel::ablageposition() const
{
return QPointF(0, 0);
}


void Basisstapel::initialisiere_karte(Karte* karte)
{
// die oberste karte,oder den stapel wenn keine karte vorhanden ist zum parent der karte machen
if (oberste_karte() == 0) karte->setParentItem(this);
else karte->setParentItem(oberste_karte());

// die position der karte einstellen
karte->setPos(ablageposition());

// der karten den stapel, zu der sie jetzt gehoert mitteilen
karte->setze_meinstapel(this);

// die rueckkehrposition der karte anpassen
karte->setze_rueckehrkoordinaten(karte->pos());

// den zwert der karte setzen
karte->setZValue(zwert());

// die karte in die kartenliste einfuegen
kartenliste.append(karte);
}


int Basisstapel::zwert() const
{
int erg = zValue();

if (kartenliste.isEmpty() == false) erg = kartenliste.last()->zValue();

erg++;

return erg;
}


Karte* Basisstapel::oberste_karte() const
{
Karte* erg = 0;

if (kartenliste.isEmpty() == false) erg = kartenliste.last();

return erg;
}


bool Basisstapel::ist_oberste_karte(Karte* karte)
{
bool erg = false;

if (oberste_karte() != 0 && oberste_karte() == karte) erg = true;

return erg;
}


void Basisstapel::entferne_karte(Karte* karte)
{
if (kartenliste.contains(karte) == true) kartenliste.removeAll(karte);
}


void Basisstapel::karte_wurde_aufgedeckt(Karte* karte)
{
emit zug(Zug(karte, this));
}


void Basisstapel::lege_child_karten_ab(Karte* karte)
{
QList<Karte*> kinder(karte->kinderkarten());

for (register int idx = 0; idx < kinder.size(); idx++)
{
// die position der karte einstellen
kinder.at(idx)->setPos(ablageposition());

// die karte aus dem alten stapel entfernen
kinder.at(idx)->eigentuemer_stapel()->entferne_karte(kinder.at(idx));

// der karten den stapel, zu der sie jetzt gehoert mitteilen
kinder.at(idx)->setze_meinstapel(this);

// die rueckkehrposition der karte anpassen
kinder.at(idx)->setze_rueckehrkoordinaten(kinder.at(idx)->pos());

// den zwert der karte setzen
kinder.at(idx)->setZValue(zwert());

// die karte in die kartenliste einfuegen
kartenliste.append(kinder.at(idx));
}
}


void Basisstapel::erhoehe_zwert()
{
setZValue(1000);
}


void Basisstapel::normalisiere_zwert()
{
setZValue(0);
}


int Basisstapel::karten() const
{
return kartenliste.size();
}


void Basisstapel::setze_kartenliste_zurueck()
{
kartenliste.clear();
}


void Basisstapel::ablage_erfolgt()
{
}


QRectF  Basisstapel::gesamt_rect() const
{
QRectF erg(sceneBoundingRect());

if (oberste_karte() != 0) erg.setBottomRight(oberste_karte()->sceneBoundingRect().bottomRight());

return erg;
}


int Basisstapel::ueberlappungs_flaeche(Karte* karte)
{
QRectF ueberlappung_rect(gesamt_rect().intersect(karte->gesamt_rect()));

return ueberlappung_rect.width() * ueberlappung_rect.height();
}


bool Basisstapel::nur_eine_wird_gezogen() const
{
return nur_eine_ziehen;
}


void Basisstapel::eine_ziehen()
{
nur_eine_ziehen = true;
}


void Basisstapel::drei_ziehen()
{
nur_eine_ziehen = false;
}


void Basisstapel::registriere_costapel(Basisstapel*)
{
}


void Basisstapel::registriere_austeilstapel(Basisstapel*)
{
}


void Basisstapel::resette_ablagenummer()
{
}


void Basisstapel::alle_karten_einreihen()
{
}


void Basisstapel::registriere_siegkontrolle(Siegkontrolle*)
{
}


void Basisstapel::registriere_nachbar_zielstapel(Basisstapel*, Basisstapel*, Basisstapel*)
{
}


void Basisstapel::registriere_rahmen(Rahmen *rahmen)
{
meinrahmen = rahmen;
}


void Basisstapel::zeige_rahmen()
{
if (meinrahmen != 0)
{
QPointF position(scenePos());

if (oberste_karte() != 0) position = oberste_karte()->scenePos();

meinrahmen->zeige(this, position);
}
}


void Basisstapel::hilfsanfrage_start(Karte* karte)
{
// wenn die ablage der karte moeglich ist
if (ablage_moeglich(karte) == true)
{
// die position des hilfszeiger setzen

meinhilfszeiger->setPos(QPointF(boundingRect().width() / 2 - meinhilfszeiger->boundingRect().width() / 2, gesamt_rect().height() + stapelgroesse / BASISSTAPEL_ABSTAND_HILFSZEIGER_VERHAELTNIS));
meinhilfszeiger->show();
}
}


void Basisstapel::hilfsanfrage_ende()
{
// den hilfszeiger verstecken
meinhilfszeiger->hide();
}


void Basisstapel::undo_karten_ablage(Karte* karte)
{
// die oberste karte,oder den stapel, wenn keine karte vorhanden ist zum parent der karte machen
if (oberste_karte() == 0) karte->setParentItem(this);
else karte->setParentItem(oberste_karte());

// die position der karte einstellen
karte->setPos(ablageposition());

// die karte aus dem alten stapel entfernen
karte->eigentuemer_stapel()->entferne_karte(karte);

// der karten den stapel, zu der sie jetzt gehoert mitteilen
karte->setze_meinstapel(this);

// die rueckkehrposition der karte anpassen
karte->setze_rueckehrkoordinaten(karte->pos());

// den zwert der karte setzen
karte->setZValue(zwert());

// die karte in die kartenliste einfuegen
kartenliste.append(karte);

// die kinder karten der karten ebenfalls ablegen
lege_child_karten_ab(karte);
}


const QStringList Basisstapel::speichere() const
{
QStringList erg;

// den namen des stapels als id speichern
erg.append(objectName());

// die enthaltenen karten speichern
QString karten;

for (register int idx = 0; idx < kartenliste.size(); idx++)
{
if (idx > 0) karten.append(BASISSTAPEL_KARTEN_SPLITTER);

karten.append(kartenliste.at(idx)->objectName());
}

erg.append(karten);

return erg;
}


bool Basisstapel::lade(const QStringList& daten)
{
bool erg = false;

if (daten.size() >= BASISSTAPEL_ANZAHL_SPEICHERELEMENTE && daten.first() == objectName())
{
erg = true;

// die enthaltenen karten laden und an den stapel anhaengen
QStringList karten(daten.at(BASISSTAPEL_KARTEN_IDX).split(BASISSTAPEL_KARTEN_SPLITTER, QString::SkipEmptyParts));

for (register int idx = 0; idx < karten.size(); idx++)
{
Karte *karte = meine_scene->suche_karte(karten.at(idx));

if (karte == 0) erg = false;
else initialisiere_karte(karte);
}
}

return erg;
}


void Basisstapel::passe_groesse_an(double wert)
{
stapelgroesse = wert;

bild_skaliert = bild.scaledToHeight(stapelgroesse, Qt::SmoothTransformation);

setPixmap(bild_skaliert);

// auch den hiflszeiger und den rahmen anpassen
if (meinhilfszeiger != 0) meinhilfszeiger->passe_groesse_an(boundingRect());
if (meinrahmen != 0) meinrahmen->passe_groesse_an(boundingRect());
}


bool Basisstapel::eine_wird_gezogen()
{
return nur_eine_ziehen;
}

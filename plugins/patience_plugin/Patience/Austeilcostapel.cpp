#include "Austeilcostapel.h"
#include "Karte.h"
#include "Proportionen.h"
#include "Scene.h"

using namespace std;

Austeilcostapel::Austeilcostapel(const QPixmap& pixmap, Scene* oparent, QGraphicsItem* gparent) : Basisstapel(pixmap, oparent, gparent), austeilstapel(0), ablagenummer(0)
{
}


Austeilcostapel::~Austeilcostapel()
{
}


void Austeilcostapel::registriere_austeilstapel(Basisstapel* stapel)
{
austeilstapel = stapel;
}


bool Austeilcostapel::ablage_moeglich(Karte * karte) const
{
bool erg = false;

if (karte->eigentuemer_stapel()->objectName() == BASISSTAPEL_AUSTEILSTAPEL) erg = true;

return erg;
}


QPointF Austeilcostapel::ablageposition() const
{
QPointF erg(Basisstapel::ablageposition());

// wenn drei karten gezogen werden und die ablagenummer groesser als 0 ist eine drift nach rechts zur position hinzufuegen
if (nur_eine_wird_gezogen() == false && ablagenummer > 0) erg.setX(erg.x() + (boundingRect().width() / AUSTEILCOSTAPEL_XDRIFT_VERHAELTNIS));

return erg;
}


bool Austeilcostapel::lege_karte_ab(Karte* karte)
{
bool erg = false;

// nur, wenn eine ablage ueberhaupt moeglich ist
if (ablage_moeglich(karte) == true)
{
if (nur_eine_wird_gezogen() == false && oberste_karte() != 0) oberste_karte()->setFlag(QGraphicsItem::ItemIsMovable, false);

// wenn bereits 3 karten aufgefaechert daliegen
if (nur_eine_wird_gezogen() == false && ablagenummer == 3)
{
// ablagenummer wieder zuruecksetzen (die nachste karte wird ohne drift eingefuegt)
ablagenummer = 0;

// die bisher im stapel befindlichen karten sollen ohne drift am stapel liegen
alle_karten_einreihen();
}

erg = Basisstapel::lege_karte_ab(karte);

// wenn drei karten gezogen werden die ablagenummer um 1 erhoehen
if (nur_eine_wird_gezogen() == false) ablagenummer++;
}

return erg;
}


void Austeilcostapel::alle_karten_einreihen()
{
// von allen karten im stapel die drift entfernen
for (register int idx = 0; idx < kartenliste.size(); idx++) kartenliste.at(idx)->setPos(0, 0);
}


void Austeilcostapel::resette_ablagenummer()
{
// die ablagenummer zuruecksetzen
ablagenummer = 0;

// von allen karten im stapel die drift entfernen
alle_karten_einreihen();
}


void Austeilcostapel::entferne_karte(Karte* karte)
{
Basisstapel::entferne_karte(karte);

if (oberste_karte() != 0) oberste_karte()->setFlag(QGraphicsItem::ItemIsMovable);
}


void Austeilcostapel::hilfsanfrage_start(Karte*)
{
}


const QStringList Austeilcostapel::speichere() const
{
QStringList erg(Basisstapel::speichere());

// die ablegenummer speichern
erg.append(QString::number(ablagenummer));

return erg;
}


bool Austeilcostapel::lade(const QStringList& daten)
{
bool erg = Basisstapel::lade(daten);

if (erg == true && daten.size() >= AUSTEILSTAPEL_ANZAHL_SPEICHERELEMENTE)
{
// die ablagenummer laden
ablagenummer = daten.at(AUSTEILSTAPEL_ABLAGENUMMER_IDX).toInt();
}

return erg;
}


void Austeilcostapel::passe_groesse_an(double wert)
{
if (oberste_karte() != 0) oberste_karte()->nach_hause();

Basisstapel::passe_groesse_an(wert);

for (register int idx1 = kartenliste.size() - 1, idx2 = 0; eine_wird_gezogen() == false && idx1 > 0 && idx2 < ablagenummer; idx1--, idx2++)
{
kartenliste.at(idx1)->setPos(QPointF(boundingRect().width() / AUSTEILCOSTAPEL_XDRIFT_VERHAELTNIS, kartenliste.at(idx1)->pos().y()));

kartenliste.at(idx1)->speichere_zuhause();
}
}

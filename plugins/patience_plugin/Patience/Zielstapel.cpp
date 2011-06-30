#include "Zielstapel.h"
#include "Karte.h"
#include "Siegkontrolle.h"
#include <QDebug>

using namespace std;

Zielstapel::Zielstapel(const QPixmap& pixmap, Scene* oparent, QGraphicsItem* gparent) : Basisstapel(pixmap, oparent, gparent), siegkontrolle(0)
{
}


Zielstapel::~Zielstapel()
{
}


bool Zielstapel::ablage_moeglich(Karte* karte) const
{
bool erg = false;

if (Basisstapel::ablage_moeglich(karte) == true && ((oberste_karte() == 0 && karte->wert() == 1) || (oberste_karte() != 0 &&oberste_karte()->farbe() == karte->farbe() && karte->wert() == (oberste_karte()->wert() + 1))) && karte->hat_kinderkarten() == false) erg = true;

return erg;
}


void Zielstapel::registriere_siegkontrolle(Siegkontrolle* siegkontrolle_)
{
siegkontrolle = siegkontrolle_;
}


void Zielstapel::ablage_erfolgt()
{
siegkontrolle->teste_auf_sieg();
}


void Zielstapel::registriere_nachbar_zielstapel(Basisstapel* erster, Basisstapel* zweiter, Basisstapel* dritter)
{
nachbarn.append(erster);
nachbarn.append(zweiter);
nachbarn.append(dritter);
}


bool Zielstapel::lege_karte_ab(Karte* karte)
{
bool erg = false;
Basisstapel *ziel = 0;

QString quelle(karte->eigentuemer_stapel()->objectName());

// wenn dieser stapel der richtige ist ...
if (objectName().contains(karte->farbe()) == true) erg = Basisstapel::lege_karte_ab(karte);

// ansonsten den richtigen stapel suchen ...
else
{
for (register int idx = 0; idx < nachbarn.size() && ziel == 0; idx++) if (nachbarn.at(idx)->objectName().contains(karte->farbe()) == true) ziel = nachbarn.at(idx);

// ... und die karte auf diesem ablegen
if (ziel != 0) erg = ziel->lege_karte_ab(karte);

else
{
qDebug() << tr("Destination not found in Zielstapel::lege_karte_ab(Karte* karte)");
}
}

return erg;
}


void Zielstapel::hilfsanfrage_start(Karte* karte)
{
// dafuer sorgen, das der hilfspfeil nur uber dem richtigen zielstapel erscheint
if (objectName().contains(karte->farbe()) == true) Basisstapel::hilfsanfrage_start(karte);
}

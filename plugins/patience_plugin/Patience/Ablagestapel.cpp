#include "Ablagestapel.h"
#include "Karte.h"
#include "Proportionen.h"
#include "Scene.h"

using namespace std;

Ablagestapel::Ablagestapel(const QPixmap& pixmap, Scene* oparent, QGraphicsItem* gparent) : Basisstapel(pixmap, oparent, gparent)
{
}


Ablagestapel::~Ablagestapel()
{
}


QPointF Ablagestapel::ablageposition() const
{
QPointF erg;

// im ablagestapel sollen die karten aufgefaechert abgelegt werden, deshalb YDRIFT hinzufuegen. die position ist relative zum parent grafik objekt
if (kartenliste.size() > 0) erg.setY(boundingRect().height() / ABLAGESTAPEL_YDRIFT_VERHAELTNIS);

return erg;
}


bool Ablagestapel::ablage_moeglich(Karte* karte) const
{
bool erg = false;

if (Basisstapel::ablage_moeglich(karte) == true && ((oberste_karte() == 0 && karte->wert() == 13) || (oberste_karte() != 0 && oberste_karte()->farbe() != karte->farbe() && karte->wert() == (oberste_karte()->wert() - 1)))) erg = true;

if (erg == true && oberste_karte() != 0)
{
if (karte->farbe() == KARTEN_KREUTZ && oberste_karte()->farbe() == KARTEN_PIK) erg = false;
else if (karte->farbe() == KARTEN_PIK && oberste_karte()->farbe() == KARTEN_KREUTZ) erg = false;
else if (karte->farbe() == KARTEN_HERZ && oberste_karte()->farbe() == KARTEN_KARO) erg = false;
else if (karte->farbe() == KARTEN_KARO && oberste_karte()->farbe() == KARTEN_HERZ) erg = false;
}

return erg;
}


void Ablagestapel::passe_groesse_an(double wert)
{
if (oberste_karte() != 0) oberste_karte()->nach_hause();

Basisstapel::passe_groesse_an(wert);

for (register int idx = 1; idx < kartenliste.size(); idx++)
{
kartenliste.at(idx)->setPos(kartenliste.at(idx)->pos().x(), (double) boundingRect().height() / ABLAGESTAPEL_YDRIFT_VERHAELTNIS);

kartenliste.at(idx)->speichere_zuhause();
}
}

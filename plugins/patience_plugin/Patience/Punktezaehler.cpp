#include "Punktezaehler.h"
#include "Scene.h"
#include "Basisstapel.h"
#include "Proportionen.h"
#include <QTimer>
#include <QDebug>

using namespace std;

Punktezaehler::Punktezaehler(Scene *parent) : QObject(parent), punkte(0), scene(parent), nur_eine_ziehen(true)
{
// straftimer erstellen und seinen intervall auf 10 sekunden einstellen
straftimer = new QTimer(this);
straftimer->setInterval(10000);

// signal - slot verbindungen
// reaktionen auf straftimer ermoeglichen
connect(straftimer, SIGNAL(timeout()), this, SLOT(reaktion_auf_timeout()));
}


Punktezaehler::~Punktezaehler()
{
}


void Punktezaehler::neuer_zug(const Zug& zug)
{
// auf ungueltigen zug pruefen
if (zug.ist_gueltig() == false)
{
qDebug() << tr("Invalid move in Punktzaehler::neuer_zug()");

exit(1);
}

// nur, wenn ein spiel laeuft
if (scene->laufendes_spiel() == true)
{
// wenn straftimer noch nicht laueft
if (straftimer->isActive() == false && scene->laufendes_spiel() == true)
{
straftimer->start();

emit erster_zug();
}

punkte += kalkuliere_punkte(zug);

if (straftimer->isActive() == true && bringt_zeitaufschub(zug) == true)
{
straftimer->stop();
straftimer->start();
}

emit neue_punktzahl(punkte);
}
}


void Punktezaehler::neues_spiel()
{
straftimer->stop();

punkte = 0;

emit neue_punktzahl(punkte);

nach_oben_liste.clear();
}


void Punktezaehler::spiel_zuende()
{
straftimer->stop();
}


void Punktezaehler::reaktion_auf_timeout()
{
if (punkte > 0)
{
if (nur_eine_ziehen == true) punkte -= PUNKTEZAEHLER_EINE_ZIEHEN_PUNKTEABZUG_ZEIT;
else punkte -= PUNKTEZAEHLER_DREI_ZIEHEN_PUNKTEABZUG_ZEIT;

if (punkte < 0) punkte = 0;

emit neue_punktzahl(punkte);
}
}


void Punktezaehler::stapel_durch()
{
if (punkte > 0)
{
if (nur_eine_ziehen == true) punkte -= PUNKTEZAEHLER_EINE_ZIEHEN_AUSTEILSTAPEL_DURCH;
else punkte -= PUNKTEZAEHLER_DREI_ZIEHEN_AUSTEILSTAPEL_DURCH;

if (punkte < 0) punkte = 0;

emit neue_punktzahl(punkte);
}
}


int Punktezaehler::punktstand() const
{
return punkte;
}


void Punktezaehler::eine_ziehen()
{
nur_eine_ziehen = true;
}


void Punktezaehler::drei_ziehen()
{
nur_eine_ziehen = false;
}


void Punktezaehler::undo_meldung(const Zug& zug)
{
// die punke des rueckgaengi gemachten zuges wieder abziehen
nach_oben_liste.removeAll(zug.karte_name());
punkte -= kalkuliere_punkte(zug);

emit neue_punktzahl(punkte);
}


int Punktezaehler::kalkuliere_punkte(const Zug& zug) const
{
int erg = 0;

// wenn es sich um eine bewegung handelt
if (zug.ist_bewegung() == true)
{
// vom austeilstapel zu einem ablagestapel bringt 5 punkte
if (zug.herkunft_name() == BASISSTAPEL_AUSTEILCOSTAPEL && zug.ziel_name().contains(BASISSTAPEL_ABLAGESTAPEL) == true)
{
if (nur_eine_ziehen == true) erg += PUNKTEZAEHLER_EINE_ZIEHEN_AUSTEILSTAPEL_ZU_ABLAGESTAPEL;
else erg += PUNKTEZAEHLER_DREI_ZIEHEN_AUSTEILSTAPEL_ZU_ABLAGESTAPEL;
}
 
// vom austeilcostapel zum zielstapel bringt 15 punkte
else if (zug.herkunft_name() == BASISSTAPEL_AUSTEILCOSTAPEL && zug.ziel_name().contains(BASISSTAPEL_ZIELSTAPEL) == true && nach_oben_liste.contains(zug.karte_name()) == false)
{
// man soll fuer das hochlegen einer karte nicht mehrfach punkte kassieren koennen
if (nur_eine_ziehen == true) erg += PUNKTEZAEHLER_EINE_ZIEHEN_AUSTEILSTAPEL_ZU_ZIELSTAPEL;
else erg += PUNKTEZAEHLER_DREI_ZIEHEN_AUSTEILSTAPEL_ZU_ZIELSTAPEL;
}

// von unten zum zielstapel bringt 10 punkte
else if (zug.herkunft_name().contains(BASISSTAPEL_ABLAGESTAPEL) == true && zug.ziel_name().contains(BASISSTAPEL_ZIELSTAPEL) == true && nach_oben_liste.contains(zug.karte_name()) == false)
{
if (nur_eine_ziehen == true) erg += PUNKTEZAEHLER_EINE_ZIEHEN_ABLAGESTAPEL_ZU_ZIELSTAPEL;
else erg += PUNKTEZAEHLER_DREI_ZIEHEN_ABLAGESTAPEL_ZU_ZIELSTAPEL;
}
}

// wenn es sich bei zug um eine aufdeckung handelt
else if (zug.ist_aufdeckgung() == true)
{
// wenn eine karte vom ablagestapel aufgedeckt wurde gibt's dafuer 5 punkte
if (zug.herkunft_name().contains(BASISSTAPEL_ABLAGESTAPEL) == true)
{
if (nur_eine_ziehen == true) erg += PUNKTEZAEHLER_EINE_ZIEHEN_KARTE_AUF_ABLAGESTAPEL_AUFGEDECKT;
else erg += PUNKTEZAEHLER_DREI_ZIEHEN_KARTE_AUF_ABLAGESTAPEL_AUFGEDECKT;
}
}

// wenn es sich weder um eine bewegung noch um eine aufdeckung handelt ist zug ungueltig
else
{
qDebug() << tr("Invalid move in Punktzaehler::kalkuliere_punkte(const Zug& zug)");

exit(1);
}

return erg;
}


bool Punktezaehler::bringt_zeitaufschub(const Zug& zug) const
{
bool erg = false;

// wenn es sich um eine bewegung handelt
if (zug.ist_bewegung() == true)
{
// vom austeilstapel zu einem ablagestapel bringt zeitaufschub
if (zug.herkunft_name() == BASISSTAPEL_AUSTEILCOSTAPEL && zug.ziel_name().contains(BASISSTAPEL_ABLAGESTAPEL) == true) erg = true;
 
// vom austeilcostapel zum zielstapel bringt zeitaufschub
else if (zug.herkunft_name() == BASISSTAPEL_AUSTEILCOSTAPEL && zug.ziel_name().contains(BASISSTAPEL_ZIELSTAPEL) == true && nach_oben_liste.contains(zug.karte_name()) == false) erg = true;

// karte unten umstapeln bringt zeitaufschub
else if (zug.herkunft_name().contains(BASISSTAPEL_ABLAGESTAPEL) == true && zug.ziel_name().contains(BASISSTAPEL_ABLAGESTAPEL) == true) erg = true;

// von unten zum zielstapel bringt zeitaufschub
else if (zug.herkunft_name().contains(BASISSTAPEL_ABLAGESTAPEL) == true && zug.ziel_name().contains(BASISSTAPEL_ZIELSTAPEL) == true && nach_oben_liste.contains(zug.karte_name()) == false) erg = true;

// karten vom zielstapel nach unten bringt nur zeitaufschub
else if (zug.herkunft_name().contains(BASISSTAPEL_ZIELSTAPEL) == true && zug.ziel_name().contains(BASISSTAPEL_ABLAGESTAPEL) == true) erg = true;
}

// wenn es sich bei zug um eine aufdeckung handelt
else if (zug.ist_aufdeckgung() == true)
{
// wenn eine karte vom ablagestapel aufgedeckt wurde gibt's dafuer 5 punkte
if (zug.herkunft_name().contains(BASISSTAPEL_ABLAGESTAPEL) == true) erg = true;
}

else
{
qDebug() << tr("Invalid move in Punktzaehler::bringt_zeitaufschub(const Zug& zug)");

exit(1);
}

return erg;
}


const QStringList Punktezaehler::speichere() const
{
QStringList erg;

// die id speichern
erg.append(objectName());

// straftimer zustand speichern
erg.append(QString::number(straftimer->isActive()));

// die nach_oben_liste speichern
QString nach_oben_string;

for (register int idx = 0; idx < nach_oben_liste.size(); idx++)
{
if (idx > 0) nach_oben_string.append(PUNKTEZAEHLER_STRING_SPLITTER);
nach_oben_string.append(nach_oben_liste.at(idx));
}

erg.append(nach_oben_string);

// die punkte speichern
erg.append(QString::number(punkte));

return erg;
}


bool Punktezaehler::lade(const QStringList& daten)
{
bool erg = false;

if (daten.size() == PUNKTEZAEHLER_ANZAHL_SPEICHERELEMENTE && daten.first() == objectName())
{
erg = true;

// noetigenfalls den straftimer starten
if (daten.at(PUNKTEZAEHLER_STRAFTIMER_ON_IDX).toInt() == 1) straftimer->start();

// die nach_oben_liste laden
nach_oben_liste = daten.at(PUNKTEZAEHLER_NACH_OBEN_LISTE_IDX).split(PUNKTEZAEHLER_STRING_SPLITTER, QString::SkipEmptyParts);

// die punkte laden
punkte = daten.at(PUNKTEZAEHLER_PUNKTE_IDX).toInt();

emit neue_punktzahl(punkte);
}

return erg;
}


bool Punktezaehler::begonnenes_spiel() const
{
return straftimer->isActive();
}

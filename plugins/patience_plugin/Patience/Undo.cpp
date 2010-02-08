#include "Undo.h"
#include "Karte.h"
#include "Basisstapel.h"
#include "Scene.h"
#include "Proportionen.h"
#include <QDebug>
#include <cstdlib>

using namespace std;

Undo::Undo(Scene *parent) : QObject(parent), scene(parent)
{
}


Undo::~Undo()
{
}


void Undo::speichere_zug(const Zug& zug)
{
if (zug.ist_bewegung() == true)
{
verlauf.append(zug);

// wenn es sich um den ersten gespeicherten zug handelt
if (verlauf.size() == 1) emit undo_verfuegbar(true);
}

else if (zug.ist_aufdeckgung() == true)
{
verlauf.append(zug);

// wenn es sich um den ersten gespeicherten zug handelt
if (verlauf.size() == 1) emit undo_verfuegbar(true);
}

else
{
qDebug() << tr("Invalid move in Undo::undo()");

exit(1);
}

// ueberschuessige undoelemente loeschen
loesche_ueberschuessige_undoelemente();
}


void Undo::undo()
{
// wenn der verlauf nicht leer ist
if (verlauf.isEmpty() == false)
{
Zug zugelement(verlauf.takeLast());

// wenn das zug element ungueltig ist eine fehlermeldung ausgeben und das programm abbrechen
if (zugelement.ist_gueltig() == false)
{
qDebug() << tr("Invalid move in Undo::undo()");

exit(1);
}

// wenn es sich um eine bewegung handelt
if (zugelement.ist_bewegung() == true)
{
// den zug unter umgehung der ueberpruefungen und signale rueckgaengig machen.
zugelement.herkunft()->undo_karten_ablage(zugelement.karte());

scene->update();
}

// wenn es sich um eine karten aufdeckung handelt
else if (zugelement.ist_aufdeckgung() == true)
{
// die karten aufdeckung unter umgehung der ueberpruefungen und signale rueckgaengig machen.
zugelement.karte()->zeige_rueckseite();

scene->update();
}

emit undo_verfuegbar(!verlauf.isEmpty());
emit undo_meldung(zugelement);
}
}

void Undo::clear()
{
emit undo_verfuegbar(false);

verlauf.clear();
}


void Undo::loesche_ueberschuessige_undoelemente()
{
float summe = 0;

// den wert der zuege, die sich derzeit in verlauf befinden berechnen
for (register int idx = 0; idx < verlauf.size(); idx++)
{
if (verlauf.at(idx).ist_bewegung() == true) summe += ZUEGE_PRO_BEWEGUNG;
else if (verlauf.at(idx).ist_aufdeckgung() == true) summe += ZUEGE_PRO_AUFDECKUNG;
}

// den ueberschuss entfernen
float ueberschuss = summe - UNDO_LIMIT;

// wenn ein ueberschuss vorhanden ist ...
if (ueberschuss > 0.1)
{
// solange, wie ein ueberschuss vorhanden ist
while (verlauf.isEmpty() == false && ueberschuss > 0.1)
{
// dem zug entsprechende punktzahl abziehen
if (verlauf.first().ist_bewegung() == true) ueberschuss -= ZUEGE_PRO_BEWEGUNG;
else if (verlauf.first().ist_aufdeckgung() == true) ueberschuss -= ZUEGE_PRO_AUFDECKUNG;

// das aelteste element entfernen
verlauf.removeFirst();
}
}
}


const QStringList Undo::speichere() const
{
QStringList erg;

// die id speichern
erg.append(objectName());

// die verlauf liste speichern
QString verlauf_string;

for (register int idx = 0; idx < verlauf.size(); idx++)
{
if (idx > 0) verlauf_string.append(ZUEGE_SPLITTER);

verlauf_string.append(verlauf.at(idx).karte_name());
verlauf_string.append(ZUEGE_NACH_SPLITTER);
verlauf_string.append(verlauf.at(idx).herkunft_name());

if (verlauf.at(idx).ist_bewegung() == true)
{
verlauf_string.append(ZUEGE_NACH_SPLITTER);
verlauf_string.append(verlauf.at(idx).ziel_name());
}
}

erg.append(verlauf_string);

return erg;
}


bool Undo::lade(const QStringList& daten)
{
bool erg = false;

if (daten.size() == UNDO_ANZAHL_SPEICHERELEMENTE && daten.first() == objectName())
{
erg = true;

QStringList tmp_daten(daten.at(UNDO_VERLAUF_IDX).split(ZUEGE_SPLITTER, QString::SkipEmptyParts));

for (register int idx = 0; idx < tmp_daten.size(); idx++)
{
QStringList tmp_zug(tmp_daten.at(idx).split(ZUEGE_NACH_SPLITTER, QString::SkipEmptyParts));

if (tmp_zug.size() == 2)
{
if (scene->enthaelt_karte(tmp_zug.at(0)) == true && scene->enthaelt_stapel(tmp_zug.at(1)) == true) verlauf.append(Zug(scene->suche_karte(tmp_zug.at(0)), scene->suche_stapel(tmp_zug.at(1))));

else erg = false;
}

else if (tmp_zug.size() == 3)
{
if (scene->enthaelt_karte(tmp_zug.at(0)) == true && scene->enthaelt_stapel(tmp_zug.at(1)) == true && scene->enthaelt_stapel(tmp_zug.at(2)) == true) verlauf.append(Zug(scene->suche_karte(tmp_zug.at(0)), scene->suche_stapel(tmp_zug.at(1)), scene->suche_stapel(tmp_zug.at(2))));

else erg = false;
}

else erg = false;
}
}

if (erg == true && verlauf.isEmpty() == false) emit undo_verfuegbar(true);

return erg;
}

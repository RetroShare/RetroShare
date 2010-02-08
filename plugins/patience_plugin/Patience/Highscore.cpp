#include "Highscore.h"
#include "Proportionen.h"
#include <QTableWidgetItem>
#include <QStringList>
#include <QDir>
#include <QInputDialog>
#include <QSettings>

using namespace std;

Highscore::Highscore(QWidget *parent) : QDialog(parent), einstellungen(0)
{
setupUi(this);

tabelle->setRowCount(HIGHSCORE_ZEILEN);

// die tabelle mit elementen befuellen
for (register int idx1 = 0; idx1 < tabelle->rowCount(); idx1++)
{
for (register int idx2 = 0; idx2 <= HIGHSCORE_ZEIT_POSITION; idx2++)
{
QTableWidgetItem *tmp_item = new QTableWidgetItem();

tmp_item->setText(HIGHSCORE_LEER);

tabelle->setItem(idx1, idx2, tmp_item);
}
}
}


Highscore::~Highscore()
{
}


bool Highscore::neues_ergebnis(int punkte, long sekunden)
{
bool erg = false;
int ziel = -1;

for (register int idx = HIGHSCORE_ZEILEN - 1; idx >= 0; idx--)
{
int element_sekundenverbrauch = 0, element_ergebnis = 0;

QStringList tmp_zeit(tabelle->item(idx, HIGHSCORE_ZEIT_POSITION)->text().split(HIGHSCORE_SPLITTER, QString::SkipEmptyParts));

// wenn das element an idx leer ist ist es auf jeden fall unterlegen
if (tabelle->item(idx, HIGHSCORE_NAME_POSITION)->text() == HIGHSCORE_LEER || tabelle->item(idx, HIGHSCORE_PUNKTE_POSITION)->text() == HIGHSCORE_LEER || tabelle->item(idx, HIGHSCORE_ZEIT_POSITION)->text() == HIGHSCORE_LEER)
{
ziel = idx;
}

// ansonsten
else if (tmp_zeit.size() == 3)
{
// die bewertung des tabellenelements an idx berechnen. je mehr mienen und felder, je mehr punkte
element_ergebnis = tabelle->item(idx, HIGHSCORE_PUNKTE_POSITION)->text().toInt();

// den sekundenverbrauch fuer des tabellenelements an idx berechnen
element_sekundenverbrauch = (tmp_zeit.at(0).toInt() * 3600) + (tmp_zeit.at(1).toInt() * 60) + tmp_zeit.at(2).toInt();
}

// wenn das neue ergebnis mehr punkte hat als das tabellenelement oder gleich viel punkte wie das tabellenelement enthaelt, aber weniger zeit verbraucht hat ist es ueberlegen
if (punkte > element_ergebnis || (punkte == element_ergebnis && sekunden < element_sekundenverbrauch)) ziel = idx;
}

// wenn das neue element in die tabelle darf
if (ziel >= 0 && ziel < tabelle->rowCount())
{
// eine neue reihe an ziel in die tabelle einfuegen ...
tabelle->insertRow(ziel);

// ... und mit elementen befuellen
tabelle->setItem(ziel, HIGHSCORE_NAME_POSITION, new QTableWidgetItem());
tabelle->setItem(ziel, HIGHSCORE_PUNKTE_POSITION, new QTableWidgetItem());
tabelle->setItem(ziel, HIGHSCORE_ZEIT_POSITION, new QTableWidgetItem());

// der letzte muss die tabelle verlassen
tabelle->removeRow(tabelle->rowCount() - 1);

erg = true;
bool ok;

// den namen des nutzers abfragen
QString name_vorbereitung(QDir::home().dirName());

// wenn name_vorbereitung nicht leer ist
if (name_vorbereitung.isEmpty() == false)
{
QChar erstes_zeichen = name_vorbereitung.at(0);

if (erstes_zeichen.isLower() == true) name_vorbereitung[0] = erstes_zeichen.toUpper();
}

QString name = QInputDialog::getText(parentWidget(), tr("Patience"), tr("You got a Highscore !"), QLineEdit::Normal, name_vorbereitung, &ok);

// wenn kein nutzername eingegeben wurde den namen auf unbekannt setzen
if (ok == false || name.isEmpty() == true) name = tr("Unknown");

// wenn der name LEER entspricht den namen ebenfalls auf unbekannt setzen
if (name == HIGHSCORE_LEER) name = tr("Unknown");

int stunden = sekunden / 3600;

int minuten = (sekunden - (stunden * 3600)) / 60;

int sekunden_ = sekunden - (minuten * 60) - (stunden * 3600);

QString stunden_string = QString::number(stunden);
QString minuten_string = QString::number(minuten);
QString sekunden_string = QString::number(sekunden_);

if (stunden < 10) stunden_string.prepend('0');
if (minuten < 10) minuten_string.prepend('0');
if (sekunden_ < 10) sekunden_string.prepend('0');

tabelle->item(ziel, HIGHSCORE_NAME_POSITION)->setText(name);
tabelle->item(ziel, HIGHSCORE_PUNKTE_POSITION)->setText(QString::number(punkte));
tabelle->item(ziel, HIGHSCORE_ZEIT_POSITION)->setText(stunden_string + HIGHSCORE_SPLITTER + minuten_string + HIGHSCORE_SPLITTER + sekunden_string);

// die highscoretabelle anzeigen
show();
}

return erg;
}


void Highscore::registriere_einstellungen(QSettings* einstellungen_)
{
einstellungen = einstellungen_;
}


void Highscore::einstellungen_laden()
{
if (einstellungen != 0)
{
// die breite der spalten laden
tabelle->setColumnWidth(HIGHSCORE_NAME_POSITION, einstellungen->value(QString("tabelle/spalte_") + QString::number(HIGHSCORE_NAME_POSITION), HIGHSCORE_NAME_SPALTE_STANDARTGROESSE).toInt());
tabelle->setColumnWidth(HIGHSCORE_PUNKTE_POSITION, einstellungen->value(QString("tabelle/spalte_") + QString::number(HIGHSCORE_PUNKTE_POSITION), tabelle->columnWidth(HIGHSCORE_PUNKTE_POSITION)).toInt());
tabelle->setColumnWidth(HIGHSCORE_ZEIT_POSITION, einstellungen->value(QString("tabelle/spalte_") + QString::number(HIGHSCORE_ZEIT_POSITION), tabelle->columnWidth(HIGHSCORE_ZEIT_POSITION)).toInt());
}

// den inhalt der tabelle laden
for (register int idx = 0; idx < tabelle->rowCount() && einstellungen != 0; idx++)
{
// den namen laden
tabelle->item(idx, HIGHSCORE_NAME_POSITION)->setText(einstellungen->value(QString("highscore/") + QString::number(idx) + QString("name"), HIGHSCORE_LEER).toString());

// die punkte laden
tabelle->item(idx, HIGHSCORE_PUNKTE_POSITION)->setText(einstellungen->value(QString("highscore/") + QString::number(idx) + QString("breite"), HIGHSCORE_LEER).toString());

// die zeit laden
tabelle->item(idx, HIGHSCORE_ZEIT_POSITION)->setText(einstellungen->value(QString("highscore/") + QString::number(idx) + QString("hoehe"), HIGHSCORE_LEER).toString());
}
}


void Highscore::einstellungen_speichern()
{
// die breite der spalten speichern
einstellungen->setValue(QString("tabelle/spalte_") + QString::number(HIGHSCORE_NAME_POSITION), tabelle->columnWidth(HIGHSCORE_NAME_POSITION));
einstellungen->setValue(QString("tabelle/spalte_") + QString::number(HIGHSCORE_PUNKTE_POSITION), tabelle->columnWidth(HIGHSCORE_PUNKTE_POSITION));
einstellungen->setValue(QString("tabelle/spalte_") + QString::number(HIGHSCORE_ZEIT_POSITION), tabelle->columnWidth(HIGHSCORE_ZEIT_POSITION));

// den inhalt der tabelle speichern
for (register int idx = 0; idx < tabelle->rowCount() && einstellungen != 0; idx++)
{
// den namen speichern
einstellungen->setValue(QString("highscore/") + QString::number(idx) + QString("name"), tabelle->item(idx, HIGHSCORE_NAME_POSITION)->text());

// die punkte speichern
einstellungen->setValue(QString("highscore/") + QString::number(idx) + QString("breite"), tabelle->item(idx, HIGHSCORE_PUNKTE_POSITION)->text());

// die zeit speichern
einstellungen->setValue(QString("highscore/") + QString::number(idx) + QString("hoehe"), tabelle->item(idx, HIGHSCORE_ZEIT_POSITION)->text());
}
}

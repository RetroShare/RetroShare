#include "Speichern_frage.h"

using namespace std;

Speichern_frage::Speichern_frage(QWidget *parent) : QDialog(parent), speichern_wert(true)
{
setupUi(this);

// das resultat voreinstellen
setResult(QDialog::Accepted);

// die groesse des dialogs auf size hint feststellen
setFixedSize(sizeHint());

// signal - slot verbindungen
// die buttons ermoeglichen
connect(nein_button, SIGNAL(clicked()), this, SLOT(nicht_speichern()));
connect(cancel_button, SIGNAL(clicked()), this, SLOT(abbruch()));
connect(ok_button, SIGNAL(clicked()), this, SLOT(speichern()));

connect(merk_box, SIGNAL(toggled(bool)), this, SLOT(releay_verbindung_merk_status(bool)));
}


Speichern_frage::~Speichern_frage()
{
}


void Speichern_frage::speichern()
{
speichern_wert = true;

emit wird_gespeichert(true);

accept();
}


void Speichern_frage::nicht_speichern()
{
speichern_wert = false;

emit wird_nicht_gespeichert(true);

accept();
}


void Speichern_frage::abbruch()
{
merk_box->setChecked(false);

reject();
}


void Speichern_frage::setze_merken(bool wert)
{
if (wert == true) setResult(QDialog::Accepted);

merk_box->setChecked(wert);

emit verbindung_merk_status(!wert);
}


bool Speichern_frage::soll_merken() const
{
return merk_box->isChecked();
}


void Speichern_frage::setze_speichern(bool wert)
{
speichern_wert = wert;

if (wert == true) ok_button->click();
else nein_button->click();
}


bool Speichern_frage::soll_speichern() const
{
return speichern_wert;
}


void Speichern_frage::verbindung_merken(bool wert)
{
setze_merken(!wert);
}


void Speichern_frage::releay_verbindung_merk_status(bool wert)
{
emit verbindung_merk_status(!wert);
}

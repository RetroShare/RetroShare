#include "MainWindow.h"
#include "Scene.h"
#include "Highscore.h"
#include "Speichern_frage.h"
#include <QMessageBox>
#include <QSettings>
#include <QMessageBox>
#include <QActionGroup>
#include <QLabel>
#include <QCloseEvent>

using namespace std;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
// die benutzeroberflaeche aufbauen
setupUi(this);

// osx spezifische einstellungen vornehmen
#ifdef Q_WS_MAC

setUnifiedTitleAndToolBarOnMac(true);

#endif

// einstellungen erstellen
einstellungen = new QSettings("konarski-wuppertal", "Patience", this);

// es ist noetigm das einige einstellungen vorab geladen werden, damit sie von vorne herein verfuegbar sind !!!
// den kartentyp laden
QString kartentyp(einstellungen->value("deckblatt", "french").toString());

// sicherstellen, das der geladene kartentyp gueltig ist
if (kartentyp != "french" && kartentyp != "german") kartentyp = "french";

// action_french und action_german zu einer gruppe verbinden
QActionGroup *deckblatt_gruppe = new QActionGroup(this);
deckblatt_gruppe->addAction(action_french);
deckblatt_gruppe->addAction(action_german);

// die action gruppe initialisieren
if (kartentyp == "french") action_french->setChecked(true);
else action_german->setChecked(true);

// action_eine_ziehen und action_drei_ziehen zu einer gruppe verbinden ...
QActionGroup *spieltyp_gruppe = new QActionGroup(this);
spieltyp_gruppe->addAction(action_eine_ziehen);
spieltyp_gruppe->addAction(action_drei_ziehen);

// ... und initialisieren
action_eine_ziehen->setChecked(true);

// action_speichern und action_nicht_speichern zu eoner gruppe verbinden
QActionGroup *speichern_gruppe = new QActionGroup(this);
speichern_gruppe->addAction(action_speichern);
speichern_gruppe->addAction(action_nicht_speichern);

// scene erstellen ...
scene = new Scene(kartentyp, viewer);
scene->setObjectName("scene");
scene->setSceneRect(0, 0, 795, 595);
scene->registriere_einstellungen(einstellungen);

// ... und in viewer registrieren
viewer->setScene(scene);

// siegmeldung erzeugen
siegmeldung = new QMessageBox(this);
siegmeldung->setWindowModality(Qt::WindowModal);
siegmeldung->setWindowTitle(tr("Patience - You won !"));
siegmeldung->setText(tr("You won, but you don't reach a highscore."));
siegmeldung->addButton(QMessageBox::Ok);

sicherheitsfrage = new QMessageBox(this);
sicherheitsfrage->setWindowModality(Qt::WindowModal);
sicherheitsfrage->setWindowTitle(tr("Patience - Start new game ?"));
sicherheitsfrage->setText(tr("Start a new Game and break the current Game ?"));
sicherheitsfrage->addButton(QMessageBox::Yes);
sicherheitsfrage->addButton(QMessageBox::No);

// punktelabel und zeitlabel erzeugen ...
punktelabel = new QLabel(this);
zeitlabel = new QLabel(this);

// ... und der statusleiste als permanentes widget hinzufuegen
statusBar()->addPermanentWidget(punktelabel);
statusBar()->addPermanentWidget(zeitlabel);

// highscore erzeugen
highscore = new Highscore(this);
highscore->registriere_einstellungen(einstellungen);

// speichern_frage erstellen
speichern_frage = new Speichern_frage(this);
speichern_frage->setObjectName("speichern_frage");

// signal - slot verbindungen
// das starten neuer spiele ermoeglichen
connect(action_neues_spiel, SIGNAL(triggered(bool)), this, SLOT(neues_spiel()));

// action_about ermoeglichen
connect(action_about, SIGNAL(triggered(bool)), this, SLOT(about()));

// action_about_qt ermoeglichen
connect(action_about_qt, SIGNAL(triggered(bool)), qApp, SLOT(aboutQt()));

// das anzeigen der siegmeldung ermoeglichen
connect(scene, SIGNAL(gewonnen(int, long)), this, SLOT(gewonnen(int, long)));

// das umschalten des deckblattes waehrend des spiels ermoeglichen
connect(action_french, SIGNAL(triggered(bool)), scene, SLOT(lade_franzoesisches_deckblatt()));
connect(action_german, SIGNAL(triggered(bool)), scene, SLOT(lade_deutsches_deckblatt()));

// das aktualisieren der punkte ermoeglichen
connect(scene, SIGNAL(neue_punktzahl(int)), this, SLOT(aktualisiere_punktelabel(int)));

// das aktualisieren der spielzeit ermoeglichen
connect(scene, SIGNAL(verstrichene_sekunden(long)), this, SLOT(aktualisiere_spielzeit(long)));

// das anzeigen der highscore ermoeglichen
connect(action_highscore, SIGNAL(triggered(bool)), highscore, SLOT(show()));

// das umschalten des spieltyps ermoeglichen
connect(action_eine_ziehen, SIGNAL(triggered(bool)), this, SLOT(eine_ziehen()));
connect(action_drei_ziehen, SIGNAL(triggered(bool)), this, SLOT(drei_ziehen()));

// das steuern der rahmen sichtbarkeit ermoeglichen
connect(action_rahmen, SIGNAL(triggered(bool)), scene, SIGNAL(rahmen_anzeigen(bool)));

// action_schliessen ermoeglichen
connect(action_beenden, SIGNAL(triggered(bool)), this, SLOT(close()));

// action_undo ermoeglichen
connect(action_undo, SIGNAL(triggered(bool)), scene, SLOT(rueckgaengig()));

// steuerung der verfuegbarkeit von action_undo ermoeglichen
connect(scene, SIGNAL(undo_verfuegbar(bool)), action_undo, SLOT(setEnabled(bool)));

// action_fragen und speichern_frage verbinden
connect(action_fragen, SIGNAL(toggled(bool)), speichern_frage, SLOT(verbindung_merken(bool)));
connect(speichern_frage, SIGNAL(verbindung_merk_status(bool)), action_fragen, SLOT(setChecked(bool)));

// den speicher status mit action_speichern und action_nicht_speichern verbinden
connect(speichern_frage, SIGNAL(wird_gespeichert(bool)), action_speichern, SLOT(setChecked(bool)));
connect(speichern_frage, SIGNAL(wird_nicht_gespeichert(bool)), action_nicht_speichern, SLOT(setChecked(bool)));
connect(action_speichern, SIGNAL(toggled(bool)), speichern_frage, SLOT(setze_speichern(bool)));

// die restlichen einstellungen laden
restliche_einstellungen_laden();
}


MainWindow::~MainWindow()
{
// die einstellungen speichern
einstellungen_speichern();
}


void MainWindow::neues_spiel()
{
// wenn kein spiel laueft, einfach eins starten ...
if (scene->laufendes_spiel() == false) scene->neues_spiel();

// ansonsten erst nach einer sicherheitsfrage ein neues spiel starten
else
{
QAbstractButton *sicherheitsfrage_ja = sicherheitsfrage->button(QMessageBox::Yes);

sicherheitsfrage->exec();

if (sicherheitsfrage->clickedButton() == sicherheitsfrage_ja) scene->neues_spiel();
}
}


void MainWindow::about()
{
QMessageBox::about(this, tr("About Patience"), tr("Patience Version " VERSION " \n\nAuthor:\tAndreas Konarski\nLizenz:\tgpl\n\nKontakt:\n\nprogrammieren@konarski-wuppertal.de\nwww.konarski-wuppertal.de"));
}


void MainWindow::restliche_einstellungen_laden()
{
// die fenstergeometrie laden
restoreGeometry(einstellungen->value("mainwindow/geometry").toByteArray());

// den spieltyp laden
if (einstellungen->value("spieltyp", 1).toInt() == 3)
{
scene->drei_ziehen();
action_drei_ziehen->setChecked(true);
}

// die sichtbarkeit des rahmens laden
action_rahmen->setChecked(einstellungen->value("mainwindow/rahmen", true).toBool());
scene->initialisiere_rahmen(action_rahmen->isChecked());

// die highscore laden
highscore->einstellungen_laden();

// den spielstand laden oder ein neues spiel starten
if (einstellungen->value("begonnenes_spiel", false).toBool() == false) scene->neues_spiel();
else scene->lade_spielstand();

// einstellungen fuer speichern_frage laden
speichern_frage->setze_merken(einstellungen->value("speichern_frage/merken", false).toBool());
speichern_frage->setze_speichern(einstellungen->value("speichern_frage/speichern", true).toBool());
}


void MainWindow::einstellungen_speichern()
{
// die fenstergeometrie speichern
einstellungen->setValue("mainwindow/geometry", saveGeometry());

// den spieltyp speichern
if (scene->nur_eine_wird_gezogen() == true) einstellungen->setValue("spieltyp", 1);
else einstellungen->setValue("spieltyp", 3);

// die sichtbarkeit des rahmens speichern
einstellungen->setValue("mainwindow/rahmen", action_rahmen->isChecked());

// die deckblatt sprache speichern
if (action_french->isChecked() == true) einstellungen->setValue("deckblatt", "french");
else einstellungen->setValue("deckblatt", "german");

// die highscore speichern
highscore->einstellungen_speichern();

// wenn ein spiel laueft und der spielstand gespeichert werden soll, den spielstand speichern
if (scene->begonnenes_spiel() == true && speichern_frage->soll_speichern() == true)
{
scene->speichere_spielstand();

// speichern, ob ein spiel begonnen wurde
einstellungen->setValue("begonnenes_spiel", scene->begonnenes_spiel());
}

else einstellungen->setValue("begonnenes_spiel", false);

// einstellungen fuer speichern_frage speichern
einstellungen->setValue("speichern_frage/merken", speichern_frage->soll_merken());
einstellungen->setValue("speichern_frage/speichern", speichern_frage->soll_speichern());
}


void MainWindow::gewonnen(int punkte, long sekunden)
{
if (highscore->neues_ergebnis(punkte, sekunden) == false) siegmeldung->show();
}


void MainWindow::aktualisiere_punktelabel(int wert)
{
punktelabel->setText(QString::number(wert) + tr(" Points"));
}


void MainWindow::aktualisiere_spielzeit(long verstrichene_sekunden)
{
// die verstrichenen sekunden aufbereiten
int stunden = verstrichene_sekunden / 3600;

int minuten = (verstrichene_sekunden - (stunden * 3600)) / 60;

int sekunden = verstrichene_sekunden - (minuten * 60) - (stunden * 3600);

QString stunden_string = QString::number(stunden);
QString minuten_string = QString::number(minuten);
QString sekunden_string = QString::number(sekunden);

if (stunden < 10) stunden_string.prepend('0');
if (minuten < 10) minuten_string.prepend('0');
if (sekunden < 10) sekunden_string.prepend('0');

zeitlabel->setText(stunden_string + ":" + minuten_string + ":" + sekunden_string);
}


void MainWindow::eine_ziehen()
{
// wenn kein spiel laueft, einfach eins starten ...
if (scene->laufendes_spiel() == false)
{
scene->eine_ziehen();
scene->neues_spiel();
}

// ansonsten erst nach einer sicherheitsfrage ein neues spiel starten
else
{
QAbstractButton *sicherheitsfrage_ja = sicherheitsfrage->button(QMessageBox::Yes);

sicherheitsfrage->exec();

if (sicherheitsfrage->clickedButton() == sicherheitsfrage_ja)
{
scene->eine_ziehen();
scene->neues_spiel();
}

else action_drei_ziehen->setChecked(true);
}
}


void MainWindow::drei_ziehen()
{
// wenn kein spiel laueft, einfach eins starten ...
if (scene->laufendes_spiel() == false)
{
scene->drei_ziehen();
scene->neues_spiel();
}

// ansonsten erst nach einer sicherheitsfrage ein neues spiel starten
else
{
QAbstractButton *sicherheitsfrage_ja = sicherheitsfrage->button(QMessageBox::Yes);

sicherheitsfrage->exec();

if (sicherheitsfrage->clickedButton() == sicherheitsfrage_ja)
{
scene->drei_ziehen();
scene->neues_spiel();
}

else action_eine_ziehen->setChecked(true);
}
}


void MainWindow::closeEvent(QCloseEvent* event)
{
bool brauche_frage = true;

// wenn kein spiel begonnen wurde oder speichern_frage auf merken (dabei jedoch nicht auf reject) eingestellt ist wird keine frage gebraucht
if (scene->begonnenes_spiel() == false || (speichern_frage->soll_merken() == true && speichern_frage->result() != QDialog::Rejected)) brauche_frage = false;

// wenn eine frage gebraucht wird
if (brauche_frage == true) speichern_frage->exec();

// wenn speichern_frage akzeptiert wurde das schliessen der anwendung erlauben, ...
if (speichern_frage->result() == QDialog::Accepted) QMainWindow::closeEvent(event);

// ... ansonsten, wenn nicht akzeptiert, verhindern
else if (speichern_frage->result() == QDialog::Rejected) event->ignore();
}

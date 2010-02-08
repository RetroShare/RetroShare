#ifndef SCENE_H
#define SCENE_H

#include <QGraphicsScene>
#include <QList>
#include <QMap>
#include <QString>
#include <QDateTime>

class Karte;
class Basisstapel;
class Ablagestapel;
class Siegkontrolle;
class Punktezaehler;
class QTimer;
class Undo;
class QSettings;
class QResizeEvent;

class Scene : public QGraphicsScene
{
Q_OBJECT

public:
Scene(const QString& deckblatt, QObject *parent = 0);
virtual ~Scene();

QList<Karte*> mischen() const;
void alle_karten_verdecken();
bool laufendes_spiel() const;
bool nur_eine_wird_gezogen() const;
void initialisiere_rahmen(bool);
Karte* suche_karte(const QString&) const;
Basisstapel *suche_stapel(const QString&) const;
bool enthaelt_karte(const QString&) const;
bool enthaelt_stapel(const QString&) const;
const QStringList speichere() const;
bool lade(const QStringList&);
void registriere_einstellungen(QSettings*);
void lade_spielstand();
void speichere_spielstand();
bool begonnenes_spiel() const;
void groessenanpassung(QResizeEvent*);

public slots:
void setze_spiel_zurueck();
void neues_spiel();
void lade_franzoesisches_deckblatt();
void lade_deutsches_deckblatt();
void eine_ziehen();
void drei_ziehen();
void rueckgaengig();

signals:
void gewonnen(int, long);
void neues_spiel_gestartet();
void neue_punktzahl(int);
void verstrichene_sekunden(long);
void relay_eine_ziehen();
void relay_drei_ziehen();
void rahmen_anzeigen(bool);
void undo_verfuegbar(bool);
void neue_groesse_karten(double);

private slots:
void blockiere_alle_karten();
void starte_spieltimer();
void stoppe_spieltimer();
void reaktionen_auf_spieltimer();
void gewonnen_relay();

private:
QList<Karte*> kartenliste;
QMap<QString, Karte*> kartenfinder;
Basisstapel *austeilstapel, *austeilcostapel, *kreutzzielstapel, *pikzielstapel, *karozielstapel, *herzzielstapel;
QList<Basisstapel*> ablagestapel;
QList<Basisstapel*> allestapel;
QMap<QString, Basisstapel*> stapelfinder;
Siegkontrolle *siegkontrolle;
bool Laufendes_spiel, nur_eine_ziehen;
Punktezaehler *punktezaehler;
QTimer *spieltimer;
QDateTime startzeitpunkt;
Undo *undo;
QSettings *einstellungen;

const QByteArray berechne_pruefsumme_spielstand();
};

#endif

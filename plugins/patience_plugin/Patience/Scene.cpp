/*

Achtung! Die Objektnamen der hier erzeugten Objekte dürfen auf keinen Fall, auch nicht zur korrektur von eventuell vorhandenen Rechtschreibfehlern geaendert werden, da sie in verschiedenen Situationen zur Laufzeit des Programms zum auffinden und identifizieren von Objekten verwendet werden!!!

*/

#include "Scene.h"
#include "Karte.h"
#include "Random.h"
#include "Basisstapel.h"
#include "Austeilstapel.h"
#include "Austeilcostapel.h"
#include "Zielstapel.h"
#include "Ablagestapel.h"
#include "Siegkontrolle.h"
#include "Punktezaehler.h"
#include "Rahmen.h"
#include "Undo.h"
#include "Zug.h"
#include "Proportionen.h"
#include <QTimer>
#include <QDebug>
#include <cstdlib>
#include <QSettings>
#include <QCryptographicHash>
#include <QResizeEvent>

using namespace std;

Scene::Scene(const QString& deckblatt, QObject *parent) : QGraphicsScene(parent), Laufendes_spiel(false), nur_eine_ziehen(true)
{
// den zufallssimulator initialisieren
Random::initialisiere();

// die hintergrundfarbe einstellen
setBackgroundBrush(Qt::darkGreen);

// die klasse Zug fuer das signal - slot system registrieren
qRegisterMetaType<Zug>("Zug");

/*

Wichtiger Hinweis:

Die Reihenfolge, in der die Karten aufgebaut und in die Kartenliste eingefuegt werden, darf auf keinen Fall veraendert werden, da es ansonsten beim aendern des Deckblattes zu fehlern kommen wuerde! Ausserdem darf auch die Anzahl der Karten auf keinen Fall geaendert werden !!!

*/

// die karten erstellen
// die 4 asse erstellen
// kreutz
Karte *kreutz_ass = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/01c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_ass->setObjectName(QString(KARTEN_KREUTZ) + "_" + KARTEN_ASS);
kreutz_ass->setze_farbe(KARTEN_KREUTZ);
kreutz_ass->setze_wert(1);
kartenliste.append(kreutz_ass);

// karo
Karte *karo_ass = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/01d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_ass->setObjectName(QString(KARTEN_KARO) + "_" + KARTEN_ASS);
karo_ass->setze_farbe(KARTEN_KARO);
karo_ass->setze_wert(1);
kartenliste.append(karo_ass);

// herz
Karte *herz_ass = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/01h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_ass->setObjectName(QString(KARTEN_HERZ) + "_" + KARTEN_ASS);
herz_ass->setze_farbe(KARTEN_HERZ);
herz_ass->setze_wert(1);
kartenliste.append(herz_ass);

// pik
Karte *pik_ass = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/01s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_ass->setObjectName(QString(KARTEN_PIK) + "_" + KARTEN_ASS);
pik_ass->setze_farbe(KARTEN_PIK);
pik_ass->setze_wert(1);
kartenliste.append(pik_ass);

// die 2er erstellen
// kreutz
Karte *kreutz_2 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/02c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_2->setObjectName(QString(KARTEN_KREUTZ) + "_2");
kreutz_2->setze_farbe(KARTEN_KREUTZ);
kreutz_2->setze_wert(2);
kartenliste.append(kreutz_2);

// karo
Karte *karo_2 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/02d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_2->setObjectName(QString(KARTEN_KARO) + "_2");
karo_2->setze_farbe(KARTEN_KARO);
karo_2->setze_wert(2);
kartenliste.append(karo_2);

// herz
Karte *herz_2 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/02h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_2->setObjectName(QString(KARTEN_HERZ) + "_2");
herz_2->setze_farbe(KARTEN_HERZ);
herz_2->setze_wert(2);
kartenliste.append(herz_2);

// pik
Karte *pik_2 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/02s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_2->setObjectName(QString(KARTEN_PIK) + "_2");
pik_2->setze_farbe(KARTEN_PIK);
pik_2->setze_wert(2);
kartenliste.append(pik_2);

// die 3er erstellen
// kreutz
Karte *kreutz_3 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/03c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_3->setObjectName(QString(KARTEN_KREUTZ) + "_3");
kreutz_3->setze_farbe(KARTEN_KREUTZ);
kreutz_3->setze_wert(3);
kartenliste.append(kreutz_3);

// karo
Karte *karo_3 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/03d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_3->setObjectName(QString(KARTEN_KARO) + "_3");
karo_3->setze_farbe(KARTEN_KARO);
karo_3->setze_wert(3);
kartenliste.append(karo_3);

// herz
Karte *herz_3 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/03h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_3->setObjectName(QString(KARTEN_HERZ) + "_3");
herz_3->setze_farbe(KARTEN_HERZ);
herz_3->setze_wert(3);
kartenliste.append(herz_3);

// pik
Karte *pik_3 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/03s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_3->setObjectName(QString(KARTEN_PIK) + "_3");
pik_3->setze_farbe(KARTEN_PIK);
pik_3->setze_wert(3);
kartenliste.append(pik_3);

// die 4er erstellen
// kreutz
Karte *kreutz_4 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/04c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_4->setObjectName(QString(KARTEN_KREUTZ) + "_4");
kreutz_4->setze_farbe(KARTEN_KREUTZ);
kreutz_4->setze_wert(4);
kartenliste.append(kreutz_4);

// karo
Karte *karo_4 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/04d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_4->setObjectName(QString(KARTEN_KARO) + "_4");
karo_4->setze_farbe(KARTEN_KARO);
karo_4->setze_wert(4);
kartenliste.append(karo_4);

// herz
Karte *herz_4 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/04h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_4->setObjectName(QString(KARTEN_HERZ) + "_4");
herz_4->setze_farbe(KARTEN_HERZ);
herz_4->setze_wert(4);
kartenliste.append(herz_4);

// pik
Karte *pik_4 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/04s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_4->setObjectName(QString(KARTEN_PIK) + "_4");
pik_4->setze_farbe(KARTEN_PIK);
pik_4->setze_wert(4);
kartenliste.append(pik_4);

// die 5er erstellen
// kreutz
Karte *kreutz_5 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/05c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_5->setObjectName(QString(KARTEN_KREUTZ) + "_5");
kreutz_5->setze_farbe(KARTEN_KREUTZ);
kreutz_5->setze_wert(5);
kartenliste.append(kreutz_5);

// karo
Karte *karo_5 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/05d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_5->setObjectName(QString(KARTEN_KARO) + "_5");
karo_5->setze_farbe(KARTEN_KARO);
karo_5->setze_wert(5);
kartenliste.append(karo_5);

// herz
Karte *herz_5 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/05h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_5->setObjectName(QString(KARTEN_HERZ) + "_5");
herz_5->setze_farbe(KARTEN_HERZ);
herz_5->setze_wert(5);
kartenliste.append(herz_5);

// pik
Karte *pik_5 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/05s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_5->setObjectName(QString(KARTEN_PIK) + "_5");
pik_5->setze_farbe(KARTEN_PIK);
pik_5->setze_wert(5);
kartenliste.append(pik_5);

// die 6er erstellen
// kreutz
Karte *kreutz_6 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/06c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_6->setObjectName(QString(KARTEN_KREUTZ) + "_6");
kreutz_6->setze_farbe(KARTEN_KREUTZ);
kreutz_6->setze_wert(6);
kartenliste.append(kreutz_6);

// karo
Karte *karo_6 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/06d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_6->setObjectName(QString(KARTEN_KARO) + "_6");
karo_6->setze_farbe(KARTEN_KARO);
karo_6->setze_wert(6);
kartenliste.append(karo_6);

// herz
Karte *herz_6 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/06h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_6->setObjectName(QString(KARTEN_HERZ) + "_6");
herz_6->setze_farbe(KARTEN_HERZ);
herz_6->setze_wert(6);
kartenliste.append(herz_6);

// pik
Karte *pik_6 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/06s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_6->setObjectName(QString(KARTEN_PIK) + "_6");
pik_6->setze_farbe(KARTEN_PIK);
pik_6->setze_wert(6);
kartenliste.append(pik_6);

// die 7er erstellen
// kreutz
Karte *kreutz_7 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/07c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_7->setObjectName(QString(KARTEN_KREUTZ) + "_7");
kreutz_7->setze_farbe(KARTEN_KREUTZ);
kreutz_7->setze_wert(7);
kartenliste.append(kreutz_7);

// karo
Karte *karo_7 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/07d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_7->setObjectName(QString(KARTEN_KARO) + "_7");
karo_7->setze_farbe(KARTEN_KARO);
karo_7->setze_wert(7);
kartenliste.append(karo_7);

// herz
Karte *herz_7 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/07h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_7->setObjectName(QString(KARTEN_HERZ) + "_7");
herz_7->setze_farbe(KARTEN_HERZ);
herz_7->setze_wert(7);
kartenliste.append(herz_7);

// pik
Karte *pik_7 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/07s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_7->setObjectName(QString(KARTEN_PIK) + "_7");
pik_7->setze_farbe(KARTEN_PIK);
pik_7->setze_wert(7);
kartenliste.append(pik_7);

// die 8er erstellen
// kreutz
Karte *kreutz_8 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/08c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_8->setObjectName(QString(KARTEN_KREUTZ) + "_8");
kreutz_8->setze_farbe(KARTEN_KREUTZ);
kreutz_8->setze_wert(8);
kartenliste.append(kreutz_8);

// karo
Karte *karo_8 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/08d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_8->setObjectName(QString(KARTEN_KARO) + "_8");
karo_8->setze_farbe(KARTEN_KARO);
karo_8->setze_wert(8);
kartenliste.append(karo_8);

// herz
Karte *herz_8 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/08h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_8->setObjectName(QString(KARTEN_HERZ) + "_8");
herz_8->setze_farbe(KARTEN_HERZ);
herz_8->setze_wert(8);
kartenliste.append(herz_8);

// pik
Karte *pik_8 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/08s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_8->setObjectName(QString(KARTEN_PIK) + "_8");
pik_8->setze_farbe(KARTEN_PIK);
pik_8->setze_wert(8);
kartenliste.append(pik_8);

// die 9er erstellen
// kreutz
Karte *kreutz_9 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/09c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_9->setObjectName(QString(KARTEN_KREUTZ) + "_9");
kreutz_9->setze_farbe(KARTEN_KREUTZ);
kreutz_9->setze_wert(9);
kartenliste.append(kreutz_9);

// karo
Karte *karo_9 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/09d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_9->setObjectName(QString(KARTEN_KARO) + "_9");
karo_9->setze_farbe(KARTEN_KARO);
karo_9->setze_wert(9);
kartenliste.append(karo_9);

// herz
Karte *herz_9 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/09h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_9->setObjectName(QString(KARTEN_HERZ) + "_9");
herz_9->setze_farbe(KARTEN_HERZ);
herz_9->setze_wert(9);
kartenliste.append(herz_9);

// pik
Karte *pik_9 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/09s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_9->setObjectName(QString(KARTEN_PIK) + "_9");
pik_9->setze_farbe(KARTEN_PIK);
pik_9->setze_wert(9);
kartenliste.append(pik_9);

// die 10er erstellen
// kreutz
Karte *kreutz_10 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/10c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_10->setObjectName(QString(KARTEN_KREUTZ) + "_10");
kreutz_10->setze_farbe(KARTEN_KREUTZ);
kreutz_10->setze_wert(10);
kartenliste.append(kreutz_10);

// karo
Karte *karo_10 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/10d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_10->setObjectName(QString(KARTEN_KARO) + "_10");
karo_10->setze_farbe(KARTEN_KARO);
karo_10->setze_wert(10);
kartenliste.append(karo_10);

// herz
Karte *herz_10 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/10h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_10->setObjectName(QString(KARTEN_HERZ) + "_10");
herz_10->setze_farbe(KARTEN_HERZ);
herz_10->setze_wert(10);
kartenliste.append(herz_10);

// pik
Karte *pik_10 = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/10s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_10->setObjectName(QString(KARTEN_PIK) + "_10");
pik_10->setze_farbe(KARTEN_PIK);
pik_10->setze_wert(10);
kartenliste.append(pik_10);

// die buben erstellen
// kreutz
Karte *kreutz_bube = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/11c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_bube->setObjectName(QString(KARTEN_KREUTZ) + "_" + KARTEN_BUBE);
kreutz_bube->setze_farbe(KARTEN_KREUTZ);
kreutz_bube->setze_wert(11);
kartenliste.append(kreutz_bube);

// karo
Karte *karo_bube = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/11d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_bube->setObjectName(QString(KARTEN_KARO) + "_" + KARTEN_BUBE);
karo_bube->setze_farbe(KARTEN_KARO);
karo_bube->setze_wert(11);
kartenliste.append(karo_bube);

// herz
Karte *herz_bube = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/11h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_bube->setObjectName(QString(KARTEN_HERZ) + "_" + KARTEN_BUBE);
herz_bube->setze_farbe(KARTEN_HERZ);
herz_bube->setze_wert(11);
kartenliste.append(herz_bube);

// pik
Karte *pik_bube = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/11s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_bube->setObjectName(QString(KARTEN_PIK) + "_" + KARTEN_BUBE);
pik_bube->setze_farbe(KARTEN_PIK);
pik_bube->setze_wert(11);
kartenliste.append(pik_bube);

// die damen erstellen
// kreutz
Karte *kreutz_dame = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/12c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_dame->setObjectName(QString(KARTEN_KREUTZ) + "_" + KARTEN_DAME);
kreutz_dame->setze_farbe(KARTEN_KREUTZ);
kreutz_dame->setze_wert(12);
kartenliste.append(kreutz_dame);

// karo
Karte *karo_dame = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/12d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_dame->setObjectName(QString(KARTEN_KARO) + "_" + KARTEN_DAME);
karo_dame->setze_farbe(KARTEN_KARO);
karo_dame->setze_wert(12);
kartenliste.append(karo_dame);

// herz
Karte *herz_dame = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/12h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_dame->setObjectName(QString(KARTEN_HERZ) + "_" + KARTEN_DAME);
herz_dame->setze_farbe(KARTEN_HERZ);
herz_dame->setze_wert(12);
kartenliste.append(herz_dame);

// pik
Karte *pik_dame = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/12s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_dame->setObjectName(QString(KARTEN_PIK) + "_" + KARTEN_DAME);
pik_dame->setze_farbe(KARTEN_PIK);
pik_dame->setze_wert(12);
kartenliste.append(pik_dame);

// die koenige erstellen
// kreutz
Karte *kreutz_koenig = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/13c.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
kreutz_koenig->setObjectName(QString(KARTEN_KREUTZ) + "_" + KARTEN_KOENIG);
kreutz_koenig->setze_farbe(KARTEN_KREUTZ);
kreutz_koenig->setze_wert(13);
kartenliste.append(kreutz_koenig);

// karo
Karte *karo_koenig = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/13d.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
karo_koenig->setObjectName(QString(KARTEN_KARO) + "_" + KARTEN_KOENIG);
karo_koenig->setze_farbe(KARTEN_KARO);
karo_koenig->setze_wert(13);
kartenliste.append(karo_koenig);

// herz
Karte *herz_koenig = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/13h.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
herz_koenig->setObjectName(QString(KARTEN_HERZ) + "_" + KARTEN_KOENIG);
herz_koenig->setze_farbe(KARTEN_HERZ);
herz_koenig->setze_wert(13);
kartenliste.append(herz_koenig);

// pik
Karte *pik_koenig = new Karte(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/13s.gif"), QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/back01.gif"), this);
pik_koenig->setObjectName(QString(KARTEN_PIK) + "_" + KARTEN_KOENIG);
pik_koenig->setze_farbe(KARTEN_PIK);
pik_koenig->setze_wert(13);
kartenliste.append(pik_koenig);

// die stapel erzeugen ...
austeilstapel = new Austeilstapel(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/shade.gif"), this);
austeilstapel->setObjectName(BASISSTAPEL_AUSTEILSTAPEL);
austeilstapel->setZValue(0);

austeilcostapel = new Austeilcostapel(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/shade.gif"), this);
austeilcostapel->setObjectName(BASISSTAPEL_AUSTEILCOSTAPEL);
austeilcostapel->setZValue(0);

// austeilstapel und austeilcostapel gegenseitig registrieren
austeilstapel->registriere_costapel(austeilcostapel);
austeilcostapel->registriere_austeilstapel(austeilstapel);

kreutzzielstapel = new Zielstapel(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/shade.gif"), this);
kreutzzielstapel->setObjectName(QString(KARTEN_KREUTZ) + BASISSTAPEL_ZIELSTAPEL);
kreutzzielstapel->setZValue(0);

Rahmen *zielrahmen = new Rahmen(kreutzzielstapel);
kreutzzielstapel->registriere_rahmen(zielrahmen);
addItem(zielrahmen);

pikzielstapel = new Zielstapel(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/shade.gif"), this);
pikzielstapel->setObjectName(QString(KARTEN_PIK) + BASISSTAPEL_ZIELSTAPEL);
pikzielstapel->setZValue(0);
pikzielstapel->registriere_rahmen(zielrahmen);

karozielstapel = new Zielstapel(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/shade.gif"), this);
karozielstapel->setObjectName(QString(KARTEN_KARO) + BASISSTAPEL_ZIELSTAPEL);
karozielstapel->setZValue(0);
karozielstapel->registriere_rahmen(zielrahmen);


herzzielstapel = new Zielstapel(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/shade.gif"), this);
herzzielstapel->setObjectName(QString(KARTEN_HERZ) + BASISSTAPEL_ZIELSTAPEL);
herzzielstapel->setZValue(0);
herzzielstapel->registriere_rahmen(zielrahmen);

// die zielstapel verknuepfen
kreutzzielstapel->registriere_nachbar_zielstapel(pikzielstapel, karozielstapel, herzzielstapel);
pikzielstapel->registriere_nachbar_zielstapel(kreutzzielstapel, karozielstapel, herzzielstapel);
karozielstapel->registriere_nachbar_zielstapel(kreutzzielstapel, pikzielstapel, herzzielstapel);
herzzielstapel->registriere_nachbar_zielstapel(kreutzzielstapel, pikzielstapel, karozielstapel);

for (register int idx = 0; idx < SCENE_ABLAGE_STAPELZAHL; idx++)
{
Basisstapel *tmp_stapel = new Ablagestapel(QPixmap(":/" + deckblatt + "/karten/" + deckblatt + "/shade.gif"), this);
tmp_stapel->setObjectName(QString(BASISSTAPEL_ABLAGESTAPEL) + "_" + QString::number(idx + 1));
tmp_stapel->setZValue(0);

// den rahmen fuer den stapel erzeugen
tmp_stapel->registriere_rahmen(zielrahmen);

ablagestapel.append(tmp_stapel);
}

// ... ihre positionen setzen
austeilstapel->setPos(SCENE_AUSGANGSBREITE / SCENE_AUSTEILSTAPEL_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_OBERE_STAPEL_Y_VERHAELTNIS);
austeilcostapel->setPos(SCENE_AUSGANGSBREITE / SCENE_AUSTEILCOSTAPEL_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_OBERE_STAPEL_Y_VERHAELTNIS);
kreutzzielstapel->setPos(SCENE_AUSGANGSBREITE / SCENE_KREUTZZIELSTAPEL_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_OBERE_STAPEL_Y_VERHAELTNIS);
pikzielstapel->setPos(SCENE_AUSGANGSBREITE / SCENE_PIKZIELSTAPEL_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_OBERE_STAPEL_Y_VERHAELTNIS);
karozielstapel->setPos(SCENE_AUSGANGSBREITE / SCENE_KAROZIELSTAPEL_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_OBERE_STAPEL_Y_VERHAELTNIS);
herzzielstapel->setPos(SCENE_AUSGANGSBREITE / SCENE_HERZZIELSTAPEL_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_OBERE_STAPEL_Y_VERHAELTNIS);
ablagestapel.at(0)->setPos(SCENE_AUSGANGSBREITE / SCENE_ABLAGESTAPEL_01_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);
ablagestapel.at(1)->setPos(SCENE_AUSGANGSBREITE / SCENE_ABLAGESTAPEL_02_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);
ablagestapel.at(2)->setPos(SCENE_AUSGANGSBREITE / SCENE_ABLAGESTAPEL_03_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);
ablagestapel.at(3)->setPos(SCENE_AUSGANGSBREITE / SCENE_ABLAGESTAPEL_04_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);
ablagestapel.at(4)->setPos(SCENE_AUSGANGSBREITE / SCENE_ABLAGESTAPEL_05_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);
ablagestapel.at(5)->setPos(SCENE_AUSGANGSBREITE / SCENE_ABLAGESTAPEL_06_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);
ablagestapel.at(6)->setPos(SCENE_AUSGANGSBREITE / SCENE_ABLAGESTAPEL_07_X_VERHAELTNIS, SCENE_AUSGANGSHOEHE / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);

// ... und in die scene einfuegen
addItem(austeilstapel);
addItem(austeilcostapel);
addItem(kreutzzielstapel);
addItem(pikzielstapel);
addItem(karozielstapel);
addItem(herzzielstapel);

for (register int idx = 0; idx < SCENE_ABLAGE_STAPELZAHL; idx++) addItem(ablagestapel.at(idx));

// eine liste erstellen, die alle stapel enthaelt
allestapel = ablagestapel;
allestapel.append(austeilstapel);
allestapel.append(austeilcostapel);
allestapel.append(kreutzzielstapel);
allestapel.append(pikzielstapel);
allestapel.append(karozielstapel);
allestapel.append(herzzielstapel);

for (register int idx = 0; idx < kartenliste.size(); idx++)
{
// die stapelliste an alle karten uebergeben
kartenliste.at(idx)->registriere_stapel(allestapel);

// den karten die zielstapel mitteilen
kartenliste.at(idx)->registriere_zielstapel(kreutzzielstapel, pikzielstapel, herzzielstapel, karozielstapel);

// alle karten in die scene einfuegen
addItem(kartenliste.at(idx));
}

// siegkontrolle erstellen ...
siegkontrolle = new Siegkontrolle(this);
siegkontrolle->setObjectName("siegkontrolle");

// ... und in den zielstapeln registrieren
kreutzzielstapel->registriere_siegkontrolle(siegkontrolle);
pikzielstapel->registriere_siegkontrolle(siegkontrolle);
karozielstapel->registriere_siegkontrolle(siegkontrolle);
herzzielstapel->registriere_siegkontrolle(siegkontrolle);

// in siegkontrolle die zielstapel registrieren
siegkontrolle->registriere_zielstapel(kreutzzielstapel, pikzielstapel, herzzielstapel, karozielstapel);

// punktezaehler erzeugen
punktezaehler = new Punktezaehler(this);
punktezaehler->setObjectName("punktezaehler");

// spieltimer erstellen
spieltimer = new QTimer(this);
spieltimer->setInterval(1000);

// die auffindhilfen aufbauen
for (register int idx = 0; idx < allestapel.size(); idx++) stapelfinder.insert(allestapel.at(idx)->objectName(), allestapel.at(idx));
for (register int idx = 0; idx < kartenliste.size(); idx++) kartenfinder.insert(kartenliste.at(idx)->objectName(), kartenliste.at(idx));

// undo erstellen
undo = new Undo(this);
undo->setObjectName("undo");

// signal - slot verbindungen
// bei spielende (sieg) sollen alle karten blockert werden
connect(siegkontrolle, SIGNAL(gewonnen()), this, SLOT(blockiere_alle_karten()));

// alle stapel am punktezaehler anschliessen, damit die stapel zuege an punktzaehler melden koennen
for (register int idx = 0; idx < allestapel.size(); idx++)
{
connect(allestapel.at(idx), SIGNAL(zug(const Zug&)), punktezaehler, SLOT(neuer_zug(const Zug&)));
}



// verbindungen herstellen, die alle karten betreffen
for (register int idx = 0; idx < kartenliste.size(); idx++)
{
for (register int ablagestapelidx = 0; ablagestapelidx < ablagestapel.size(); ablagestapelidx++)
{
// allen karten das senden von hilfsanfragen ermoeglichen
// das starten von hilfsanfragen ermoeglichen
connect(kartenliste.at(idx), SIGNAL(hilfsanfrage_start(Karte*)), ablagestapel.at(ablagestapelidx), SLOT(hilfsanfrage_start(Karte*)));

// das stoppen von hilfsanfragen ermoeglichen
connect(kartenliste.at(idx), SIGNAL(hilfsanfrage_ende()), ablagestapel.at(ablagestapelidx), SLOT(hilfsanfrage_ende()));
}

// das starten von hilfsanfragen ermoeglichen
connect(kartenliste.at(idx), SIGNAL(hilfsanfrage_start(Karte*)), herzzielstapel, SLOT(hilfsanfrage_start(Karte*)));
connect(kartenliste.at(idx), SIGNAL(hilfsanfrage_start(Karte*)), karozielstapel, SLOT(hilfsanfrage_start(Karte*)));
connect(kartenliste.at(idx), SIGNAL(hilfsanfrage_start(Karte*)), pikzielstapel, SLOT(hilfsanfrage_start(Karte*)));
connect(kartenliste.at(idx), SIGNAL(hilfsanfrage_start(Karte*)), kreutzzielstapel, SLOT(hilfsanfrage_start(Karte*)));

// das stoppen von hilfsanfragen ermoeglichen
connect(kartenliste.at(idx), SIGNAL(hilfsanfrage_ende()), herzzielstapel, SLOT(hilfsanfrage_ende()));
connect(kartenliste.at(idx), SIGNAL(hilfsanfrage_ende()), karozielstapel, SLOT(hilfsanfrage_ende()));
connect(kartenliste.at(idx), SIGNAL(hilfsanfrage_ende()), pikzielstapel, SLOT(hilfsanfrage_ende()));
connect(kartenliste.at(idx), SIGNAL(hilfsanfrage_ende()), kreutzzielstapel, SLOT(hilfsanfrage_ende()));

// allen karten das verstecken des zielrahmens ermoeglichen und das steuern der sichtbarkeit des rahmens ermoeglichen
connect(kartenliste.at(idx), SIGNAL(rahmen_verstecken()), zielrahmen, SLOT(verstecke()));
connect(this, SIGNAL(rahmen_anzeigen(bool)), kartenliste.at(idx), SLOT(sichtbarkeit_rahmen(bool)));

// das skalieren der kartengroesse ermoeglichen
connect(this, SIGNAL(neue_groesse_karten(double)), kartenliste.at(idx), SLOT(passe_groesse_an(double)));
}



// den punktezaehler ueber den start neuer spiele unterrichten
connect(this, SIGNAL(neues_spiel_gestartet()), punktezaehler, SLOT(neues_spiel()));

// den punktezaehler ueber spielende unterrichten
connect(siegkontrolle, SIGNAL(gewonnen()), punktezaehler, SLOT(spiel_zuende()));

// punktezaehler das melden der punktzahl ermoeglichen
connect(punktezaehler, SIGNAL(neue_punktzahl(int)), this, SIGNAL(neue_punktzahl(int)));

// wenn bei austeilstapel der stapel durch ist dies an punktzaehler melden
connect(austeilstapel, SIGNAL(stapel_durch()), punktezaehler, SLOT(stapel_durch()));

// starten und stoppen von spieltimer ermoeglichen. erste der erste zug startet die zeit
connect(punktezaehler, SIGNAL(erster_zug()), this, SLOT(starte_spieltimer()));
connect(siegkontrolle, SIGNAL(gewonnen()), this, SLOT(stoppe_spieltimer()));

// reaktionen auf spieltimer ermoeglichen
connect(spieltimer, SIGNAL(timeout()), this, SLOT(reaktionen_auf_spieltimer()));

// das durchreichen von siegmeldungen ermoeglichen
connect(siegkontrolle, SIGNAL(gewonnen()), this, SLOT(gewonnen_relay()));

// wenn ein neues spiel gestartet wird, muss austeilcostapel die ablagenummer resetten
connect(this, SIGNAL(neues_spiel_gestartet()), austeilcostapel, SLOT(resette_ablagenummer()));

// austeilstapel und austeilcostapel sollen den spieltyp kennen
connect(this, SIGNAL(relay_eine_ziehen()), austeilstapel, SLOT(eine_ziehen()));
connect(this, SIGNAL(relay_eine_ziehen()), austeilcostapel, SLOT(eine_ziehen()));
connect(this, SIGNAL(relay_drei_ziehen()), austeilstapel, SLOT(drei_ziehen()));
connect(this, SIGNAL(relay_drei_ziehen()), austeilcostapel, SLOT(drei_ziehen()));

// auch der punktezaehler soll den spieltyp kennen
connect(this, SIGNAL(relay_eine_ziehen()), punktezaehler, SLOT(eine_ziehen()));
connect(this, SIGNAL(relay_drei_ziehen()), punktezaehler, SLOT(drei_ziehen()));

// allen ablagestapel stapel ausser austeilstapel an undo anschliessen
for (register int idx = 0; idx < ablagestapel.size(); idx++)
{
connect(ablagestapel.at(idx), SIGNAL(zug(const Zug&)), undo, SLOT(speichere_zug(const Zug&)));
}

connect(austeilcostapel, SIGNAL(zug(const Zug&)), undo, SLOT(speichere_zug(const Zug&)));
connect(herzzielstapel, SIGNAL(zug(const Zug&)), undo, SLOT(speichere_zug(const Zug&)));
connect(karozielstapel, SIGNAL(zug(const Zug&)), undo, SLOT(speichere_zug(const Zug&)));
connect(pikzielstapel, SIGNAL(zug(const Zug&)), undo, SLOT(speichere_zug(const Zug&)));
connect(kreutzzielstapel, SIGNAL(zug(const Zug&)), undo, SLOT(speichere_zug(const Zug&)));

// undo meldungen an punktezaehler ermoeglichen
connect(undo, SIGNAL(undo_meldung(const Zug&)), punktezaehler, SLOT(undo_meldung(const Zug&)));

// verfuegbarkeit von undo melden
connect(undo, SIGNAL(undo_verfuegbar(bool)), this, SIGNAL(undo_verfuegbar(bool)));

// verbindungen, die alle stapel betreffen
for (register int idx = 0; idx < allestapel.size(); idx++)
{
// groessenanpassung aller stapel ermoeglichen
connect(this, SIGNAL(neue_groesse_karten(double)), allestapel.at(idx), SLOT(passe_groesse_an(double)));
}
}


Scene::~Scene()
{
}


QList<Karte*> Scene::mischen() const
{
QList<Karte*> gemischt, mischquelle(kartenliste);

while (mischquelle.isEmpty() == false) gemischt.append(mischquelle.takeAt(Random::random(0, mischquelle.size() - 1)));

if (mischquelle.isEmpty() == false)
{
qDebug() << tr("Missing Cards in Scene !");

exit(1);
}

return gemischt;
}


void Scene::neues_spiel()
{
// zunaechst den spieltimer stoppen
stoppe_spieltimer();

// die verstrichenen sekunden ohne den spieltimer auf 0 setzen, weil die startzeit noch vom letzten spiel stammt und dies zu falschen werten fuehren wuerde. dies wird, wenn punktzaehler den ersten zug meldet angepasst
emit verstrichene_sekunden(0);

// das spielfeld aufraeumen
setze_spiel_zurueck();

// die karten mischen
QList<Karte*> aufbauliste(mischen());

// 1 karte auf den ersten ablagestapel legen
ablagestapel.at(0)->initialisiere_karte(aufbauliste.takeFirst());

// 2 karte auf den zweiten ablagestapel legen
for (register int idx = 0; idx < 2; idx++) ablagestapel.at(1)->initialisiere_karte(aufbauliste.takeFirst());

// 3 karte auf den dritten ablagestapel legen
for (register int idx = 0; idx < 3; idx++) ablagestapel.at(2)->initialisiere_karte(aufbauliste.takeFirst());

// 4 karte auf den vierten ablagestapel legen
for (register int idx = 0; idx < 4; idx++) ablagestapel.at(3)->initialisiere_karte(aufbauliste.takeFirst());

// 5 karte auf den fuenften ablagestapel legen
for (register int idx = 0; idx < 5; idx++) ablagestapel.at(4)->initialisiere_karte(aufbauliste.takeFirst());

// 6 karte auf den sechsten ablagestapel legen
for (register int idx = 0; idx < 6; idx++) ablagestapel.at(5)->initialisiere_karte(aufbauliste.takeFirst());

// 7 karte auf den siebten ablagestapel legen
for (register int idx = 0; idx < 7; idx++) ablagestapel.at(6)->initialisiere_karte(aufbauliste.takeFirst());

// die restlichen karten auf den austeilstapel legen
while (aufbauliste.isEmpty() == false) austeilstapel->initialisiere_karte(aufbauliste.takeFirst());

// die obersten karten auf den ablagestapeln aufdecken
for (register int idx = 0; idx < ablagestapel.size(); idx++)
{
if (ablagestapel.at(idx)->oberste_karte() != 0) ablagestapel.at(idx)->oberste_karte()->zeige_vorderseite();
}

// undo leeren. damit wird undo auch gleich blockiert.
undo->clear();

Laufendes_spiel = true;

emit neues_spiel_gestartet();
}


void Scene::alle_karten_verdecken()
{
for (register int idx = 0; idx < kartenliste.size(); idx++) kartenliste.at(idx)->zeige_rueckseite();
}


void Scene::setze_spiel_zurueck()
{
// undo leeren. damit wird undo auch gleich blockiert.
undo->clear();

// alle karten sollen wieder die rueckseite zeigen
alle_karten_verdecken();

for (register int idx = 0; idx < kartenliste.size(); idx++)
{
// meinstapel wieder zuruecksetzen
kartenliste.at(idx)->setze_meinstapel(0);

// die rueckkehrkoordinaten zuruecksetzen
kartenliste.at(idx)->setze_rueckehrkoordinaten(QPointF(0, 0));
}

// die kartenlisten der stapel zuruecksetzen
for (register int idx = 0; idx < allestapel.size(); idx++) allestapel.at(idx)->setze_kartenliste_zurueck();
}


void Scene::blockiere_alle_karten()
{
// undo leeren. damit wird undo auch gleich blockiert.
undo->clear();

Laufendes_spiel = false;

for (register int idx = 0; idx < kartenliste.size(); idx++) kartenliste.at(idx)->setFlag(QGraphicsItem::ItemIsMovable, false);
}


bool Scene::laufendes_spiel() const
{
return Laufendes_spiel;
}


void Scene::lade_franzoesisches_deckblatt()
{
// sicherstellen, das alle 52 karten vorhanden sind
if (kartenliste.size() == 52)
{
int idx = 0;

// die franzoesischen kartenbilder laden
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/01c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/01d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/01h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/01s.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/02c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/02d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/02h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/02s.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/03c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/03d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/03h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/03s.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/04c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/04d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/04h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/04s.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/05c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/05d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/05h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/05s.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/06c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/06d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/06h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/06s.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/07c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/07d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/07h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/07s.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/08c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/08d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/08h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/08s.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/09c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/09d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/09h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/09s.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/10c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/10d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/10h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/10s.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/11c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/11d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/11h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/11s.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/12c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/12d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/12h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/12s.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/13c.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/13d.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/13h.gif"), QPixmap(":/french/karten/french/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/french/karten/french/13s.gif"), QPixmap(":/french/karten/french/back01.gif"));
}
}


void Scene::lade_deutsches_deckblatt()
{
// sicherstellen, das alle 52 karten vorhanden sind
if (kartenliste.size() == 52)
{
int idx = 0;

// die deutschen kartenbilder laden
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/01c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/01d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/01h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/01s.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/02c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/02d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/02h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/02s.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/03c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/03d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/03h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/03s.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/04c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/04d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/04h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/04s.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/05c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/05d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/05h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/05s.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/06c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/06d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/06h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/06s.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/07c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/07d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/07h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/07s.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/08c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/08d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/08h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/08s.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/09c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/09d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/09h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/09s.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/10c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/10d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/10h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/10s.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/11c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/11d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/11h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/11s.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/12c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/12d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/12h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/12s.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/13c.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/13d.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/13h.gif"), QPixmap(":/german/karten/german/back01.gif"));
kartenliste.at(idx++)->setze_kartenbilder(QPixmap(":/german/karten/german/13s.gif"), QPixmap(":/german/karten/german/back01.gif"));
}
}


void Scene::starte_spieltimer()
{
// den startzeitpunkt erfassen
startzeitpunkt = QDateTime::currentDateTime();

// den timer zur aktualisierung der spielzeit starten
spieltimer->start();

// die berechnete zeit einmal sofort senden
reaktionen_auf_spieltimer();
}


void Scene::stoppe_spieltimer()
{
spieltimer->stop();
}


void Scene::reaktionen_auf_spieltimer()
{
emit verstrichene_sekunden(startzeitpunkt.secsTo(QDateTime::currentDateTime()));
}


void Scene::gewonnen_relay()
{
// die daten fuer eine vollstaendige siegmeldung senden
emit gewonnen(punktezaehler->punktstand(), startzeitpunkt.secsTo(QDateTime::currentDateTime()));
}


void Scene::eine_ziehen()
{
nur_eine_ziehen = true;

emit relay_eine_ziehen();
}


void Scene::drei_ziehen()
{
nur_eine_ziehen = false;

emit relay_drei_ziehen();
}


bool Scene::nur_eine_wird_gezogen() const
{
return nur_eine_ziehen;
}


void Scene::initialisiere_rahmen(bool wert)
{
emit rahmen_anzeigen(wert);
}


void Scene::rueckgaengig()
{
undo->undo();
}


Karte* Scene::suche_karte(const QString& name) const
{
Karte *erg = 0;

if (kartenfinder.contains(name) == true) erg = kartenfinder.value(name);

return erg;
}


const QStringList Scene::speichere() const
{
QStringList erg;

// die id speichern
erg.append(objectName());

// nur_eine_ziehen speichern
erg.append(QString::number(nur_eine_ziehen));

// die verstrichene zeit speichern
erg.append(QString::number(startzeitpunkt.secsTo(QDateTime::currentDateTime())));

return erg;
}


bool Scene::lade(const QStringList& daten)
{
bool erg = false;

if (daten.size() == SCENE_ANZAHL_SPEICHERELEMENTE && daten.first() == objectName())
{
erg = true;

// nur_eine_ziehen laden
nur_eine_ziehen = daten.at(SCENE_EINE_ZIEHEN_IDX).toInt();

// eine zur verstrichenen zeit passende startzeit berechnen
startzeitpunkt = QDateTime::currentDateTime().addSecs(-daten.at(SCENE_VERSTRICHENE_ZEIT_IDX).toInt());
}

return erg;
}


void Scene::registriere_einstellungen(QSettings* einstellungen_)
{
einstellungen = einstellungen_;
}


void Scene::lade_spielstand()
{
bool erg = true;

// den zustand der scene laden
erg = lade(einstellungen->value("Spielstand/" + objectName(), QStringList()).toStringList());

if (erg == false) qDebug() << tr("Unable to restore scene !");

// den zustand der stapel laden
if (erg == true)
{
for (register int idx = 0; idx < allestapel.size() && erg == true; idx++) erg = allestapel.at(idx)->lade(einstellungen->value("Spielstand/" + allestapel.at(idx)->objectName(), QStringList()).toStringList());

if (erg == false) qDebug() << tr("Unable to restore stacks !");
}

// den zustand der karten laden
if (erg == true)
{
for (register int idx = 0; idx < kartenliste.size() && erg == true; idx++) erg = kartenliste.at(idx)->lade(einstellungen->value("Spielstand/" + kartenliste.at(idx)->objectName(), QStringList()).toStringList());

if (erg == false) qDebug() << tr("Unable to restore cards !");
}

// den zustand von undo laden
if (erg == true)
{
erg = undo->lade(einstellungen->value("Spielstand/" + undo->objectName(), QStringList()).toStringList());

if (erg == false) qDebug() << tr("Unable to restore undo !");
}

// den zustand des punktezaehlers laden
if (erg == true)
{
erg = punktezaehler->lade(einstellungen->value("Spielstand/" + punktezaehler->objectName(), QStringList()).toStringList());

if (erg == false) qDebug() << tr("Unable to restore points !");
}

// testen, ob alle karten einen eigentuemer stapel haben
if (erg == true)
{
for (register int idx = 0; idx < kartenliste.size() && erg == true; idx++) if (kartenliste.at(idx)->eigentuemer_stapel() == 0) erg = false;

if (erg == false) qDebug() << tr("Card with 0 batch after restoring gamestate !");
}

// die pruefsumme des geladenen spiels testen
if (erg == true)
{
if (berechne_pruefsumme_spielstand() != einstellungen->value(QString("Spielstand/") + "pruefsumme", QByteArray()).toByteArray())
{
erg = false;

qDebug() << tr("Wrong checksumm while restoring saved game !");
}
}

if (erg == true)
{
Laufendes_spiel = true;

spieltimer->start();

emit verstrichene_sekunden(startzeitpunkt.secsTo(QDateTime::currentDateTime()));
}

// wenn beim laden etwas schief gegangen ist ein neues spiel starten
else
{
if (erg == false) qDebug() << tr("Unable to restore saved game. Trying to start a new game ...");

neues_spiel();
}
}


void Scene::speichere_spielstand()
{
// den zustand der scene speichern
einstellungen->setValue("Spielstand/" + objectName(), speichere());

// den zustand der stapel speichern
for (register int idx = 0; idx < allestapel.size(); idx++) einstellungen->setValue("Spielstand/" + allestapel.at(idx)->objectName(), allestapel.at(idx)->speichere());

// den zustand der karten speichern
for (register int idx = 0; idx < kartenliste.size(); idx++) einstellungen->setValue("Spielstand/" + kartenliste.at(idx)->objectName(), kartenliste.at(idx)->speichere());

// den zustand von undo speichern
einstellungen->setValue("Spielstand/" + undo->objectName(), undo->speichere());

// den zustand des punktezaehlers speichern
einstellungen->setValue("Spielstand/" + punktezaehler->objectName(), punktezaehler->speichere());

// die md5 des spielstands speichern
einstellungen->setValue(QString("Spielstand/") + "pruefsumme", berechne_pruefsumme_spielstand());
}


bool Scene::begonnenes_spiel() const
{
return punktezaehler->begonnenes_spiel();
}


Basisstapel* Scene::suche_stapel(const QString& name) const
{
Basisstapel *stapel = 0;

if (stapelfinder.contains(name) == true) stapel = stapelfinder.value(name);

return stapel;
}


bool Scene::enthaelt_karte(const QString& name) const
{
return kartenfinder.contains(name);
}


bool Scene::enthaelt_stapel(const QString& name) const
{
return stapelfinder.contains(name);
}


const QByteArray Scene::berechne_pruefsumme_spielstand()
{
QByteArray erg;
QCryptographicHash hash(QCryptographicHash::Sha1);

// den wert von scene abfragen
hash.addData(einstellungen->value("Spielstand/" + objectName(), QByteArray()).toByteArray());

// den wert der stapel abfragen
for (register int idx = 0; idx < allestapel.size(); idx++) hash.addData(einstellungen->value("Spielstand/" + allestapel.at(idx)->objectName(), QByteArray()).toByteArray());

// den wert von karten abfragen
for (register int idx = 0; idx < kartenliste.size(); idx++) hash.addData(einstellungen->value("Spielstand/" + kartenliste.at(idx)->objectName(), QByteArray()).toByteArray());

// den wert von undo abfragen
hash.addData(einstellungen->value("Spielstand/" + undo->objectName(), QByteArray()).toByteArray());

// den wert von punktezaehlers abfragen
hash.addData(einstellungen->value("Spielstand/" + punktezaehler->objectName(), QByteArray()).toByteArray());

erg = hash.result();

return erg;
}


void Scene::groessenanpassung(QResizeEvent* event)
{
if (event->size().width() > 0 && event->size().height() > 0)
{
double breite = event->size().width(), hoehe = event->size().height();

if (hoehe > (breite / SCENE_VERHAELTNIS_BREITE_ZU_HOEHE)) hoehe = breite / SCENE_VERHAELTNIS_BREITE_ZU_HOEHE;

setSceneRect(QRectF(0, 0, event->size().width() - 5, event->size().height() - 5));

austeilstapel->setPos(breite / SCENE_AUSTEILSTAPEL_X_VERHAELTNIS, hoehe / SCENE_OBERE_STAPEL_Y_VERHAELTNIS);
austeilcostapel->setPos(breite / SCENE_AUSTEILCOSTAPEL_X_VERHAELTNIS, hoehe / SCENE_OBERE_STAPEL_Y_VERHAELTNIS);
kreutzzielstapel->setPos(breite / SCENE_KREUTZZIELSTAPEL_X_VERHAELTNIS, hoehe / SCENE_OBERE_STAPEL_Y_VERHAELTNIS);
pikzielstapel->setPos(breite / SCENE_PIKZIELSTAPEL_X_VERHAELTNIS, hoehe / SCENE_OBERE_STAPEL_Y_VERHAELTNIS);
karozielstapel->setPos(breite / SCENE_KAROZIELSTAPEL_X_VERHAELTNIS, hoehe / SCENE_OBERE_STAPEL_Y_VERHAELTNIS);
herzzielstapel->setPos(breite / SCENE_HERZZIELSTAPEL_X_VERHAELTNIS, hoehe / SCENE_OBERE_STAPEL_Y_VERHAELTNIS);
ablagestapel.at(0)->setPos(breite / SCENE_ABLAGESTAPEL_01_X_VERHAELTNIS, hoehe / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);
ablagestapel.at(1)->setPos(breite / SCENE_ABLAGESTAPEL_02_X_VERHAELTNIS, hoehe / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);
ablagestapel.at(2)->setPos(breite / SCENE_ABLAGESTAPEL_03_X_VERHAELTNIS, hoehe / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);
ablagestapel.at(3)->setPos(breite / SCENE_ABLAGESTAPEL_04_X_VERHAELTNIS, hoehe / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);
ablagestapel.at(4)->setPos(breite / SCENE_ABLAGESTAPEL_05_X_VERHAELTNIS, hoehe / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);
ablagestapel.at(5)->setPos(breite / SCENE_ABLAGESTAPEL_06_X_VERHAELTNIS, hoehe / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);
ablagestapel.at(6)->setPos(breite / SCENE_ABLAGESTAPEL_07_X_VERHAELTNIS, hoehe / SCENE_ABLAGESTAPEL_Y_VERHAELTNIS);

emit neue_groesse_karten(hoehe / SCENE_HOEHE_ZU_KARTENHOEHE);
}
}

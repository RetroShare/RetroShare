#include "Random.h"
#include <QtGlobal>
#include <QDateTime>
#include <QDebug>
#include <QObject>
#include <climits>
#include <cstdlib>

using namespace std;

namespace Random
{

int random(int min, int max)
{
int ergebnis = 0;
unsigned int differenz = 0;
unsigned int zufall = qrand();

// min muss kleiner als max sein
if (min < max)
{
// die differenz zwischen min und max berechnen
if (min < 0)
{
differenz = max + (0 - min);
}

else
{
differenz = max - min;
}

// dafuer sorgen, das auch der max als ergebnis moeglich ist. jedoch nur, wenn differenz kleiner als UINT_MAX ist !
if (differenz < UINT_MAX) differenz++;

// wenn die differenz groesser als RAND_MAX ist und RAND_MAX kleiner als UINT_MAX ist zufall durch addition zusaetzlicher zufallszahlen vergroessern
if (differenz > RAND_MAX && RAND_MAX < UINT_MAX)
{
// wie oft passt RAND_MAX in ULONG_MAX ?
int schleifen = UINT_MAX / RAND_MAX;

// so oft, wie RAND_MAX in UINT_MAX passt die schleife durchlaufen lassen. dabei beruecksichtigen, das eine abfrage bereits erledigt ist
for (register int idx = 1; idx < schleifen; idx++) zufall += qrand();
}

// zufall in den bereich der differenz bringen
if (zufall > differenz)
{
zufall = zufall % differenz;
}

ergebnis = min + zufall;
}

// wenn min gleich max ist, ist onehin nur ein wert moeglich
else if (min == max)
{
ergebnis = min;
}

else
{
qDebug() << QObject::tr("min is bigger than max !");

exit(1);
}

return ergebnis;
}

void initialisiere()
{
// den zufallssimulator initialisieren
qsrand(QDateTime::currentDateTime().toTime_t() + QTime::currentTime().msec());
}
}

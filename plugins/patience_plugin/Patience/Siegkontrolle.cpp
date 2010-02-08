#include "Siegkontrolle.h"
#include "Basisstapel.h"

using namespace std;

Siegkontrolle::Siegkontrolle(QObject *parent) : QObject(parent)
{
}


Siegkontrolle::~Siegkontrolle()
{
}


void Siegkontrolle::teste_auf_sieg()
{
// wenn alle zielstapel 13 karten enthalten hat der spieler gewonnen
if (zielstapel.at(0)->karten() == 13 && zielstapel.at(1)->karten() == 13 && zielstapel.at(2)->karten() == 13 && zielstapel.at(3)->karten() == 13) emit gewonnen();
}


void Siegkontrolle::registriere_zielstapel(Basisstapel* kreutzstapel, Basisstapel* pikstapel, Basisstapel* herzstapel, Basisstapel* karostapel)
{
zielstapel.append(kreutzstapel);
zielstapel.append(pikstapel);
zielstapel.append(herzstapel);
zielstapel.append(karostapel);
}

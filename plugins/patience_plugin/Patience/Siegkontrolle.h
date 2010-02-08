#ifndef SIEGKONTROLLE_H
#define SIEGKONTROLLE_H

#include <QObject>

class Basisstapel;
class Karte;

class Siegkontrolle : public QObject
{
Q_OBJECT

public:
Siegkontrolle(QObject *parent = 0);
virtual ~Siegkontrolle();

void teste_auf_sieg();
void registriere_zielstapel(Basisstapel* kreutzstapel, Basisstapel* pikstapel, Basisstapel* herzstapel, Basisstapel *karostapel);

signals:
void gewonnen();

private:
QList<Basisstapel*> zielstapel;
};

#endif

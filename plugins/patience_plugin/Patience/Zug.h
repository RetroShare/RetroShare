#ifndef ZUG_H
#define ZUG_H

#include <QString>

class Karte;
class Basisstapel;

class Zug
{
public:
Zug();
Zug(Karte* karte, Basisstapel *herkunft, Basisstapel *ziel);
Zug(Karte* karte, Basisstapel *ort);
virtual ~Zug();

bool ist_gueltig() const;
bool ist_bewegung() const;
bool ist_aufdeckgung() const;

void setze_bewegung(Karte* karte, Basisstapel *herkunft_, Basisstapel *ziel_);
void setze_aufdeckung(Karte* karte, Basisstapel *ort);

Karte* karte() const;
Basisstapel* herkunft() const;
Basisstapel* ziel() const;


QString karte_name() const;
QString herkunft_name() const;
QString ziel_name() const;

bool operator==(const Zug& anderer) const;

private:
Karte *Karte_;
Basisstapel *Herkunft, *Ziel;
};

#endif

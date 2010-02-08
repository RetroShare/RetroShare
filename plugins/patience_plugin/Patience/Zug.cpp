#include "Zug.h"
#include "Karte.h"
#include "Basisstapel.h"
#include <QObject>

using namespace std;

Zug::Zug() : Karte_(0), Herkunft(0), Ziel(0)
{
}


Zug::~Zug()
{
}


bool Zug::ist_gueltig() const
{
bool erg = false;

if (ist_bewegung() == true) erg = true;

else if (ist_aufdeckgung() == true) erg = true;

return erg;
}


bool Zug::ist_bewegung() const
{
bool erg = false;

if (Karte_ != 0 && Herkunft != 0 && Ziel != 0) erg = true;

return erg;
}


bool Zug::ist_aufdeckgung() const
{
bool erg = false;

if (Karte_ != 0 && Herkunft != 0 && Ziel == 0) erg = true;

return erg;
}


void Zug::setze_bewegung(Karte* karte, Basisstapel *herkunft_, Basisstapel *ziel_)
{
Karte_ = karte;
Herkunft = herkunft_;
Ziel = ziel_;
}


void Zug::setze_aufdeckung(Karte* karte, Basisstapel *ort)
{
Karte_ = karte;
Herkunft = ort;
Ziel = 0;
}


Karte* Zug::karte() const
{
return Karte_;
}


Basisstapel* Zug::herkunft() const
{
return Herkunft;
}


Basisstapel* Zug::ziel() const
{
return Ziel;
}


QString Zug::karte_name() const
{
QString erg;

if (Karte_ != 0) erg = Karte_->objectName();

return erg;
}


QString Zug::herkunft_name() const
{
QString erg;

if (Herkunft != 0) erg = Herkunft->objectName();

return erg;
}


QString Zug::ziel_name() const
{
QString erg;

if (Ziel != 0) erg = Ziel->objectName();

return erg;
}


Zug::Zug(Karte* karte, Basisstapel *herkunft, Basisstapel *ziel) : Karte_(karte), Herkunft(herkunft), Ziel(ziel)
{
}


Zug::Zug(Karte* karte, Basisstapel *ort) : Karte_(karte), Herkunft(ort), Ziel(0)
{
}


bool Zug::operator==(const Zug& anderer) const
{
bool erg = false;

if (karte() == anderer.karte() && herkunft() == anderer.herkunft() && ziel() == anderer.ziel()) erg = true;

return erg;
}

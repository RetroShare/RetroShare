#ifndef UNDO_H
#define UNDO_H

#include <QObject>
#include <QMap>
#include <QList>
#include "Zug.h"

class Karte;
class Basisstapel;
class Scene;

class Undo : public QObject
{
Q_OBJECT

public:
Undo(Scene *parent);
virtual ~Undo();

const QStringList speichere() const;
bool lade(const QStringList&);

public slots:
void speichere_zug(const Zug&);
void undo();
void clear();

signals:
void undo_meldung(const Zug&);
void undo_verfuegbar(bool);

private:
QList<Zug> verlauf;
Scene *scene;

void loesche_ueberschuessige_undoelemente();
};

#endif

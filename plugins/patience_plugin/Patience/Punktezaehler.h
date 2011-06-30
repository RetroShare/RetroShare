#ifndef PUNKTEZAEHLER_H
#define PUNKTEZAEHLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include "Zug.h"

class QTimer;
class Scene;

class Punktezaehler : public QObject
{
Q_OBJECT

public:
Punktezaehler(Scene *parent);
virtual ~Punktezaehler();

int punktstand() const;
const QStringList speichere() const;
bool lade(const QStringList&);
bool begonnenes_spiel() const;

public slots:
void neuer_zug(const Zug&);
void stapel_durch();
void neues_spiel();
void spiel_zuende();
void eine_ziehen();
void drei_ziehen();
void undo_meldung(const Zug&);

signals:
void neue_punktzahl(int);
void erster_zug();

private slots:
void reaktion_auf_timeout();

private:
int punkte;
QTimer *straftimer;
Scene *scene;
QStringList nach_oben_liste;

bool nur_eine_ziehen;
int kalkuliere_punkte(const Zug&) const;
bool bringt_zeitaufschub(const Zug&) const;
};

#endif

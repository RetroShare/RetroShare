#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include <QDialog>
#include <QString>
#include "ui_Highscore.h"

class QSettings;

class Highscore : public QDialog, private Ui::Highscore
{
Q_OBJECT

public:
Highscore(QWidget *parent);
virtual ~Highscore();

bool neues_ergebnis(int, long);
void registriere_einstellungen(QSettings*);
void einstellungen_laden();
void einstellungen_speichern();

private:
QSettings *einstellungen;
};

#endif

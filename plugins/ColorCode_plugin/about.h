#ifndef ABOUT_H
#define ABOUT_H

#include "ui_about.h"

#include <QDialog>
#include <QFile>

class About : public QDialog, public Ui::About
{
    Q_OBJECT

    public:
        About(QWidget* parent = 0, Qt::WindowFlags f = 0);
        ~About();

        virtual QSize sizeHint () const;
};

#endif // ABOUT_H

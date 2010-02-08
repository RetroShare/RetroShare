/********************************************************************************
** Form generated from reading ui file 'Speichern_frage.ui'
**
** Created: Mon 8. Feb 22:40:48 2010
**      by: Qt User Interface Compiler version 4.5.2
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_SPEICHERN_FRAGE_H
#define UI_SPEICHERN_FRAGE_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_Speichern_frage
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer_3;
    QLabel *label;
    QSpacerItem *horizontalSpacer_4;
    QHBoxLayout *horizontalLayout_5;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *nein_button;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout;
    QPushButton *cancel_button;
    QPushButton *ok_button;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_2;
    QCheckBox *merk_box;

    void setupUi(QDialog *Speichern_frage)
    {
        if (Speichern_frage->objectName().isEmpty())
            Speichern_frage->setObjectName(QString::fromUtf8("Speichern_frage"));
        Speichern_frage->setWindowModality(Qt::WindowModal);
        Speichern_frage->resize(365, 127);
        verticalLayout = new QVBoxLayout(Speichern_frage);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(5, -1, 5, 5);
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_3);

        label = new QLabel(Speichern_frage);
        label->setObjectName(QString::fromUtf8("label"));
        QFont font;
        font.setPointSize(21);
        label->setFont(font);

        horizontalLayout_4->addWidget(label);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_4);


        verticalLayout->addLayout(horizontalLayout_4);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        nein_button = new QPushButton(Speichern_frage);
        nein_button->setObjectName(QString::fromUtf8("nein_button"));
        nein_button->setAutoDefault(false);

        horizontalLayout_2->addWidget(nein_button);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);


        horizontalLayout_5->addLayout(horizontalLayout_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        cancel_button = new QPushButton(Speichern_frage);
        cancel_button->setObjectName(QString::fromUtf8("cancel_button"));
        cancel_button->setAutoDefault(false);
        cancel_button->setDefault(true);

        horizontalLayout->addWidget(cancel_button);

        ok_button = new QPushButton(Speichern_frage);
        ok_button->setObjectName(QString::fromUtf8("ok_button"));
        ok_button->setAutoDefault(false);

        horizontalLayout->addWidget(ok_button);


        horizontalLayout_5->addLayout(horizontalLayout);


        verticalLayout->addLayout(horizontalLayout_5);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_2);

        merk_box = new QCheckBox(Speichern_frage);
        merk_box->setObjectName(QString::fromUtf8("merk_box"));
        QFont font1;
        font1.setPointSize(15);
        merk_box->setFont(font1);
        merk_box->setLayoutDirection(Qt::RightToLeft);

        horizontalLayout_3->addWidget(merk_box);


        verticalLayout->addLayout(horizontalLayout_3);


        retranslateUi(Speichern_frage);

        QMetaObject::connectSlotsByName(Speichern_frage);
    } // setupUi

    void retranslateUi(QDialog *Speichern_frage)
    {
        Speichern_frage->setWindowTitle(QApplication::translate("Speichern_frage", "Patience - Save game and quit ?", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Speichern_frage", "Save the game and quit ?", 0, QApplication::UnicodeUTF8));
        nein_button->setText(QApplication::translate("Speichern_frage", "Don't Save", 0, QApplication::UnicodeUTF8));
        cancel_button->setText(QApplication::translate("Speichern_frage", "Cancel", 0, QApplication::UnicodeUTF8));
        ok_button->setText(QApplication::translate("Speichern_frage", "OK", 0, QApplication::UnicodeUTF8));
        merk_box->setText(QApplication::translate("Speichern_frage", "Remember answer", 0, QApplication::UnicodeUTF8));
        Q_UNUSED(Speichern_frage);
    } // retranslateUi

};

namespace Ui {
    class Speichern_frage: public Ui_Speichern_frage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SPEICHERN_FRAGE_H

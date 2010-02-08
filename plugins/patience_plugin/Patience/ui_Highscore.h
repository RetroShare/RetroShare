/********************************************************************************
** Form generated from reading ui file 'Highscore.ui'
**
** Created: Mon 8. Feb 22:40:48 2010
**      by: Qt User Interface Compiler version 4.5.2
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_HIGHSCORE_H
#define UI_HIGHSCORE_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QTableWidget>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_Highscore
{
public:
    QVBoxLayout *verticalLayout;
    QTableWidget *tabelle;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *ok_button;

    void setupUi(QDialog *Highscore)
    {
        if (Highscore->objectName().isEmpty())
            Highscore->setObjectName(QString::fromUtf8("Highscore"));
        Highscore->setWindowModality(Qt::WindowModal);
        Highscore->resize(528, 300);
        Highscore->setMinimumSize(QSize(528, 300));
        verticalLayout = new QVBoxLayout(Highscore);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        tabelle = new QTableWidget(Highscore);
        if (tabelle->columnCount() < 3)
            tabelle->setColumnCount(3);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        tabelle->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        tabelle->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        tabelle->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        tabelle->setObjectName(QString::fromUtf8("tabelle"));
        tabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tabelle->setSelectionBehavior(QAbstractItemView::SelectRows);
        tabelle->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        tabelle->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        tabelle->horizontalHeader()->setStretchLastSection(true);

        verticalLayout->addWidget(tabelle);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        ok_button = new QPushButton(Highscore);
        ok_button->setObjectName(QString::fromUtf8("ok_button"));
        ok_button->setAutoDefault(false);
        ok_button->setDefault(true);

        horizontalLayout->addWidget(ok_button);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(Highscore);
        QObject::connect(ok_button, SIGNAL(clicked()), Highscore, SLOT(accept()));

        QMetaObject::connectSlotsByName(Highscore);
    } // setupUi

    void retranslateUi(QDialog *Highscore)
    {
        Highscore->setWindowTitle(QApplication::translate("Highscore", "Patience - Highscore", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem = tabelle->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QApplication::translate("Highscore", "Name", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem1 = tabelle->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QApplication::translate("Highscore", "Points", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem2 = tabelle->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QApplication::translate("Highscore", "Time", 0, QApplication::UnicodeUTF8));
        ok_button->setText(QApplication::translate("Highscore", "Ok", 0, QApplication::UnicodeUTF8));
        Q_UNUSED(Highscore);
    } // retranslateUi

};

namespace Ui {
    class Highscore: public Ui_Highscore {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_HIGHSCORE_H

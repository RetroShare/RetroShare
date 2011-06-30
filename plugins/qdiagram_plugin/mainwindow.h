/****************************************************************************
**
** Copyright (C) 2007-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the example classes of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QShortcut>

#include "diagramitem.h"
#include "diagramdrawitem.h"
#include "diagrampathitem.h"

class DiagramScene;

QT_BEGIN_NAMESPACE
class QAction;
class QToolBox;
class QSpinBox;
class QComboBox;
class QFontComboBox;
class QButtonGroup;
class QLineEdit;
class QGraphicsTextItem;
class QFont;
class QToolButton;
class QAbstractButton;
class QGraphicsView;
QT_END_NAMESPACE

//! [0]
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
   MainWindow();

private slots:
    void backgroundButtonGroupClicked(QAbstractButton *button);
    void buttonGroupClicked(int id);
    void deleteItem();
    void pointerGroupClicked(int id);
    void bringToFront();
    void sendToBack();
    void rotateRight();
    void rotateLeft();
    void flipX();
    void flipY();
    void abort();
    void print();
    void exportImage();
    void copyItems();
    void groupItems();
    void ungroupItems();
    void itemInserted(DiagramItem *item);
    void textInserted(QGraphicsTextItem *item);
    void currentFontChanged(const QFont &font);
    void fontSizeChanged(const QString &size);
    void textColorChanged();
    void itemColorChanged();
    void lineColorChanged();
    void lineArrowChanged();
    void textButtonTriggered();
    void fillButtonTriggered();
    void lineButtonTriggered();
    void lineArrowButtonTriggered();
    void handleFontChange();
    void itemSelected(QGraphicsItem *item);
    void about();
    void activateShortcuts();
    void deactivateShortcuts();
    void zoomIn();
    void zoomOut();
    void zoom(const qreal factor);
    void zoomRect();
    void doZoomRect(QPointF p1,QPointF p2);
    void zoomFit();
    void toggleGrid(bool grid);
    void save();
    void saveAs();
    void load();
    void moveItems();

private:
    void createToolBox();
    void createActions();
    void createMenus();
    void createToolbars();
    void setGrid();
    QWidget *createBackgroundCellWidget(const QString &text,
                                        const QString &image);
    QWidget *createCellWidget(const QString &text,
                              DiagramItem::DiagramType type);
    QWidget *createCellWidget(const QString &text,
                          DiagramDrawItem::DiagramType type);
    QMenu *createColorMenu(const char *slot, QColor defaultColor);
    QIcon createColorToolButtonIcon(const QString &image, QColor color);
    QIcon createColorIcon(QColor color);

    QMenu *createArrowMenu(const char *slot, const int def);
    QIcon createArrowIcon(const int i);

    DiagramScene *scene;
    QGraphicsView *view;

    QAction *exitAction;
    QAction *addAction;
    QAction *deleteAction;

    QAction *toFrontAction;
    QAction *sendBackAction;
    QAction *rotateRightAction;
    QAction *rotateLeftAction;
    QAction *flipXAction;
    QAction *flipYAction;
    QAction *copyAction;
    QAction *moveAction;
    QAction *groupAction;
    QAction *ungroupAction;
    QAction *aboutAction;

    QAction *zoomInAction;
    QAction *zoomOutAction;
    QAction *zoomAction;
    QAction *zoomFitAction;
    QAction *showGridAction;

    QAction *printAction;
    QAction *exportAction;

    QShortcut *escShortcut;

    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *itemMenu;
    QMenu *aboutMenu;

    QToolBar *textToolBar;
    QToolBar *editToolBar;
    QToolBar *colorToolBar;
    QToolBar *pointerToolbar;
    QToolBar *zoomToolbar;

    QComboBox *itemColorCombo;
    QComboBox *textColorCombo;
    QComboBox *fontSizeCombo;
    QFontComboBox *fontCombo;

    QToolBox *toolBox;
    QButtonGroup *buttonGroup;
    QButtonGroup *pointerTypeGroup;
    QButtonGroup *backgroundButtonGroup;
    QToolButton *fontColorToolButton;
    QToolButton *fillColorToolButton;
    QToolButton *lineColorToolButton;
    QToolButton *linePointerButton;
    QAction *boldAction;
    QAction *underlineAction;
    QAction *italicAction;
    QAction *textAction;
    QAction *fillAction;
    QAction *lineAction;
    QAction *arrowAction;
    QAction *loadAction;
    QAction *saveAction;
    QAction *saveAsAction;

    QList<QAction*> listOfActions;
    QList<QShortcut*> listOfShortcuts;

    bool myShowGrid; // Grid visible ?
    qreal myGrid; // Grid distance (dx=dy)

    QString myFileName; // aktueller Filename

};
//! [0]

#endif

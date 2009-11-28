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

#include <QtGui>
#include <QLabel>
#include <iostream>

#include "mainwindow.h"
#include "diagramitem.h"
#include "diagramdrawitem.h"
#include "diagrampathitem.h"
#include "diagramscene.h"
#include "diagramtextitem.h"

const int InsertTextButton = 10;
const int InsertDrawItemButton = 64;

#define NDEBUG

//! [0]
MainWindow::MainWindow()
{
    createActions();
    createToolBox();
    createMenus();

    scene = new DiagramScene(itemMenu);
    scene->setSceneRect(QRectF(0, 0, 5000, 5000));
    myGrid=10.0;
    scene->setGrid(myGrid);
    connect(scene, SIGNAL(itemInserted(DiagramItem *)),
            this, SLOT(itemInserted(DiagramItem *)));
    connect(scene, SIGNAL(textInserted(QGraphicsTextItem *)),
        this, SLOT(textInserted(QGraphicsTextItem *)));
    connect(scene, SIGNAL(itemSelected(QGraphicsItem *)),
        this, SLOT(itemSelected(QGraphicsItem *)));
    // activate/deactivate shortcuts when text is edited in scene
    connect(scene, SIGNAL(editorHasReceivedFocus()),
    	this, SLOT(deactivateShortcuts()));
    connect(scene, SIGNAL(editorHasLostFocus()),
        	this, SLOT(activateShortcuts()));
    connect(scene, SIGNAL(zoomRect(QPointF,QPointF)),
    		this, SLOT(doZoomRect(QPointF,QPointF)));
    connect(scene, SIGNAL(zoom(const qreal)),
        		this, SLOT(zoom(const qreal)));
    createToolbars();

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(toolBox);
    view = new QGraphicsView(scene);
    view->setDragMode(QGraphicsView::RubberBandDrag);
    view->setCacheMode(QGraphicsView::CacheBackground);
    layout->addWidget(view);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    setCentralWidget(widget);
    setWindowTitle(tr("QDiagram"));

    myFileName.clear();

    myShowGrid=false;
}
//! [0]

//! [1]
void MainWindow::backgroundButtonGroupClicked(QAbstractButton *button)
{
    QList<QAbstractButton *> buttons = backgroundButtonGroup->buttons();
    foreach (QAbstractButton *myButton, buttons) {
    if (myButton != button)
        button->setChecked(false);
    }
    QString text = button->text();
    if (text == tr("Blue Grid"))
        scene->setBackgroundBrush(QPixmap(":/images/background1.png"));
    else if (text == tr("White Grid"))
        scene->setBackgroundBrush(QPixmap(":/images/background2.png"));
    else if (text == tr("Gray Grid"))
        scene->setBackgroundBrush(QPixmap(":/images/background3.png"));
    else
        scene->setBackgroundBrush(QPixmap(":/images/background4.png"));

    scene->update();
    view->update();
}
//! [1]

//! [2]
void MainWindow::buttonGroupClicked(int id)
{
    QList<QAbstractButton *> buttons = buttonGroup->buttons();
    foreach (QAbstractButton *button, buttons) {
    if (buttonGroup->button(id) != button)
        button->setChecked(false);
    }
    if (id == InsertTextButton) {
        scene->setMode(DiagramScene::InsertText);
    }
    else
    if ((id&192) == InsertDrawItemButton){
    	scene->setItemType(DiagramDrawItem::DiagramType(id&63));
        scene->setMode(DiagramScene::InsertDrawItem);
    }
    else {
        scene->setItemType(DiagramItem::DiagramType(id));
        scene->setMode(DiagramScene::InsertItem);
    }
    view->setDragMode(QGraphicsView::NoDrag);
    //pointerTypeGroup->checkedButton()->setChecked(false);
    buttons = pointerTypeGroup->buttons();
    foreach (QAbstractButton *button, buttons) {
    	button->setChecked(false);
    }
}
//! [2]

//! [3]
void MainWindow::deleteItem()
{
    foreach (QGraphicsItem *item, scene->selectedItems()) {
        if (item->type() == DiagramItem::Type) {
            //qgraphicsitem_cast<DiagramItem *>(item)->removeArrows();
        }
        scene->removeItem(item);
    }
}
//! [3]

//! [4]
void MainWindow::pointerGroupClicked(int id)
{
	QList<QAbstractButton *> buttons = pointerTypeGroup->buttons();
	foreach (QAbstractButton *button, buttons) {
		if (pointerTypeGroup->button(id) != button)
			button->setChecked(false);
	}
	if(pointerTypeGroup->checkedId()!=DiagramScene::MoveItem) view->setDragMode(QGraphicsView::NoDrag);
	else view->setDragMode(QGraphicsView::RubberBandDrag);
    scene->setMode(DiagramScene::Mode(pointerTypeGroup->checkedId()));
}
//! [4]

//! [5]
void MainWindow::bringToFront()
{
	//std::cout << "bring to front" << std::endl;
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    foreach (QGraphicsItem *item, overlapItems) {
        if (item->zValue() >= zValue) //&&
            //item->type() == DiagramItem::Type)
            zValue = item->zValue() + 0.1;
    }
    selectedItem->setZValue(zValue);
}
//! [5]

//! [6]
void MainWindow::sendToBack()
{
	//std::cout << "send to back" << std::endl;
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    foreach (QGraphicsItem *item, overlapItems) {
        if (item->zValue() <= zValue)// &&
            //item->type() == DiagramItem::Type)
            zValue = item->zValue() - 0.1;
    }
    selectedItem->setZValue(zValue);
}

void MainWindow::rotateRight()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    //selectedItem->rotate(90);
    QTransform trans=selectedItem->transform();
    selectedItem->setTransform(trans*QTransform().rotate(90),false);
#ifdef DEBUG
    std::cout << "rotate done" << std::endl;
#endif
}

void MainWindow::rotateLeft()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    //selectedItem->rotate(-90);
    QTransform trans=selectedItem->transform();
    selectedItem->setTransform(trans*QTransform().rotate(-90),false);
#ifdef DEBUG
    std::cout << "rotate left done" << std::endl;
#endif
}

void MainWindow::flipX()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    //selectedItem->setTransform(QTransform(-1,0,0,1,0,0),true);
    QTransform trans=selectedItem->transform();
    selectedItem->setTransform(trans*QTransform(-1,0,0,1,0,0),false);
    //selectedItem->scale(-1,1);
#ifdef DEBUG
    std::cout << trans.m11() << ","
			  << trans.m12() << ","
			  << trans.m21() << ","
			  << trans.m22() << "," << std::endl;

    std::cout << "flipX done" << std::endl;
#endif
}

void MainWindow::flipY()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    QTransform trans=selectedItem->transform();
    selectedItem->setTransform(trans*QTransform(1,0,0,-1,0,0),false);
    //selectedItem->scale(1,-1);
#ifdef DEBUG
    std::cout << "flipY done" << std::endl;
#endif
}

void MainWindow::moveItems()
{
	scene->setMode(DiagramScene::MoveItems);
	view->setDragMode(QGraphicsView::RubberBandDrag);
#ifdef DEBUG
	std::cout << "set move mode" << std::endl;
#endif
}

void MainWindow::abort()
{
    scene->abort();
    scene->setMode(DiagramScene::MoveItem);
    view->setDragMode(QGraphicsView::RubberBandDrag);
#ifdef DEBUG
    std::cout << "abort done" << std::endl;
#endif
}

void MainWindow::copyItems()
{
	scene->setMode(DiagramScene::CopyItem);
	view->setDragMode(QGraphicsView::RubberBandDrag);
#ifdef DEBUG
	std::cout << "copy mode" << std::endl;
#endif
}

void MainWindow::groupItems()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItemGroup *test = scene->createItemGroup(scene->selectedItems());
    test->setFlag(QGraphicsItem::ItemIsMovable, true);
    test->setFlag(QGraphicsItem::ItemIsSelectable, true);
}

void MainWindow::ungroupItems()
{
    if (scene->selectedItems().isEmpty())
        return;
#ifdef DEBUG
    std::cout << "ungroup" << std::endl;
    std::cout << scene->selectedItems().count() << std::endl;
#endif
    foreach (QGraphicsItem *item, scene->selectedItems()) {
#ifdef DEBUG
    	std::cout << item->type() << std::endl;
    	std::cout << typeid(item).name() << std::endl;
#endif
    	if (item->type()==10) {
    		QGraphicsItemGroup *group = (QGraphicsItemGroup*) item;
    		scene->destroyItemGroup(group);
    	}
    }
#ifdef DEBUG
    //std::cout << qPrintable(scene->selectedItems().first()->objectName())<< std::endl;
#endif
    //QGraphicsItemGroup *group = (QGraphicsItemGroup*)scene->selectedItems().first();
    //scene->destroyItemGroup(group);
    //scene->destroyItemGroup(scene->selectedItems());
}
//! [6]

//! [7]
void MainWindow::itemInserted(DiagramItem *item)
{
    //scene->setMode(DiagramScene::Mode(pointerTypeGroup->checkedId()));
    buttonGroup->button(int(item->diagramType()))->setChecked(false);
}
//! [7]

//! [8]
void MainWindow::textInserted(QGraphicsTextItem *)
{
    buttonGroup->button(InsertTextButton)->setChecked(false);
    scene->setMode(DiagramScene::MoveItem);
    view->setDragMode(QGraphicsView::RubberBandDrag);
    pointerTypeGroup->button(DiagramScene::MoveItem)->setChecked(true);
}
//! [8]
void MainWindow::activateShortcuts()
{
    foreach(QAction* item,listOfActions){
    	item->setEnabled(true);
    }
    foreach(QShortcut* item,listOfShortcuts){
		item->setEnabled(true);
	}
#ifdef DEBUG
    std::cout<<"activate"<<std::endl;
#endif
}

void MainWindow::deactivateShortcuts()
{
	foreach(QAction* item,listOfActions){
		item->setEnabled(false);
	}
	foreach(QShortcut* item,listOfShortcuts){
		item->setEnabled(false);
	}
#ifdef DEBUG
	std::cout<<"deactivate"<<std::endl;
#endif
}
//! [9]
void MainWindow::currentFontChanged(const QFont &)
{
    handleFontChange();
}
//! [9]

//! [10]
void MainWindow::fontSizeChanged(const QString &)
{
    handleFontChange();
}
//! [10]

//! [12]
void MainWindow::textColorChanged()
{
    textAction = qobject_cast<QAction *>(sender());
    fontColorToolButton->setIcon(createColorToolButtonIcon(
                ":/images/textpointer.png",
                qVariantValue<QColor>(textAction->data())));
    textButtonTriggered();
}
//! [12]

//! [13]
void MainWindow::itemColorChanged()
{
    fillAction = qobject_cast<QAction *>(sender());
    fillColorToolButton->setIcon(createColorToolButtonIcon(
                 ":/images/floodfill.png",
                 qVariantValue<QColor>(fillAction->data())));
    fillButtonTriggered();
}
//! [13]

//! [14]
void MainWindow::lineColorChanged()
{
    lineAction = qobject_cast<QAction *>(sender());
    lineColorToolButton->setIcon(createColorToolButtonIcon(
                 ":/images/linecolor.png",
                 qVariantValue<QColor>(lineAction->data())));
    lineButtonTriggered();
}
//! [14]

//! [15]
void MainWindow::textButtonTriggered()
{
    scene->setTextColor(qVariantValue<QColor>(textAction->data()));
}
//! [15]

//! [16]
void MainWindow::fillButtonTriggered()
{
    scene->setItemColor(qVariantValue<QColor>(fillAction->data()));
}
//! [16]

//! [17]
void MainWindow::lineButtonTriggered()
{
    scene->setLineColor(qVariantValue<QColor>(lineAction->data()));
}
//! [17]

//! [18]
void MainWindow::handleFontChange()
{
    QFont font = fontCombo->currentFont();
    font.setPointSize(fontSizeCombo->currentText().toInt());
    font.setWeight(boldAction->isChecked() ? QFont::Bold : QFont::Normal);
    font.setItalic(italicAction->isChecked());
    font.setUnderline(underlineAction->isChecked());

    scene->setFont(font);
}
//! [18]

//! [19]
void MainWindow::itemSelected(QGraphicsItem *item)
{
    DiagramTextItem *textItem =
    qgraphicsitem_cast<DiagramTextItem *>(item);

    QFont font = textItem->font();
    QColor color = textItem->defaultTextColor();
    fontCombo->setCurrentFont(font);
    fontSizeCombo->setEditText(QString().setNum(font.pointSize()));
    boldAction->setChecked(font.weight() == QFont::Bold);
    italicAction->setChecked(font.italic());
    underlineAction->setChecked(font.underline());
}
//! [19]

//! [20]
void MainWindow::about()
{
    QMessageBox::about(this, tr("About Diagram Scene"),
                       tr("The <b>Diagram Scene</b> example shows "
                          "use of the graphics framework."));
}
//! [20]
void MainWindow::print()
{
	QPrinter printer;
	if (QPrintDialog(&printer).exec() == QDialog::Accepted) {
		QPainter painter(&printer);
		painter.setRenderHint(QPainter::Antialiasing);
		scene->render(&painter);
	}
}

void MainWindow::exportImage()
{
	QFileDialog::Options options;
	options = 0;
	QString selectedFilter;
	QString fileName = QFileDialog::getSaveFileName(this,
			tr("Export Diagram to ..."),
			".jpg",
			tr("Jpg (*.jpg);;Png (*.png);;Pdf (*.pdf);;Postscript (*.ps)"),
			&selectedFilter,
			options);
	if (!fileName.isEmpty()){
#ifdef DEBUG
		std::cout << "Selected Filter: " << qPrintable(selectedFilter) << std::endl;
#endif
		if((selectedFilter=="Pdf (*.pdf)")or(selectedFilter=="Postscript (*.ps)")) {
			QRectF rect=scene->itemsBoundingRect(); // Bonding der Elemente in scene
			QPrinter printer;
			printer.setOutputFileName(fileName);
			QSizeF size=printer.paperSize(QPrinter::Millimeter); // definiere Paper mit gleichen Aspectratio wie rect
			size.setHeight(size.width()*rect.height()/rect.width());
			printer.setPaperSize(size,QPrinter::Millimeter);
			printer.setPageMargins(0,0,0,0,QPrinter::Millimeter);
			QPainter painter(&printer);// generate PDF/PS
			painter.setRenderHint(QPainter::Antialiasing);
			scene->render(&painter,QRectF(),rect);
		}
		else {
			QPixmap pixmap(1000,1000);
			pixmap.fill();
			QPainter painter(&pixmap);
			painter.setRenderHint(QPainter::Antialiasing);
			QRectF rect=scene->itemsBoundingRect();
			scene->render(&painter,QRectF(),rect);
			painter.end();

			pixmap.save(fileName);
		}

	}
}

//! [21]
void MainWindow::createToolBox()
{
    buttonGroup = new QButtonGroup;
    buttonGroup->setExclusive(false);
    connect(buttonGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(buttonGroupClicked(int)));
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(createCellWidget(tr("Conditional"),
                               DiagramItem::Conditional), 0, 0);
    layout->addWidget(createCellWidget(tr("Process"),
                      DiagramItem::Step),0, 1);
    layout->addWidget(createCellWidget(tr("Input/Output"),
                      DiagramItem::Io), 1, 0);
// added DrawItem
    layout->addWidget(createCellWidget(tr("Rectangle"),
                          DiagramDrawItem::Rectangle), 2, 0);
    layout->addWidget(createCellWidget(tr("Ellipse"),
                              DiagramDrawItem::Ellipse), 2, 1);
//! [21]

    QToolButton *textButton = new QToolButton;
    textButton->setCheckable(true);
    buttonGroup->addButton(textButton, InsertTextButton);
    textButton->setIcon(QIcon(QPixmap(":/images/textpointer.png")
                        .scaled(30, 30)));
    textButton->setIconSize(QSize(50, 50));
    QGridLayout *textLayout = new QGridLayout;
    textLayout->addWidget(textButton, 0, 0, Qt::AlignHCenter);
    textLayout->addWidget(new QLabel(tr("Text")), 1, 0, Qt::AlignCenter);
    QWidget *textWidget = new QWidget;
    textWidget->setLayout(textLayout);
    layout->addWidget(textWidget, 1, 1);

    layout->setRowStretch(3, 10);
    layout->setColumnStretch(2, 10);

    QWidget *itemWidget = new QWidget;
    itemWidget->setLayout(layout);

    backgroundButtonGroup = new QButtonGroup;
    connect(backgroundButtonGroup, SIGNAL(buttonClicked(QAbstractButton *)),
            this, SLOT(backgroundButtonGroupClicked(QAbstractButton *)));

    QGridLayout *backgroundLayout = new QGridLayout;
    backgroundLayout->addWidget(createBackgroundCellWidget(tr("Blue Grid"),
                ":/images/background1.png"), 0, 0);
    backgroundLayout->addWidget(createBackgroundCellWidget(tr("White Grid"),
                ":/images/background2.png"), 0, 1);
    backgroundLayout->addWidget(createBackgroundCellWidget(tr("Gray Grid"),
                    ":/images/background3.png"), 1, 0);
    backgroundLayout->addWidget(createBackgroundCellWidget(tr("No Grid"),
                ":/images/background4.png"), 1, 1);

    backgroundLayout->setRowStretch(2, 10);
    backgroundLayout->setColumnStretch(2, 10);

    QWidget *backgroundWidget = new QWidget;
    backgroundWidget->setLayout(backgroundLayout);


//! [22]
    toolBox = new QToolBox;
    toolBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Ignored));
    toolBox->setMinimumWidth(itemWidget->sizeHint().width());
    toolBox->addItem(itemWidget, tr("Basic Flowchart Shapes"));
    toolBox->addItem(backgroundWidget, tr("Backgrounds"));
}
//! [22]

//! [23]
void MainWindow::createActions()
{
    toFrontAction = new QAction(QIcon(":/images/lc_bringtofront.png"),
                                tr("Bring to &Front"), this);
    toFrontAction->setShortcut(tr("Ctrl+F"));
    toFrontAction->setStatusTip(tr("Bring item to front"));
    connect(toFrontAction, SIGNAL(triggered()),
            this, SLOT(bringToFront()));
//! [23]

    sendBackAction = new QAction(QIcon(":/images/lc_sendtoback.png"),
                                 tr("Send to &Back"), this);
    sendBackAction->setShortcut(tr("Ctrl+B"));
    sendBackAction->setStatusTip(tr("Send item to back"));
    connect(sendBackAction, SIGNAL(triggered()),
        this, SLOT(sendToBack()));

    QPixmap pixmap2=QPixmap(":/images/lc_toggleobjectrotatemode.png").transformed(QTransform(-1,0,0,1,0,0));
    rotateRightAction = new QAction(QIcon(pixmap2),
                                     tr("rotate &Right"), this);
	rotateRightAction->setShortcut(tr("R"));
	rotateRightAction->setStatusTip(tr("rotate item 90 degrees right"));
	connect(rotateRightAction, SIGNAL(triggered()),
		this, SLOT(rotateRight()));
	listOfActions.append(rotateRightAction);

    rotateLeftAction = new QAction(QIcon(":/images/lc_toggleobjectrotatemode.png"),
                                     tr("rotate &Left"), this);
	rotateLeftAction->setShortcut(tr("Shift+R"));
	rotateLeftAction->setStatusTip(tr("rotate item 90 degrees left"));
	connect(rotateLeftAction, SIGNAL(triggered()),
		this, SLOT(rotateLeft()));
	listOfActions.append(rotateLeftAction);

	groupAction = new QAction(QIcon(":/images/lc_group.png"),
									 tr("&group Items"), this);
	groupAction->setShortcut(tr("Ctrl+G"));
	groupAction->setStatusTip(tr("group Items"));
	connect(groupAction, SIGNAL(triggered()),
		this, SLOT(groupItems()));

	ungroupAction = new QAction(QIcon(":/images/lc_ungroup.png"),
									 tr("&ungroup Item"), this);
	ungroupAction->setShortcut(tr("Shift+Ctrl+G"));
	ungroupAction->setStatusTip(tr("ungruoup Items"));
	connect(ungroupAction, SIGNAL(triggered()),
		this, SLOT(ungroupItems()));

    deleteAction = new QAction(QIcon(":/images/delete.png"),
                               tr("&Delete"), this);
    deleteAction->setShortcut(tr("Delete"));
    deleteAction->setStatusTip(tr("Delete item from diagram"));
    connect(deleteAction, SIGNAL(triggered()),
        this, SLOT(deleteItem()));
    listOfActions.append(deleteAction);

    printAction = new QAction(tr("&Print"), this);
    printAction->setStatusTip(tr("Print Diagram"));
    connect(printAction, SIGNAL(triggered()), this, SLOT(print()));

    exportAction = new QAction(tr("&Export Diagram"), this);
    exportAction->setStatusTip(tr("Export Diagram to image"));
    connect(exportAction, SIGNAL(triggered()), this, SLOT(exportImage()));

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(tr("Ctrl+X"));
    exitAction->setStatusTip(tr("Quit Scenediagram example"));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    boldAction = new QAction(tr("Bold"), this);
    boldAction->setCheckable(true);
    QPixmap pixmap(":/images/lc_bold.png");
    boldAction->setIcon(QIcon(pixmap));
    //boldAction->setShortcut(tr("Ctrl+B"));
    connect(boldAction, SIGNAL(triggered()),
            this, SLOT(handleFontChange()));

    italicAction = new QAction(QIcon(":/images/lc_italic.png"),
                               tr("Italic"), this);
    italicAction->setCheckable(true);
    //italicAction->setShortcut(tr("Ctrl+I"));
    connect(italicAction, SIGNAL(triggered()),
            this, SLOT(handleFontChange()));

    underlineAction = new QAction(QIcon(":/images/lc_underline.png"),
                                  tr("Underline"), this);
    underlineAction->setCheckable(true);
    //underlineAction->setShortcut(tr("Ctrl+U"));
    connect(underlineAction, SIGNAL(triggered()),
            this, SLOT(handleFontChange()));

    aboutAction = new QAction(tr("A&bout"), this);
    //aboutAction->setShortcut(tr("Ctrl+B"));
    connect(aboutAction, SIGNAL(triggered()),
            this, SLOT(about()));

    copyAction = new QAction(tr("&Copy"), this);
	copyAction->setShortcut(tr("c"));
	connect(copyAction, SIGNAL(triggered()),
			this, SLOT(copyItems()));
	listOfActions.append(copyAction);

	moveAction = new QAction(tr("&Move"), this);
	moveAction->setShortcut(tr("m"));
	connect(moveAction, SIGNAL(triggered()),
			this, SLOT(moveItems()));
	listOfActions.append(moveAction);

	flipXAction = new QAction(QIcon(":/images/lc_flipvertical.png"),
									tr("Flip &X"), this);
	flipXAction->setShortcut(tr("f"));
	connect(flipXAction, SIGNAL(triggered()),
			this, SLOT(flipX()));
	listOfActions.append(flipXAction);

	flipYAction = new QAction(QIcon(":/images/lc_fliphorizontal.png"),tr("Flip &Y"), this);
	flipYAction->setShortcut(tr("Shift+F"));
	connect(flipYAction, SIGNAL(triggered()),
			this, SLOT(flipY()));
	listOfActions.append(flipYAction);

	escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape),
            this);
	connect(escShortcut,SIGNAL(activated()),this,SLOT(abort()));

	// Zoom in/out
	zoomInAction = new QAction(QIcon(":/images/lc_zoomin.png"),tr("Zoom &in"), this);
	zoomInAction->setShortcut(tr("Shift+z"));
	connect(zoomInAction, SIGNAL(triggered()),
			this, SLOT(zoomIn()));
	listOfActions.append(zoomInAction);

	zoomOutAction = new QAction(QIcon(":/images/lc_zoomminus.png"),tr("Zoom &out"), this);
	zoomOutAction->setShortcut(tr("Ctrl+z"));
	connect(zoomOutAction, SIGNAL(triggered()),
			this, SLOT(zoomOut()));

	zoomAction = new QAction(QIcon(":/images/lc_zoomoptimal.png"),tr("&Zoom area"), this);
	zoomAction->setShortcut(tr("z"));
	connect(zoomAction, SIGNAL(triggered()),
			this, SLOT(zoomRect()));
	listOfActions.append(zoomAction);

	zoomFitAction = new QAction(QIcon(":/images/lc_zoompagewidth.png"),tr("Zoom &Fit"), this);
	zoomFitAction->setShortcut(tr("v"));
	connect(zoomFitAction, SIGNAL(triggered()),
			this, SLOT(zoomFit()));
	listOfActions.append(zoomFitAction);

	showGridAction = new QAction(QIcon(":/images/lc_gridvisible.png"),tr("Show &Grid"), this);
	showGridAction->setCheckable(true);
	showGridAction->setChecked(false);
	connect(showGridAction, SIGNAL(toggled(bool)),
			this, SLOT(toggleGrid(bool)));

	loadAction = new QAction(QIcon(":/images/lc_open.png"),tr("L&oad ..."), this);
	loadAction->setShortcut(tr("Ctrl+o"));
	connect(loadAction, SIGNAL(triggered()),
			this, SLOT(load()));

	saveAction = new QAction(QIcon(":/images/lc_save.png"),tr("&Save ..."), this);
	saveAction->setShortcut(tr("Ctrl+s"));
	connect(saveAction, SIGNAL(triggered()),
			this, SLOT(save()));

	saveAsAction = new QAction(QIcon(":/images/lc_saveas.png"),tr("Save &As ..."), this);
	saveAsAction->setShortcut(tr("Ctrl+s"));
	connect(saveAsAction, SIGNAL(triggered()),
			this, SLOT(saveAs()));
}

//! [24]
void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(loadAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addAction(printAction);
    fileMenu->addAction(exportAction);
    fileMenu->addAction(exitAction);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(zoomInAction);
    viewMenu->addAction(zoomOutAction);
    viewMenu->addAction(zoomAction);
    viewMenu->addAction(zoomFitAction);
    viewMenu->addSeparator();
    viewMenu->addAction(showGridAction);

    itemMenu = menuBar()->addMenu(tr("&Item"));
    itemMenu->addAction(deleteAction);
    itemMenu->addAction(copyAction);
    itemMenu->addAction(moveAction);
    itemMenu->addSeparator();
    itemMenu->addAction(toFrontAction);
    itemMenu->addAction(sendBackAction);
    itemMenu->addSeparator();
    itemMenu->addAction(rotateRightAction);
    itemMenu->addAction(rotateLeftAction);
    itemMenu->addAction(flipXAction);
    itemMenu->addAction(flipYAction);
    itemMenu->addAction(groupAction);
    itemMenu->addAction(ungroupAction);

    aboutMenu = menuBar()->addMenu(tr("&Help"));
    aboutMenu->addAction(aboutAction);
}
//! [24]

//! [25]
void MainWindow::createToolbars()
{
//! [25]
    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(deleteAction);
    editToolBar->addAction(toFrontAction);
    editToolBar->addAction(sendBackAction);
    editToolBar->addAction(flipXAction);
    editToolBar->addAction(flipYAction);
    editToolBar->addAction(rotateRightAction);
    editToolBar->addAction(rotateLeftAction);

    fontCombo = new QFontComboBox();
    fontSizeCombo = new QComboBox();
    connect(fontCombo, SIGNAL(currentFontChanged(const QFont &)),
            this, SLOT(currentFontChanged(const QFont &)));

    fontSizeCombo = new QComboBox;
    fontSizeCombo->setEditable(true);
    for (int i = 8; i < 30; i = i + 2)
        fontSizeCombo->addItem(QString().setNum(i));
    QIntValidator *validator = new QIntValidator(2, 64, this);
    fontSizeCombo->setValidator(validator);
    connect(fontSizeCombo, SIGNAL(currentIndexChanged(const QString &)),
            this, SLOT(fontSizeChanged(const QString &)));

    fontColorToolButton = new QToolButton;
    fontColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
    fontColorToolButton->setMenu(createColorMenu(SLOT(textColorChanged()),
                                                 Qt::black));
    textAction = fontColorToolButton->menu()->defaultAction();
    fontColorToolButton->setIcon(createColorToolButtonIcon(
    ":/images/textpointer.png", Qt::black));
    fontColorToolButton->setAutoFillBackground(true);
    connect(fontColorToolButton, SIGNAL(clicked()),
            this, SLOT(textButtonTriggered()));

//! [26]
    fillColorToolButton = new QToolButton;
    fillColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
    fillColorToolButton->setMenu(createColorMenu(SLOT(itemColorChanged()),
                         Qt::white));
    fillAction = fillColorToolButton->menu()->defaultAction();
    fillColorToolButton->setIcon(createColorToolButtonIcon(
    ":/images/floodfill.png", Qt::white));
    connect(fillColorToolButton, SIGNAL(clicked()),
            this, SLOT(fillButtonTriggered()));
//! [26]

    lineColorToolButton = new QToolButton;
    lineColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
    lineColorToolButton->setMenu(createColorMenu(SLOT(lineColorChanged()),
                                 Qt::black));
    lineAction = lineColorToolButton->menu()->defaultAction();
    lineColorToolButton->setIcon(createColorToolButtonIcon(
        ":/images/linecolor.png", Qt::black));
    connect(lineColorToolButton, SIGNAL(clicked()),
            this, SLOT(lineButtonTriggered()));

    textToolBar = addToolBar(tr("Font"));
    textToolBar->addWidget(fontCombo);
    textToolBar->addWidget(fontSizeCombo);
    textToolBar->addAction(boldAction);
    textToolBar->addAction(italicAction);
    textToolBar->addAction(underlineAction);

    colorToolBar = addToolBar(tr("Color"));
    colorToolBar->addWidget(fontColorToolButton);
    colorToolBar->addWidget(fillColorToolButton);
    colorToolBar->addWidget(lineColorToolButton);

    QToolButton *pointerButton = new QToolButton;
    pointerButton->setCheckable(true);
    pointerButton->setChecked(true);
    pointerButton->setIcon(QIcon(":/images/pointer.png"));
    linePointerButton = new QToolButton;
    linePointerButton->setCheckable(true);
    //linePointerButton->setIcon(QIcon(":/images/linepointer.png"));
    linePointerButton->setIcon(createArrowIcon(0));
    linePointerButton->setPopupMode(QToolButton::MenuButtonPopup);
    linePointerButton->setMenu(createArrowMenu(SLOT(lineArrowChanged()),
                                     0));
    arrowAction = linePointerButton->menu()->defaultAction();
    connect(linePointerButton, SIGNAL(clicked()),
                this, SLOT(lineArrowButtonTriggered()));

    pointerTypeGroup = new QButtonGroup;
    pointerTypeGroup->setExclusive(false);
    pointerTypeGroup->addButton(pointerButton, int(DiagramScene::MoveItem));
    pointerTypeGroup->addButton(linePointerButton,
                                int(DiagramScene::InsertLine));
    connect(pointerTypeGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(pointerGroupClicked(int)));

    pointerToolbar = addToolBar(tr("Pointer type"));
    pointerToolbar->addWidget(pointerButton);
    pointerToolbar->addWidget(linePointerButton);

    zoomToolbar = addToolBar(tr("Zoom"));
    zoomToolbar->addAction(zoomInAction);
    zoomToolbar->addAction(zoomOutAction);
    zoomToolbar->addAction(zoomAction);
    zoomToolbar->addAction(zoomFitAction);
//! [27]
}
//! [27]

//! [28]
QWidget *MainWindow::createBackgroundCellWidget(const QString &text,
                        const QString &image)
{
    QToolButton *button = new QToolButton;
    button->setText(text);
    button->setIcon(QIcon(image));
    button->setIconSize(QSize(50, 50));
    button->setCheckable(true);
    backgroundButtonGroup->addButton(button);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(button, 0, 0, Qt::AlignHCenter);
    layout->addWidget(new QLabel(text), 1, 0, Qt::AlignCenter);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    return widget;
}
//! [28]

//! [29]
QWidget *MainWindow::createCellWidget(const QString &text,
                      DiagramItem::DiagramType type)
{

    DiagramItem item(type, itemMenu);
    QIcon icon(item.image());

    QToolButton *button = new QToolButton;
    button->setIcon(icon);
    button->setIconSize(QSize(50, 50));
    button->setCheckable(true);
    buttonGroup->addButton(button, int(type));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(button, 0, 0, Qt::AlignHCenter);
    layout->addWidget(new QLabel(text), 1, 0, Qt::AlignCenter);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    return widget;
}
//! [29]
QWidget *MainWindow::createCellWidget(const QString &text,
                      DiagramDrawItem::DiagramType type)
{

    DiagramDrawItem item(type, itemMenu);
    item.setPos2(230,230);
    QIcon icon(item.image());

    QToolButton *button = new QToolButton;
    button->setIcon(icon);
    button->setIconSize(QSize(50, 50));
    button->setCheckable(true);
    buttonGroup->addButton(button, int(type)+64);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(button, 0, 0, Qt::AlignHCenter);
    layout->addWidget(new QLabel(text), 1, 0, Qt::AlignCenter);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    return widget;
}

//! [30]
QMenu *MainWindow::createColorMenu(const char *slot, QColor defaultColor)
{
    QList<QColor> colors;
    colors << Qt::black << Qt::white << Qt::red << Qt::blue << Qt::yellow;
    QStringList names;
    names << tr("black") << tr("white") << tr("red") << tr("blue")
          << tr("yellow");

    QMenu *colorMenu = new QMenu;
    for (int i = 0; i < colors.count(); ++i) {
        QAction *action = new QAction(names.at(i), this);
        action->setData(colors.at(i));
        action->setIcon(createColorIcon(colors.at(i)));
        connect(action, SIGNAL(triggered()),
                this, slot);
        colorMenu->addAction(action);
        if (colors.at(i) == defaultColor) {
            colorMenu->setDefaultAction(action);
        }
    }
    return colorMenu;
}
//! [30]

//! [31]
QIcon MainWindow::createColorToolButtonIcon(const QString &imageFile,
                        QColor color)
{
    QPixmap pixmap(50, 80);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    QPixmap image(imageFile);
    QRect target(0, 0, 50, 60);
    QRect source(0, 0, 42, 42);
    painter.fillRect(QRect(0, 60, 50, 80), color);
    painter.drawPixmap(target, image, source);

    return QIcon(pixmap);
}
//! [31]

//! [32]
QIcon MainWindow::createColorIcon(QColor color)
{
    QPixmap pixmap(20, 20);
    QPainter painter(&pixmap);
    painter.setPen(Qt::NoPen);
    painter.fillRect(QRect(0, 0, 20, 20), color);

    return QIcon(pixmap);
}
//! [32]

QMenu *MainWindow::createArrowMenu(const char *slot, const int def)
{
	QStringList names;
	    names << tr("Path") << tr("Start") << tr("End") << tr("StartEnd");
    QMenu *arrowMenu = new QMenu;
    for (int i = 0; i < names.count(); ++i) {
        QAction *action = new QAction(names.at(i), this);
        action->setData(i);
        action->setIcon(createArrowIcon(i));
        connect(action, SIGNAL(triggered()),
                this, slot);
        arrowMenu->addAction(action);
        if (i == def) {
            arrowMenu->setDefaultAction(action);
        }
    }
    return arrowMenu;
}

QIcon MainWindow::createArrowIcon(const int i)
{
    QPixmap pixmap(50, 80);
    DiagramPathItem* item=new DiagramPathItem(DiagramPathItem::DiagramType(i),0,0);
    pixmap=item->icon();
    delete item;

    return QIcon(pixmap);
}

void MainWindow::lineArrowButtonTriggered()
{
    scene->setArrow(qVariantValue<int>(arrowAction->data()));
    pointerTypeGroup->button(int(DiagramScene::MoveItem))->setChecked(false);
}

void MainWindow::lineArrowChanged()
{
    arrowAction = qobject_cast<QAction *>(sender());
    linePointerButton->setIcon(createArrowIcon(qVariantValue<int>(arrowAction->data())));
    lineArrowButtonTriggered();
}

void MainWindow::zoomIn()
{
    zoom(2.0);
}

void MainWindow::zoomOut()
{
	zoom(0.5);
}

void MainWindow::zoom(const qreal factor)
{
	QPointF topLeft     = view->mapToScene( 0, 0 );
	QPointF bottomRight = view->mapToScene( view->viewport()->width() - 1, view->viewport()->height() - 1 );
	qreal width=bottomRight.x()-topLeft.x();
	qreal height=bottomRight.y()-topLeft.y();
	//std::cout << width << "/" << height << " : " << factor << std::endl;
	if((width/factor<=5000)&&(height/factor<=5000)){
		QMatrix oldMatrix = view->matrix();
		qreal newScale=oldMatrix.m11()*factor;
		view->resetMatrix();
		view->translate(oldMatrix.dx(), oldMatrix.dy());
		view->scale(newScale, newScale);

		setGrid();
	}
}

void MainWindow::zoomRect()
{
	scene->setMode(DiagramScene::Zoom);
	view->setDragMode(QGraphicsView::RubberBandDrag);
	setGrid();
}

void MainWindow::doZoomRect(QPointF p1,QPointF p2)
{
	view->fitInView(QRectF(p1,p2),Qt::KeepAspectRatio);
	setGrid();
}

void MainWindow::zoomFit()
{
	scene->setCursorVisible(false);
	view->fitInView(scene->itemsBoundingRect(),Qt::KeepAspectRatio);
	scene->setCursorVisible(true);
	setGrid();
}

void MainWindow::toggleGrid(bool grid)
{
	scene->setGridVisible(grid);
	//view->repaint();
	QPointF topLeft     = view->mapToScene( 0, 0 );
	QPointF bottomRight = view->mapToScene( view->viewport()->width() - 1, view->viewport()->height() - 1 );
	scene->invalidate(topLeft.x(),topLeft.y(),bottomRight.x()-topLeft.x(),bottomRight.y()-topLeft.y());
	//scene->update();
	//view->update();

}

void MainWindow::setGrid()
{
	if(scene->isGridVisible()){
		QPointF topLeft     = view->mapToScene( 0, 0 );
		QPointF bottomRight = view->mapToScene( view->viewport()->width() - 1, view->viewport()->height() - 1 );
		qreal width=bottomRight.x()-topLeft.x();
		qreal height=bottomRight.y()-topLeft.y();
		qreal zw=width;
		std::cout << width << "/" << height << std::endl;
		if(zw<height) zw=height;
		int n=int(zw)/int(myGrid);
		int k=1;
		while(n/k>50)
		{
			k=k*2;
		}
		std::cout << k << std::endl;
		scene->setGridScale(k);
		scene->update();
	}
}

void MainWindow::saveAs()
{
	QFileDialog::Options options;
	options = 0;
	QString selectedFilter;
	QString fileName = QFileDialog::getSaveFileName(this,
			tr("Save Diagram as ..."),
			".qdiag",
			tr("QDiagram (*.qdiag)"),
			&selectedFilter,
			options);
	if (!fileName.isEmpty()){
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
			QMessageBox::warning(this,tr("File operation error"),file.errorString());
		}
		else
		{
			if(scene->save(&file)){
				myFileName=fileName;
			}
			file.close();
			if(file.error()){
				std::cerr << "Error: cannot write file "
				<< qPrintable(file.fileName())
				<< qPrintable(file.errorString())
				<< std::endl ;
			}
		}
	}
}

void MainWindow::save()
{
	if (!myFileName.isEmpty()){
		QFile file(myFileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
			QMessageBox::warning(this,tr("File operation error"),file.errorString());
		}
		else
		{
			scene->save(&file);
		}
	}
}

void MainWindow::load()
{
	QFileDialog::Options options;
	options = 0;
	QString selectedFilter;
	QString fileName = QFileDialog::getOpenFileName(this,
			tr("Save Diagram as ..."),
			".qdiag",
			tr("QDiagram (*.qdiag)"),
			&selectedFilter,
			options);
	if (!fileName.isEmpty()){
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
			QMessageBox::warning(this,tr("File operation error"),file.errorString());
		}
		else
		{
			scene->clear();
			scene->load(&file);
			myFileName=fileName;
		}

	}
}

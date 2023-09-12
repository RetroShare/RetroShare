/*******************************************************************************
 * retroshare-gui/src/gui/msgs/MessageWindow.cpp                               *
 *                                                                             *
 * Copyright (C) 2011 by Retroshare Team     <retroshare.project@gmail.com>    *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "gui/common/FilesDefs.h"
#include "MessageWindow.h"
#include "MessageWidget.h"
#include "MessageComposer.h"
#include "TagsMenu.h"
#include "gui/settings/rsharesettings.h"

#include <retroshare/rsmsgs.h>

#include "gui/msgs/MessageInterface.h"

/** Constructor */
MessageWindow::MessageWindow(QWidget *parent, Qt::WindowFlags flags)
: RWindow("MessageWindow", parent, flags)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);
        
	setAttribute ( Qt::WA_DeleteOnClose, true );

	actionSaveAs = NULL;
	actionPrint = NULL;
	actionPrintPreview = NULL;

	setupFileActions();

	connect(ui.newmessageButton, SIGNAL(clicked()), this, SLOT(newmessage()));

	connect(ui.actionTextBesideIcon, SIGNAL(triggered()), this, SLOT(buttonStyle()));
	connect(ui.actionIconOnly, SIGNAL(triggered()), this, SLOT(buttonStyle()));
	connect(ui.actionTextUnderIcon, SIGNAL(triggered()), this, SLOT(buttonStyle()));

	ui.actionTextBesideIcon->setData(Qt::ToolButtonTextBesideIcon);
	ui.actionIconOnly->setData(Qt::ToolButtonIconOnly);
	ui.actionTextUnderIcon->setData(Qt::ToolButtonTextUnderIcon);

	msgWidget = NULL;
	
	// create tag menu
	TagsMenu *menu = new TagsMenu (tr("Tags"), this);
	connect(menu, SIGNAL(aboutToShow()), this, SLOT(tagAboutToShow()));
	connect(menu, SIGNAL(tagSet(int, bool)), this, SLOT(tagSet(int, bool)));
	connect(menu, SIGNAL(tagRemoveAll()), this, SLOT(tagRemoveAll()));

	ui.tagButton->setMenu(menu);

	// create view menu
	QMenu *viewmenu = new QMenu();
	viewmenu->addAction(ui.actionTextBesideIcon);
	viewmenu->addAction(ui.actionIconOnly);
	//viewmenu->addAction(ui.actionTextUnderIcon);
	ui.viewtoolButton->setMenu(viewmenu);

	processSettings(true);
}

MessageWindow::~MessageWindow()
{
	processSettings(false);
}

void MessageWindow::processSettings(bool load)
{
	Settings->beginGroup(QString("MessageDialog"));

	if (load) {
		// load settings

		/* toolbar button style */
		Qt::ToolButtonStyle style = (Qt::ToolButtonStyle) Settings->value("ToolButon_Stlye", Qt::ToolButtonTextBesideIcon).toInt();
		setToolbarButtonStyle(style);
	} else {
		// save settings

		/* toolbar button style */
		Settings->setValue("ToolButon_Stlye", ui.newmessageButton->toolButtonStyle());
	}

	Settings->endGroup();
}

void MessageWindow::addWidget(MessageWidget *widget)
{
	if (msgWidget) {
		delete(msgWidget);
	}

	msgWidget = widget;
	if (msgWidget) {
		ui.msgLayout->addWidget(msgWidget);
		setWindowTitle(msgWidget->subject(true));

		msgWidget->connectAction(MessageWidget::ACTION_PRINT, ui.actionPrint);
		msgWidget->connectAction(MessageWidget::ACTION_PRINT, actionPrint);
		msgWidget->connectAction(MessageWidget::ACTION_PRINT_PREVIEW, ui.actionPrint_Preview);
		msgWidget->connectAction(MessageWidget::ACTION_PRINT_PREVIEW, actionPrintPreview);
//		msgWidget->connectAction(ACTION_SAVE,
		msgWidget->connectAction(MessageWidget::ACTION_SAVE_AS, actionSaveAs);
	} else {
		setWindowTitle("");
	}
}

void MessageWindow::newmessage()
{
	MessageComposer *msgComposer = MessageComposer::newMsg();
	if (msgComposer == NULL) {
		return;
	}

	/* fill it in */
	msgComposer->show();
	msgComposer->activateWindow();

	/* window will destroy itself! */
}

void MessageWindow::tagAboutToShow()
{
	if (msgWidget == NULL) {
		return;
	}

	TagsMenu *menu = dynamic_cast<TagsMenu*>(ui.tagButton->menu());
	if (menu == NULL) {
		return;
	}

	// activate actions
	MsgTagInfo tagInfo;
	rsMail->getMessageTag(msgWidget->msgId(), tagInfo);

    menu->activateActions(tagInfo);
}

void MessageWindow::tagRemoveAll()
{
	if (msgWidget == NULL) {
		return;
	}

	rsMail->setMessageTag(msgWidget->msgId(), 0, false);
}

void MessageWindow::tagSet(int tagId, bool set)
{
	if (msgWidget == NULL) {
		return;
	}

	if (tagId == 0) {
		return;
	}

	rsMail->setMessageTag(msgWidget->msgId(), tagId, set);
}

void MessageWindow::setupFileActions()
{
	QMenu *menu = new QMenu(tr("&File"), this);
	menuBar()->addMenu(menu);

	actionSaveAs = menu->addAction(tr("Save &As File"));
    actionPrint = menu->addAction(FilesDefs::getIconFromQtResourcePath(":/images/textedit/fileprint.png"), tr("&Print..."));
	actionPrint->setShortcut(QKeySequence::Print);

    actionPrintPreview = menu->addAction(FilesDefs::getIconFromQtResourcePath(":/images/textedit/fileprint.png"), tr("Print Preview..."));

//	a = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/textedit/exportpdf.png"), tr("&Export PDF..."), this);
//	a->setShortcut(Qt::CTRL + Qt::Key_D);
//	connect(a, SIGNAL(triggered()), this, SLOT(filePrintPdf()));
//	menu->addAction(a);

	menu->addSeparator();

	QAction *action = menu->addAction(tr("&Quit"), this, SLOT(close()));
	action->setShortcut(Qt::CTRL + Qt::Key_Q);
}

void MessageWindow::setToolbarButtonStyle(Qt::ToolButtonStyle style)
{
	ui.newmessageButton->setToolButtonStyle(style);
	ui.tagButton->setToolButtonStyle(style);
}

void MessageWindow::buttonStyle()
{
	setToolbarButtonStyle((Qt::ToolButtonStyle) dynamic_cast<QAction*>(sender())->data().toInt());
}

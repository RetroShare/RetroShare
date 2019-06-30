/*******************************************************************************
 * gui/common/Emoticons.cpp                                                    *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <QApplication>
#include <QDesktopWidget>
#include <QFile>
#include <QDir>
#include <QGridLayout>
#include <QHash>
#include <QIcon>
#include <QPushButton>
#include <QTabWidget>
#include <QWidget>
#include <QMessageBox>

#include <iostream>
#include <math.h>

#include "Emoticons.h"
#include "util/HandleRichText.h"
#include "retroshare/rsinit.h"

#define ICONNAME "groupicon.png"

static QHash<QString, QPair<QVector<QString>, QHash<QString, QString> > > Smileys;
static QVector<QString> grpOrdered;
static QVector<QString > StickerGroups;
static QStringList filters;
static QHash<QString, QString> tooltipcache;

void Emoticons::load()
{
	loadSmiley();
	filters << "*.png" << "*.jpg" << "*.gif";
	loadSticker(QString::fromStdString(RsAccounts::ConfigDirectory()) + "/stickers");		//under .retroshare, shared between users
	loadSticker(QString::fromStdString(RsAccounts::AccountDirectory()) + "/stickers");		//under account, unique for user
	loadSticker(QString::fromStdString(RsAccounts::systemDataDirectory()) + "/stickers");	//exe's folder, shipped with RS
}

void Emoticons::loadSmiley()
{
	QString sm_AllLines;
	bool internalFiles = true;

	// First try external emoticons
	QFile sm_File(QApplication::applicationDirPath() + "/emoticons/emotes.acs");
	if(sm_File.open(QIODevice::ReadOnly))
		internalFiles = false;
	else
	{
		// Then embedded emotions
		sm_File.setFileName(":/emojione/emotes.acs");
		if(!sm_File.open(QIODevice::ReadOnly))
		{
			std::cout << "error opening ressource file" << std::endl;
			return;
		}
	}

	sm_AllLines = sm_File.readAll();
	sm_File.close();

	sm_AllLines.remove("\n");
	sm_AllLines.remove("\r");

	int i = 0 ;
	QString smGroup;
	QString smCode;
	QString smFile;
	while(i < sm_AllLines.length() && sm_AllLines[i] != '{')
		++i ;//Ignore text before {

	while (i < sm_AllLines.length()-2)
	{
		// Type of lines:
		// "Group"|":code:":"emojione/file.png";
		smGroup = "";
		smCode = "";
		smFile = "";

		//Reading Groupe (First into "")
		while(sm_AllLines[i] != '\"')
			++i; //Ignore text outside ""

		++i; //'"'
		while (sm_AllLines[i] != '\"')
			smGroup += sm_AllLines[i++]; //Get group char by char

		++i; //'"'

		if (sm_AllLines[i] == '|')
		{
			//File in new format with group
			++i; //'|'
			//Reading Code (Second into "")
			while(sm_AllLines[i] != '\"')
				++i; //Ignore text outside ""

			++i; //'"'
			while (sm_AllLines[i] != '\"')
				smCode += sm_AllLines[i++]; //Get code char by char

			++i; //'"'
		} else {
			//Old file without group
			++i; //':'
			smCode = smGroup;
			smGroup = "NULL";
		}

		//Reading File (Third into "")
		while(sm_AllLines[i] != '\"')
			++i; //Ignore text outside ""

		++i; //'"'
		while(sm_AllLines[i] != '\"' && sm_AllLines[i+1] != ';')
			smFile += sm_AllLines[i++]; //Get file char by char

		++i; //'"'
		++i; //';'

		if(!smGroup.isEmpty() && !smCode.isEmpty() && !smFile.isEmpty())
		{
			while (smCode.right(1) == "|")
				smCode.remove(smCode.length() - 1, 1);

			if (internalFiles)
			{
				smFile = ":/" + smFile;
				if (smGroup.right(4).toLower() == ".png")
					smGroup = ":/" + smGroup;
			}

			QVector<QString> ordered;
			if (Smileys.contains(smGroup)) ordered = Smileys[smGroup].first;
			ordered.append(smCode);
			Smileys[smGroup].first = ordered;
			Smileys[smGroup].second.insert(smCode, smFile);

			if (!grpOrdered.contains(smGroup)) grpOrdered.append(smGroup);
		}
	}

	// init <img> embedder
	RsHtml::initEmoticons(Smileys);
}

void Emoticons::showSmileyWidget(QWidget *parent, QWidget *button, const char *slotAddMethod, bool above)
{
	QWidget *smWidget = new QWidget(parent, Qt::Popup) ;
	smWidget->setAttribute(Qt::WA_DeleteOnClose) ;
	smWidget->setWindowTitle("Emoticons") ;

	//QTabWidget::setTabBarAutoHide(true) is from QT5.4, no way to hide TabBar before.
	bool bOnlyOneGroup = (Smileys.count() == 1);

	QTabWidget *smTab = NULL;
	if (! bOnlyOneGroup)
	{
		smTab =  new QTabWidget(smWidget);
		QGridLayout *smGLayout = new QGridLayout(smWidget);
		smGLayout->setContentsMargins(0,0,0,0);
		smGLayout->addWidget(smTab);
	}

	const int buttonWidth = QFontMetricsF(smWidget->font()).height()*2.6;
	const int buttonHeight = QFontMetricsF(smWidget->font()).height()*2.6;
	int maxRowCount = 0;
	int maxCountPerLine = 0;

	QVectorIterator<QString> grp(grpOrdered);
	while(grp.hasNext())
	{
		QString groupName = grp.next();
		QHash<QString,QString> group = Smileys.value(groupName).second;

		QWidget *tabGrpWidget = NULL;
		if (! bOnlyOneGroup)
		{
			tabGrpWidget = new QWidget(smTab);

			// (Cyril) Never use an absolute size. It needs to be scaled to the actual font size on the screen.
			//
			QFontMetricsF fm(parent->font()) ;
			smTab->setIconSize(QSize(28*fm.height()/14.0,28*fm.height()/14.0));
			smTab->setMinimumWidth(400);
			smTab->setTabPosition(QTabWidget::South);
			smTab->setStyleSheet("QTabBar::tab { height: 44px; width: 44px; }");

			if (groupName.right(4).toLower() == ".png")
				smTab->addTab( tabGrpWidget, QIcon(groupName), "");
			else
				smTab->addTab( tabGrpWidget, groupName);
		} else {
			tabGrpWidget = smWidget;
		}

		QGridLayout *tabGLayout = new QGridLayout(tabGrpWidget);
		tabGLayout->setContentsMargins(0,0,0,0);
		tabGLayout->setSpacing(0);

		int rowCount = (int)sqrt((double)group.size());
		int countPerLine = (group.size()/rowCount) + ((group.size() % rowCount) ? 1 : 0);
		maxRowCount = qMax(maxRowCount, rowCount);
		maxCountPerLine = qMax(maxCountPerLine, countPerLine);

		int lin = 0;
		int col = 0;
		QVector<QString> ordered = Smileys.value(groupName).first;
		QVectorIterator<QString> it(ordered);
		while(it.hasNext())
		{

			QString key = it.next();
			QPushButton *button = new QPushButton("", tabGrpWidget);
			button->setIconSize(QSize(buttonWidth, buttonHeight));
			button->setFixedSize(QSize(buttonWidth, buttonHeight));
			button->setIcon(QPixmap(group.value(key)));
			button->setToolTip(key);
			button->setStyleSheet("QPushButton:hover {border: 3px solid #0099cc; border-radius: 3px;}");
			button->setFlat(true);
			tabGLayout->addWidget(button,col,lin);
			++lin;
			if(lin >= countPerLine)
			{
				lin = 0;
				++col;
			}
			QObject::connect(button, SIGNAL(clicked()), parent, slotAddMethod);
			QObject::connect(button, SIGNAL(clicked()), smWidget, SLOT(close()));
		}

	}

	//Get left up pos of button
	QPoint butTopLeft = button->mapToGlobal(QPoint(0,0));
	//Get widget's size
	QSize sizeWidget = smWidget->sizeHint();
	//Get screen's size
	QSize sizeScreen = QApplication::desktop()->size();

	//Calculate left distance to screen start
	int distToScreenLeft = butTopLeft.x();
	//Calculate right distance to screen end
	int distToRightScreen = sizeScreen.width() - (butTopLeft.x() + button->width());

	//Calculate left position
	int x;
	if (distToScreenLeft >= distToRightScreen) //More distance in left than right in screen
		x = butTopLeft.x() - sizeWidget.width(); //Place widget on left of button
	else
		x = butTopLeft.x() + button->width(); //Place widget on right of button

	//Calculate top position
	int y;
	if (above) //Widget must be above the button
		y = butTopLeft.y() + button->height() - sizeWidget.height();
	else
		y = butTopLeft.y() + button->height()/2 - sizeWidget.height()/2; //Centered on button height

	if (y + sizeWidget.height() > sizeScreen.height()) //Widget will be too low
		y = sizeScreen.height() - sizeWidget.height(); //Place widget bottom at screen bottom

	if (y < 0) //Widget will be too high
		y = 0; //Place widget top at screen top

	smWidget->move(x, y) ;
	smWidget->show() ;
}

void Emoticons::loadSticker(QString foldername)
{
	QDir dir(foldername);
	if(!dir.exists()) return;

	//If it contains at a least one png then add it as a group
	QStringList files = dir.entryList(filters, QDir::Files);
	if(files.count() > 0)
		StickerGroups.append(foldername);

	//Check subfolders
	QFileInfoList subfolders = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot);
	for(int i = 0; i<subfolders.length(); i++)
		loadSticker(subfolders[i].filePath());
}

void Emoticons::showStickerWidget(QWidget *parent, QWidget *button, const char *slotAddMethod, bool above)
{
	QWidget *smWidget = new QWidget(parent, Qt::Popup) ;
	smWidget->setAttribute(Qt::WA_DeleteOnClose) ;
	smWidget->setWindowTitle("Stickers") ;

	if(StickerGroups.count() == 0) {
		QMessageBox::warning(parent, "Stickers", "No stickers installed");
		return;
	}
	bool bOnlyOneGroup = (StickerGroups.count() == 1);

	QTabWidget *smTab = nullptr;
	if (! bOnlyOneGroup)
	{
		smTab =  new QTabWidget(smWidget);
		QGridLayout *smGLayout = new QGridLayout(smWidget);
		smGLayout->setContentsMargins(0,0,0,0);
		smGLayout->addWidget(smTab);
	}

	const int buttonWidth = QFontMetricsF(smWidget->font()).height()*5;
	const int buttonHeight = QFontMetricsF(smWidget->font()).height()*5;
	int maxRowCount = 0;
	int maxCountPerLine = 0;

	QVectorIterator<QString> grp(StickerGroups);
	while(grp.hasNext())
	{
		QDir groupDir = QDir(grp.next());
		QString groupName = groupDir.dirName();
		groupDir.setNameFilters(filters);

		QWidget *tabGrpWidget = nullptr;
		if (! bOnlyOneGroup)
		{
			//Lazy load tooltips for the current tab
			QObject::connect(smTab, &QTabWidget::currentChanged, [=](int index){
				QWidget* current = smTab->widget(index);
				loadToolTips(current);
			});

			tabGrpWidget = new QWidget(smTab);

			// (Cyril) Never use an absolute size. It needs to be scaled to the actual font size on the screen.
			//
			QFontMetricsF fm(parent->font()) ;
			smTab->setIconSize(QSize(28*fm.height()/14.0,28*fm.height()/14.0));
			smTab->setMinimumWidth(400);
			smTab->setTabPosition(QTabWidget::South);
			smTab->setStyleSheet("QTabBar::tab { height: 44px; width: 44px; }");

			int index;
			if (groupDir.exists(ICONNAME)) //use groupicon.png if exists, else the first png as a group icon
				index = smTab->addTab( tabGrpWidget, QIcon(groupDir.absoluteFilePath(ICONNAME)), "");
			else
				index = smTab->addTab( tabGrpWidget, QIcon(groupDir.entryInfoList(QDir::Files)[0].canonicalFilePath()), "");
			smTab->setTabToolTip(index, groupName);
		} else {
			tabGrpWidget = smWidget;
		}

		QGridLayout *tabGLayout = new QGridLayout(tabGrpWidget);
		tabGLayout->setContentsMargins(0,0,0,0);
		tabGLayout->setSpacing(0);

		QFileInfoList group = groupDir.entryInfoList(QDir::Files, QDir::Name);
		int rowCount = (int)sqrt((double)group.size());
		int countPerLine = (group.size()/rowCount) + ((group.size() % rowCount) ? 1 : 0);
		maxRowCount = qMax(maxRowCount, rowCount);
		maxCountPerLine = qMax(maxCountPerLine, countPerLine);

		int lin = 0;
		int col = 0;
		for(int i = 0; i < group.length(); ++i)
		{
			QFileInfo fi = group[i];
			if(fi.fileName().compare(ICONNAME, Qt::CaseInsensitive) == 0)
				continue;
			QPushButton *button = new QPushButton("", tabGrpWidget);
			button->setIconSize(QSize(buttonWidth, buttonHeight));
			button->setFixedSize(QSize(buttonWidth, buttonHeight));
			button->setIcon(QPixmap(fi.absoluteFilePath()));
			button->setToolTip(fi.fileName());
			button->setStatusTip(fi.absoluteFilePath());
			button->setStyleSheet("QPushButton:hover {border: 3px solid #0099cc; border-radius: 3px;}");
			button->setFlat(true);
			tabGLayout->addWidget(button,col,lin);
			++lin;
			if(lin >= countPerLine)
			{
				lin = 0;
				++col;
			}
			QObject::connect(button, SIGNAL(clicked()), parent, slotAddMethod);
			QObject::connect(button, SIGNAL(clicked()), smWidget, SLOT(close()));
		}

	}

	//Load tooltips for the first page
	QWidget * firstpage;
	if(bOnlyOneGroup) {
		firstpage = smWidget;
	} else {
		firstpage = smTab->currentWidget();
	}
	loadToolTips(firstpage);

	//Get left up pos of button
	QPoint butTopLeft = button->mapToGlobal(QPoint(0,0));
	//Get widget's size
	QSize sizeWidget = smWidget->sizeHint();
	//Get screen's size
	QSize sizeScreen = QApplication::desktop()->size();

	//Calculate left distance to screen start
	int distToScreenLeft = butTopLeft.x();
	//Calculate right distance to screen end
	int distToRightScreen = sizeScreen.width() - (butTopLeft.x() + button->width());

	//Calculate left position
	int x;
	if (distToScreenLeft >= distToRightScreen) //More distance in left than right in screen
		x = butTopLeft.x() - sizeWidget.width(); //Place widget on left of button
	else
		x = butTopLeft.x() + button->width(); //Place widget on right of button

	//Calculate top position
	int y;
	if (above) //Widget must be above the button
		y = butTopLeft.y() + button->height() - sizeWidget.height();
	else
		y = butTopLeft.y() + button->height()/2 - sizeWidget.height()/2; //Centered on button height

	if (y + sizeWidget.height() > sizeScreen.height()) //Widget will be too low
		y = sizeScreen.height() - sizeWidget.height(); //Place widget bottom at screen bottom

	if (y < 0) //Widget will be too high
		y = 0; //Place widget top at screen top

	smWidget->move(x, y);
	smWidget->show();
}

void Emoticons::loadToolTips(QWidget *container)
{
	QList<QPushButton *> children = container->findChildren<QPushButton *>();
	for(int i = 0; i < children.length(); ++i) {
		if(!children[i]->toolTip().contains('<')) {
			if(tooltipcache.contains(children[i]->statusTip())) {
				children[i]->setToolTip(tooltipcache[children[i]->statusTip()]);
			} else {
				QString tooltip;
				if(RsHtml::makeEmbeddedImage(children[i]->statusTip(), tooltip, 300*300)) {
					tooltipcache.insert(children[i]->statusTip(), tooltip);
					children[i]->setToolTip(tooltip);
				}

			}

		}
	}
}

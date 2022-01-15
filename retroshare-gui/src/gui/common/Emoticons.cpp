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

#include "Emoticons.h"
#include "util/HandleRichText.h"
#include "retroshare/rsinit.h"
#include "gui/common/FilesDefs.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QFile>
#include <QDir>
#include <QGridLayout>
#include <QIcon>
#include <QPushButton>
#include <QTabWidget>
#include <QWidget>
#include <QMessageBox>
#include <QDir>

#include <iostream>
#include <math.h>

#define ICONNAME "groupicon.png"

Q_GLOBAL_STATIC(Emoticons, emoticons)

Emoticons* Emoticons::operator->() const { return emoticons; }

void Emoticons::load()
{
	loadSmiley();
	m_filters << "*.png" << "*.jpg" << "*.jpeg" << "*.gif" << "*.webp";
	m_stickerFolders << (QString::fromStdString(RsAccounts::AccountDirectory()) + "/stickers");		//under account, unique for user
	m_stickerFolders << (QString::fromStdString(RsAccounts::ConfigDirectory()) + "/stickers");		//under .retroshare, shared between users
	m_stickerFolders << (QString::fromStdString(RsAccounts::systemDataDirectory()) + "/stickers");	//exe's folder, shipped with RS

	QDir dir(QString::fromStdString(RsAccounts::AccountDirectory()));
	dir.mkpath("stickers/imported");
}

//*************************************************************************************
// TODO: Look at this file: https://unicode.org/Public/emoji/14.0/emoji-sequences.txt
//*************************************************************************************

void Emoticons::loadSmiley()
{
	QString sm_AllLines;

	// First try external Unicode emoji list
	QFile sm_File(QApplication::applicationDirPath() + "/fonts/NotoColorEmoji.acs");
	if(!sm_File.open(QIODevice::ReadOnly))
	{
		// Then embedded
		sm_File.setFileName(":/fonts/NotoColorEmoji.acs");
		if(!sm_File.open(QIODevice::ReadOnly))
		{
			RS_ERR(" Error opening font ressource file");
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
	QString smUnicode;
	while(i < sm_AllLines.length() && sm_AllLines[i] != '{')
		++i ;//Ignore text before {

	while (i < sm_AllLines.length()-2)
	{
		// Type of lines:
		// "Group"|":code:":"Unicode";
		smGroup = "";
		smCode = "";
		smUnicode = "";

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
			smUnicode += sm_AllLines[i++]; //Get file char by char

		++i; //'"'
		++i; //';'

		if(!smGroup.isEmpty() && !smCode.isEmpty() && !smUnicode.isEmpty())
		{
			while (smCode.right(1) == "|")
				smCode.remove(smCode.length() - 1, 1);

			if (smGroup.right(4).toLower() == ".png")
				smGroup = ":/" + smGroup;

			QVector<QString> ordered;
			if (m_smileys.contains(smGroup)) ordered = m_smileys[smGroup].first;
			ordered.append(smCode);
			m_smileys[smGroup].first = ordered;
			m_smileys[smGroup].second.insert(smCode, smUnicode);

			if (!m_grpOrdered.contains(smGroup)) m_grpOrdered.append(smGroup);
		}
	}

	// init <img> embedder
	RsHtml::initEmoticons(m_smileys);
}

void Emoticons::showSmileyWidget(QWidget *parent, QWidget *callerButton, const char *slotAddMethod, bool above)
{
	QWidget *smWidget = new QWidget(parent, Qt::Popup) ;
	smWidget->setAttribute(Qt::WA_DeleteOnClose) ;
	smWidget->setWindowTitle("Emoticons") ;

	//QTabWidget::setTabBarAutoHide(true) is from QT5.4, no way to hide TabBar before.
	bool bOnlyOneGroup = (m_smileys.count() == 1);

	QTabWidget *smTab = NULL;
	if (! bOnlyOneGroup)
	{
		smTab =  new QTabWidget(smWidget);
		QGridLayout *smGLayout = new QGridLayout(smWidget);
		smGLayout->setContentsMargins(0,0,0,0);
		smGLayout->addWidget(smTab);
	}

	QFont noto = QFont("Noto Color Emoji",20,QFont::Normal);
	QFontMetrics fm = QFontMetrics(noto);

	const int buttonHeight = fm.height()+12;
	const int buttonWidth = buttonHeight;//Generally square
	int maxRowCount = 0;
	int maxCountPerLine = 0;

	QVectorIterator<QString> grp(m_grpOrdered);
	while(grp.hasNext())
	{
		QString groupName = grp.next();
		QHash<QString,QString> group = m_smileys.value(groupName).second;

		QWidget *tabGrpWidget = NULL;
		if (! bOnlyOneGroup)
		{
			tabGrpWidget = new QWidget(smTab);

			// (Cyril) Never use an absolute size. It needs to be scaled to the actual font size on the screen.
			//
			QFontMetricsF fm(parent->font()) ;
			QSize size(fm.height()*2.0,fm.height()*2.0);
			smTab->setIconSize(size);
			smTab->setMinimumWidth(400);
			smTab->setTabPosition(QTabWidget::South);
			smTab->setStyleSheet(QString("QTabBar::tab { height: %1px; width: %1px; padding: 0px; margin: 0px;}").arg(size.width()*1.1));

			if (groupName.right(4).toLower() == ".png")
				smTab->addTab( tabGrpWidget, FilesDefs::getIconFromQtResourcePath(groupName), "");
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

		QVector<QString> ordered = m_smileys.value(groupName).first;
		QVectorIterator<QString> it(ordered);
		while(it.hasNext())
		{
			bool bOK = false;
			QStringList keys = group.value(it.next()).split("-");
			uint c[keys.count()];
			QString tp = "";
			for (int i = 0 ; i < keys.count(); ++i)
			{
				c[i] = keys.at(i).toUInt(&bOK,16);
				tp += "U+" + QString::number( c[i], 16 ).toUpper() + " ";
			}

			if (bOK)
			{
				QPushButton *pb = new QPushButton(QString::fromUcs4(c,keys.count()), tabGrpWidget);
				pb->setFont(noto);
				pb->setFixedSize(QSize(buttonWidth, buttonHeight));
				pb->setToolTip( tp + "\n" + pb->text());
				pb->setStyleSheet("QPushButton:hover { subcontrol-origin: margin; subcontrol-position: top left; padding:0; border: 3px solid #0099cc; border-radius: 3px;}");
				pb->setFlat(true);
				tabGLayout->addWidget(pb,col,lin);
				++lin;
				if(lin >= countPerLine)
				{
					lin = 0;
					++col;
				}
				QObject::connect(pb, SIGNAL(clicked()), parent, slotAddMethod);
				QObject::connect(pb, SIGNAL(clicked()), smWidget, SLOT(close()));
			}
		}

	}

	//Get left up pos of button
	QPoint butTopLeft = callerButton->mapToGlobal(QPoint(0,0));
	//Get widget's size
	QSize sizeWidget = smWidget->sizeHint();
	//Get screen's size
	QSize sizeScreen = QApplication::desktop()->size();

	//Calculate left distance to screen start
	int distToScreenLeft = butTopLeft.x();
	//Calculate right distance to screen end
	int distToRightScreen = sizeScreen.width() - (butTopLeft.x() + callerButton->width());

	//Calculate left position
	int x;
	if (distToScreenLeft >= distToRightScreen) //More distance in left than right in screen
		x = butTopLeft.x() - sizeWidget.width(); //Place widget on left of button
	else
		x = butTopLeft.x() + callerButton->width(); //Place widget on right of button

	//Calculate top position
	int y;
	if (above) //Widget must be above the button
		y = butTopLeft.y() + callerButton->height() - sizeWidget.height();
	else
		y = butTopLeft.y() + callerButton->height()/2 - sizeWidget.height()/2; //Centered on button height

	if (y + sizeWidget.height() > sizeScreen.height()) //Widget will be too low
		y = sizeScreen.height() - sizeWidget.height(); //Place widget bottom at screen bottom

	if (y < 0) //Widget will be too high
		y = 0; //Place widget top at screen top

	smWidget->move(x, y) ;
	smWidget->show() ;
}

void Emoticons::refreshStickerTabs(QVector<QString>& stickerTabs)
{
	for(int i = 0; i < m_stickerFolders.count(); ++i)
		refreshStickerTabs(stickerTabs, m_stickerFolders[i]);
}

void Emoticons::refreshStickerTabs(QVector<QString>& stickerTabs, QString foldername)
{
	QDir dir(foldername);
	if(!dir.exists()) return;

	//If it contains at a least one png then add it as a group
	QStringList files = dir.entryList(m_filters, QDir::Files);
	if(files.count() > 0)
		stickerTabs.append(foldername);

	//Check subfolders
	QFileInfoList subfolders = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot);
	for(int i = 0; i<subfolders.length(); i++)
		refreshStickerTabs(stickerTabs, subfolders[i].filePath());
}

void Emoticons::showStickerWidget(QWidget *parent, QWidget *callerButton, const char *slotAddMethod, bool above)
{
	QVector<QString> stickerTabs;
	refreshStickerTabs(stickerTabs);
	if(stickerTabs.count() == 0) {
		QString message = "No stickers installed.\nYou can install them by putting images into one of these folders:\n" + m_stickerFolders.join('\n');
		QMessageBox::warning(parent, "Stickers", message);
		return;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	QWidget *smWidget = new QWidget(parent, Qt::Popup) ;
	smWidget->setAttribute(Qt::WA_DeleteOnClose) ;
	smWidget->setWindowTitle("Stickers") ;

	bool bOnlyOneGroup = (stickerTabs.count() == 1);

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

	QVectorIterator<QString> grp(stickerTabs);
	while(grp.hasNext())
	{
		QDir groupDir = QDir(grp.next());
		QString groupName = groupDir.dirName();
		groupDir.setNameFilters(m_filters);

		QWidget *tabGrpWidget = nullptr;
		if (! bOnlyOneGroup)
		{
			//Lazy load tooltips for the current tab
			QObject::connect(smTab, &QTabWidget::currentChanged, smTab, [=](int index){
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
				index = smTab->addTab( tabGrpWidget, FilesDefs::getIconFromQtResourcePath(groupDir.absoluteFilePath(ICONNAME)), "");
			else {
				QFileInfoList fil = groupDir.entryInfoList(QDir::Files);
				index = smTab->addTab( tabGrpWidget, FilesDefs::getIconFromQtResourcePath(fil[0].canonicalFilePath()), "");
			}
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
			if(!m_iconcache.contains(fi.absoluteFilePath()))
			{
				m_iconcache.insert(fi.absoluteFilePath(), FilesDefs::getPixmapFromQtResourcePath(fi.absoluteFilePath()).scaled(buttonWidth, buttonHeight, Qt::KeepAspectRatio));
			}
			button->setIcon(m_iconcache[fi.absoluteFilePath()]);
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
	QPoint butTopLeft = callerButton->mapToGlobal(QPoint(0,0));
	//Get widget's size
	QSize sizeWidget = smWidget->sizeHint();
	//Get screen's size
	QSize sizeScreen = QApplication::desktop()->size();

	//Calculate left distance to screen start
	int distToScreenLeft = butTopLeft.x();
	//Calculate right distance to screen end
	int distToRightScreen = sizeScreen.width() - (butTopLeft.x() + callerButton->width());

	//Calculate left position
	int x;
	if (distToScreenLeft >= distToRightScreen) //More distance in left than right in screen
		x = butTopLeft.x() - sizeWidget.width(); //Place widget on left of button
	else
		x = butTopLeft.x() + callerButton->width(); //Place widget on right of button

	//Calculate top position
	int y;
	if (above) //Widget must be above the button
		y = butTopLeft.y() + callerButton->height() - sizeWidget.height();
	else
		y = butTopLeft.y() + callerButton->height()/2 - sizeWidget.height()/2; //Centered on button height

	if (y + sizeWidget.height() > sizeScreen.height()) //Widget will be too low
		y = sizeScreen.height() - sizeWidget.height(); //Place widget bottom at screen bottom

	if (y < 0) //Widget will be too high
		y = 0; //Place widget top at screen top

	smWidget->move(x, y);
	smWidget->show();
	QApplication::restoreOverrideCursor();
}

QString Emoticons::importedStickerPath()
{
	QDir dir(m_stickerFolders[0]);
	return dir.absoluteFilePath("imported");
}

void Emoticons::loadToolTips(QWidget *container)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QList<QPushButton *> children = container->findChildren<QPushButton *>();
	for(int i = 0; i < children.length(); ++i) {
		if(!children[i]->toolTip().contains('<')) {
			if(m_tooltipcache.contains(children[i]->statusTip())) {
				children[i]->setToolTip(m_tooltipcache[children[i]->statusTip()]);
			} else {
				QString tooltip;
				if(RsHtml::makeEmbeddedImage(children[i]->statusTip(), tooltip, 300*300)) {
					m_tooltipcache.insert(children[i]->statusTip(), tooltip);
					children[i]->setToolTip(tooltip);
				}

			}

		}
	}
	QApplication::restoreOverrideCursor();
}

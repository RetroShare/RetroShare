/*******************************************************************************
 * gui/common/AvatarDialog.cpp                                                 *
 *                                                                             *
 * Copyright (C) 2012, Robert Fernie <retroshare.project@gmail.com>            *
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

#include <QBuffer>
#include <QFile>
#include <QDir>
#include <QGridLayout>
#include <QHash>
#include <QIcon>
#include <QPushButton>
#include <QTabWidget>
#include <QToolTip>
#include <QWidget>
#include <QMessageBox>
#include <QDir>

#include <iostream>
#include <math.h>

#include "AvatarDialog.h"
#include "ui_AvatarDialog.h"
#include "AvatarDefs.h"
#include "util/misc.h"
#include "gui/common/FilesDefs.h"
#include "util/HandleRichText.h"
#include "retroshare/rsinit.h"

#define ICONNAME "groupicon.png"

static QStringList filters;
static QStringList stickerFolders;
static QHash<QString, QString> tooltipcache;
static QHash<QString, QPixmap> iconcache;

/** Constructor */
AvatarDialog::AvatarDialog(QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new(Ui::AvatarDialog))
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui->setupUi(this);

    ui->headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/images/no_avatar_70.png"));
	ui->headerFrame->setHeaderText(tr("Set your Avatar picture"));

	connect(ui->avatarButton, SIGNAL(clicked(bool)), this, SLOT(changeAvatar()));
	connect(ui->removeButton, SIGNAL(clicked(bool)), this, SLOT(removeAvatar()));

	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	updateInterface();

	loadAvatarWidget();
}

const int AvatarDialog::RS_AVATAR_DEFAULT_IMAGE_W = 96;
const int AvatarDialog::RS_AVATAR_DEFAULT_IMAGE_H = 96;

AvatarDialog::~AvatarDialog()
{
	delete(ui);
}

void AvatarDialog::changeAvatar()
{
    QString image_filename ;

    if(!misc::getOpenFileName(this,RshareSettings::LASTDIR_IMAGES,tr("Import image"), tr("Image files (*.jpg *.png);;All files (*)"),image_filename))
        return;

    QImage img(image_filename);

    ui->avatarLabel->setPicture(QPixmap::fromImage(img));
    ui->avatarLabel->setEnableZoom(true);
    ui->avatarLabel->setToolTip(tr("Use the mouse to zoom and adjust the image for your avatar."));

    // shows the tooltip for a while
    QToolTip::showText( ui->avatarLabel->mapToGlobal( QPoint( 0, 0 ) ), ui->avatarLabel->toolTip() );
	updateInterface();
}

void AvatarDialog::removeAvatar()
{
	ui->avatarLabel->setPicture(QPixmap());
	updateInterface();
}

void AvatarDialog::updateInterface()
{
	const QPixmap *pixmap = ui->avatarLabel->pixmap();
	if (pixmap && !pixmap->isNull()) {
		ui->removeButton->setEnabled(true);
	} else {
		ui->removeButton->setEnabled(false);
	}
}

void AvatarDialog::setAvatar(const QPixmap &avatar)
{
	ui->avatarLabel->setPicture(avatar);
	updateInterface();
}

void AvatarDialog::getAvatar(QPixmap &avatar)
{
	const QPixmap *pixmap = ui->avatarLabel->pixmap();
	if (!pixmap) {
		avatar = QPixmap();
		return;
	}

	avatar = *pixmap;
}

void AvatarDialog::getAvatar(QByteArray &avatar)
{
	pixmap = ui->avatarLabel->extractCroppedScaledPicture();
	if (!pixmap) {
		avatar.clear();
		return;
	}

	QBuffer buffer(&avatar);

	buffer.open(QIODevice::WriteOnly);
	pixmap.save(&buffer, "PNG"); // writes image into ba in PNG format
}

void AvatarDialog::load()
{
	filters << "*.png" << "*.jpg" << "*.gif";
	stickerFolders << (QString::fromStdString(RsAccounts::AccountDirectory()) + "/stickers");		//under account, unique for user
	stickerFolders << (QString::fromStdString(RsAccounts::ConfigDirectory()) + "/stickers");		//under .retroshare, shared between users
	stickerFolders << (QString::fromStdString(RsAccounts::systemDataDirectory()) + "/stickers");	//exe's folder, shipped with RS

	QDir dir(QString::fromStdString(RsAccounts::AccountDirectory()));
	dir.mkpath("stickers/imported");
}

void AvatarDialog::loadAvatarWidget()
{
	QVector<QString> stickerTabs;
	refreshStickerTabs(stickerTabs);

	if(stickerTabs.count() == 0) {
		ui->nostickersLabel->setText("");
		QString message = "No Avatars or Stickers installed.\nYou can install them by putting images into one of these folders:\n" + stickerFolders.join('\n');
		message += "\n RetroShare/stickers\n RetroShare/Data/stickers\n RetroShare/Data/Location/stickers";
		ui->nostickersLabel->setText(message);
	} else {
		ui->infoframe->hide();
	}

	bool bOnlyOneGroup = (stickerTabs.count() == 1);

	QTabWidget *avatarTab = nullptr;
	if (! bOnlyOneGroup)
	{
		avatarTab =  new QTabWidget(ui->avatarWidget);
		QGridLayout *avatarGLayout = new QGridLayout(ui->avatarWidget);
		avatarGLayout->setContentsMargins(0,0,0,0);
		avatarGLayout->addWidget(avatarTab);
	}

	const int buttonWidth = QFontMetricsF(ui->avatarWidget->font()).height()*5;
	const int buttonHeight = QFontMetricsF(ui->avatarWidget->font()).height()*5;
	int maxRowCount = 0;
	int maxCountPerLine = 0;

	QVectorIterator<QString> grp(stickerTabs);
	while(grp.hasNext())
	{
		QDir groupDir = QDir(grp.next());
		QString groupName = groupDir.dirName();
		groupDir.setNameFilters(filters);

		QWidget *tabGrpWidget = nullptr;
		if (! bOnlyOneGroup)
		{
			//Lazy load tooltips for the current tab
			QObject::connect(avatarTab, &QTabWidget::currentChanged, [=](int index){
				QWidget* current = avatarTab->widget(index);
				loadToolTips(current);
			});

			tabGrpWidget = new QWidget(avatarTab);

			// (Cyril) Never use an absolute size. It needs to be scaled to the actual font size on the screen.
			//
			QFontMetricsF fm(font()) ;
			avatarTab->setIconSize(QSize(28*fm.height()/14.0,28*fm.height()/14.0));
			avatarTab->setMinimumWidth(200);
			//avatarTab->setTabPosition(QTabWidget::West);
			avatarTab->setStyleSheet("QTabBar::tab { height: 44px; width: 44px; }");

			int index;
			if (groupDir.exists(ICONNAME)) //use groupicon.png if exists, else the first png as a group icon
                index = avatarTab->addTab( tabGrpWidget, FilesDefs::getIconFromQtResourcePath(groupDir.absoluteFilePath(ICONNAME)), "");
			else
                index = avatarTab->addTab( tabGrpWidget, FilesDefs::getIconFromQtResourcePath(groupDir.entryInfoList(QDir::Files)[0].canonicalFilePath()), "");
			avatarTab->setTabToolTip(index, groupName);
		} else {
			tabGrpWidget = ui->avatarWidget;
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
			if(!iconcache.contains(fi.absoluteFilePath()))
			{
				iconcache.insert(fi.absoluteFilePath(), FilesDefs::getPixmapFromQtResourcePath(fi.absoluteFilePath()).scaled(buttonWidth, buttonHeight, Qt::KeepAspectRatio));
			}
			button->setIcon(iconcache[fi.absoluteFilePath()]);
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
			QObject::connect(button, SIGNAL(clicked()), this, SLOT(addAvatar()));
		}

	}

	//Load tooltips for the first page
	QWidget * firstpage;
	if(bOnlyOneGroup) {
		firstpage = ui->avatarWidget;
	} else {
		firstpage = avatarTab->currentWidget();
	}
	loadToolTips(firstpage);
	
	//Get widget's size
	QSize sizeWidget = ui->avatarWidget->sizeHint();

}

QString AvatarDialog::importedStickerPath()
{
	QDir dir(stickerFolders[0]);
	return dir.absoluteFilePath("imported");
}

void AvatarDialog::loadToolTips(QWidget *container)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
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
	QApplication::restoreOverrideCursor();
}

void AvatarDialog::refreshStickerTabs(QVector<QString>& stickerTabs)
{
	for(int i = 0; i < stickerFolders.count(); ++i)
		refreshStickerTabs(stickerTabs, stickerFolders[i]);
}

void AvatarDialog::refreshStickerTabs(QVector<QString>& stickerTabs, QString foldername)
{
	QDir dir(foldername);
	if(!dir.exists()) return;

	//If it contains at a least one png then add it as a group
	QStringList files = dir.entryList(filters, QDir::Files);
	if(files.count() > 0)
		stickerTabs.append(foldername);

	//Check subfolders
	QFileInfoList subfolders = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot);
	for(int i = 0; i<subfolders.length(); i++)
		refreshStickerTabs(stickerTabs, subfolders[i].filePath());
}

void AvatarDialog::addAvatar()
{
	QString sticker = qobject_cast<QPushButton*>(sender())->statusTip();
	QPixmap pixmap(sticker);

	ui->avatarLabel->setPicture(pixmap);
	updateInterface();
}

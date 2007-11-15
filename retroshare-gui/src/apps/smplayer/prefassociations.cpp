/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


	prefassociations.cpp
	Handles file associations in Windows
	Author: Florin Braghis (florin@libertv.ro)
*/


#include "prefassociations.h"
#include "images.h"
#include "preferences.h"
#include <QSettings>
#include <QApplication>
#include <QMessageBox>
#include "winfileassoc.h"


static Qt::CheckState CurItemCheckState = Qt::Unchecked; 

PrefAssociations::PrefAssociations(QWidget * parent, Qt::WindowFlags f)
: PrefWidget(parent, f )
{
	setupUi(this);

	connect(selectAll, SIGNAL(clicked(bool)), this, SLOT(selectAllClicked(bool)));
	connect(selectNone, SIGNAL(clicked(bool)), this, SLOT(selectNoneClicked(bool)));
	connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(listItemClicked(QListWidgetItem*))); 
	connect(listWidget, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(listItemPressed(QListWidgetItem*))); 

	//Video for windows
	addItem("avi");
	addItem("vfw");
	addItem("divx");

	//MPEG
	addItem("mpg");
	addItem("mpeg");
	addItem("m1v");
	addItem("m2v");
	addItem("mpv");
	addItem("dv");
	addItem("3gp");

	//QT
	addItem("mov");
	addItem("mp4");
	addItem("m4v");
	addItem("mqv");

	//VCD
	addItem("dat");
	addItem("vcd");

	//OGG
	addItem("ogg");
	addItem("ogm");

	//WMV
	addItem("asf");
	addItem("wmv");

	//Matroska
	addItem("mkv");

	//NSV
	addItem("nsv"); 

	//REAL
	addItem("ram"); 

	//DVD
	addItem("bin");
	addItem("iso");
	addItem("vob");	

	//FLV
	addItem("flv");
	retranslateStrings();
}

PrefAssociations::~PrefAssociations()
{

}

void PrefAssociations::selectAllClicked(bool)
{
	for (int k = 0; k < listWidget->count(); k++)
		listWidget->item(k)->setCheckState(Qt::Checked);
	listWidget->setFocus(); 
}

void PrefAssociations::selectNoneClicked(bool)
{
	for (int k = 0; k < listWidget->count(); k++)
		listWidget->item(k)->setCheckState(Qt::Unchecked);
	listWidget->setFocus(); 
}

void PrefAssociations::listItemClicked(QListWidgetItem* item)
{
	if (item->checkState() == CurItemCheckState)
	{
		//Clicked on the list item (not checkbox)
		if (item->checkState() == Qt::Checked)
			item->setCheckState(Qt::Unchecked);
		else
			item->setCheckState(Qt::Checked); 
	}
	//else - clicked on the checkbox itself, do nothing
}

void PrefAssociations::listItemPressed(QListWidgetItem* item)
{
	CurItemCheckState = item->checkState(); 
}

void PrefAssociations::addItem(const char* label)
{
	QListWidgetItem* item = new QListWidgetItem(listWidget); 
	item->setText(label);
	item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
}

void PrefAssociations::setData(Preferences * pref)
{
	QStringList extensions = pref->extensions.split(",");
	for (int k = 0; k < listWidget->count(); k++)
	{
		QListWidgetItem* pItem = listWidget->item(k); 
		if (pItem)
		{
			//pItem->setSelected(extensions.contains(pItem->text()));
			if (extensions.contains(pItem->text()))
				pItem->setCheckState(Qt::Checked);
			else
				pItem->setCheckState(Qt::Unchecked);

		}
	}
}

int PrefAssociations::ProcessAssociations(QStringList& current, QStringList& old)
{
	int processed = 0; 

	WinFileAssoc RegAssoc("SMPlayer.exe"); 

	//Restore unselected associations
	for (int k = 0; k < old.count(); k++)
	{
		const QString& ext = old[k];
		if (!current.contains(ext))
		{
			RegAssoc.RestoreFileAssociation(ext);
		}
	}
	
	//Set current associations
	if (current.count() > 0)
	{
		RegAssoc.CreateClassId(QApplication::applicationFilePath(), "SMPlayer Video Player");

		for (int k = 0; k < current.count(); k++)
		{
			if (RegAssoc.CreateFileAssociation(current[k]))
				processed++; 
		}
	}
	else
		RegAssoc.RemoveClassId();  

	return processed; 
}

void PrefAssociations::getData(Preferences * pref)
{
	QStringList extensions; 

	for (int k = 0; k < listWidget->count(); k++)
	{
		QListWidgetItem* pItem = listWidget->item(k); 
		if (pItem && pItem->checkState() == Qt::Checked)
			extensions.append(pItem->text()); 
	}

	QStringList old = pref->extensions.split(","); 

	int processed = ProcessAssociations(extensions, old); 

	//Save the new associations
	pref->extensions = extensions.join(","); 

	if (processed != extensions.count())
	{
		QMessageBox::warning(this, tr("Warning"), 
            tr("Not all files could be associated. Please check your "
               "security permissions and retry."), QMessageBox::Ok);
	}

}

QString PrefAssociations::sectionName() {
	return tr("File Types");
}

QPixmap PrefAssociations::sectionIcon() {
	return Images::icon("pref_associations");
}

void PrefAssociations::retranslateStrings() {
	retranslateUi(this);
	createHelp();
}

void PrefAssociations::createHelp()
{
	clearHelp();

	setWhatsThis(selectAll, tr("Select all"), 
		tr("Check all file types in the list"));

	setWhatsThis(selectNone, tr("Select none"), 
		tr("Uncheck all file types in the list"));

	setWhatsThis(listWidget, tr("List of file types"), 
		tr("Check the media file extensions you would like SMPlayer to handle. "
		   "When you click Apply, the checked files will be associated with "
		   "SMPlayer. If you uncheck a media type, the file association will "
		   "be restored."));
}

#include "moc_prefassociations.cpp"

